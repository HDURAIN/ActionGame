// Fill out your copyright notice in the Description page of Project Settings.

#include "AbilitySystem/Abilities/GA_Dash.h"

#include "ActionGameCharacter.h"
#include "Abilities/Tasks/AbilityTask_WaitDelay.h"
#include "Animation/AnimInstance.h"
#include "Animation/AnimSequenceBase.h"
#include "Components/SkeletalMeshComponent.h"
#include "GameFramework/Character.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/Controller.h"
#include "GameFramework/Pawn.h"
#include "AbilitySystemLog.h"

UGA_Dash::UGA_Dash()
{
	NetExecutionPolicy = EGameplayAbilityNetExecutionPolicy::LocalPredicted;
	InstancingPolicy = EGameplayAbilityInstancingPolicy::InstancedPerActor;
}

void UGA_Dash::ActivateAbility(
	const FGameplayAbilitySpecHandle Handle,
	const FGameplayAbilityActorInfo* ActorInfo,
	const FGameplayAbilityActivationInfo ActivationInfo,
	const FGameplayEventData* TriggerEventData
)
{
	Super::ActivateAbility(Handle, ActorInfo, ActivationInfo, TriggerEventData);

	AActionGameCharacter* Character = GetCharacter();
	if (!Character || !ActorInfo || !ActorInfo->AvatarActor.IsValid())
	{
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
		return;
	}

	if (!HasAuthorityOrPredictionKey(ActorInfo, &ActivationInfo))
	{
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
		return;
	}

	UCharacterMovementComponent* MoveComp = Character->GetCharacterMovement();
	if (!MoveComp)
	{
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
		return;
	}

	if (!bAllowAirDash && MoveComp->IsFalling())
	{
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
		return;
	}

	const FVector DashDir = ComputeDashDirection(ActorInfo);
	if (DashDir.IsNearlyZero())
	{
		UE_LOG(LogAbilitySystem, Warning, TEXT("[%s] Dash failed: direction is zero."), *GetName());
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
		return;
	}

	if (!CommitAbilityChecked())
	{
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
		return;
	}

	if (bRotateToDashDirection)
	{
		const FRotator DashRot = DashDir.Rotation();
		Character->SetActorRotation(FRotator(0.f, DashRot.Yaw, 0.f));
	}

	CachedAirControl = MoveComp->AirControl;
	bAirControlCached = true;
	MoveComp->AirControl = 0.0f;

	Character->SetAnimRootMotionTranslationScale(0.0f);
	bRootMotionScaleOverridden = true;

	if (DashAnimation)
	{
		if (UAnimInstance* AnimInstance = Character->GetMesh() ? Character->GetMesh()->GetAnimInstance() : nullptr)
		{
			AnimInstance->PlaySlotAnimationAsDynamicMontage(DashAnimation, DashAnimSlotName, 0.02f, 0.02f, DashAnimPlayRate);
		}
	}

	const float DashTime = FMath::Max(0.01f, DashDuration);
	const float DashSpeed = (DashTime > KINDA_SMALL_NUMBER) ? (FMath::Max(0.f, DashDistance) / DashTime) : 0.f;
	const FVector LaunchVelocity = DashDir * DashSpeed;
	Character->LaunchCharacter(LaunchVelocity, true, false);

	DashWaitTask = UAbilityTask_WaitDelay::WaitDelay(this, DashTime);
	if (DashWaitTask)
	{
		DashWaitTask->OnFinish.AddDynamic(this, &UGA_Dash::OnDashFinished);
		DashWaitTask->ReadyForActivation();
	}
	else
	{
		EndAbility(Handle, ActorInfo, ActivationInfo, true, false);
	}
}

void UGA_Dash::EndAbility(
	const FGameplayAbilitySpecHandle Handle,
	const FGameplayAbilityActorInfo* ActorInfo,
	const FGameplayAbilityActivationInfo ActivationInfo,
	bool bReplicateEndAbility,
	bool bWasCancelled
)
{
	AActionGameCharacter* Character = GetCharacter();
	if (Character)
	{
		if (UCharacterMovementComponent* MoveComp = Character->GetCharacterMovement())
		{
			if (bAirControlCached)
			{
				MoveComp->AirControl = CachedAirControl;
			}
		}

		if (bRootMotionScaleOverridden)
		{
			Character->SetAnimRootMotionTranslationScale(1.0f);
		}
	}

	bAirControlCached = false;
	bRootMotionScaleOverridden = false;
	DashWaitTask = nullptr;
	Super::EndAbility(Handle, ActorInfo, ActivationInfo, bReplicateEndAbility, bWasCancelled);
}

FVector UGA_Dash::ComputeDashDirection(const FGameplayAbilityActorInfo* ActorInfo) const
{
	const AActionGameCharacter* Character = GetCharacter();
	if (!Character)
	{
		return FVector::ZeroVector;
	}

	auto FlattenAndNormalize = [](const FVector& V) -> FVector
	{
		FVector Flat = FVector(V.X, V.Y, 0.f);
		return Flat.Normalize() ? Flat : FVector::ZeroVector;
	};

	const float InputThresholdSq = FMath::Square(FMath::Max(0.f, MinInputThreshold));
	const FVector LastInput = Character->GetLastMovementInputVector();
	const FVector FlatInput = FlattenAndNormalize(LastInput);
	if (!FlatInput.IsNearlyZero() && LastInput.SizeSquared2D() >= InputThresholdSq)
	{
		return FlatInput;
	}

	const float VelThresholdSq = FMath::Square(FMath::Max(0.f, MinVelocityThreshold));
	const FVector Velocity = Character->GetVelocity();
	const FVector FlatVel = FlattenAndNormalize(Velocity);
	if (!FlatVel.IsNearlyZero() && Velocity.SizeSquared2D() >= VelThresholdSq)
	{
		return FlatVel;
	}

	const bool bFaceCameraMode = Character->bUseControllerRotationYaw;
	if (bFaceCameraMode)
	{
		if (const AController* Controller = ActorInfo ? ActorInfo->PlayerController.Get() : Character->GetController())
		{
			const FVector CtrlForward = FRotator(0.f, Controller->GetControlRotation().Yaw, 0.f).Vector();
			const FVector FlatCtrlForward = FlattenAndNormalize(CtrlForward);
			if (!FlatCtrlForward.IsNearlyZero())
			{
				return FlatCtrlForward;
			}
		}
	}

	return FlattenAndNormalize(Character->GetActorForwardVector());
}

void UGA_Dash::OnDashFinished()
{
	EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, true, false);
}


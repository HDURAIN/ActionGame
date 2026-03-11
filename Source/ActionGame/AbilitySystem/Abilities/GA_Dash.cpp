#include "AbilitySystem/Abilities/GA_Dash.h"

#include "ActionGameCharacter.h"
#include "Abilities/Tasks/AbilityTask_ApplyRootMotionConstantForce.h"
#include "Abilities/Tasks/AbilityTask_PlayMontageAndWait.h"
#include "Animation/AnimMontage.h"
#include "GameFramework/CharacterMovementComponent.h"
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

	const FVector DashDir = ComputeDashDirection();
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

	Character->PushMoveInputBlock();
	bMoveInputBlockedByDash = true;

	if (DashMontage)
	{
		const float SafePlayRate = FMath::Max(0.01f, DashAnimPlayRate);
		DashMontageTask = UAbilityTask_PlayMontageAndWait::CreatePlayMontageAndWaitProxy(
			this,
			NAME_None,
			DashMontage,
			SafePlayRate
		);
		if (DashMontageTask)
		{
			DashMontageTask->ReadyForActivation();
		}
	}

	const float SafeDashDistance = FMath::Max(0.f, DashDistance);
	const float SafeDashDuration = FMath::Max(0.01f, DashDuration);
	const float DashSpeed = SafeDashDistance / SafeDashDuration;

	DashMoveTask = UAbilityTask_ApplyRootMotionConstantForce::ApplyRootMotionConstantForce(
		this,
		NAME_None,
		DashDir,
		DashSpeed,
		SafeDashDuration,
		false,
		nullptr,
		ERootMotionFinishVelocityMode::SetVelocity,
		FVector::ZeroVector,
		0.0f,
		false
	);

	if (!DashMoveTask)
	{
		if (DashMontageTask)
		{
			DashMontageTask->EndTask();
			DashMontageTask = nullptr;
		}
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
		return;
	}

	UE_LOG(LogAbilitySystem, Verbose, TEXT("[%s] Dash start. Distance=%.2f Duration=%.2f Speed=%.2f"),
		*GetName(), SafeDashDistance, SafeDashDuration, DashSpeed);

	DashMoveTask->OnFinish.AddDynamic(this, &UGA_Dash::OnDashMoveFinished);
	DashMoveTask->ReadyForActivation();
}

void UGA_Dash::EndAbility(
	const FGameplayAbilitySpecHandle Handle,
	const FGameplayAbilityActorInfo* ActorInfo,
	const FGameplayAbilityActivationInfo ActivationInfo,
	bool bReplicateEndAbility,
	bool bWasCancelled
)
{
	if (bMoveInputBlockedByDash)
	{
		if (AActionGameCharacter* Character = GetCharacter())
		{
			Character->PopMoveInputBlock();
		}
		bMoveInputBlockedByDash = false;
	}

	if (DashMoveTask)
	{
		UAbilityTask_ApplyRootMotionConstantForce* MoveTask = DashMoveTask;
		DashMoveTask = nullptr;
		MoveTask->OnFinish.RemoveAll(this);
		MoveTask->EndTask();
	}

	if (DashMontageTask)
	{
		UAbilityTask_PlayMontageAndWait* MontageTask = DashMontageTask;
		DashMontageTask = nullptr;
		MontageTask->EndTask();
	}

	Super::EndAbility(Handle, ActorInfo, ActivationInfo, bReplicateEndAbility, bWasCancelled);
}

FVector UGA_Dash::ComputeDashDirection() const
{
	const AActionGameCharacter* Character = GetCharacter();
	if (!Character)
	{
		return FVector::ZeroVector;
	}

	FVector InputDir = Character->GetCachedMoveInputDirection();
	InputDir.Z = 0.f;

	if (InputDir.SizeSquared2D() < FMath::Square(FMath::Max(0.f, MinInputThreshold)))
	{
		return FVector::ZeroVector;
	}

	return InputDir.GetSafeNormal2D();
}

void UGA_Dash::OnDashMoveFinished()
{
	EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, true, false);
}

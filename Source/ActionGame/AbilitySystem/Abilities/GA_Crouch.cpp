// Fill out your copyright notice in the Description page of Project Settings.

#include "AbilitySystem/Abilities/GA_Crouch.h"

#include "GameFramework/Character.h"
#include "AbilitySystemComponent.h"
#include "GameFramework/CharacterMovementComponent.h"

UGA_Crouch::UGA_Crouch()
{
	NetExecutionPolicy = EGameplayAbilityNetExecutionPolicy::ServerOnly;
	InstancingPolicy = EGameplayAbilityInstancingPolicy::InstancedPerActor;
}

void UGA_Crouch::ActivateAbility(
	const FGameplayAbilitySpecHandle Handle,
	const FGameplayAbilityActorInfo* ActorInfo,
	const FGameplayAbilityActivationInfo ActivationInfo,
	const FGameplayEventData* TriggerEventData
)
{
	Super::ActivateAbility(Handle, ActorInfo, ActivationInfo, TriggerEventData);

	if (!ActorInfo || !ActorInfo->AvatarActor.IsValid())
	{
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
		return;
	}

	if (!HasAuthorityOrPredictionKey(ActorInfo, &ActivationInfo))
	{
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
		return;
	}

	if (!CommitAbility(Handle, ActorInfo, ActivationInfo))
	{
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
		return;
	}
	// 交由CharacterMovement执行Crouch
	ACharacter* Character = Cast<ACharacter>(ActorInfo->AvatarActor.Get());

	UCharacterMovementComponent* MovementComponent = Character->GetCharacterMovement();
	if (!MovementComponent || MovementComponent->IsFalling())
	{
		// 空中不允许蹲
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
		return;
	}

	if (Character->CanCrouch())
	{
		Character->Crouch();
	}

	// Gameplay 状态声明(不存Handle)
	if (CrouchStateEffect && ActorInfo->AbilitySystemComponent.IsValid())
	{
		FGameplayEffectContextHandle EffectContext = ActorInfo->AbilitySystemComponent->MakeEffectContext();
		FGameplayEffectSpecHandle Spec = ActorInfo->AbilitySystemComponent->MakeOutgoingSpec(CrouchStateEffect, 1, EffectContext);

		if (Spec.IsValid())
		{
			CrouchEffectHandle = ActorInfo->AbilitySystemComponent->ApplyGameplayEffectSpecToSelf(*Spec.Data.Get());
		}
	}

	// 与 Jump 不同 Crouch 由 输入 / Character 触发结束
	// 不会 直接EndAbility
}

void UGA_Crouch::EndAbility(
	const FGameplayAbilitySpecHandle Handle,
	const FGameplayAbilityActorInfo* ActorInfo,
	const FGameplayAbilityActivationInfo ActivationInfo,
	bool bReplicateEndAbility,
	bool bWasCancelled
)
{
	if (ActorInfo && ActorInfo->AvatarActor.IsValid())
	{
		if (ACharacter* Character = Cast<ACharacter>(ActorInfo->AvatarActor.Get()))
		{
			if (Character->bIsCrouched)
			{
				Character->UnCrouch();
			}
		}
	}

	if (CrouchEffectHandle.IsValid() && ActorInfo && ActorInfo->AbilitySystemComponent.IsValid())
	{
		ActorInfo->AbilitySystemComponent->RemoveActiveGameplayEffect(CrouchEffectHandle);
	}

	CrouchEffectHandle.Invalidate();

	Super::EndAbility(Handle, ActorInfo, ActivationInfo, bReplicateEndAbility, bWasCancelled);
}

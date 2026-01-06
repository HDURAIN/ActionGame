// Fill out your copyright notice in the Description page of Project Settings.


#include "AbilitySystem/Abilities/GA_Sprint.h"

#include "GameFramework/Character.h"
#include "AbilitySystemComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include <Abilities/Tasks/AbilityTask_WaitGameplayEvent.h>

UGA_Sprint::UGA_Sprint()
{
	NetExecutionPolicy = EGameplayAbilityNetExecutionPolicy::LocalPredicted;
	InstancingPolicy = EGameplayAbilityInstancingPolicy::InstancedPerActor;
}

void UGA_Sprint::ActivateAbility(
	const FGameplayAbilitySpecHandle Handle,
	const FGameplayAbilityActorInfo* ActorInfo,
	const FGameplayAbilityActivationInfo ActivationInfo,
	const FGameplayEventData* TriggerEventData
)
{
	Super::ActivateAbility(Handle, ActorInfo, ActivationInfo, TriggerEventData);

	// 参数校验
	if (!ActorInfo || !ActorInfo->AvatarActor.IsValid())
	{
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
		return;
	}

	// 权限校验
	if (!HasAuthorityOrPredictionKey(ActorInfo, &ActivationInfo))
	{
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
		return;
	}


	// Commit成功校验
	if (!CommitAbility(Handle, ActorInfo, ActivationInfo))
	{
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
		return;
	}

	// 物理校验
	ACharacter* Character = Cast<ACharacter>(ActorInfo->AvatarActor.Get());
	if (!Character || !Character->GetCharacterMovement() || Character->GetCharacterMovement()->IsFalling())
	{
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
		return;
	}

	// Apply Effect
	if (SprintStateEffect && ActorInfo->AbilitySystemComponent.IsValid())
	{
		FGameplayEffectContextHandle EffectContext = ActorInfo->AbilitySystemComponent->MakeEffectContext();
		FGameplayEffectSpecHandle Spec = ActorInfo->AbilitySystemComponent->MakeOutgoingSpec(SprintStateEffect, 1.f, EffectContext);
		if (Spec.IsValid())
		{
			SprintEffectHandle = ActorInfo->AbilitySystemComponent->ApplyGameplayEffectSpecToSelf(*Spec.Data.Get());
		}
	}
}

void UGA_Sprint::EndAbility(
	const FGameplayAbilitySpecHandle Handle,
	const FGameplayAbilityActorInfo* ActorInfo,
	const FGameplayAbilityActivationInfo ActivationInfo,
	bool bReplicateEndAbility,
	bool bWasCancelled
)
{	
	if (SprintEffectHandle.IsValid() && ActorInfo && ActorInfo->AbilitySystemComponent.IsValid())
	{
		ActorInfo->AbilitySystemComponent->RemoveActiveGameplayEffect(SprintEffectHandle);
	}

	SprintEffectHandle.Invalidate();

	Super::EndAbility(Handle, ActorInfo, ActivationInfo, bReplicateEndAbility, bWasCancelled);
}

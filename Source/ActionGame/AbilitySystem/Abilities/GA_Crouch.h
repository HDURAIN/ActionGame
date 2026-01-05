// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Abilities/GameplayAbility.h"
#include "GA_Crouch.generated.h"


class ACharacter;
class UGameplayEffect;
/**
 * 
 */
UCLASS()
class ACTIONGAME_API UGA_Crouch : public UGameplayAbility
{
	GENERATED_BODY()

public:
	UGA_Crouch();

	// 必须重载的函数 ActivateAbility EndAbility

	virtual void ActivateAbility(
		const FGameplayAbilitySpecHandle Handle,
		const FGameplayAbilityActorInfo* ActorInfo,
		const FGameplayAbilityActivationInfo ActivationInfo,
		const FGameplayEventData* TriggerEventData
	) override;

	virtual void EndAbility(
		const FGameplayAbilitySpecHandle Handle,
		const FGameplayAbilityActorInfo* ActorInfo,
		const FGameplayAbilityActivationInfo ActivationInfo,
		bool bReplicateEndAbility,
		bool bWasCancelled
	) override;

protected:
	UPROPERTY(EditDefaultsOnly, Category = "Crouch")
	TSubclassOf<UGameplayEffect> CrouchStateEffect;

	FActiveGameplayEffectHandle CrouchEffectHandle;
};

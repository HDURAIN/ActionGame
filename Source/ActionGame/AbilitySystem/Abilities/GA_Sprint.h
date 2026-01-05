// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Abilities/GameplayAbility.h"
#include "GameplayEffectTypes.h"
#include "GA_Sprint.generated.h"

class UGameplayEffect;
/**
 * 
 */
UCLASS()
class ACTIONGAME_API UGA_Sprint : public UGameplayAbility
{
	GENERATED_BODY()
	
public:
	UGA_Sprint();

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

	UPROPERTY(EditDefaultsOnly, Category = "Sprint")
	TSubclassOf<UGameplayEffect> SprintStateEffect;

	FActiveGameplayEffectHandle SprintEffectHandle;
};

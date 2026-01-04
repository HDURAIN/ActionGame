// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Abilities/GameplayAbility.h"
#include "AG_GameplayAbility.generated.h"


class AActionGameCharacter;

UCLASS()
class ACTIONGAME_API UAG_GameplayAbility : public UGameplayAbility
{
	GENERATED_BODY()
	
public:
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

	// Ability一激活就Apply的一批GE
	UPROPERTY(EditDefaultsOnly, Category = "Effects")
	TArray<TSubclassOf<UGameplayEffect>> OngoingEffectsToJustApplyOnstart;

	// Ability一结束就自动Remove的那批GE
	UPROPERTY(EditDefaultsOnly, Category = "Effects")
	TArray<TSubclassOf<UGameplayEffect>> OngoingEffectsToRemoveOnEnd;

	// 必须保存Handle才能精确移除
	UPROPERTY(EditDefaultsOnly, Category = "Effects")
	TArray<FActiveGameplayEffectHandle> RemoveOnEndEffectHandles;

	// 从ActorInfo获取游戏基础角色的函数 集中管理Cast逻辑
	UFUNCTION(BlueprintCallable, BlueprintPure)
	AActionGameCharacter* GetActionGameCharacterFromActorInfo() const;
};

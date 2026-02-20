// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Abilities/GameplayAbility.h"
#include "ActionGameTypes.h"
#include "AG_GameplayAbility.generated.h"

class AActionGameCharacter;
class UAbilitySystemComponent;

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

	/* ---------- 子类重写入口 ---------- */
	/** 技能真正逻辑 */
	virtual void OnAbilityActivated();

	/** 技能结束逻辑 */
	virtual void OnAbilityEnded(bool bWasCancelled);

	// 蓝图表现入口
	UFUNCTION(BlueprintImplementableEvent)
	void K2_OnAbilityActivated();

	UFUNCTION(BlueprintImplementableEvent)
	void K2_OnAbilityEnded(bool bCancelled);

protected:
	/* ---------- 快捷获取 ---------- */
	UFUNCTION(BlueprintPure, Category = "Ability")
	AActionGameCharacter* GetCharacter() const;

	UFUNCTION(BlueprintPure, Category = "Ability")
	UAbilitySystemComponent* GetASC() const;

	/** Commit封装（统一报错日志） */
	bool CommitAbilityChecked();

protected:

	/* ---------- Ability配置 ---------- */
	/** 输入绑定ID（Skill1/Skill2等） 告诉ASC这个技能属于哪个输入槽位 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Ability")
	EAbilityInputID AbilityInputID = EAbilityInputID::None;

	/** 技能类型（主动/被动/持续）控制技能行为模式 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Ability")
	EAbilityType AbilityType = EAbilityType::Active;

	UPROPERTY(EditDefaultsOnly, Category = "Ability")
	bool bAutoEndAbility = true;

	/* ---------- GameplayEffects ---------- */
	/** 激活时自动应用 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Effects")
	TArray<TSubclassOf<UGameplayEffect>> EffectsOnStart;

	/** 结束时自动应用 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Effects")
	TArray<TSubclassOf<UGameplayEffect>> EffectsOnEnd;

	/** 自动记录Start效果句柄 */
	UPROPERTY()
	TArray<FActiveGameplayEffectHandle> ActiveEffectHandles;
};

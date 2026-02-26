#pragma once

#include "CoreMinimal.h"
#include "AbilitySystem/Abilities/AG_GameplayAbility.h"
#include "GA_PrimaryAttack.generated.h"

struct FGameplayEventData;

UCLASS()
class ACTIONGAME_API UGA_PrimaryAttack : public UAG_GameplayAbility
{
	GENERATED_BODY()

public:
	UGA_PrimaryAttack();

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

	/** 蓝图在收到 Shoot 事件时调用：服务端做 Trace 并触发 GameplayCue（可同步） */
	UFUNCTION(BlueprintCallable, Category = "PrimaryAttack")
	void Shoot_TraceAndCue();

protected:
	virtual const FGameplayTagContainer* GetCooldownTags() const override;

	virtual void ApplyCooldown(
		const FGameplayAbilitySpecHandle Handle,
		const FGameplayAbilityActorInfo* ActorInfo,
		const FGameplayAbilityActivationInfo ActivationInfo) const override;

	/** 蓝图实现：激活后播放Montage、等待Shoot事件做trace */
	UFUNCTION(BlueprintImplementableEvent, Category = "PrimaryAttack")
	void K2_OnActivateFromEvent();

	// ===== Trace 配置 =====
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "PrimaryAttack|Trace")
	float TraceDistance = 10000.f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "PrimaryAttack|Trace")
	FName MuzzleSocketName = TEXT("Muzzle");

	// ===== Cue 配置 =====
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "PrimaryAttack|Cue")
	FGameplayTag ImpactCueTag;

	// PrimaryAttack|Tuning
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "PrimaryAttack|Tuning")
	float BaseCooldown = 0.1f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "PrimaryAttack|Tuning")
	float DamageCoefficient = 1.0f;

	// Damage
	UPROPERTY(EditDefaultsOnly, Category = "PrimaryAttack|Damage")
	TSubclassOf<UGameplayEffect> DamageEffectClass = nullptr;

	UPROPERTY(EditDefaultsOnly, Category = "PrimaryAttack|Damage")
	FGameplayTag DamageDataTag;

	// Cooldown
	UPROPERTY(EditDefaultsOnly, Category = "PrimaryAttack|Cooldown")
	FGameplayTag CooldownDataTag;

	UPROPERTY(EditDefaultsOnly, Category = "PrimaryAttack|Cooldown")
	FGameplayTagContainer CooldownTagContainer;
};
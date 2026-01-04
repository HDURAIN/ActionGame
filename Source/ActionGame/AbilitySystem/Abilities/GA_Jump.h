// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Abilities/GameplayAbility.h"
#include "GA_Jump.generated.h"

class UGameplayEffect;

/**
 * Jump Gameplay Ability
 *
 * 设计原则：
 * - Ability 只负责“触发跳跃行为”
 * - 空中 / 跳跃状态由 GameplayEffect 负责（Grant Tags）
 * - 状态清理由 Character::Landed() 统一兜底
 *
 * 这是一个：
 * - 瞬发（Instant-like）
 * - 本地预测（LocalPredicted）
 * - 不需要持续 Tick 的 Ability
 */
UCLASS()
class ACTIONGAME_API UGA_Jump : public UGameplayAbility
{
	GENERATED_BODY()

public:
	UGA_Jump();

	/**
	 * 判断 Ability 是否可以被激活
	 *
	 * GAS 层面：
	 * - 是否被 Block
	 * - 是否在 Cooldown
	 * - 是否能支付 Cost
	 *
	 * 物理层面：
	 * - CharacterMovement::CanJump()
	 */
	virtual bool CanActivateAbility(
		const FGameplayAbilitySpecHandle Handle,
		const FGameplayAbilityActorInfo* ActorInfo,
		const FGameplayTagContainer* SourceTags,
		const FGameplayTagContainer* TargetTags,
		OUT FGameplayTagContainer* OptionalRelevantTags
	) const override;

	/**
	 * Ability 激活入口
	 *
	 * 执行流程：
	 * 1. CommitAbility（Cost / Cooldown）
	 * 2. 调用 Character::Jump()
	 * 3. Apply Jump GameplayEffect（Grant InAir / Jumping Tags）
	 */
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
	/**
	 * Jump 状态 GameplayEffect
	 *
	 * 典型配置：
	 * - DurationPolicy = Infinite
	 * - Granted Tags: State.InAir, State.InAir.Jumping
	 *
	 * 移除时机：
	 * - Character::Landed()
	 */
	UPROPERTY(EditDefaultsOnly, Category = "Effects")
	TSubclassOf<UGameplayEffect> JumpEffect;
};

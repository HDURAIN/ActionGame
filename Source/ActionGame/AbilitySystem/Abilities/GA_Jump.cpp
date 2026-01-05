// Fill out your copyright notice in the Description page of Project Settings.

#include "AbilitySystem/Abilities/GA_Jump.h"

#include "GameFramework/Character.h"
#include "AbilitySystemComponent.h"

UGA_Jump::UGA_Jump()
{
	// 本地预测执行：
	// 客户端立即响应输入以保证手感，
	// 服务器随后校验并同步最终结果
	NetExecutionPolicy = EGameplayAbilityNetExecutionPolicy::ServerOnly;

	// 每个 Avatar 拥有一个独立的 Ability 实例：
	// - 允许 Ability 内部保存临时状态
	// - 避免并发预测时状态互相覆盖
	InstancingPolicy = EGameplayAbilityInstancingPolicy::NonInstanced;
}


bool UGA_Jump::CanActivateAbility(
	const FGameplayAbilitySpecHandle Handle,
	const FGameplayAbilityActorInfo* ActorInfo,
	const FGameplayTagContainer* SourceTags,
	const FGameplayTagContainer* TargetTags,
	OUT FGameplayTagContainer* OptionalRelevantTags
) const
{
	// 执行 GAS 通用可激活性校验：
	// - 是否被 Blocked
	// - 是否处于 Cooldown
	// - 是否能支付 Cost
	// - ASC / Avatar 是否有效
	if (!Super::CanActivateAbility(
		Handle,
		ActorInfo,
		SourceTags,
		TargetTags,
		OptionalRelevantTags))
	{
		return false;
	}

	// 获取物理 Avatar（Character）
	const ACharacter* Character =
		Cast<ACharacter>(ActorInfo->AvatarActor.Get());

	// Avatar 不存在时不可激活
	if (!Character)
	{
		return false;
	}

	// 物理层跳跃合法性判断：
	// - 是否在地面
	// - JumpMaxCount
	// - MovementMode
	// - crouch 等
	return Character->CanJump();
}


void UGA_Jump::ActivateAbility(
	const FGameplayAbilitySpecHandle Handle,
	const FGameplayAbilityActorInfo* ActorInfo,
	const FGameplayAbilityActivationInfo ActivationInfo,
	const FGameplayEventData* TriggerEventData
)
{
	// 仅允许：
	// - 服务器
	// - 或持有有效 PredictionKey 的客户端
	// 执行核心逻辑	
	if (!HasAuthorityOrPredictionKey(ActorInfo, &ActivationInfo))
	{
		return;
	}

	// 提交 Ability：
	// - 扣除 Cost
	// - 应用 Cooldown
	// - 锁定 Ability，防止并发激活
	if (!CommitAbility(Handle, ActorInfo, ActivationInfo))
	{
		EndAbility(
			Handle,
			ActorInfo,
			ActivationInfo,
			true,   // 同步 Ability 结束
			true    // 标记为取消
		);
		return;
	}

	Super::ActivateAbility(Handle, ActorInfo, ActivationInfo, TriggerEventData);

	// 执行物理跳跃：
	// - 调用 CharacterMovement::DoJump
	// - 切换 MovementMode 为 Falling
	// - 驱动动画、位移与网络同步
	ACharacter* Character =
		CastChecked<ACharacter>(ActorInfo->AvatarActor.Get());

	Character->Jump();

	// 应用跳跃状态 GameplayEffect：
	// - Grant Tags（如 State.InAir.Jumping）
	// - GE 通常配置为 Infinite
	if (UAbilitySystemComponent* ASC =
		ActorInfo->AbilitySystemComponent.Get())
	{
		// 构建 Effect 上下文
		FGameplayEffectContextHandle EffectContext =
			ASC->MakeEffectContext();
		EffectContext.AddSourceObject(Character);

		// 创建 GE Spec
		FGameplayEffectSpecHandle SpecHandle =
			ASC->MakeOutgoingSpec(JumpEffect, 1.f, EffectContext);

		if (SpecHandle.IsValid())
		{
			// 将 GE 应用到自身（角色）
			ASC->ApplyGameplayEffectSpecToSelf(
				*SpecHandle.Data.Get());
		}
	}

	// Jump Ability 为瞬发 Ability：
	// - 不持有持续状态
	// - 状态由 GameplayEffect 管理
	// - 必须显式结束 Ability
	EndAbility(
		Handle,
		ActorInfo,
		ActivationInfo,
		false,
		false
	);
}


void UGA_Jump::EndAbility(
	const FGameplayAbilitySpecHandle Handle,
	const FGameplayAbilityActorInfo* ActorInfo,
	const FGameplayAbilityActivationInfo ActivationInfo,
	bool bReplicateEndAbility,
	bool bWasCancelled
)
{
	// Jump Ability 本身不管理状态，
	// 此处仅调用父类结束流程
	Super::EndAbility(
		Handle,
		ActorInfo,
		ActivationInfo,
		bReplicateEndAbility,
		bWasCancelled
	);
}

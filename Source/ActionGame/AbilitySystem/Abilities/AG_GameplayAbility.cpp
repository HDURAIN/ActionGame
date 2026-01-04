// Fill out your copyright notice in the Description page of Project Settings.


#include "AbilitySystem/Abilities/AG_GameplayAbility.h"
#include "ActionGameCharacter.h"
#include "AbilitySystemComponent.h"
#include <AbilitySystemLog.h>




/*
*	Handle :					这次Ability激活的唯一ID
*	ActorInfo:					Ability运行所需的一切上下文
*	ActivationInfo:				网络/预测相关信息
*	TriggerEventData:			如果是GameplayEvent触发，会有payload在其中
*/
void UAG_GameplayAbility::ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData)
{
	/*
	*	调用父类
	*	1、注册Ability为Active 2、初始化内部状态 3、处理预测/复制bookkeeping
	*/
	Super::ActivateAbility(Handle, ActorInfo, ActivationInfo, TriggerEventData);

	// 记录是谁施加的Effect 记录 来源对象 / HitResult / Instigator
	// * 从ActorInfo拿 *
	FGameplayEffectContextHandle EffectContext = ActorInfo->AbilitySystemComponent->MakeEffectContext();

	// Ability 激活时施加，但不需要在 EndAbility 时清理
	for (auto GameplayEffect : OngoingEffectsToJustApplyOnstart)
	{
		if (!GameplayEffect.Get()) continue;

		// 获取ASC
		if (UAbilitySystemComponent* AbilityComponent = ActorInfo->AbilitySystemComponent.Get())
		{	
			// 应用Effect的标准流程
			// SpecHandle = GameplayEffect 的“可执行规格说明书”
			FGameplayEffectSpecHandle SpecHandle = AbilityComponent->MakeOutgoingSpec(GameplayEffect, 1, EffectContext);
			if (SpecHandle.IsValid())
			{	
				// ActiveGEHandle = 已经成功应用到 ASC 上的 Effect 实例的唯一 ID
				FActiveGameplayEffectHandle ActiveGEHandle = AbilityComponent->ApplyGameplayEffectSpecToSelf(*SpecHandle.Data.Get());
				if (!ActiveGEHandle.WasSuccessfullyApplied())
				{
					ABILITY_LOG(Log, TEXT("Ability %s failed to apply startup effect %s"), *GetName(), *GetNameSafe(GameplayEffect));
				}
			}
		}
	}

	// Ability实例化才允许存成员变量状态
	// 否则所有角色公用一个Ability对象
	// 存Handle会互相覆盖
	if (IsInstantiated())
	{
		for (auto GameplayEffect : OngoingEffectsToRemoveOnEnd)
		{
			if (!GameplayEffect.Get()) continue;

			if (UAbilitySystemComponent* AbilityComponent = ActorInfo->AbilitySystemComponent.Get())
			{
				FGameplayEffectSpecHandle SpecHandle = AbilityComponent->MakeOutgoingSpec(GameplayEffect, 1, EffectContext);
				if (SpecHandle.IsValid())
				{
					FActiveGameplayEffectHandle ActiveGEHandle = AbilityComponent->ApplyGameplayEffectSpecToSelf(*SpecHandle.Data.Get());
					if (!ActiveGEHandle.WasSuccessfullyApplied())
					{
						ABILITY_LOG(Log, TEXT("Ability %s failed to apply runtime effect %s"), *GetName(), *GetNameSafe(GameplayEffect));
					}
					else
					{
						// 存Handle
						// 通常意味着：这个 Effect 的生命周期 = Ability 生命周期
						RemoveOnEndEffectHandles.Add(ActiveGEHandle);
					}
				}
			}
		}
	}
}

// 生命周期结束钩子
void UAG_GameplayAbility::EndAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, bool bReplicateEndAbility, bool bWasCancelled)
{
	if (IsInstantiated())
	{
		for (FActiveGameplayEffectHandle ActiveEffectHandle : RemoveOnEndEffectHandles)
		{
			if (ActiveEffectHandle.IsValid())
			{
				// 清理生命周期与Ability绑定的Effect
				ActorInfo->AbilitySystemComponent->RemoveActiveGameplayEffect(ActiveEffectHandle);
			}
		}

		// 清空数组
		// 避免下一次激活残留状态
		RemoveOnEndEffectHandles.Empty();
	}

	/*
	* 1、正式标记 Ability 为 Ended 2、通知客户端 / 服务器 3、释放预测 Key
	*/
	Super::EndAbility(Handle, ActorInfo, ActivationInfo, bReplicateEndAbility, bWasCancelled);
}

// 纯Helper 减少 Cast 重复代码
AActionGameCharacter* UAG_GameplayAbility::GetActionGameCharacterFromActorInfo() const
{
	return Cast<AActionGameCharacter>(GetAvatarActorFromActorInfo());
}

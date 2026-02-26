// Fill out your copyright notice in the Description page of Project Settings.

#include "AbilitySystem/Abilities/GA_PrimaryAttack.h"

#include "AbilitySystemComponent.h"
#include "AbilitySystemLog.h"

UGA_PrimaryAttack::UGA_PrimaryAttack()
{
	// 本地预测：客户端先响应输入，服务器校验并同步
	NetExecutionPolicy = EGameplayAbilityNetExecutionPolicy::LocalPredicted;

	// 每个角色一个独立实例，方便后续在Ability里保存运行时状态
	InstancingPolicy = EGameplayAbilityInstancingPolicy::InstancedPerActor;
}

void UGA_PrimaryAttack::ActivateAbility(
	const FGameplayAbilitySpecHandle Handle,
	const FGameplayAbilityActorInfo* ActorInfo,
	const FGameplayAbilityActivationInfo ActivationInfo,
	const FGameplayEventData* TriggerEventData
)
{
	// 基础校验（避免蓝图事件在无效上下文下执行）
	if (!ActorInfo || !ActorInfo->AvatarActor.IsValid())
	{
		UE_LOG(LogAbilitySystem, Warning,
			TEXT("[%s] ActivateAbility FAILED: invalid ActorInfo/Avatar"),
			*GetName());

		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
		return;
	}

	// 进入蓝图流程：
	// - PlayMontageAndWait
	// - WaitGameplayEvent(Shoot)
	K2_OnActivateFromEvent();
}

void UGA_PrimaryAttack::EndAbility(
	const FGameplayAbilitySpecHandle Handle,
	const FGameplayAbilityActorInfo* ActorInfo,
	const FGameplayAbilityActivationInfo ActivationInfo,
	bool bReplicateEndAbility,
	bool bWasCancelled
)
{
	Super::EndAbility(Handle, ActorInfo, ActivationInfo, bReplicateEndAbility, bWasCancelled);
}
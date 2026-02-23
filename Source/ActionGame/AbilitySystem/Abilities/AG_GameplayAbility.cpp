// Fill out your copyright notice in the Description page of Project Settings.

#include "AbilitySystem/Abilities/AG_GameplayAbility.h"
#include "ActionGameCharacter.h"
#include "AbilitySystemComponent.h"
#include <AbilitySystemLog.h>

void UAG_GameplayAbility::ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData)
{
	if (!ActorInfo || !ActorInfo->AvatarActor.IsValid())
	{
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
		return;
	}

	if (!CommitAbilityChecked())
	{
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
		return;
	}

	// ===== 自动Apply Start Effects =====
	if (UAbilitySystemComponent* ASC = GetASC())
	{
		for (auto EffectClass : EffectsOnStart)
		{
			if (!EffectClass) continue;

			// 创建Context
			FGameplayEffectContextHandle ContextHandle = ASC->MakeEffectContext();
			ContextHandle.AddSourceObject(this);
			// 创建Spec
			FGameplayEffectSpecHandle Spec = ASC->MakeOutgoingSpec(EffectClass, GetAbilityLevel(), ContextHandle);

			if (Spec.IsValid())
			{
				// 应用到角色身上
				FActiveGameplayEffectHandle Handle =
					ASC->ApplyGameplayEffectSpecToSelf(*Spec.Data.Get());

				// 记录句柄，技能结束时自动移除
				ActiveEffectHandles.Add(Handle);
			}
		}
	}

	// ===== 子类逻辑 =====
	if (HasAuthority(&CurrentActivationInfo))
	{
		OnAbilityActivated();
	}

	// ===== 蓝图表现 =====
	K2_OnAbilityActivated();

	// ===== 自动结束 =====
	if (bAutoEndAbility)
	{
		EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, true, false);
	}
}

void UAG_GameplayAbility::EndAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, bool bReplicateEndAbility, bool bWasCancelled)
{
	UAbilitySystemComponent* ASC = GetASC();

	// ===== Remove Start Effects (Server Only) =====
	if (ASC && HasAuthority(&ActivationInfo))
	{
		for (auto& EffectHandle : ActiveEffectHandles)
		{
			if (EffectHandle.IsValid())
			{
				ASC->RemoveActiveGameplayEffect(EffectHandle);
			}
		}
	}

	ActiveEffectHandles.Empty();

	if (ASC)
	{
		for (auto EffectClass : EffectsOnEnd)
		{
			if (!EffectClass) continue;

			FGameplayEffectContextHandle ContextHandle = ASC->MakeEffectContext();
			ContextHandle.AddSourceObject(this);

			FGameplayEffectSpecHandle Spec = ASC->MakeOutgoingSpec(EffectClass, GetAbilityLevel(), ContextHandle);

			if (Spec.IsValid())
			{
				ASC->ApplyGameplayEffectSpecToSelf(*Spec.Data.Get());
			}
		}
	}

	// ===== 子类结束逻辑 =====
	if (HasAuthority(&CurrentActivationInfo))
	{
		OnAbilityEnded(bWasCancelled);
	}

	// ===== 蓝图表现 =====
	K2_OnAbilityEnded(bWasCancelled);

	Super::EndAbility(Handle, ActorInfo, ActivationInfo, bReplicateEndAbility, bWasCancelled);
}

void UAG_GameplayAbility::OnAbilityActivated()
{
}

void UAG_GameplayAbility::OnAbilityEnded(bool bWasCancelled)
{
}

AActionGameCharacter* UAG_GameplayAbility::GetCharacter() const
{
	return Cast<AActionGameCharacter>(GetAvatarActorFromActorInfo());
}

UAbilitySystemComponent* UAG_GameplayAbility::GetASC() const
{
	return GetAbilitySystemComponentFromActorInfo();
}

bool UAG_GameplayAbility::CommitAbilityChecked()
{
	if (!CommitAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo))
	{
		UE_LOG(LogAbilitySystem, Warning, TEXT("[%s] CommitAbility FAILED"), *GetName());
		return false;
	}
	return true;
}
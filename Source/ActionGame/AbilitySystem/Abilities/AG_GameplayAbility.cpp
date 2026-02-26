// Fill out your copyright notice in the Description page of Project Settings.

#include "AbilitySystem/Abilities/AG_GameplayAbility.h"
#include "ActionGameCharacter.h"
#include "AbilitySystemComponent.h"
#include <AbilitySystemLog.h>

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
	// 1) 基础上下文保护
	if (!CurrentActorInfo)
	{
		UE_LOG(LogAbilitySystem, Warning,
			TEXT("[%s] CommitAbility FAILED: CurrentActorInfo is null"),
			*GetName());
		return false;
	}

	UAbilitySystemComponent* ASC = GetASC();
	if (!ASC)
	{
		UE_LOG(LogAbilitySystem, Warning,
			TEXT("[%s] CommitAbility FAILED: ASC is null"),
			*GetName());
		return false;
	}

	if (!CurrentSpecHandle.IsValid())
	{
		const AActor* AvatarActor = CurrentActorInfo->AvatarActor.Get();
		UE_LOG(LogAbilitySystem, Warning,
			TEXT("[%s] CommitAbility FAILED: CurrentSpecHandle is invalid (Avatar=%s)"),
			*GetName(),
			*GetNameSafe(AvatarActor));
		return false;
	}

	// 2) 真正提交（消耗 / 冷却等）
	if (!CommitAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo))
	{
		const AActor* AvatarActor = CurrentActorInfo->AvatarActor.Get();
		UE_LOG(LogAbilitySystem, Warning,
			TEXT("[%s] CommitAbility FAILED: CommitAbility returned false (Avatar=%s)"),
			*GetName(),
			*GetNameSafe(AvatarActor));
		return false;
	}

	return true;
}
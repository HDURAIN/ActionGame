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

protected:
	/* ---------- 快捷获取 ---------- */
	UFUNCTION(BlueprintPure, Category = "Ability")
	AActionGameCharacter* GetCharacter() const;

	UFUNCTION(BlueprintPure, Category = "Ability")
	UAbilitySystemComponent* GetASC() const;

	/** Commit封装（统一报错日志） */
	bool CommitAbilityChecked();
};

// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "AbilitySystem/Abilities/AG_GameplayAbility.h"
#include "GA_PrimaryAttack.generated.h"

/**
 * 
 */
UCLASS()
class ACTIONGAME_API UGA_PrimaryAttack : public UAG_GameplayAbility
{
	GENERATED_BODY()

	/** 技能真正逻辑 */
	virtual void OnAbilityActivated() override;

	/** 技能结束逻辑 */
	virtual void OnAbilityEnded(bool bWasCancelled) override;

	bool DoCameraTrace(FHitResult& OutHit, FVector& OutAimPoint);

	void DoWeaponTrace(const FVector& AimPoint);
};

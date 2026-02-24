// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "AbilitySystem/Abilities/AG_GameplayAbility.h"
#include "GameplayEffectTypes.h"
#include "GA_PrimaryAttack.generated.h"

class UGameplayEffect;
class UNiagaraSystem;
/**
 * 
 */
UCLASS()
class ACTIONGAME_API UGA_PrimaryAttack : public UAG_GameplayAbility
{
	GENERATED_BODY()

	UGA_PrimaryAttack();

protected:
	UPROPERTY(EditDefaultsOnly, Category = "Animation")
	UAnimMontage* FireMontage;

	/** 技能真正逻辑 */
	virtual void OnAbilityActivated() override;

	/** 技能结束逻辑 */
	virtual void OnAbilityEnded(bool bWasCancelled) override;

	bool DoCameraTrace(FHitResult& OutHit, FVector& OutAimPoint);

	void DoWeaponTrace(const FVector& AimPoint);

	void DoTrace();

	void ApplyEffect();

	UPROPERTY(EditDefaultsOnly, Category = "Sprint")
	TSubclassOf<UGameplayEffect> FireStateEffect;

	FActiveGameplayEffectHandle FireEffectHandle;

	UPROPERTY(EditDefaultsOnly, Category = "VFX")
	TObjectPtr<UNiagaraSystem> ImpactNiagara = nullptr;
};

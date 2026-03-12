// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "AbilitySystem/Abilities/AG_GameplayAbility.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GA_Ultimate.generated.h"

class UGameplayEffect;
struct FGameplayEventData;

UCLASS()
class ACTIONGAME_API UGA_Ultimate : public UAG_GameplayAbility
{
	GENERATED_BODY()

public:
	UGA_Ultimate();

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

	/** Called from AnimNotify: server applies one wave of AoE damage + knockback. */
	UFUNCTION(BlueprintCallable, Category = "Ultimate")
	void TriggerUltimateWave();

	/** Called when the Ultimate montage ends or is interrupted. */
	UFUNCTION(BlueprintCallable, Category = "Ultimate")
	void NotifyUltimateMontageFinished(bool bWasCancelled);

protected:
	/** Blueprint should play Ultimate montage and call TriggerUltimateWave from notifies. */
	UFUNCTION(BlueprintImplementableEvent, Category = "Ultimate")
	void K2_OnUltimateActivated();

private:
	void EnterUltimateMovementState();
	void ExitUltimateMovementState();
	void ApplyWaveDamageAndKnockback();

private:
	// Damage
	UPROPERTY(EditDefaultsOnly, Category = "Ultimate|Damage")
	TSubclassOf<UGameplayEffect> DamageEffectClass = nullptr;

	UPROPERTY(EditDefaultsOnly, Category = "Ultimate|Damage")
	FGameplayTag DamageDataTag;

	/** Damage formula per wave: AttackPower * DamageMultiplier * UltimateDamage. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Ultimate|Damage", meta = (AllowPrivateAccess = "true", ClampMin = "0.0"))
	float UltimateDamage = 1.0f;

	// Waves
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Ultimate|Wave", meta = (AllowPrivateAccess = "true", ClampMin = "1"))
	int32 WaveCount = 6;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Ultimate|Wave", meta = (AllowPrivateAccess = "true", ClampMin = "0.0"))
	float WaveRadius = 450.f;

	// Movement
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Ultimate|Movement", meta = (AllowPrivateAccess = "true", ClampMin = "0.0"))
	float UltimateBaseMoveSpeed = 180.f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Ultimate|Movement", meta = (AllowPrivateAccess = "true"))
	float UltimateGravityScale = 0.0f;

	// Knockback
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Ultimate|Knockback", meta = (AllowPrivateAccess = "true", ClampMin = "0.0"))
	float KnockbackStrength = 900.f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Ultimate|Knockback", meta = (AllowPrivateAccess = "true", ClampMin = "0.0"))
	float KnockbackUpward = 180.f;

private:
	UPROPERTY(Transient)
	int32 TriggeredWaveCount = 0;

	UPROPERTY(Transient)
	bool bMovementStateApplied = false;

	UPROPERTY(Transient)
	TEnumAsByte<EMovementMode> CachedMovementMode = MOVE_Walking;

	UPROPERTY(Transient)
	uint8 CachedCustomMovementMode = 0;

	UPROPERTY(Transient)
	float CachedGravityScale = 1.f;
};

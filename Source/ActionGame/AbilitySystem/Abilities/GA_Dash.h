// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "AbilitySystem/Abilities/AG_GameplayAbility.h"
#include "GA_Dash.generated.h"

class UAbilityTask_WaitDelay;
class UAnimSequenceBase;

UCLASS()
class ACTIONGAME_API UGA_Dash : public UAG_GameplayAbility
{
	GENERATED_BODY()

public:
	UGA_Dash();

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

private:
	FVector ComputeDashDirection(const FGameplayAbilityActorInfo* ActorInfo) const;

	UFUNCTION()
	void OnDashFinished();

private:
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Dash", meta = (AllowPrivateAccess = "true", ClampMin = "0.01"))
	float DashDuration = 0.20f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Dash", meta = (AllowPrivateAccess = "true", ClampMin = "0.0"))
	float DashDistance = 650.0f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Dash", meta = (AllowPrivateAccess = "true", ClampMin = "0.0"))
	float MinInputThreshold = 0.10f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Dash", meta = (AllowPrivateAccess = "true", ClampMin = "0.0"))
	float MinVelocityThreshold = 10.0f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Dash", meta = (AllowPrivateAccess = "true"))
	bool bAllowAirDash = false;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Dash", meta = (AllowPrivateAccess = "true"))
	bool bRotateToDashDirection = true;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Dash|Animation", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UAnimSequenceBase> DashAnimation = nullptr;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Dash|Animation", meta = (AllowPrivateAccess = "true", ClampMin = "0.01"))
	float DashAnimPlayRate = 1.0f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Dash|Animation", meta = (AllowPrivateAccess = "true"))
	FName DashAnimSlotName = TEXT("DefaultSlot");

	UPROPERTY(Transient)
	TObjectPtr<UAbilityTask_WaitDelay> DashWaitTask = nullptr;

	UPROPERTY(Transient)
	float CachedAirControl = 0.0f;

	UPROPERTY(Transient)
	bool bAirControlCached = false;

	UPROPERTY(Transient)
	bool bRootMotionScaleOverridden = false;
};

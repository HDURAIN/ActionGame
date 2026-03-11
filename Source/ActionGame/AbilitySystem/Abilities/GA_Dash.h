#pragma once

#include "CoreMinimal.h"
#include "AbilitySystem/Abilities/AG_GameplayAbility.h"
#include "GA_Dash.generated.h"

class UAnimMontage;
class UAbilityTask_ApplyRootMotionConstantForce;
class UAbilityTask_PlayMontageAndWait;

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
	FVector ComputeDashDirection() const;

	UFUNCTION()
	void OnDashMoveFinished();

private:
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Dash", meta = (AllowPrivateAccess = "true", ClampMin = "0.0"))
	float DashDistance = 650.0f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Dash", meta = (AllowPrivateAccess = "true", ClampMin = "0.01"))
	float DashDuration = 0.93f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Dash", meta = (AllowPrivateAccess = "true", ClampMin = "0.0"))
	float MinInputThreshold = 0.10f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Dash", meta = (AllowPrivateAccess = "true"))
	bool bAllowAirDash = false;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Dash", meta = (AllowPrivateAccess = "true"))
	bool bRotateToDashDirection = true;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Dash|Animation", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UAnimMontage> DashMontage = nullptr;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Dash|Animation", meta = (AllowPrivateAccess = "true", ClampMin = "0.01"))
	float DashAnimPlayRate = 1.0f;

	UPROPERTY(Transient)
	TObjectPtr<UAbilityTask_ApplyRootMotionConstantForce> DashMoveTask = nullptr;

	UPROPERTY(Transient)
	TObjectPtr<UAbilityTask_PlayMontageAndWait> DashMontageTask = nullptr;

	UPROPERTY(Transient)
	bool bMoveInputBlockedByDash = false;
};

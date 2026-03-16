// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "AbilitySystem/Abilities/AG_GameplayAbility.h"
#include "GA_SecondAttack.generated.h"

class UGameplayEffect;
class UAbilitySystemComponent;
struct FGameplayEventData;

UCLASS()
class ACTIONGAME_API UGA_SecondAttack : public UAG_GameplayAbility
{
	GENERATED_BODY()

public:
	UGA_SecondAttack();

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

	UFUNCTION(BlueprintCallable, Category = "SecondAttack")
	bool Fire_PenetratingSweep(
		const FVector& StartLocation,
		FVector& OutActualStart,
		FVector& OutActualEnd,
		float& OutActualLength,
		bool& bOutBlockedByWorld
	);

	void ServerExecutePenetratingSweepFromClient(const FVector& StartLocation);

	UFUNCTION(BlueprintCallable, Category = "SecondAttack")
	void NotifySecondAttackMontageFinished(bool bWasCancelled);

	/** Faces actor to camera yaw and blocks movement input until released/end. */
	UFUNCTION(BlueprintCallable, Category = "SecondAttack|Control")
	void FaceToCameraAndLockMoveInput();

	/** Manually releases movement lock from FaceToCameraAndLockMoveInput. */
	UFUNCTION(BlueprintCallable, Category = "SecondAttack|Control")
	void ReleaseMoveInputLock();

	/** Returns beam start/end using the same muzzle and aim logic as damage sweep. */
	UFUNCTION(BlueprintCallable, Category = "SecondAttack|FX")
	bool GetSecondAttackBeamStartEnd(FVector& OutStart, FVector& OutEnd) const;

protected:
	virtual const FGameplayTagContainer* GetCooldownTags() const override;

	virtual void ApplyCooldown(
		const FGameplayAbilitySpecHandle Handle,
		const FGameplayAbilityActorInfo* ActorInfo,
		const FGameplayAbilityActivationInfo ActivationInfo
	) const override;

	UFUNCTION(BlueprintImplementableEvent, Category = "SecondAttack")
	void K2_OnActivateFromEvent();

private:
	void UpdateFacingToCamera();
	void StartFacingToCameraLoop();
	void StopFacingToCameraLoop();

	void ExecutePenetratingSweepServer(const FVector& StartLocation);
	bool IsClientProvidedStartLocationValid(const FVector& StartLocation) const;
	bool ComputeActualSweepSegment(
		const FVector& StartLocation,
		FVector& OutSweepStart,
		FVector& OutSweepEnd,
		float& OutSweepLength,
		bool& bOutBlockedByWorld
	) const;

	void ApplyDamageToTargetASC(
		UAbilitySystemComponent* SourceASC,
		UAbilitySystemComponent* TargetASC,
		const FHitResult& Hit
	) const;

	bool BuildAimDirectionFromCamera(const FVector& StartLocation, FVector& OutAimDir) const;
	bool BuildAimFromCameraAndMuzzle(FVector& OutMuzzleLoc, FVector& OutAimDir) const;
	bool TryGetRightWeaponMuzzleLocation(FVector& OutMuzzleLoc) const;

private:
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "SecondAttack|Trace", meta = (AllowPrivateAccess = "true", ClampMin = "1.0"))
	float TraceDistance = 10000.f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "SecondAttack|Trace", meta = (AllowPrivateAccess = "true", ClampMin = "1.0"))
	float SweepRange = 2600.f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "SecondAttack|Trace", meta = (AllowPrivateAccess = "true", ClampMin = "1.0"))
	float SweepRadius = 90.f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "SecondAttack|Trace", meta = (AllowPrivateAccess = "true"))
	FName MuzzleSocketName = TEXT("Muzzle");

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "SecondAttack|Trace", meta = (AllowPrivateAccess = "true"))
	FName WeaponTraceChannelName = TEXT("WeaponTrace");

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "SecondAttack|Trace", meta = (AllowPrivateAccess = "true"))
	FName BlockTraceChannelName = TEXT("AbilityBlock");

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "SecondAttack|Trace", meta = (AllowPrivateAccess = "true"))
	bool bStopOnBlockingWorld = true;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "SecondAttack|Trace", meta = (AllowPrivateAccess = "true", ClampMin = "0.0"))
	float MaxClientStartDistanceFromCharacter = 500.f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "SecondAttack|Trace", meta = (AllowPrivateAccess = "true", ClampMin = "0.0"))
	float MaxClientStartDistanceFromWeaponActor = 300.f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "SecondAttack|Tuning", meta = (AllowPrivateAccess = "true", ClampMin = "0.0"))
	float BaseCooldown = 5.0f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "SecondAttack|Tuning", meta = (AllowPrivateAccess = "true", ClampMin = "0.0"))
	float DamageCoefficient = 2.0f;

	UPROPERTY(EditDefaultsOnly, Category = "SecondAttack|Damage")
	TSubclassOf<UGameplayEffect> DamageEffectClass = nullptr;

	UPROPERTY(EditDefaultsOnly, Category = "SecondAttack|Damage")
	FGameplayTag DamageDataTag;

	UPROPERTY(EditDefaultsOnly, Category = "SecondAttack|Cooldown")
	FGameplayTag CooldownDataTag;

	UPROPERTY(EditDefaultsOnly, Category = "SecondAttack|Cooldown")
	FGameplayTagContainer CooldownTagContainer;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "SecondAttack|Debug", meta = (AllowPrivateAccess = "true"))
	bool bDebugDrawSweep = false;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "SecondAttack|Debug", meta = (AllowPrivateAccess = "true", ClampMin = "0.0"))
	float DebugDrawDuration = 1.0f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "SecondAttack|Control", meta = (AllowPrivateAccess = "true"))
	bool bFaceCameraUntilAbilityEnd = true;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "SecondAttack|Control", meta = (AllowPrivateAccess = "true", ClampMin = "0.01"))
	float FaceCameraUpdateInterval = 0.02f;

	UPROPERTY(Transient)
	bool bMoveInputLockedBySecondAttack = false;

	FTimerHandle FaceCameraLoopTimerHandle;
};

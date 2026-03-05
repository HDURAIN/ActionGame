// EnemyCharacterBase.h

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "AbilitySystemInterface.h"
#include "GameplayTagContainer.h"
#include "AbilitySystemComponent.h"
#include "GameplayEffect.h"
#include "ActionGameTypes.h"
#include "EnemyCharacterBase.generated.h"

class UAbilitySystemComponent;
class UAG_EnemyAttributeSet;

/**
 * Base enemy character:
 * - Owns GAS ASC + Enemy AttributeSet
 * - Locks onto nearest alive player Character at spawn
 * - Listens to target's ASC for State.Dead; when dead, reacquires and rebinds
 *
 * Assumptions:
 * - Player characters have ASC on the Character actor
 * - Death is represented by gameplay tag: State.Dead
 */
UCLASS(Abstract)
class ACTIONGAME_API AEnemyCharacterBase : public ACharacter, public IAbilitySystemInterface
{
	GENERATED_BODY()

public:
	AEnemyCharacterBase();

	// IAbilitySystemInterface
	virtual UAbilitySystemComponent* GetAbilitySystemComponent() const override;

	/** SpawnManager 生成后灌入本次生成配置（Demo 先用最小字段） */
	void ApplySpawnEntryConfig(const FEnemySpawnEntry& InConfig);

public:
	// =========================================================================
	// Death
	// =========================================================================
	void StartRagdoll();

	void OnHealthAttributeChanged(const FOnAttributeChangeData& Data);

protected:
	// =========================================================================
	// Death
	// =========================================================================

	UFUNCTION()
	void OnRagdollStateTagChanged(const FGameplayTag CallbackTag, int32 NewCount);

protected:
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

	// =========================
	// GAS
	// =========================
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "GAS", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UAbilitySystemComponent> AbilitySystemComponent;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "GAS", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UAG_EnemyAttributeSet> EnemyAttributeSet;

	UPROPERTY(EditDefaultsOnly, Category = "Abilities|Events")
	FGameplayTag ZeroHealthEventTag;

	// =========================
	// Targeting
	// =========================
	UFUNCTION(BlueprintPure, Category = "Enemy|Target")
	ACharacter* GetTargetCharacter() const { return TargetCharacter.Get(); }

	UFUNCTION(BlueprintCallable, Category = "Enemy|Target")
	void ReacquireTarget();

	virtual bool IsValidTargetCandidate(ACharacter* Candidate) const;
	virtual float ComputeTargetScoreSq(const ACharacter* Candidate) const;

protected:
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Enemy|Target")
	FGameplayTag DeadTag;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Abilities|Death")
	TSubclassOf<UGameplayAbility> DeathAbilityClass;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Abilities|Startup")
	TArray<TSubclassOf<UGameplayEffect>> StartupEffects;

	/** 测试用：BeginPlay 后延迟一次重新锁定目标（解决开局时序）。0 表示不延迟 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Enemy|Target")
	float InitialAcquireDelay = 0.5f;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Enemy|Runtime")
	EEnemyMovementType EnemyMovementType = EEnemyMovementType::Ground;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Enemy|Runtime")
	float TargetAcceptanceRadius = 100.f;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Enemy|Runtime")
	bool bUsePathfinding = true;

public:
	UFUNCTION(BlueprintPure, Category = "Enemy|Movement")
	EEnemyMovementType GetEnemyMovementType() const { return EnemyMovementType; }

	UFUNCTION(BlueprintPure, Category = "Enemy|Movement")
	float GetTargetAcceptanceRadius() const { return TargetAcceptanceRadius; }

	UFUNCTION(BlueprintPure, Category = "Enemy|Movement")
	bool ShouldUsePathfinding() const { return bUsePathfinding; }

private:
	UPROPERTY(Transient)
	TWeakObjectPtr<ACharacter> TargetCharacter;

	FDelegateHandle DeadTagChangedHandle;

	FTimerHandle ReacquireRetryHandle;

private:
	void SetTarget(ACharacter* NewTarget);
	ACharacter* FindNearestAliveCharacter() const;

	bool IsCharacterDead(const ACharacter* Character) const;
	UAbilitySystemComponent* GetASC(AActor* Actor) const;

	void BindToTargetDeath(ACharacter* Target);
	void UnbindFromTargetDeath();
	void OnTargetDeadTagChanged(const FGameplayTag Tag, int32 NewCount);
	void GiveDeathAbility();
	void ApplyStartupEffects();
};
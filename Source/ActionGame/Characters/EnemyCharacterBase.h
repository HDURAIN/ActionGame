// EnemyCharacterBase.h

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "AbilitySystemInterface.h"
#include "GameplayTagContainer.h"
#include "AbilitySystemComponent.h"
#include "GameplayEffectTypes.h"
#include "ActionGameTypes.h"
#include "EnemyCharacterBase.generated.h"

class UAG_EnemyAttributeSet;
class UGameplayEffect;
class UGameplayAbility;

UCLASS(Abstract)
class ACTIONGAME_API AEnemyCharacterBase : public ACharacter, public IAbilitySystemInterface
{
	GENERATED_BODY()

public:
	AEnemyCharacterBase();

	virtual UAbilitySystemComponent* GetAbilitySystemComponent() const override;

	void ApplySpawnEntryConfig(const FEnemySpawnEntry& InConfig);

	/** BT Service 调用：选择目标 */
	UFUNCTION(BlueprintCallable, Category = "Enemy|Target")
	ACharacter* FindBestTarget() const;

public:
	// Death / Ragdoll
	void StartRagdoll();

	void OnHealthAttributeChanged(const FOnAttributeChangeData& Data);

protected:
	UFUNCTION()
	void OnRagdollStateTagChanged(const FGameplayTag CallbackTag, int32 NewCount);

protected:
	virtual void BeginPlay() override;

protected:
	// GAS
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "GAS", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UAbilitySystemComponent> AbilitySystemComponent;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "GAS", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UAG_EnemyAttributeSet> EnemyAttributeSet;

	UPROPERTY(EditDefaultsOnly, Category = "Abilities|Events")
	FGameplayTag ZeroHealthEventTag;

protected:
	// Targeting helpers
	virtual bool IsValidTargetCandidate(ACharacter* Candidate) const;
	virtual float ComputeTargetScoreSq(const ACharacter* Candidate) const;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Enemy|Target")
	FGameplayTag DeadTag;

protected:
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Abilities|Death")
	TSubclassOf<UGameplayAbility> DeathAbilityClass;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Abilities|Startup")
	TArray<TSubclassOf<UGameplayEffect>> StartupEffects;

protected:
	// Runtime config
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Enemy|Runtime")
	EEnemyMovementType EnemyMovementType = EEnemyMovementType::Ground;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Enemy|Runtime")
	float TargetAcceptanceRadius = 100.f;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Enemy|Runtime")
	bool bUsePathfinding = true;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Enemy|Runtime")
	float AttackRange = 150.f;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Enemy|Runtime")
	float AttackCooldown = 1.0f;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Enemy|Runtime")
	bool bCanAttack = true;

public:
	UFUNCTION(BlueprintPure, Category = "Enemy|Movement")
	float GetTargetAcceptanceRadius() const { return TargetAcceptanceRadius; }

	UFUNCTION(BlueprintPure, Category = "Enemy|Movement")
	bool ShouldUsePathfinding() const { return bUsePathfinding; }

	UFUNCTION(BlueprintPure, Category = "Enemy|Combat")
	float GetAttackRange() const { return AttackRange; }

	UFUNCTION(BlueprintPure, Category = "Enemy|Combat")
	float GetAttackCooldown() const { return AttackCooldown; }

	UFUNCTION(BlueprintPure, Category = "Enemy|Combat")
	bool CanAttack() const { return bCanAttack; }

	// 攻击接口 由子类实现具体攻击逻辑（如近战攻击、远程攻击等）
	UFUNCTION(BlueprintCallable, Category = "Enemy|Combat")
	virtual void PerformAttack(AActor* TargetActor);

private:
	ACharacter* FindNearestAliveCharacter() const;
	bool IsCharacterDead(const ACharacter* Character) const;
	UAbilitySystemComponent* GetASC(AActor* Actor) const;

	void GiveDeathAbility();
	void ApplyStartupEffects();
	void ApplyMovementTypeConfig();
};
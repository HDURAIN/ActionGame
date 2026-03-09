// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "ActionGameTypes.generated.h"

class AEnemyCharacterBase;
class UEnemyConfigDataAsset;

USTRUCT(BlueprintType)
struct FCharacterData
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "GAS")
	TArray<TSubclassOf<class UGameplayEffect>> Effects;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "GAS")
	TArray<TSubclassOf<class UGameplayAbility>> Abilities;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "AnimDataAsset")
	class UCharacterAnimDataAsset* CharacterAnimDataAsset;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "AbilitySetDataAsset")
	class UAbilitySetDataAsset* CharacterSkillDataAsset;
};

USTRUCT(BlueprintType)
struct FCharacterAnimationData
{
	GENERATED_BODY()

	UPROPERTY(EditDefaultsOnly)
	class UBlendSpace* MovementBlendspace = nullptr;

	UPROPERTY(EditDefaultsOnly)
	class UAnimSequenceBase* IdleAnimationAsset = nullptr;

	UPROPERTY(EditDefaultsOnly)
	class UBlendSpace* CrouchMovementBlendspace = nullptr;

	UPROPERTY(EditDefaultsOnly)
	class UAnimSequenceBase* CrouchIdleAnimationAsset = nullptr;

	UPROPERTY(EditDefaultsOnly)
	class UBlendSpace* FireMovementBlendSpace = nullptr;

	UPROPERTY(EditDefaultsOnly)
	class UAnimSequenceBase* FireIdleAnimationAsset = nullptr;
};

USTRUCT(BlueprintType)
struct FInteractDisplayData
{
	GENERATED_BODY()

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly)
	FText DisplayName;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly)
	int32 Cost = 0;
};

USTRUCT(BlueprintType)
struct FAbilitySetData
{
	GENERATED_BODY()

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly)
	TArray<TSubclassOf<class UGameplayAbility>> Abilities;
};

UENUM(BlueprintType)
enum class EEnemyMovementType : uint8
{
	Ground UMETA(DisplayName = "Ground"),
	Flying UMETA(DisplayName = "Flying")
};

USTRUCT(BlueprintType)
struct FEnemyConfigData
{
	GENERATED_BODY()

	/** 移动类型：走地 / 飞行 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Movement")
	EEnemyMovementType MovementType = EEnemyMovementType::Ground;

	/** MoveTo 的接受半径 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Movement", meta = (ClampMin = "0.0"))
	float AcceptanceRadius = 100.f;

	/** 是否启用寻路 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Movement")
	bool bUsePathfinding = true;

	/** 进入攻击状态的距离（<= 就打） */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Combat", meta = (ClampMin = "0.0"))
	float AttackRange = 150.f;

	/** 攻击间隔（秒） */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Combat", meta = (ClampMin = "0.01"))
	float AttackCooldown = 1.0f;

	// 伤害类型
	UPROPERTY(EditDefaultsOnly, Category = "PrimaryAttack|Damage")
	TSubclassOf<UGameplayEffect> DamageEffectClass = nullptr;

	/** 基础伤害 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Combat", meta = (ClampMin = "0.01"))
	float BaseAttackPower = 10.f;

	/** 伤害倍率 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Combat", meta = (ClampMin = "0.01"))
	float AttackMultiplier = 10.f;

	/** 击杀奖励金币 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Reward", meta = (ClampMin = "0.01"))
	float BountyGold = 126.f;

	/** 生命值 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Health", meta = (ClampMin = "0.01"))
	float Health = 100.f;

	/** 最大生命值 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Health", meta = (ClampMin = "0.01"))
	float MaxHealth = 100.f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Combat")
	bool bCanAttack = true;
};

USTRUCT(BlueprintType)
struct FEnemySpawnEntry
{
	GENERATED_BODY()

	/** 要生成的敌人类 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Spawn")
	TSubclassOf<AEnemyCharacterBase> EnemyClass;

	/** 权重 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Spawn", meta = (ClampMin = "0"))
	int32 Weight = 1;

	/** 生成高度偏移 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Spawn", meta = (ClampMin = "0.0"))
	float SpawnHeightOffset = 0.f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Config")
	TObjectPtr<UEnemyConfigDataAsset> EnemyConfig;
};

UENUM(BlueprintType)
enum class EFoot : uint8
{	
	// UMETA - UE元数据宏，为枚举值添加额外信息
	Left UMETA(DisplayName = "Left"),
	Right UMETA(DisplayName = "Right")
};

UENUM(BlueprintType)
enum class EInteractType : uint8
{
	Pickup UMETA(DisplayName = "Pickup"),
	Use	UMETA(DisplayName = "Use"),
	None UMETA(DisplayName = "None")
};

// 已废弃
UENUM(BlueprintType)
enum class EAbilityInputID : uint8
{
	None   UMETA(DisplayName = "None"),
	Skill1 UMETA(DisplayName = "Skill1"),
	Skill2 UMETA(DisplayName = "Skill2"),
	Skill3 UMETA(DisplayName = "Skill3"),
	Skill4 UMETA(DisplayName = "Skill4")
};

UENUM(BlueprintType)
enum class EAbilityType : uint8
{
	Active,
	Passive,
	Toggle
};

namespace InteractTags
{
	static const FName InteractTarget(TEXT("InteractTarget"));
}
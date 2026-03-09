#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "ActionGameTypes.h"
#include "EnemySpawnManager.generated.h"

class AEnemyCharacterBase;

UCLASS()
class ACTIONGAME_API AEnemySpawnManager : public AActor
{
	GENERATED_BODY()

public:
	AEnemySpawnManager();

protected:
	virtual void BeginPlay() override;

public:
	/** 开启/关闭刷怪 */
	UFUNCTION(BlueprintCallable, Category = "Spawn")
	void SetSpawningEnabled(bool bEnabled);

	/** 设置刷怪围绕的目标 */
	UFUNCTION(BlueprintCallable, Category = "Spawn")
	void SetFocusActor(AActor* NewFocus);

	/** 立即尝试刷一只，便于调试/特殊波次调用 */
	UFUNCTION(BlueprintCallable, Category = "Spawn")
	void SpawnEnemyOnce();

	/** 当前是否允许刷怪 */
	UFUNCTION(BlueprintPure, Category = "Spawn")
	bool IsSpawningEnabled() const { return bSpawningEnabled; }

	/** 剩余可生成数量 */
	UFUNCTION(BlueprintPure, Category = "Spawn")
	int32 GetRemainingSpawnCount() const;

	/** 是否无限生成 */
	UFUNCTION(BlueprintPure, Category = "Spawn")
	bool IsInfiniteSpawnEnabled() const { return bSpawnInfinitely; }

private:
	void StartSpawnTimer();
	void StopSpawnTimer();
	void HandleSpawnTimerElapsed();
	bool CanSpawn() const;

	/** 只计算基础随机位置，不含具体敌人的额外高度偏移 */
	bool FindSpawnLocation(FVector& OutSpawnLocation) const;

	/** 实际执行生成 */
	bool SpawnEnemyInternal();

	/** 按权重挑选生成 Entry */
	bool PickEnemySpawnEntryWeighted(FEnemySpawnEntry& OutEntry) const;

private:
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Spawn", meta = (AllowPrivateAccess = "true"))
	bool bSpawningEnabled = false;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Spawn", meta = (AllowPrivateAccess = "true"))
	bool bSpawnInfinitely = false;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Spawn",
		meta = (ClampMin = "0", EditCondition = "!bSpawnInfinitely", AllowPrivateAccess = "true"))
	int32 TotalEnemyToSpawn = 20;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Spawn", meta = (AllowPrivateAccess = "true"))
	int32 SpawnedEnemyCount = 0;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Spawn",
		meta = (ClampMin = "0.01", AllowPrivateAccess = "true"))
	float SpawnInterval = 2.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Spawn",
		meta = (ClampMin = "0.0", AllowPrivateAccess = "true"))
	float SpawnRadiusMin = 600.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Spawn",
		meta = (ClampMin = "0.0", AllowPrivateAccess = "true"))
	float SpawnRadiusMax = 1200.0f;

	/** 所有敌人的通用基础高度偏移 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Spawn", meta = (AllowPrivateAccess = "true"))
	float SpawnHeightOffset = 0.0f;

	// 各类型Enemy生成配置表
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Spawn", meta = (AllowPrivateAccess = "true"))
	TArray<FEnemySpawnEntry> EnemySpawnTable;

	UPROPERTY(Transient)
	TWeakObjectPtr<AActor> FocusActor;

	FTimerHandle SpawnTimerHandle;
};
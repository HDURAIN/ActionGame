// Fill out your copyright notice in the Description page of Project Settings.

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

	// 是否是无限生成怪物
	UFUNCTION(BlueprintPure, Category = "Spawn")
	bool IsInfiniteSpawnEnabled() const { return bSpawnInfinitely; }

private:
	/** 刷怪计时逻辑 */
	void StartSpawnTimer();
	void StopSpawnTimer();

	/** 定时器回调 */
	void HandleSpawnTimerElapsed();

	/** 是否还能继续生成 */
	bool CanSpawn() const;

	/** 计算生成位置 */
	bool FindSpawnLocation(FVector& OutSpawnLocation) const;

	/** 实际执行生成 */
	bool SpawnEnemyInternal();

	// 按照权重挑选生成Enemy的类
	bool PickEnemyClassWeighted(TSubclassOf<AEnemyCharacterBase>& OutEnemyClass) const;

private:
	/** 是否开启刷怪 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Spawn", meta = (AllowPrivateAccess = "true"))
	bool bSpawningEnabled = false;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Spawn", meta = (AllowPrivateAccess = "true"))
	bool bSpawnInfinitely = false;

	/** 本局/本关总共允许生成多少只 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Spawn",
		meta = (ClampMin = "0", EditCondition = "!bSpawnInfinitely", AllowPrivateAccess = "true"))
	int32 TotalEnemyToSpawn = 20;

	/** 已生成数量 */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Spawn", meta = (AllowPrivateAccess = "true"))
	int32 SpawnedEnemyCount = 0;

	/** 生成间隔（秒） */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Spawn", meta = (ClampMin = "0.01", AllowPrivateAccess = "true"))
	float SpawnInterval = 2.0f;

	/** 围绕 FocusActor 的最小生成半径 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Spawn", meta = (ClampMin = "0.0", AllowPrivateAccess = "true"))
	float SpawnRadiusMin = 600.0f;

	/** 围绕 FocusActor 的最大生成半径 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Spawn", meta = (ClampMin = "0.0", AllowPrivateAccess = "true"))
	float SpawnRadiusMax = 1200.0f;

	/** 生成时抬高一点，避免埋地 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Spawn", meta = (AllowPrivateAccess = "true"))
	float SpawnHeightOffset = 50.0f;

	/** 要生成的敌人类 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Spawn", meta = (AllowPrivateAccess = "true"))
	TArray<FEnemySpawnEntry> EnemySpawnTable;

	/** 围绕谁刷怪 */
	UPROPERTY(Transient)
	TWeakObjectPtr<AActor> FocusActor;

	/** 定时器句柄 */
	FTimerHandle SpawnTimerHandle;
};

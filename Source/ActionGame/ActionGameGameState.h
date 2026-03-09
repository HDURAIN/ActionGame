#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameStateBase.h"
#include "ActionGameGameState.generated.h"

/**
 * 全局游戏状态：
 * - 保存当前生存时间
 * - 保存当前难度阶段
 * - 由 GameMode 在服务器权威更新
 * - 自动复制到客户端，供 UI / Enemy 初始化 / 调试读取
 */
UCLASS()
class ACTIONGAME_API AActionGameGameState : public AGameStateBase
{
	GENERATED_BODY()

public:
	AActionGameGameState();

	// =========================
	// Read API
	// =========================

	UFUNCTION(BlueprintPure, Category = "Difficulty")
	int32 GetDifficultyStage() const { return DifficultyStage; }

	UFUNCTION(BlueprintPure, Category = "Difficulty")
	float GetElapsedSurvivalTime() const { return ElapsedSurvivalTime; }

	UFUNCTION(BlueprintPure, Category = "Difficulty")
	float GetStageDuration() const { return StageDuration; }

	// =========================
	// Server Write API
	// 约定：只允许服务器调用
	// =========================

	/** 直接设置当前生存时间，并按规则重算阶段 */
	void SetElapsedSurvivalTime(float InElapsedTime);

	/** 直接设置当前难度阶段（一般调试/特殊流程时用） */
	void SetDifficultyStage(int32 InStage);

	/** 根据当前生存时间，重算难度阶段 */
	void RefreshDifficultyStageFromTime();

protected:
	// =========================
	// Replication
	// =========================

	UPROPERTY(ReplicatedUsing = OnRep_ElapsedSurvivalTime, VisibleAnywhere, BlueprintReadOnly, Category = "Difficulty")
	float ElapsedSurvivalTime;

	UPROPERTY(ReplicatedUsing = OnRep_DifficultyStage, VisibleAnywhere, BlueprintReadOnly, Category = "Difficulty")
	int32 DifficultyStage;

	/** 每多少秒升 1 个难度阶段，默认 180 秒 = 3 分钟 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Difficulty", meta = (ClampMin = "1.0"))
	float StageDuration;

	UFUNCTION()
	void OnRep_ElapsedSurvivalTime();

	UFUNCTION()
	void OnRep_DifficultyStage();

	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;
};
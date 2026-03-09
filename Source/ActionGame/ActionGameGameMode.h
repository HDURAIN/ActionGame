#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameModeBase.h"
#include "ActionGameGameMode.generated.h"

class AActionGamePlayerController;
class AActionGameGameState;
class AEnemySpawnManager;
class AController;
class APlayerController;

UCLASS(Abstract)
class ACTIONGAME_API AActionGameGameMode : public AGameModeBase
{
	GENERATED_BODY()

public:
	AActionGameGameMode();

	virtual void RestartPlayer(AController* NewPlayer) override;

protected:
	virtual void BeginPlay() override;
	virtual void Tick(float DeltaSeconds) override;
	virtual void PostLogin(APlayerController* NewPlayer) override;
	virtual void Logout(AController* Exiting) override;

public:
	void NotifyPlayerDied(AActionGamePlayerController* PlayerController);

private:
	// =========================
	// Difficulty / Survival
	// =========================

	/** 开局后经过多少秒算作生存时间起点 */
	float SurvivalStartTime = 0.f;

	/** 上一次同步到 GameState 的生存时间，避免每帧都重复写日志/状态 */
	float LastSyncedElapsedTime = -1.f;

	/** 生存时间同步间隔（秒） */
	UPROPERTY(EditDefaultsOnly, Category = "Difficulty", meta = (ClampMin = "0.05"))
	float SurvivalTimeSyncInterval = 1.f;

	/** 当前累计的同步计时 */
	float SurvivalTimeSyncAccumulator = 0.f;

private:
	// =========================
	// Spawn
	// =========================

	/** SpawnManager 蓝图类（推荐填 BP_EnemySpawnManager） */
	UPROPERTY(EditDefaultsOnly, Category = "Spawn")
	TSubclassOf<AEnemySpawnManager> EnemySpawnManagerClass;

	/** 每个 Controller 对应一个 SpawnManager */
	UPROPERTY()
	TMap<TObjectPtr<AController>, TObjectPtr<AEnemySpawnManager>> PlayerSpawnManagers;

private:
	AEnemySpawnManager* CreateSpawnManagerForController(AController* InController);
	AEnemySpawnManager* GetSpawnManagerForController(AController* InController) const;
	void DestroySpawnManagerForController(AController* InController);
	void RefreshSpawnManagerForController(AController* InController);

private:
	// =========================
	// Helpers
	// =========================

	AActionGameGameState* GetActionGameGameState() const;
	void UpdateSurvivalDifficultyState();
};
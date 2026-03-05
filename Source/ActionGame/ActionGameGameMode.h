#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameModeBase.h"
#include "ActionGameGameMode.generated.h"

class AActionGamePlayerController;
class AEnemySpawnManager;
class AController;

UCLASS(Abstract)
class ACTIONGAME_API AActionGameGameMode : public AGameModeBase
{
	GENERATED_BODY()

public:
	AActionGameGameMode();

	virtual void RestartPlayer(AController* NewPlayer) override;

protected:
	virtual void BeginPlay() override;
	virtual void PostLogin(APlayerController* NewPlayer) override;
	virtual void Logout(AController* Exiting) override;

public:
	void NotifyPlayerDied(AActionGamePlayerController* PlayerController);

private:
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
};
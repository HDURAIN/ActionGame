#include "ActionGameGameMode.h"

#include "ActionGamePlayerController.h"
#include "Actors/EnemySpawnManager.h"
#include "Engine/World.h"
#include "GameFramework/Controller.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/PlayerController.h"
#include "ActionGameGameState.h"

AActionGameGameMode::AActionGameGameMode()
{
	PrimaryActorTick.bCanEverTick = true;
	PrimaryActorTick.bStartWithTickEnabled = true;

	GameStateClass = AActionGameGameState::StaticClass();
}

void AActionGameGameMode::BeginPlay()
{
	Super::BeginPlay();

	SurvivalStartTime = GetWorld() ? GetWorld()->GetTimeSeconds() : 0.f;
	LastSyncedElapsedTime = -1.f;
	SurvivalTimeSyncAccumulator = 0.f;

	UpdateSurvivalDifficultyState();

	UE_LOG(LogTemp, Warning, TEXT("AActionGameGameMode::BeginPlay - SurvivalStartTime=%.2f"), SurvivalStartTime);
}

void AActionGameGameMode::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);

	SurvivalTimeSyncAccumulator += DeltaSeconds;

	const float SafeSyncInterval = FMath::Max(0.05f, SurvivalTimeSyncInterval);
	if (SurvivalTimeSyncAccumulator >= SafeSyncInterval)
	{
		SurvivalTimeSyncAccumulator = 0.f;
		UpdateSurvivalDifficultyState();
	}
}

void AActionGameGameMode::PostLogin(APlayerController* NewPlayer)
{
	Super::PostLogin(NewPlayer);

	UE_LOG(LogTemp, Warning, TEXT("ActionGameGameMode: PostLogin -> %s"),
		NewPlayer ? *NewPlayer->GetName() : TEXT("None"));

	CreateSpawnManagerForController(NewPlayer);
}

void AActionGameGameMode::Logout(AController* Exiting)
{
	UE_LOG(LogTemp, Warning, TEXT("ActionGameGameMode: Logout -> %s"),
		Exiting ? *Exiting->GetName() : TEXT("None"));

	DestroySpawnManagerForController(Exiting);

	Super::Logout(Exiting);
}

void AActionGameGameMode::RestartPlayer(AController* NewPlayer)
{
	Super::RestartPlayer(NewPlayer);

	RefreshSpawnManagerForController(NewPlayer);
}

void AActionGameGameMode::NotifyPlayerDied(AActionGamePlayerController* PlayerController)
{
	if (!PlayerController)
	{
		return;
	}

	if (AEnemySpawnManager* SpawnManager = GetSpawnManagerForController(PlayerController))
	{
		SpawnManager->SetSpawningEnabled(false);
		SpawnManager->SetFocusActor(nullptr);

		UE_LOG(LogTemp, Warning, TEXT("ActionGameGameMode: Disabled SpawnManager for %s"),
			*PlayerController->GetName());
	}

	PlayerController->RestartPlayerIn(3.f);
}

AEnemySpawnManager* AActionGameGameMode::CreateSpawnManagerForController(AController* InController)
{
	if (!IsValid(InController))
	{
		return nullptr;
	}

	if (AEnemySpawnManager* Existing = GetSpawnManagerForController(InController))
	{
		return Existing;
	}

	if (!EnemySpawnManagerClass)
	{
		UE_LOG(LogTemp, Warning, TEXT("ActionGameGameMode: EnemySpawnManagerClass is null."));
		return nullptr;
	}

	UWorld* World = GetWorld();
	if (!World)
	{
		return nullptr;
	}

	FActorSpawnParameters SpawnParams;
	SpawnParams.Owner = this;
	SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;

	AEnemySpawnManager* NewSpawnManager = World->SpawnActor<AEnemySpawnManager>(
		EnemySpawnManagerClass,
		FVector::ZeroVector,
		FRotator::ZeroRotator,
		SpawnParams
	);

	if (!NewSpawnManager)
	{
		UE_LOG(LogTemp, Warning, TEXT("ActionGameGameMode: Failed to spawn SpawnManager for %s"),
			*InController->GetName());
		return nullptr;
	}

	NewSpawnManager->SetSpawningEnabled(false);
	NewSpawnManager->SetFocusActor(nullptr);

	PlayerSpawnManagers.Add(InController, NewSpawnManager);

	UE_LOG(LogTemp, Warning, TEXT("ActionGameGameMode: SpawnManager %s created for %s"),
		*NewSpawnManager->GetName(),
		*InController->GetName());

	return NewSpawnManager;
}

AEnemySpawnManager* AActionGameGameMode::GetSpawnManagerForController(AController* InController) const
{
	if (!IsValid(InController))
	{
		return nullptr;
	}

	if (const TObjectPtr<AEnemySpawnManager>* Found = PlayerSpawnManagers.Find(InController))
	{
		return Found->Get();
	}

	return nullptr;
}

void AActionGameGameMode::DestroySpawnManagerForController(AController* InController)
{
	if (!IsValid(InController))
	{
		return;
	}

	AEnemySpawnManager* SpawnManager = GetSpawnManagerForController(InController);
	if (SpawnManager)
	{
		UE_LOG(LogTemp, Warning, TEXT("ActionGameGameMode: Destroy SpawnManager %s for %s"),
			*SpawnManager->GetName(),
			*InController->GetName());

		SpawnManager->Destroy();
	}

	PlayerSpawnManagers.Remove(InController);
}

void AActionGameGameMode::RefreshSpawnManagerForController(AController* InController)
{
	if (!IsValid(InController))
	{
		return;
	}

	AEnemySpawnManager* SpawnManager = GetSpawnManagerForController(InController);
	if (!SpawnManager)
	{
		SpawnManager = CreateSpawnManagerForController(InController);
	}

	if (!SpawnManager)
	{
		return;
	}

	APawn* Pawn = InController->GetPawn();
	if (!IsValid(Pawn))
	{
		UE_LOG(LogTemp, Warning, TEXT("ActionGameGameMode: Controller %s has no pawn after RestartPlayer."),
			*InController->GetName());
		return;
	}

	SpawnManager->SetFocusActor(Pawn);
	SpawnManager->SetSpawningEnabled(true);

	UE_LOG(LogTemp, Warning, TEXT("ActionGameGameMode: SpawnManager %s now focuses %s"),
		*SpawnManager->GetName(),
		*Pawn->GetName());
}

AActionGameGameState* AActionGameGameMode::GetActionGameGameState() const
{
	return GetGameState<AActionGameGameState>();
}

void AActionGameGameMode::UpdateSurvivalDifficultyState()
{
	UWorld* World = GetWorld();
	if (!World)
	{
		return;
	}

	AActionGameGameState* AGGameState = GetActionGameGameState();
	if (!AGGameState)
	{
		UE_LOG(LogTemp, Warning, TEXT("ActionGameGameMode: GameState is not AActionGameGameState."));
		return;
	}

	const float CurrentTime = World->GetTimeSeconds();
	const float ElapsedSurvivalTime = FMath::Max(0.f, CurrentTime - SurvivalStartTime);

	if (FMath::IsNearlyEqual(ElapsedSurvivalTime, LastSyncedElapsedTime, KINDA_SMALL_NUMBER))
	{
		return;
	}

	AGGameState->SetElapsedSurvivalTime(ElapsedSurvivalTime);
	LastSyncedElapsedTime = ElapsedSurvivalTime;
}
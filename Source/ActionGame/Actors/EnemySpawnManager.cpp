#include "EnemySpawnManager.h"

#include "Characters/EnemyCharacterBase.h"
#include "Engine/World.h"
#include "GameFramework/Actor.h"
#include "TimerManager.h"

AEnemySpawnManager::AEnemySpawnManager()
{
	PrimaryActorTick.bCanEverTick = false;
}

void AEnemySpawnManager::BeginPlay()
{
	Super::BeginPlay();

	if (bSpawningEnabled)
	{
		StartSpawnTimer();
	}
}

void AEnemySpawnManager::SetSpawningEnabled(bool bEnabled)
{
	if (bSpawningEnabled == bEnabled)
	{
		return;
	}

	bSpawningEnabled = bEnabled;

	if (bSpawningEnabled)
	{
		StartSpawnTimer();
	}
	else
	{
		StopSpawnTimer();
	}
}

void AEnemySpawnManager::SetFocusActor(AActor* NewFocus)
{
	FocusActor = NewFocus;

	UE_LOG(LogTemp, Warning, TEXT("EnemySpawnManager: FocusActor set to %s"),
		NewFocus ? *NewFocus->GetName() : TEXT("None"));
}

void AEnemySpawnManager::SpawnEnemyOnce()
{
	if (!CanSpawn())
	{
		return;
	}

	SpawnEnemyInternal();
}

int32 AEnemySpawnManager::GetRemainingSpawnCount() const
{
	if (bSpawnInfinitely)
	{
		return -1;
	}

	return FMath::Max(0, TotalEnemyToSpawn - SpawnedEnemyCount);
}

void AEnemySpawnManager::StartSpawnTimer()
{
	UWorld* World = GetWorld();
	if (!World)
	{
		return;
	}

	if (SpawnInterval <= 0.f)
	{
		UE_LOG(LogTemp, Warning, TEXT("EnemySpawnManager: SpawnInterval must be > 0."));
		return;
	}

	if (World->GetTimerManager().IsTimerActive(SpawnTimerHandle))
	{
		return;
	}

	World->GetTimerManager().SetTimer(
		SpawnTimerHandle,
		this,
		&AEnemySpawnManager::HandleSpawnTimerElapsed,
		SpawnInterval,
		true
	);
}

void AEnemySpawnManager::StopSpawnTimer()
{
	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().ClearTimer(SpawnTimerHandle);
	}
}

void AEnemySpawnManager::HandleSpawnTimerElapsed()
{
	if (!CanSpawn())
	{
		UE_LOG(LogTemp, Warning, TEXT("EnemySpawnManager: CanSpawn = false"));

		if (!bSpawnInfinitely && SpawnedEnemyCount >= TotalEnemyToSpawn)
		{
			StopSpawnTimer();
		}
		return;
	}

	UE_LOG(LogTemp, Warning, TEXT("EnemySpawnManager: CanSpawn = true, trying to spawn"));

	SpawnEnemyInternal();

	if (!bSpawnInfinitely && SpawnedEnemyCount >= TotalEnemyToSpawn)
	{
		StopSpawnTimer();
	}
}

bool AEnemySpawnManager::CanSpawn() const
{
	if (!bSpawningEnabled)
	{
		UE_LOG(LogTemp, Warning, TEXT("EnemySpawnManager: CanSpawn failed - bSpawningEnabled is false"));
		return false;
	}

	if (!FocusActor.IsValid())
	{
		UE_LOG(LogTemp, Warning, TEXT("EnemySpawnManager: CanSpawn failed - FocusActor invalid"));
		return false;
	}

	if (SpawnRadiusMax < SpawnRadiusMin)
	{
		UE_LOG(LogTemp, Warning, TEXT("EnemySpawnManager: CanSpawn failed - SpawnRadiusMax < SpawnRadiusMin"));
		return false;
	}

	if (EnemySpawnTable.Num() <= 0)
	{
		UE_LOG(LogTemp, Warning, TEXT("EnemySpawnManager: CanSpawn failed - EnemySpawnTable empty"));
		return false;
	}

	if (!bSpawnInfinitely && SpawnedEnemyCount >= TotalEnemyToSpawn)
	{
		UE_LOG(LogTemp, Warning, TEXT("EnemySpawnManager: CanSpawn failed - Reached total spawn count"));
		return false;
	}

	return true;
}

bool AEnemySpawnManager::FindSpawnLocation(FVector& OutSpawnLocation) const
{
	if (!FocusActor.IsValid())
	{
		return false;
	}

	const FVector FocusLocation = FocusActor->GetActorLocation();

	const float Radius = FMath::FRandRange(SpawnRadiusMin, SpawnRadiusMax);
	const FVector RandomDirection3D = FVector(FMath::VRand());
	FVector FlatDirection(RandomDirection3D.X, RandomDirection3D.Y, 0.f);

	if (!FlatDirection.Normalize())
	{
		FlatDirection = FVector::ForwardVector;
	}

	OutSpawnLocation = FocusLocation + FlatDirection * Radius;
	OutSpawnLocation.Z += SpawnHeightOffset; // 这里只加通用基础偏移

	return true;
}

bool AEnemySpawnManager::PickEnemySpawnEntryWeighted(FEnemySpawnEntry& OutEntry) const
{
	OutEntry = FEnemySpawnEntry();

	if (EnemySpawnTable.Num() <= 0)
	{
		return false;
	}

	int32 TotalWeight = 0;

	for (const FEnemySpawnEntry& Entry : EnemySpawnTable)
	{
		if (!Entry.EnemyClass)
		{
			continue;
		}

		if (Entry.Weight <= 0)
		{
			continue;
		}

		TotalWeight += Entry.Weight;
	}

	if (TotalWeight <= 0)
	{
		return false;
	}

	const int32 RandomValue = FMath::RandRange(1, TotalWeight);
	int32 RunningWeight = 0;

	for (const FEnemySpawnEntry& Entry : EnemySpawnTable)
	{
		if (!Entry.EnemyClass || Entry.Weight <= 0)
		{
			continue;
		}

		RunningWeight += Entry.Weight;
		if (RandomValue <= RunningWeight)
		{
			OutEntry = Entry;
			return true;
		}
	}

	return false;
}

bool AEnemySpawnManager::SpawnEnemyInternal()
{
	UWorld* World = GetWorld();
	if (!World)
	{
		return false;
	}

	FVector SpawnLocation = FVector::ZeroVector;
	if (!FindSpawnLocation(SpawnLocation))
	{
		UE_LOG(LogTemp, Warning, TEXT("EnemySpawnManager: Failed to find spawn location."));
		return false;
	}

	FEnemySpawnEntry SelectedEntry;
	if (!PickEnemySpawnEntryWeighted(SelectedEntry))
	{
		UE_LOG(LogTemp, Warning, TEXT("EnemySpawnManager: Failed to pick enemy entry from spawn table."));
		return false;
	}

	if (!SelectedEntry.EnemyClass)
	{
		UE_LOG(LogTemp, Warning, TEXT("EnemySpawnManager: Selected enemy entry has null EnemyClass."));
		return false;
	}

	// 再叠加该敌人自己的额外生成高度偏移
	SpawnLocation.Z += SelectedEntry.SpawnHeightOffset;

	const FRotator SpawnRotation = FRotator::ZeroRotator;

	FActorSpawnParameters SpawnParams;
	SpawnParams.Owner = this;
	SpawnParams.SpawnCollisionHandlingOverride =
		ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButAlwaysSpawn;

	AEnemyCharacterBase* SpawnedEnemy = World->SpawnActor<AEnemyCharacterBase>(
		SelectedEntry.EnemyClass,
		SpawnLocation,
		SpawnRotation,
		SpawnParams
	);

	if (!SpawnedEnemy)
	{
		UE_LOG(LogTemp, Warning, TEXT("EnemySpawnManager: SpawnActor failed."));
		return false;
	}

	SpawnedEnemy->ApplySpawnEntryConfig(SelectedEntry);

	++SpawnedEnemyCount;

	UE_LOG(LogTemp, Log, TEXT("EnemySpawnManager: Spawned enemy. Count = %d"), SpawnedEnemyCount);

	return true;
}
#include "EnemySpawnManager.h"

#include "Characters/EnemyCharacterBase.h"
#include "Engine/World.h"
#include "GameFramework/Actor.h"
#include "GameFramework/CharacterMovementComponent.h"
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
		// 计时器刷怪
		StartSpawnTimer();
	}
}

// 开启/关闭刷怪，开启后会根据 SpawnInterval 定时尝试刷怪，关闭则停止定时器
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

// 设置刷怪目标，通常是玩家角色，刷怪点会围绕目标位置生成
void AEnemySpawnManager::SetFocusActor(AActor* NewFocus)
{
	FocusActor = NewFocus;

	UE_LOG(LogTemp, Warning, TEXT("EnemySpawnManager: FocusActor set to %s"),
		NewFocus ? *NewFocus->GetName() : TEXT("None"));
}

// 这个接口主要是为了调试和特殊波次调用，正常刷怪逻辑走定时器
void AEnemySpawnManager::SpawnEnemyOnce()
{
	if (!CanSpawn())
	{
		return;
	}

	SpawnEnemyInternal();
}

// 计算剩余可生成数量，-1 代表无限生成
int32 AEnemySpawnManager::GetRemainingSpawnCount() const
{
	if (bSpawnInfinitely)
	{
		return -1;
	}

	return FMath::Max(0, TotalEnemyToSpawn - SpawnedEnemyCount);
}

// 开启定时器，定时调用 HandleSpawnTimerElapsed() 来尝试刷怪
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

	// 设置定时器，循环调用 HandleSpawnTimerElapsed
	World->GetTimerManager().SetTimer(
		SpawnTimerHandle,
		this,
		&AEnemySpawnManager::HandleSpawnTimerElapsed,
		SpawnInterval,
		true
	);
}

// 停止定时器，停止后不再尝试刷怪
void AEnemySpawnManager::StopSpawnTimer()
{
	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().ClearTimer(SpawnTimerHandle);
	}
}

// 定时器回调，尝试刷怪，如果条件不满足则不刷怪，如果达到了总刷怪数量上限则停止定时器
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

// 检查是否满足刷怪条件：生成开关、目标有效、生成半径合理、配置表非空、未达总生成上限
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

// 计算一个随机生成位置，围绕 FocusActor 的位置在 SpawnRadiusMin 和 SpawnRadiusMax 范围内随机分布，Z 轴上加上 SpawnHeightOffset 基础偏移
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

// 从 EnemySpawnTable 中按权重随机挑选一个有效的 FEnemySpawnEntry，返回 false 代表挑选失败（表里没有有效条目）
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

// 执行生成逻辑：找到生成位置，挑选生成配置，调用 SpawnActorDeferred 生成敌人，灌入配置并完成生成，最后如果需要的话再生成 Controller
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

	// 叠加该敌人自己的额外生成高度偏移
	SpawnLocation.Z += SelectedEntry.SpawnHeightOffset;

	const FRotator SpawnRotation = FRotator::ZeroRotator;
	const FTransform SpawnTM(SpawnRotation, SpawnLocation);

	// Deferred Spawn：先拿到“半成品”Actor，BeginPlay 还没跑
	AEnemyCharacterBase* SpawnedEnemy = World->SpawnActorDeferred<AEnemyCharacterBase>(
		SelectedEntry.EnemyClass,
		SpawnTM,
		this,        // Owner
		nullptr,     // Instigator（可按需传）
		ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButAlwaysSpawn
	);

	if (!SpawnedEnemy)
	{
		UE_LOG(LogTemp, Warning, TEXT("EnemySpawnManager: SpawnActorDeferred failed."));
		return false;
	}

	// 在 BeginPlay 设置配置文件（关键：避免 BT/Movement/ASC 初始化时序问题）
	SpawnedEnemy->InitFromSpawnEntry(SelectedEntry);

	// 完成生成：这一步后才会触发 Construction/BeginPlay
	SpawnedEnemy->FinishSpawning(SpawnTM);

	// 再创建 Controller（让 BT 从“配置已就绪”的状态开始跑）
	if (SpawnedEnemy->HasAuthority() && SpawnedEnemy->GetController() == nullptr)
	{
		SpawnedEnemy->SpawnDefaultController();
	}

	++SpawnedEnemyCount;
	return true;
}
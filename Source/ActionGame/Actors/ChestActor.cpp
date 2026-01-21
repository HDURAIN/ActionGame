// Fill out your copyright notice in the Description page of Project Settings.


#include "Actors/ChestActor.h"

#include "DropVisualActor.h"
#include "WorldItemActor.h"

#include "Components/StaticMeshComponent.h"
#include "Components/SphereComponent.h"
#include "Components/BoxComponent.h"
#include "Components/SceneComponent.h"

#include "Net/UnrealNetwork.h"

#include "Engine/World.h"

// Sets default values
AChestActor::AChestActor()
{
 	// Set this actor to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = false;
	bReplicates = true;

	// Root
	SceneRootComponent = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
	SetRootComponent(SceneRootComponent);

	// Mesh (世界障碍 + 外观表现)
	MeshComponent = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Mesh"));
	MeshComponent->SetupAttachment(RootComponent);
	// Collision
	MeshComponent->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
	MeshComponent->SetCollisionObjectType(ECC_WorldDynamic);
	MeshComponent->SetCollisionResponseToAllChannels(ECR_Ignore);
	MeshComponent->SetCollisionResponseToChannel(ECC_WorldStatic, ECR_Block);
	MeshComponent->SetCollisionResponseToChannel(ECC_WorldDynamic, ECR_Block);
	MeshComponent->SetCollisionResponseToChannel(ECC_Pawn, ECR_Block);
	MeshComponent->SetCollisionResponseToChannel(ECC_GameTraceChannel1, ECR_Block);

	// 用于射线判断
	InteractTargetBox = CreateDefaultSubobject<UBoxComponent>(TEXT("InteractTargetBox"));
	InteractTargetBox->SetupAttachment(RootComponent);
	// Collision
	InteractTargetBox->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
	InteractTargetBox->SetCollisionResponseToAllChannels(ECR_Ignore);
	InteractTargetBox->SetCollisionResponseToChannel(ECC_GameTraceChannel2, ECR_Block);
	InteractTargetBox->ComponentTags.Add(InteractTags::InteractTarget);

	// Sphere 判定球
	OverlapSphereComponent = CreateDefaultSubobject<USphereComponent>(TEXT("Sphere"));
	OverlapSphereComponent->SetupAttachment(RootComponent);
	OverlapSphereComponent->InitSphereRadius(150.f);
	// Collision
	OverlapSphereComponent->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
	OverlapSphereComponent->SetGenerateOverlapEvents(true);
	OverlapSphereComponent->SetCollisionResponseToAllChannels(ECR_Ignore);
	OverlapSphereComponent->SetCollisionResponseToChannel(ECC_Pawn, ECR_Overlap);
}

// Called when the game starts or when spawned
void AChestActor::BeginPlay()
{
	Super::BeginPlay();

}

bool AChestActor::CanInteract_Implementation(AActor* Interactor) const
{
	if (bOpened) {
		return false;
	}

	return true;
}

void AChestActor::ExecuteInteract_Implementation(AActor* Interactor)
{
	if (!HasAuthority())
	{
		return;
	}

	if (!CanInteract_Implementation(Interactor))
	{
		return;
	}

	bOpened = true;

	StartDropFlow();

	// 播放开箱动画
	// 切换宝箱Mesh和状态
	// 蓝图中实现
	OnChestOpened();
}

EInteractType AChestActor::GetInteractType_Implementation() const
{
	return InteractType;
}

void AChestActor::StartDropFlow()
{
	if (!DropVisualClass)
	{
		UE_LOG(LogTemp, Warning, TEXT("ChestActor: DropVisualClass is not set."));
		return;
	}

	UWorld* World = GetWorld();
	if (!World)
	{
		return;
	}

	const FVector SpawnLocation = GetDropSpawnLocation();
	const FRotator SpawnRotation = FRotator::ZeroRotator;

	FActorSpawnParameters SpawnParams;
	SpawnParams.Owner = this;
	SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;

	ADropVisualActor* DropActor = World->SpawnActor<ADropVisualActor>(DropVisualClass, SpawnLocation, SpawnRotation, SpawnParams);

	if (!DropActor)
	{
		return;
	}

	DropActor->StartDrop(GetActorForwardVector());

	// 绑定掉落完成回调
	// Chest不关心Drop的物理细节，只需要一个落地完成的GroundLocation
	DropActor->BindOnDropFinished(FOnDropFinished::CreateUObject(this, &AChestActor::HandleDropLanded));
}

FVector AChestActor::GetDropSpawnLocation() const
{
	const FVector ChestLocation = GetActorLocation();
	return ChestLocation + FVector(0.f, 0.f, 80.f);
}

void AChestActor::HandleDropLanded(const FVector& GroundLocation)
{
	if (!WorldItemClass || DropItems.Num() == 0)
	{
		return;
	}

	// Spawn WorldItemActor
	UWorld* World = GetWorld();
	if (!World)
	{
		return;
	}
	const FVector SpawnLocation = GroundLocation + FVector(0.f, 0.f, 50.f);
	const FRotator SpawnRotation = FRotator::ZeroRotator;

	FActorSpawnParameters Params;
	Params.Owner = this;
	Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;

	AWorldItemActor* ItemActor = World->SpawnActor<AWorldItemActor>(WorldItemClass, SpawnLocation, SpawnRotation, Params);

	if (ItemActor)
	{
		const int32 Index = FMath::RandRange(0, DropItems.Num() - 1);
		ItemActor->InitWithItemData(DropItems[Index]);
	}
}

void AChestActor::OnRep_Opened()
{
	OnChestOpened();
}

void AChestActor::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(AChestActor, bOpened);
}
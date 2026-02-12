// Fill out your copyright notice in the Description page of Project Settings.


#include "Actors/ChestActor.h"

#include "DropVisualActor.h"
#include "WorldItemActor.h"

#include "Components/StaticMeshComponent.h"
#include "Components/SphereComponent.h"
#include "Components/BoxComponent.h"
#include "Components/SceneComponent.h"
#include "Components/WidgetComponent.h"
#include "AbilitySystem/Components/InteractableComponent.h"
#include "UserWidget/WorldObjectPromptWidget.h"

#include "Kismet/KismetMathLibrary.h"
#include "Camera/PlayerCameraManager.h"
#include "GameFramework/PlayerController.h"
#include "DataAssets/WorldObjectDataAsset.h"

#include "Net/UnrealNetwork.h"

#include "Engine/World.h"

// Sets default values
AChestActor::AChestActor()
{
 	// Set this actor to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;
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

	InteractableComponent = CreateDefaultSubobject<UInteractableComponent>(TEXT("InteractableComponent"));


	// UI
	InteractWidgetComponent = CreateDefaultSubobject<UWidgetComponent>(TEXT("InteractWidget"));
	InteractWidgetComponent->SetupAttachment(RootComponent);

	// 世界空间 UI
	InteractWidgetComponent->SetWidgetSpace(EWidgetSpace::World);
	InteractWidgetComponent->SetDrawSize(FVector2D(200.f, 80.f));
	InteractWidgetComponent->SetRelativeLocation(FVector(0.f, 0.f, 120.f));

	// 默认隐藏
	InteractWidgetComponent->SetHiddenInGame(true);
	InteractWidgetComponent->SetVisibility(false);
}

// Called when the game starts or when spawned
void AChestActor::BeginPlay()
{
	Super::BeginPlay();

	if (!InteractWidgetComponent || !WorldObjectDataAsset)
	{
		return;
	}

	UUserWidget* Widget = InteractWidgetComponent->GetUserWidgetObject();
	if (!Widget)
	{
		return;
	}
	UE_LOG(LogTemp, Warning, TEXT("Widget = %s"),
		Widget ? TEXT("Valid") : TEXT("Null"));

	if (UWorldObjectPromptWidget* Prompt = Cast<UWorldObjectPromptWidget>(Widget))
	{
		Prompt->InitWithData(WorldObjectDataAsset);
	}
}

void AChestActor::Tick(float DeltaTime)
{
	if (!InteractWidgetComponent || !InteractWidgetComponent->IsVisible())
		return;

	APlayerController* PC = GetWorld()->GetFirstPlayerController();
	if (!PC) return;

	FVector CamLoc = PC->PlayerCameraManager->GetCameraLocation();
	FVector WidgetLoc = InteractWidgetComponent->GetComponentLocation();

	FRotator LookAt = UKismetMathLibrary::FindLookAtRotation(WidgetLoc, CamLoc);
	LookAt.Pitch = 0.f;
	LookAt.Roll = 0.f;

	InteractWidgetComponent->SetWorldRotation(LookAt);
}

bool AChestActor::CanInteract_Implementation(AActor* Interactor) const
{	
	if (bOpened)
	{
		return false;
	}

	if (!InteractableComponent)
	{
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

	SetInteractUIVisible(false);

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

float AChestActor::GetInteractCost_Implementation() const
{
	return WorldObjectDataAsset->WorldObjectData.Cost;
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
	const FRotator SpawnRotation = GetActorRotation();

	FActorSpawnParameters SpawnParams;
	SpawnParams.Owner = this;
	SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;

	ADropVisualActor* DropActor = World->SpawnActor<ADropVisualActor>(DropVisualClass, SpawnLocation, SpawnRotation, SpawnParams);

	if (!DropActor)
	{
		return;
	}

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
	UE_LOG(LogTemp, Warning, TEXT("AChestActor::HandleDropLanded"));

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

	SetInteractUIVisible(false);
}

void AChestActor::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(AChestActor, bOpened);
}

void AChestActor::SetInteractUIVisible(bool bVisible)
{
	if (!InteractWidgetComponent)
	{
		return;
	}

	if (bOpened) bVisible = false;

	// UI 只在客户端可见
	if (!GetWorld() || GetWorld()->IsNetMode(NM_DedicatedServer))
	{
		return;
	}

	InteractWidgetComponent->SetHiddenInGame(!bVisible);
	InteractWidgetComponent->SetVisibility(bVisible);
}

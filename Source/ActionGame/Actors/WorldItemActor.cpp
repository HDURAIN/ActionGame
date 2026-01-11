// Fill out your copyright notice in the Description page of Project Settings.


#include "Actors/WorldItemActor.h"

#include "Components/StaticMeshComponent.h"
#include "Components/SphereComponent.h"

#include "DataAssets/DA_Item.h"
#include "ActorComponents/ItemContainerComponent.h"
#include "GameFramework/Actor.h"
#include "Net/UnrealNetwork.h"

// Sets default values
AWorldItemActor::AWorldItemActor()
{
 	// Set this actor to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = false;

	bReplicates = true;

	// 碰撞组件 用于 Trace 和 Overlap
	CollisionComponent = CreateDefaultSubobject<USphereComponent>(TEXT("Collision"));
	SetRootComponent(CollisionComponent);

	CollisionComponent->InitSphereRadius(50.f);
	CollisionComponent->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
	CollisionComponent->SetCollisionProfileName(TEXT("OverlapAll"));

	// Mesh 仅作表现
	MeshComponent = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Mesh"));
	MeshComponent->SetupAttachment(CollisionComponent);
	MeshComponent->SetCollisionResponseToAllChannels(ECR_Block);
	MeshComponent->SetCollisionResponseToChannel(ECC_Pawn, ECR_Ignore);
	MeshComponent->SetCollisionResponseToChannel(ECC_Camera, ECR_Ignore);
}

// Called when the game starts or when spawned
void AWorldItemActor::BeginPlay()
{
	Super::BeginPlay();

}

bool AWorldItemActor::CanInteract_Implementation(AActor* Interactor) const
{
	if (bConsumed)
	{
		return false;
	}

	if (!ItemDef)
	{
		return false;
	}

	if (!Interactor)
	{
		return false;
	}

	UE_LOG(LogTemp, Warning, TEXT("CanInteract_Implementation return true"));
	return true;
}

void AWorldItemActor::ExecuteInteract_Implementation(AActor* Interactor)
{
	if (!HasAuthority())
	{
		return;
	}

	if (!CanInteract_Implementation(Interactor))
	{
		return;
	}

	UE_LOG(LogTemp, Warning, TEXT("Call [AWorldItemActor::ExecuteInteract_Implementation]"));
	// 尝试把物品交给交互者
	const bool bGiven = GiveItemTo(Interactor);
	if (!bGiven)
	{
		return;
	}

	ConsumeAndDestroy();
}

bool AWorldItemActor::GiveItemTo(AActor* Interactor)
{
	if (!ItemDef || !Interactor)
	{
		return false;
	}

	UE_LOG(LogTemp, Warning, TEXT("Call [GiveItemTo]"));
	if (UItemContainerComponent* Container = Interactor->FindComponentByClass<UItemContainerComponent>())
	{
		return Container->AddItem(ItemDef, 1);
	}

	// 间接交互 后面可能会用到？暂存
	if (AActor* OwnerActor = Interactor->GetOwner())
	{
		if (UItemContainerComponent* OwnerContainer = OwnerActor->FindComponentByClass<UItemContainerComponent>())
		{
			return OwnerContainer->AddItem(ItemDef, 1);
		}
	}

	return false;
}

void AWorldItemActor::ConsumeAndDestroy()
{
	if (bConsumed)
	{
		return;
	}

	bConsumed = true;

	// 先禁用交互 避免Destroy前被重复命中
	SetActorEnableCollision(false);
	SetActorHiddenInGame(true);

	Destroy();
}


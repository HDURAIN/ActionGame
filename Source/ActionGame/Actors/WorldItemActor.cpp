// Fill out your copyright notice in the Description page of Project Settings.


#include "Actors/WorldItemActor.h"

#include "Components/StaticMeshComponent.h"
#include "Components/SphereComponent.h"
#include "Components/BoxComponent.h"
#include "Components/SceneComponent.h"

#include "DataAssets/DA_Item.h"
#include "ActorComponents/ItemContainerComponent.h"
#include "AbilitySystem/Components/InteractableComponent.h"
#include "GameFramework/Actor.h"
#include "Net/UnrealNetwork.h"

// Sets default values
AWorldItemActor::AWorldItemActor()
{
 	// Set this actor to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = false;
	bReplicates = true;

	SceneRootComponent = CreateDefaultSubobject<USceneComponent>(TEXT("SceneRoot"));
	SetRootComponent(SceneRootComponent);

	// Mesh 外观表现 + 阻挡射线
	MeshComponent = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Mesh"));
	MeshComponent->SetupAttachment(RootComponent);

	MeshComponent->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
	MeshComponent->SetCollisionResponseToAllChannels(ECR_Ignore);
	MeshComponent->SetVisibility(false, true);

	InteractTargetBox = CreateDefaultSubobject<UBoxComponent>(TEXT("InteractTargetBox"));
	InteractTargetBox->SetupAttachment(RootComponent);
	InteractTargetBox->SetCollisionResponseToAllChannels(ECR_Ignore);
	InteractTargetBox->SetCollisionResponseToChannel(ECC_GameTraceChannel1, ECR_Block);
	InteractTargetBox->ComponentTags.Add(InteractTags::InteractTarget);
	InteractTargetBox->SetBoxExtent(InteractBoxExtent);

	InteractableComponent = CreateDefaultSubobject<UInteractableComponent>(TEXT("InteractableComponent"));
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

	if (!InteractableComponent || !InteractableComponent->IsInteractorInRange(Interactor))
	{
		return false;
	}

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

	// 尝试把物品交给交互者
	const bool bGiven = GiveItemTo(Interactor);
	if (!bGiven)
	{
		return;
	}

	ConsumeAndDestroy();
}

EInteractType AWorldItemActor::GetInteractType_Implementation() const
{
	return InteractType;
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
	SetLifeSpan(0.1f);
	Destroy();
}

void AWorldItemActor::ApplyItemVisual()
{
	if (!MeshComponent || !ItemDef)
	{
		return;
	}

	if (ItemDef->WorldMesh)
	{
		MeshComponent->SetStaticMesh(ItemDef->WorldMesh);

		const FBoxSphereBounds Bounds = ItemDef->WorldMesh->GetBounds();
		const float MeshDiameter = Bounds.BoxExtent.GetMax() * 2.f;

		if (MeshDiameter > KINDA_SMALL_NUMBER)
		{
			const float Scale = TargetWorldSize / MeshDiameter;
			MeshComponent->SetRelativeScale3D(FVector(Scale));
		}
	}

	if (ItemDef->OverrideMaterial)
	{
		MeshComponent->SetMaterial(0, ItemDef->OverrideMaterial);
	}

	MeshComponent->SetVisibility(true, true);
}

void AWorldItemActor::OnRep_ItemDef()
{
	ApplyItemVisual();
}

void AWorldItemActor::OnRep_Consumed()
{
	SetActorHiddenInGame(true);
	SetActorEnableCollision(false);
}

void AWorldItemActor::InitWithItemData(UDA_Item* InItemDef)
{
	if (ItemDef || !InItemDef)
	{
		return;
	}

	ItemDef = InItemDef;
	ApplyItemVisual();
}

void AWorldItemActor::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(AWorldItemActor, ItemDef);
	DOREPLIFETIME(AWorldItemActor, bConsumed);
}

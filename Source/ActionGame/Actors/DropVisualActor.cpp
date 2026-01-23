// Fill out your copyright notice in the Description page of Project Settings.


#include "Actors/DropVisualActor.h"

#include "Components/StaticMeshComponent.h"
#include "Engine/World.h"
#include "DrawDebugHelpers.h"
#include "ChestActor.h"

// Sets default values
ADropVisualActor::ADropVisualActor()
{
 	// Set this actor to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = false;
	bReplicates = true;

	// Mesh
	MeshComponent = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Mesh"));
	SetRootComponent(MeshComponent);

	MeshComponent->SetSimulatePhysics(true);
	MeshComponent->SetEnableGravity(true);
	MeshComponent->SetNotifyRigidBodyCollision(true);

	MeshComponent->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
	MeshComponent->SetCollisionObjectType(ECC_WorldDynamic);
	MeshComponent->SetCollisionResponseToAllChannels(ECR_Ignore);
	MeshComponent->SetCollisionResponseToChannel(ECC_WorldStatic, ECR_Block);
	MeshComponent->SetCollisionResponseToChannel(ECC_WorldDynamic, ECR_Block);

	MeshComponent->OnComponentHit.AddDynamic(
		this,
		&ADropVisualActor::OnMeshHit
	);
}

// Called when the game starts or when spawned
void ADropVisualActor::BeginPlay()
{
	Super::BeginPlay();

	if (!MeshComponent || !MeshComponent->IsSimulatingPhysics())
	{
		return;
	}

	// Ê©¼Ó³åÁ¿
	const FVector ForwardDir = GetActorForwardVector();
	const FVector Impulse = ForwardDir * ForwardImpulse + FVector(0.f, 0.f, UpImpulse);
	MeshComponent->WakeAllRigidBodies();
	MeshComponent->AddImpulse(
		Impulse,
		NAME_None,
		true // Velocity Change
	);
}

void ADropVisualActor::BindOnDropFinished(FOnDropFinished InDelegate)
{
	OnDropFinished = InDelegate;
}

void ADropVisualActor::OnMeshHit(UPrimitiveComponent* HitComp, AActor* OtherActor, UPrimitiveComponent* OtherComp, FVector NormalImpulse, const FHitResult& Hit)
{
	UE_LOG(LogTemp, Warning, TEXT("ADropVisualActor::OnMeshHit"));

	if (bDropFinished)
	{
		return;
	}

	if (!OtherComp || OtherActor == this)
	{
		return;
	}

	// ÅÅ³ý Pawn / Character
	if (OtherActor->IsA<APawn>())
	{
		return;
	}

	bDropFinished = true;

	if (OnDropFinished.IsBound())
	{
		OnDropFinished.Execute(Hit.ImpactPoint);
	}

	Destroy();
}


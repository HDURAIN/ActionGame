// Fill out your copyright notice in the Description page of Project Settings.


#include "AbilitySystem/Components/InteractableComponent.h"
#include "Components/SphereComponent.h"
#include "ActorComponents/InteractCandidateComponent.h"

#include "ActionGameCharacter.h"

#include "GameFramework/Pawn.h"
#include "GameFramework/Actor.h"


// Sets default values for this component's properties
UInteractableComponent::UInteractableComponent()
{
	// Set this component to be initialized when the game starts, and to be ticked every frame.  You can turn these features
	// off to improve performance if you don't need them.
	PrimaryComponentTick.bCanEverTick = true;

	// Sphere 判定球
	InteractSphere = CreateDefaultSubobject<USphereComponent>(TEXT("Sphere"));
	InteractSphere->InitSphereRadius(250.f);
	// Collision
	InteractSphere->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
	InteractSphere->SetGenerateOverlapEvents(true);
	InteractSphere->SetCollisionResponseToAllChannels(ECR_Ignore);
	InteractSphere->SetCollisionResponseToChannel(ECC_Pawn, ECR_Overlap);
}


// Called when the game starts
void UInteractableComponent::BeginPlay()
{
	Super::BeginPlay();

	AActor* Owner = GetOwner();
	if (!Owner)
	{
		return;
	}

	// 将 Sphere 挂到 Owner 的 Root 上
	if (USceneComponent* Root = Owner->GetRootComponent())
	{
		InteractSphere->AttachToComponent(
			Root,
			FAttachmentTransformRules::KeepRelativeTransform);
	}

	// 绑定 Overlap
	InteractSphere->OnComponentBeginOverlap.AddDynamic(
		this, &UInteractableComponent::OnSphereBeginOverlap);

	InteractSphere->OnComponentEndOverlap.AddDynamic(
		this, &UInteractableComponent::OnSphereEndOverlap);

	TArray<AActor*> OverlappingActors;
	InteractSphere->GetOverlappingActors(OverlappingActors, APawn::StaticClass());

	for (AActor* Actor : OverlappingActors)
	{
		if (UInteractCandidateComponent* CandidateComp =
			Actor->FindComponentByClass<UInteractCandidateComponent>())
		{
			CandidateComp->AddCandidate(GetOwner());
		}
	}
}

void UInteractableComponent::OnSphereBeginOverlap(
	UPrimitiveComponent* OverlappedComp,
	AActor* OtherActor,
	UPrimitiveComponent* OtherComp,
	int32 OtherBodyIndex,
	bool bFromSweep,
	const FHitResult& SweepResult)
{
	if (!OtherActor)
	{
		return;
	}

	if (UInteractCandidateComponent* CandidateComp =
		OtherActor->FindComponentByClass<UInteractCandidateComponent>())
	{
		CandidateComp->AddCandidate(GetOwner());
	}
}

void UInteractableComponent::OnSphereEndOverlap(
	UPrimitiveComponent* OverlappedComp,
	AActor* OtherActor,
	UPrimitiveComponent* OtherComp,
	int32 OtherBodyIndex)
{
	if (!OtherActor)
	{
		return;
	}

	if (UInteractCandidateComponent* CandidateComp =
		OtherActor->FindComponentByClass<UInteractCandidateComponent>())
	{
		CandidateComp->RemoveCandidate(GetOwner());
	}
}



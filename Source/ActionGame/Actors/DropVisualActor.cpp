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
	PrimaryActorTick.bCanEverTick = true;
	bReplicates = true;

	// Root
	RootComponent = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));

	// Mesh
	MeshComponent = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Mesh"));
	MeshComponent->SetupAttachment(RootComponent);

	/**
	 * - DropVisualActor 是“表现体”，可以用物理
	 * - 但它绝不参与角色交互 / Movement 解算
	 */

	MeshComponent->SetSimulatePhysics(true);
	MeshComponent->SetEnableGravity(true);

	// 碰撞只用于“落地检测”
	MeshComponent->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
	MeshComponent->SetCollisionObjectType(ECC_WorldDynamic);

	MeshComponent->SetCollisionResponseToAllChannels(ECR_Ignore);
	MeshComponent->SetCollisionResponseToChannel(ECC_WorldStatic, ECR_Block);
}

// Called when the game starts or when spawned
void ADropVisualActor::BeginPlay()
{
	Super::BeginPlay();


}

void ADropVisualActor::StartDrop(const FVector& ForwardDirection)
{
	if (!MeshComponent || !MeshComponent->IsSimulatingPhysics())
	{
		return;
	}

	const FVector LaunchDir = ForwardDirection.GetSafeNormal();

	const FVector Impulse = LaunchDir * ForwardImpulse + FVector(0.f, 0.f, UpImpulse);
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

void ADropVisualActor::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	if (bDropFinished)
	{
		return;
	}

	CheckDropFinished();
}

void ADropVisualActor::CheckDropFinished()
{
	if (!MeshComponent || !MeshComponent->IsSimulatingPhysics())
	{
		return;
	}

	/**
	 * 判断“落地完成”的标准：
	 * - 当前已经发生物理碰撞
	 * - 且 Z 方向速度很小（基本静止）
	 *
	 * 注意：
	 * 不用 OnHit 事件，是为了避免：
	 * - 弹跳
	 * - 斜坡多次触发
	 */
	const FVector Velocity = MeshComponent->GetPhysicsLinearVelocity();

	const bool bAlmostStopped = FMath::Abs(Velocity.Z) < 5.f && Velocity.SizeSquared() < 25.f;

	if (!bAlmostStopped) {
		return;
	}

	// 向下做一次Trace 获取“地面点”
	FVector Start = GetActorLocation();
	FVector End = Start - FVector(0.f, 0.f, GroundTraceDistance);

	FHitResult Hit;
	FCollisionQueryParams Params;
	Params.AddIgnoredActor(this);

	const bool bHitGround = GetWorld()->LineTraceSingleByChannel(Hit, Start, End, ECC_WorldStatic, Params);

	if (!bHitGround)
	{
		return;
	}

	NotifyDropLanded(Hit.ImpactPoint);
}

void ADropVisualActor::NotifyDropLanded(const FVector& GroundLocation)
{
	if (bDropFinished)
	{
		return;
	}

	bDropFinished = true;

	if (OnDropFinished.IsBound())
	{
		OnDropFinished.Execute(GroundLocation);
	}

	Destroy();
}



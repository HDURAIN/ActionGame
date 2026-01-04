// Fill out your copyright notice in the Description page of Project Settings.


#include "ActorComponents/FootstepsComponent.h"
#include "PhysicalMaterials/AG_PhysicalMaterial.h"
#include "ActionGameCharacter.h"
#include "DrawDebugHelpers.h"
#include <Kismet/GameplayStatics.h>

static TAutoConsoleVariable<int32> CVarShowFootsteps(
	TEXT("ShowDebugFootsteps"),
	0,
	TEXT("Draws debug info about footsteps\n")
	TEXT(" 0: off\n")
	TEXT(" 1: on"),
	ECVF_Cheat
);

// Sets default values for this component's properties
UFootstepsComponent::UFootstepsComponent()
{
	// Set this component to be initialized when the game starts, and to be ticked every frame.  You can turn these features
	// off to improve performance if you don't need them.
	PrimaryComponentTick.bCanEverTick = false;

	// ...
}


// Called when the game starts
void UFootstepsComponent::BeginPlay()
{
	Super::BeginPlay();

	// ...
	
}

void UFootstepsComponent::HandleFootstep(EFoot Foot)
{
	UWorld* World = GetWorld();
	if (!World)
	{
		return;
	}

	AActionGameCharacter* Character = Cast<AActionGameCharacter>(GetOwner());
	if (!Character)
	{
		return;
	}

	const int32 DebugShowFootsteps = CVarShowFootsteps.GetValueOnGameThread();

	USkeletalMeshComponent* Mesh = Character->GetMesh();
	if (!Mesh)
	{
		return;
	}

	// 获取脚步位置
	const FName SocketName = (Foot == EFoot::Left) ? LeftFootSocketName : RightFootSocketName;
	if (!Mesh->DoesSocketExist(SocketName))
	{
		return;
	}

	// 拿到脚的位置
	const FVector SocketLocation = Mesh->GetSocketLocation(SocketName);
	const FVector Location = SocketLocation + FVector::UpVector * 20.f;

	// Trace 参数
	FHitResult HitResult; // 是否命中 命中位置 法线 命中的Actor/component 物理材质
	FCollisionQueryParams QueryParam(SCENE_QUERY_STAT(FootstepTrace), false, Character); // 定义了怎么查 参数：射线名 简单碰撞/三角形网格 *忽略对象*
	QueryParam.bReturnPhysicalMaterial = true;  // 返回物理材质必须

	const FVector TraceEnd = Location + FVector::UpVector * -50.f;

	// ECC_WorldStatic 只检测静态物体
	if (World->LineTraceSingleByChannel(
		HitResult,
		Location,
		TraceEnd,
		ECC_WorldStatic,
		QueryParam))
	{
		if (HitResult.bBlockingHit && HitResult.PhysMaterial.IsValid())
		{
			UAG_PhysicalMaterial* PhysicalMaterial =
				Cast<UAG_PhysicalMaterial>(HitResult.PhysMaterial.Get());

			if (PhysicalMaterial)
			{
				// 播放声音（安全）
				if (PhysicalMaterial->FootstepSound)
				{
					UGameplayStatics::PlaySoundAtLocation(
						World,
						PhysicalMaterial->FootstepSound,
						Location,
						1.f
					);
				}

				// Debug 文本（安全）
				if (DebugShowFootsteps > 0)
				{	
					const FString PhysMatName = PhysicalMaterial->GetName();
					DrawDebugString(
						World,
						Location,
						PhysMatName,
						nullptr,
						FColor::White,
						4.f
					);
				}
			}

			if (DebugShowFootsteps > 0)
			{
				DrawDebugSphere(World, Location, 16.f, 16, FColor::Green, false, 4.f);
			}
		}
	}
	else if (DebugShowFootsteps > 0)
	{
		DrawDebugLine(World, Location, TraceEnd, FColor::Red, false, 4.f, 0, 1.f);
		DrawDebugSphere(World, Location, 16.f, 16, FColor::Red, false, 4.f);
	}
}


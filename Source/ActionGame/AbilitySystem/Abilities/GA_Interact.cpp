// Fill out your copyright notice in the Description page of Project Settings.


#include "AbilitySystem/Abilities/GA_Interact.h"

#include "Interfaces/Interactable.h"

#include "Camera/CameraComponent.h"
#include "GameFramework/Actor.h"
#include "GameFramework/Character.h"
#include "GameFramework/PlayerController.h"
#include "Engine/World.h"

UGA_Interact::UGA_Interact()
{
	InstancingPolicy = EGameplayAbilityInstancingPolicy::InstancedPerActor;
	// 交互结果必须由服务器决定
	NetExecutionPolicy = EGameplayAbilityNetExecutionPolicy::ServerOnly;
}


/*
* Client：1负责发起交互意图 2不产生世界副作用
* Server：1重新选目标 2再校验一次 3执行真正的交互
*/
void UGA_Interact::ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData)
{
	if (!ActorInfo || !ActorInfo->AvatarActor.IsValid())
	{
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
		return;
	}

	// ServerOnly 客户端触发时 GAS会自动转发到服务器执行
	if (!HasAuthority(&ActivationInfo))
	{
		return;
	}

	Super::ActivateAbility(Handle, ActorInfo, ActivationInfo, TriggerEventData);

	UE_LOG(LogTemp, Warning, TEXT("Call [ActivateAbility]"));

	// 服务器端选择并校验交互目标（权威判定）
	AActor* TargetActor = FindBestInteractableTarget(ActorInfo);
	if (TargetActor)
	{
		TryInteractWithTarget(ActorInfo, TargetActor);
	}

	EndAbility(Handle, ActorInfo, ActivationInfo, false, false);
}


/*
* 不做： AddItem Destroy 改状态
* 只做：Trace 距离 接口判断
* 无副作用 Client和Server都可以调用
*/
AActor* UGA_Interact::FindBestInteractableTarget(const FGameplayAbilityActorInfo* ActorInfo) const
{	
	if (!ActorInfo || !ActorInfo->AvatarActor.IsValid())
	{
		return nullptr;
	}

	AActor* Avatar = ActorInfo->AvatarActor.Get();
	UWorld* World = Avatar->GetWorld();
	if (!World)
	{
		return nullptr;
	}

	UE_LOG(LogTemp, Warning, TEXT("Call [FindBestInteractableTarget]"));

	// 使用视线进行一次 LineTrace
	FVector ViewLocation;
	FRotator ViewRotation;

	if (APlayerController* PC = ActorInfo->PlayerController.Get())
	{
		PC->GetPlayerViewPoint(ViewLocation, ViewRotation);
	}
	else
	{
		// 兜底：没有PC时使用角色朝向
		ViewLocation = Avatar->GetActorLocation();
		ViewRotation = Avatar->GetActorRotation();
	}

	ACharacter* Character = Cast<ACharacter>(ActorInfo->AvatarActor.Get());
	if (!Character)
	{
		return nullptr;
	}

	UCameraComponent* Camera = Character->FindComponentByClass<UCameraComponent>();
	if (!Camera)
	{
		return nullptr;
	}

	const FVector TraceStart = Camera->GetComponentLocation();
	const FVector TraceEnd = TraceStart + Camera->GetForwardVector() * TraceDistance;

	FCollisionQueryParams Params(SCENE_QUERY_STAT(GA_Interact), false);
	Params.AddIgnoredActor(Avatar);

	FHitResult Hit;
	const bool bHit = World->LineTraceSingleByChannel(Hit, TraceStart, TraceEnd, ECC_Visibility, Params);
	DrawDebugLine(
		World,
		TraceStart,
		TraceEnd,
		FColor::Red,
		false,
		2.0f,
		0,
		2.0f
	);

	if (!bHit)
	{
		return nullptr;
	}

	AActor* HitActor = Hit.GetActor();
	if (!HitActor)
	{
		return nullptr;
	}

	const float DistSq =
		FVector::DistSquared(
			Avatar->GetActorLocation(),
			HitActor->GetActorLocation()
		);

	if (DistSq > FMath::Square(InteractDistance))
	{
		return nullptr;
	}

	// 视线命中的Actor实现了接口，就认为是可交互候选
	if (HitActor->Implements<UInteractable>())
	{
		return HitActor;
	}

	return nullptr;
}


/*
* 接口驱动的执行入口
* 不关心Item/Chest/Door
* 只做三件事：1是否实现接口 2CanInterAct 3ExecuteInteract(Server权威)
*/
void UGA_Interact::TryInteractWithTarget(const FGameplayAbilityActorInfo* ActorInfo, AActor* TargetActor) const
{
	if (!ActorInfo || !ActorInfo->AvatarActor.IsValid())
	{
		return;
	}

	AActor* Interactor = ActorInfo->AvatarActor.Get();

	// 接口判断 （Server再校验）
	if (!TargetActor->Implements<UInteractable>())
	{
		return;
	}

	UE_LOG(LogTemp, Warning, TEXT("UGA_Interact Call [TryInteractWithTarget]"));
	UE_LOG(LogTemp, Warning, TEXT("UGA_Interact Call [Execute_CanInteract]"));
	const bool bCanInteract = IInteractable::Execute_CanInteract(TargetActor, Interactor);

	if (!bCanInteract)
	{
		return;
	}

	UE_LOG(LogTemp, Warning, TEXT("UGA_Interact Call [Execute_ExecuteInteract]"));
	IInteractable::Execute_ExecuteInteract(TargetActor, Interactor);
}

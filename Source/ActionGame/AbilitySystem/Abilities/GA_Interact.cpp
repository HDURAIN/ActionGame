// Fill out your copyright notice in the Description page of Project Settings.


#include "AbilitySystem/Abilities/GA_Interact.h"

#include "Interfaces/Interactable.h"

#include "ActionGameCharacter.h"
#include "ActorComponents/InteractCandidateComponent.h"

#include "AbilitySystem/AttributeSets/AG_AttributeSetBase.h"

#include "Camera/CameraComponent.h"
#include "GameFramework/Actor.h"
#include "GameFramework/Character.h"
#include "GameFramework/PlayerController.h"
#include "Engine/World.h"
#include "Components/PrimitiveComponent.h"

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
	
	// 必须先调用 Super
	Super::ActivateAbility(Handle, ActorInfo, ActivationInfo, TriggerEventData);
	UE_LOG(LogTemp, Warning, TEXT("Call [ActivateAbility]"));

	// ServerOnly 客户端触发时 GAS会自动转发到服务器执行
	if (!HasAuthority(&ActivationInfo))
	{
		return;
	}

	// 服务器端选择并校验交互目标（权威判定）
	AActor* TargetActor = FindBestInteractableTarget(ActorInfo);
	if (!TargetActor)
	{
		EndAbility(Handle, ActorInfo, ActivationInfo, true, false);
		return;
	}

	// 提交Ability，消耗资源
	if (!CommitAbility(Handle, ActorInfo, ActivationInfo))
	{
		UE_LOG(LogTemp, Warning, TEXT("CommitAbility failed (cost or cooldown)"));
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
		return;
	}

	TryInteractWithTarget(ActorInfo, TargetActor);

	EndAbility(Handle, ActorInfo, ActivationInfo, false, false);
}

void UGA_Interact::ApplyCost(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo) const
{
	if (!ActorInfo || !ActorInfo->AvatarActor.IsValid())
		return;

	AActor* TargetActor = FindBestInteractableTarget(ActorInfo);
	if (!TargetActor)
		return;

	// 从接口读取 cost
	float Cost = 0.f;
	if (TargetActor->Implements<UInteractable>())
	{
		Cost = IInteractable::Execute_GetInteractCost(TargetActor);
	}

	// 创建 GE Spec
	FGameplayEffectSpecHandle SpecHandle =
		MakeOutgoingGameplayEffectSpec(CostGameplayEffectClass, GetAbilityLevel());

	if (SpecHandle.IsValid())
	{
		// 写入 SetByCaller（注意负号）
		SpecHandle.Data->SetSetByCallerMagnitude(
			FGameplayTag::RequestGameplayTag("Data.Cost.Gold"),
			-Cost
		);

		// 应用 GE
		ApplyGameplayEffectSpecToOwner(
			Handle,
			ActorInfo,
			ActivationInfo,
			SpecHandle
		);
	}
}

bool UGA_Interact::CheckCost(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, OUT FGameplayTagContainer* OptionalRelevantTags) const
{
	if (!ActorInfo || !ActorInfo->AvatarActor.IsValid())
		return false;

	// 找目标
	AActor* TargetActor = FindBestInteractableTarget(ActorInfo);
	if (!TargetActor)
		return false;

	// 读取交互 cost
	float Cost = 0.f;
	if (TargetActor->Implements<UInteractable>())
	{
		Cost = IInteractable::Execute_GetInteractCost(TargetActor);
	}

	// 读取当前 Gold
	const UAbilitySystemComponent* ASC = ActorInfo->AbilitySystemComponent.Get();
	if (!ASC)
		return false;

	float CurrentGold =
		ASC->GetNumericAttribute(UAG_AttributeSetBase::GetGoldAttribute());

	if (CurrentGold < Cost)
	{
		UE_LOG(LogTemp, Warning, TEXT("Not enough gold. Need %.1f have %.1f"), Cost, CurrentGold);
		return false;
	}

	return true;
}


/*
* 不做： AddItem Destroy 改状态
* 无副作用 Client和Server都可以调用
*/
AActor* UGA_Interact::FindBestInteractableTarget(const FGameplayAbilityActorInfo* ActorInfo) const
{	
	// 基础校验
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

	// 检测玩家视线是否对准物体
	// 获取视角
	FVector ViewLocation;
	FRotator ViewRotation;

	if (APlayerController* PC = ActorInfo->PlayerController.Get())
	{
		PC->GetPlayerViewPoint(ViewLocation, ViewRotation);
	}
	else
	{
		ViewLocation = Avatar->GetActorLocation();
		ViewRotation = Avatar->GetActorRotation();
	}

	const FVector TraceStart = ViewLocation;
	const FVector TraceEnd = TraceStart + ViewRotation.Vector() * TraceDistance;

	FCollisionQueryParams Params(SCENE_QUERY_STAT(GA_Interact), false);
	Params.AddIgnoredActor(Avatar);

	FHitResult Hit;

	// Trace Item
	const bool bHitPickup = World->LineTraceSingleByChannel(
		Hit,
		TraceStart,
		TraceEnd,
		ECC_GameTraceChannel1,
		Params
	);

	if (bHitPickup)
	{
		UPrimitiveComponent* HitComp = Hit.GetComponent();
		if (HitComp && HitComp->ComponentHasTag(InteractTags::InteractTarget))
		{
			AActor* TargetActor = HitComp->GetOwner();

			if (TargetActor->Implements<UInteractable>())
			{
				APawn* Pawn = Cast<APawn>(Avatar);
				if (IInteractable::Execute_CanInteract(TargetActor, Pawn))
				{
					return TargetActor;
				}
			}
		}
	}

	// Trace Chest
	const bool bHitUse = World->LineTraceSingleByChannel(
		Hit,
		TraceStart,
		TraceEnd,
		ECC_GameTraceChannel2,
		Params
	);

	if (bHitUse)
	{
		UPrimitiveComponent* HitComp = Hit.GetComponent();
		if (HitComp && HitComp->ComponentHasTag(InteractTags::InteractTarget))
		{
			AActor* TargetActor = HitComp->GetOwner();

			if (TargetActor->Implements<UInteractable>())
			{
				APawn* Pawn = Cast<APawn>(Avatar);
				if (IInteractable::Execute_CanInteract(TargetActor, Pawn))
				{
					return TargetActor;
				}
			}
		}
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

	// 判断目标Actor是否实现了接口 （Server再校验）
	if (!TargetActor->Implements<UInteractable>())
	{
		return;
	}

	const bool bCanInteract = IInteractable::Execute_CanInteract(TargetActor, Interactor);

	if (UInteractCandidateComponent* CandidateComp = Interactor->FindComponentByClass<UInteractCandidateComponent>())
	{
		// 是否仍在交互候选集合中
		if (!CandidateComp->IsInteractCandidate(TargetActor))
		{
			return;
		}
	}

	if (!bCanInteract)
	{
		return;
	}

	IInteractable::Execute_ExecuteInteract(TargetActor, Interactor);
}

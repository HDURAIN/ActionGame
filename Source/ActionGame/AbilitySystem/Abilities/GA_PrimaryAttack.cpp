// Fill out your copyright notice in the Description page of Project Settings.


#include "AbilitySystem/Abilities/GA_PrimaryAttack.h"
#include "ActionGame/ActionGameCharacter.h"
#include "Abilities/Tasks/AbilityTask_PlayMontageAndWait.h"
#include "Abilities/Tasks/AbilityTask_WaitDelay.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "TimerManager.h"
#include "NiagaraFunctionLibrary.h"
#include "NiagaraSystem.h"
#include "NiagaraComponent.h"

FORCEINLINE ECollisionChannel GetChannel(FName Name)
{
	const UCollisionProfile* Profile = UCollisionProfile::Get();

	for (int32 i = 0; i < ECC_MAX; i++)
	{
		if (Profile->ReturnChannelNameFromContainerIndex(i) == Name)
		{
			return (ECollisionChannel)i;
		}
	}

	UE_LOG(LogTemp, Error, TEXT("Collision Channel not found: %s"), *Name.ToString());
	return ECC_Visibility;
}
namespace CollisionChannels
{
	FORCEINLINE ECollisionChannel Weapon()
	{
		static ECollisionChannel Channel = GetChannel("WeaponTrace");
		return Channel;
	}
}

UGA_PrimaryAttack::UGA_PrimaryAttack()
{
	// 本地预测执行：
	// 客户端立即响应输入以保证手感，
	// 服务器随后校验并同步最终结果
	NetExecutionPolicy = EGameplayAbilityNetExecutionPolicy::LocalPredicted;

	// 每个 Avatar 拥有一个独立的 Ability 实例：
	// - 允许 Ability 内部保存临时状态
	// - 避免并发预测时状态互相覆盖
	InstancingPolicy = EGameplayAbilityInstancingPolicy::InstancedPerActor;
}

void UGA_PrimaryAttack::OnAbilityActivated()
{
	ApplyEffect();
	DoTrace();
}

void UGA_PrimaryAttack::OnAbilityEnded(bool bWasCancelled)
{
	
}

bool UGA_PrimaryAttack::DoCameraTrace(FHitResult& OutHit, FVector& OutAimPoint)
{
	ACharacter* Avatar = Cast<ACharacter>(GetAvatarActorFromActorInfo());
	if (!Avatar) return false;

	APlayerController* PC = Cast<APlayerController>(Avatar->GetController());
	if (!PC) return false;

	FVector CamLoc;
	FRotator CamRot;
	PC->GetPlayerViewPoint(CamLoc, CamRot);

	FVector End = CamLoc + CamRot.Vector() * 10000.f;

	FCollisionQueryParams Params;
	Params.AddIgnoredActor(Avatar);

	ECollisionChannel WeaponChannel = CollisionChannels::Weapon();

	bool bHit = GetWorld()->LineTraceSingleByChannel(
		OutHit,
		CamLoc,
		End,
		WeaponChannel,
		Params
	);

	OutAimPoint = bHit ? OutHit.ImpactPoint : End;

	DrawDebugLine(
		GetWorld(),
		CamLoc,
		OutAimPoint,
		FColor::Green,
		false,
		1.0f,
		0,
		1.0f
	);

	return true;
}

void UGA_PrimaryAttack::DoWeaponTrace(const FVector& AimPoint)
{
	ACharacter* Avatar = Cast<ACharacter>(GetAvatarActorFromActorInfo());
	if (!Avatar) return;

	USkeletalMeshComponent* Mesh = Avatar->GetMesh();
	FVector MuzzleLoc = Mesh->GetSocketLocation("Muzzle");

	FVector Dir = (AimPoint - MuzzleLoc).GetSafeNormal();
	FVector End = MuzzleLoc + Dir * 10000.f;

	FHitResult Hit;

	FCollisionQueryParams Params;
	Params.AddIgnoredActor(Avatar);

	ECollisionChannel WeaponChannel = CollisionChannels::Weapon();

	bool bHit = GetWorld()->LineTraceSingleByChannel(
		Hit,
		MuzzleLoc,
		End,
		WeaponChannel,
		Params
	);

	DrawDebugLine(
		GetWorld(),
		MuzzleLoc,
		bHit ? Hit.ImpactPoint : End,
		FColor::Red,
		false,
		1.0f,
		0,
		1.5f
	);

	if (bHit && Hit.bBlockingHit)
	{
		if (UAbilitySystemComponent* ASC = GetASC())
		{
			FGameplayCueParameters CueParams;
			CueParams.Location = Hit.ImpactPoint;
			CueParams.Normal = Hit.ImpactNormal;
			CueParams.Instigator = GetAvatarActorFromActorInfo();
			CueParams.EffectCauser = GetAvatarActorFromActorInfo();

			// 可选：把HitResult也带进去（有些版本字段/接口略有差异）
			// CueParams.PhysicalMaterial = Hit.PhysMaterial.Get();

			static const FGameplayTag ImpactCueTag =
				FGameplayTag::RequestGameplayTag(FName("GameplayCue.Weapon.Impact"));

			ASC->ExecuteGameplayCue(ImpactCueTag, CueParams);
		}
	}
}

void UGA_PrimaryAttack::DoTrace()
{
	FHitResult CameraHit;
	FVector AimPoint;

	if (DoCameraTrace(CameraHit, AimPoint))
	{
		DoWeaponTrace(AimPoint);
	}
}

void UGA_PrimaryAttack::ApplyEffect()
{
	const FGameplayAbilityActorInfo* ActorInfo = GetCurrentActorInfo();

	if (FireStateEffect && ActorInfo->AbilitySystemComponent.IsValid())
	{
		FGameplayEffectContextHandle EffectContext = ActorInfo->AbilitySystemComponent->MakeEffectContext();
		FGameplayEffectSpecHandle Spec = ActorInfo->AbilitySystemComponent->MakeOutgoingSpec(FireStateEffect, 1.f, EffectContext);
		if (Spec.IsValid())
		{
			FireEffectHandle = ActorInfo->AbilitySystemComponent->ApplyGameplayEffectSpecToSelf(*Spec.Data.Get());
		}
	}
}

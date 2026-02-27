// Fill out your copyright notice in the Description page of Project Settings.

#include "AbilitySystem/Abilities/GA_PrimaryAttack.h"

#include "ActionGameCharacter.h"
#include "AbilitySystemComponent.h"
#include "AbilitySystemBlueprintLibrary.h"
#include "AbilitySystemLog.h"
#include "GameFramework/PlayerController.h"
#include "DrawDebugHelpers.h"
#include "Components/SkeletalMeshComponent.h"
#include "Engine/World.h"
#include "AbilitySystem/AttributeSets/AG_EnemyAttributeSet.h"
#include "AbilitySystem/AttributeSets/AG_AttributeSetBase.h"

static ECollisionChannel GetChannelByName(FName Name)
{
	const UCollisionProfile* Profile = UCollisionProfile::Get();
	if (!Profile) return ECC_Visibility;

	for (int32 i = 0; i < ECC_MAX; i++)
	{
		if (Profile->ReturnChannelNameFromContainerIndex(i) == Name)
		{
			return (ECollisionChannel)i;
		}
	}

	UE_LOG(LogTemp, Warning, TEXT("Collision Channel not found: %s (fallback to Visibility)"), *Name.ToString());
	return ECC_Visibility;
}

UGA_PrimaryAttack::UGA_PrimaryAttack()
{
	// 本地预测：客户端先响应输入，服务器校验并同步
	NetExecutionPolicy = EGameplayAbilityNetExecutionPolicy::LocalPredicted;

	// 每个角色一个独立实例，方便后续在Ability里保存运行时状态
	InstancingPolicy = EGameplayAbilityInstancingPolicy::InstancedPerActor;

	// 默认命中Cue（也可以在蓝图里覆盖这个Tag）
	if (!ImpactCueTag.IsValid())
	{
		ImpactCueTag = FGameplayTag::RequestGameplayTag(TEXT("GameplayCue.Weapon.Impact"));
	}

	DamageDataTag = FGameplayTag::RequestGameplayTag(TEXT("Data.Damage"));
	CooldownDataTag = FGameplayTag::RequestGameplayTag(TEXT("Data.CD.PrimaryAttack"));
	CooldownTagContainer.AddTag(FGameplayTag::RequestGameplayTag(TEXT("Cooldown.PrimaryAttack")));
}

void UGA_PrimaryAttack::ActivateAbility(
	const FGameplayAbilitySpecHandle Handle,
	const FGameplayAbilityActorInfo* ActorInfo,
	const FGameplayAbilityActivationInfo ActivationInfo,
	const FGameplayEventData* TriggerEventData
)
{
	// 基础校验（避免蓝图事件在无效上下文下执行）
	if (!ActorInfo || !ActorInfo->AvatarActor.IsValid())
	{
		UE_LOG(LogAbilitySystem, Warning,
			TEXT("[%s] ActivateAbility FAILED: invalid ActorInfo/Avatar"),
			*GetName());

		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
		return;
	}

	// 进入蓝图流程：
	// - PlayMontageAndWait
	// - WaitGameplayEvent(Shoot)
	K2_OnActivateFromEvent();
}

void UGA_PrimaryAttack::EndAbility(
	const FGameplayAbilitySpecHandle Handle,
	const FGameplayAbilityActorInfo* ActorInfo,
	const FGameplayAbilityActivationInfo ActivationInfo,
	bool bReplicateEndAbility,
	bool bWasCancelled
)
{
	Super::EndAbility(Handle, ActorInfo, ActivationInfo, bReplicateEndAbility, bWasCancelled);
}

const FGameplayTagContainer* UGA_PrimaryAttack::GetCooldownTags() const
{
	return &CooldownTagContainer;
}

void UGA_PrimaryAttack::ApplyCooldown(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo) const
{
	if (!ActorInfo || !ActorInfo->AbilitySystemComponent.IsValid() || !CooldownGameplayEffectClass)
	{
		return;
	}

	UAbilitySystemComponent* ASC = ActorInfo->AbilitySystemComponent.Get();

	const float CDR =
		ASC->GetNumericAttribute(UAG_AttributeSetBase::GetCooldownReductionAttribute());

	const float FinalCD = FMath::Max(0.f, BaseCooldown * (1.f - CDR));

	FGameplayEffectSpecHandle SpecHandle =
		MakeOutgoingGameplayEffectSpec(CooldownGameplayEffectClass, GetAbilityLevel(Handle, ActorInfo));

	if (!SpecHandle.IsValid())
	{
		return;
	}

	SpecHandle.Data->SetSetByCallerMagnitude(CooldownDataTag, FinalCD);
	ASC->ApplyGameplayEffectSpecToSelf(*SpecHandle.Data.Get());
}

void UGA_PrimaryAttack::Shoot_TraceAndCue()
{
	AActionGameCharacter* Character = GetCharacter();
	if (!Character) return;

	// 只让服务器做权威 Trace + GameplayCue 广播
	if (!Character->HasAuthority())
	{
		return;
	}

	UAbilitySystemComponent* ASC = GetASC();
	if (!ASC)
	{
		UE_LOG(LogAbilitySystem, Warning, TEXT("[%s] Shoot_TraceAndCue FAILED: ASC is null"), *GetName());
		return;
	}

	// 如果你希望“扣子弹/进冷却”也发生在 Shoot 帧，这里提交
	if (!CommitAbilityChecked())
	{
		// Commit失败通常表示资源/冷却/Tag不满足
		EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, true, true);
		return;
	}

	// 1) 相机射线：求瞄准点
	APlayerController* PC = Cast<APlayerController>(Character->GetController());
	if (!PC)
	{
		UE_LOG(LogAbilitySystem, Warning, TEXT("[%s] Shoot_TraceAndCue FAILED: PlayerController is null"), *GetName());
		return;
	}

	FVector CamLoc;
	FRotator CamRot;
	PC->GetPlayerViewPoint(CamLoc, CamRot);

	const FVector CamEnd = CamLoc + CamRot.Vector() * TraceDistance;

	FCollisionQueryParams CamParams(SCENE_QUERY_STAT(PrimaryAttack_CamTrace), false);
	CamParams.AddIgnoredActor(Character);

	const ECollisionChannel WeaponChannel = GetChannelByName(TEXT("WeaponTrace"));

	FHitResult CamHit;
	const bool bCamHit = Character->GetWorld()->LineTraceSingleByChannel(
		CamHit,
		CamLoc,
		CamEnd,
		WeaponChannel,
		CamParams
	);

	const FVector AimPoint = bCamHit ? CamHit.ImpactPoint : CamEnd;

	// 2) 枪口射线：从Muzzle朝AimPoint打出去
	USkeletalMeshComponent* Mesh = Character->GetMesh();
	if (!Mesh)
	{
		UE_LOG(LogAbilitySystem, Warning, TEXT("[%s] Shoot_TraceAndCue FAILED: Mesh is null"), *GetName());
		return;
	}

	const FVector MuzzleLoc = Mesh->GetSocketLocation(MuzzleSocketName);
	const FVector Dir = (AimPoint - MuzzleLoc).GetSafeNormal();
	const FVector MuzzleEnd = MuzzleLoc + Dir * TraceDistance;

	FCollisionQueryParams WeaponParams(SCENE_QUERY_STAT(PrimaryAttack_WeaponTrace), false);
	WeaponParams.AddIgnoredActor(Character);

	FHitResult Hit;
	const bool bHit = Character->GetWorld()->LineTraceSingleByChannel(
		Hit,
		MuzzleLoc,
		MuzzleEnd,
		WeaponChannel,
		WeaponParams
	);

	if (bHit && Hit.bBlockingHit)
	{
		// 3) 用 ASC 触发 Cue（关键：不要用 ExecuteGameplayCueOnActor）
		if (ImpactCueTag.IsValid())
		{
			FGameplayCueParameters Params;
			Params.Location = Hit.ImpactPoint;
			Params.Normal = Hit.ImpactNormal;
			Params.Instigator = Character;
			Params.EffectCauser = Character;

			ASC->ExecuteGameplayCue(ImpactCueTag, Params);
		}
		else
		{
			UE_LOG(LogAbilitySystem, Warning, TEXT("[%s] Shoot_TraceAndCue: ImpactCueTag is invalid"), *GetName());
		}

		if (DamageEffectClass)
		{
			AActor* TargetActor = Hit.GetActor();
			if (TargetActor)
			{
				UAbilitySystemComponent* TargetASC =
					UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent(TargetActor);

				if (TargetASC)
				{
					const float AttackPower =
						ASC->GetNumericAttribute(UAG_AttributeSetBase::GetAttackPowerAttribute());

					const float DmgMul =
						ASC->GetNumericAttribute(UAG_AttributeSetBase::GetDamageMultiplierAttribute());

					const float FinalDamage = FMath::Max(0.f, AttackPower * DamageCoefficient * DmgMul);

					FGameplayEffectContextHandle Context = ASC->MakeEffectContext();
					Context.AddSourceObject(this);
					Context.AddHitResult(Hit);

					FGameplayEffectSpecHandle SpecHandle =
						ASC->MakeOutgoingSpec(DamageEffectClass, GetAbilityLevel(), Context);

					if (SpecHandle.IsValid())
					{
						SpecHandle.Data->SetSetByCallerMagnitude(DamageDataTag, -FinalDamage);
						ASC->ApplyGameplayEffectSpecToTarget(*SpecHandle.Data.Get(), TargetASC);
					}
				}
			}
		}
	}
}

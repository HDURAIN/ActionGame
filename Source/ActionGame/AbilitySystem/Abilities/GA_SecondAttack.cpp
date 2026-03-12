#include "AbilitySystem/Abilities/GA_SecondAttack.h"

#include "ActionGameCharacter.h"
#include "AbilitySystemBlueprintLibrary.h"
#include "AbilitySystemComponent.h"
#include "AbilitySystemLog.h"
#include "AbilitySystem/AttributeSets/AG_AttributeSetBase.h"
#include "Characters/EnemyCharacterBase.h"
#include "Components/SceneComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "DrawDebugHelpers.h"
#include "Engine/CollisionProfile.h"
#include "Engine/OverlapResult.h"
#include "Engine/World.h"
#include "GameFramework/PlayerController.h"
#include "GameplayEffect.h"

namespace
{
ECollisionChannel GetChannelByName(const FName Name)
{
	const UCollisionProfile* Profile = UCollisionProfile::Get();
	if (!Profile)
	{
		return ECC_Visibility;
	}

	for (int32 i = 0; i < ECC_MAX; ++i)
	{
		if (Profile->ReturnChannelNameFromContainerIndex(i) == Name)
		{
			return static_cast<ECollisionChannel>(i);
		}
	}

	return ECC_Visibility;
}
}

UGA_SecondAttack::UGA_SecondAttack()
{
	NetExecutionPolicy = EGameplayAbilityNetExecutionPolicy::LocalPredicted;
	InstancingPolicy = EGameplayAbilityInstancingPolicy::InstancedPerActor;

	DamageDataTag = FGameplayTag::RequestGameplayTag(TEXT("Data.Damage"));
	CooldownDataTag = FGameplayTag::RequestGameplayTag(TEXT("Data.CD.Skill2"));
	CooldownTagContainer.AddTag(FGameplayTag::RequestGameplayTag(TEXT("Cooldown.Skill2")));
}

void UGA_SecondAttack::ActivateAbility(
	const FGameplayAbilitySpecHandle Handle,
	const FGameplayAbilityActorInfo* ActorInfo,
	const FGameplayAbilityActivationInfo ActivationInfo,
	const FGameplayEventData* TriggerEventData
)
{
	if (!ActorInfo || !ActorInfo->AvatarActor.IsValid())
	{
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
		return;
	}

	K2_OnActivateFromEvent();
}

void UGA_SecondAttack::EndAbility(
	const FGameplayAbilitySpecHandle Handle,
	const FGameplayAbilityActorInfo* ActorInfo,
	const FGameplayAbilityActivationInfo ActivationInfo,
	bool bReplicateEndAbility,
	bool bWasCancelled
)
{
	Super::EndAbility(Handle, ActorInfo, ActivationInfo, bReplicateEndAbility, bWasCancelled);
}

const FGameplayTagContainer* UGA_SecondAttack::GetCooldownTags() const
{
	return &CooldownTagContainer;
}

void UGA_SecondAttack::ApplyCooldown(
	const FGameplayAbilitySpecHandle Handle,
	const FGameplayAbilityActorInfo* ActorInfo,
	const FGameplayAbilityActivationInfo ActivationInfo
) const
{
	if (!ActorInfo || !ActorInfo->AbilitySystemComponent.IsValid() || !CooldownGameplayEffectClass)
	{
		return;
	}

	UAbilitySystemComponent* ASC = ActorInfo->AbilitySystemComponent.Get();

	const float CDR = ASC->GetNumericAttribute(UAG_AttributeSetBase::GetCooldownReductionAttribute());
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

bool UGA_SecondAttack::Fire_PenetratingSweep(
	const FVector& StartLocation,
	FVector& OutActualStart,
	FVector& OutActualEnd,
	float& OutActualLength,
	bool& bOutBlockedByWorld
)
{
	OutActualStart = StartLocation;
	OutActualEnd = StartLocation;
	OutActualLength = 0.f;
	bOutBlockedByWorld = false;

	if (!IsActive())
	{
		return false;
	}

	AActionGameCharacter* Character = GetCharacter();
	if (!Character || !Character->IsLocallyControlled())
	{
		return false;
	}

	FVector LocalSweepStart = FVector::ZeroVector;
	FVector LocalSweepEnd = FVector::ZeroVector;
	float LocalSweepLength = 0.f;
	bool bLocalBlockedByWorld = false;
	if (!ComputeActualSweepSegment(StartLocation, LocalSweepStart, LocalSweepEnd, LocalSweepLength, bLocalBlockedByWorld))
	{
		return false;
	}

	OutActualStart = LocalSweepStart;
	OutActualEnd = LocalSweepEnd;
	OutActualLength = LocalSweepLength;
	bOutBlockedByWorld = bLocalBlockedByWorld;

	if (bDebugDrawSweep)
	{
		if (UWorld* World = GetWorld())
		{
			const float DebugRadius = FMath::Max(1.f, SweepRadius);
			const FVector DebugDir = (LocalSweepEnd - LocalSweepStart).GetSafeNormal();
			const FVector DebugCenter = (LocalSweepStart + LocalSweepEnd) * 0.5f;
			const float DebugHalfHeight = LocalSweepLength * 0.5f;
			const FQuat DebugCapsuleRot = FRotationMatrix::MakeFromZ(DebugDir).ToQuat();

			DrawDebugCapsule(
				World,
				DebugCenter,
				DebugHalfHeight,
				DebugRadius,
				DebugCapsuleRot,
				FColor::Cyan,
				false,
				DebugDrawDuration,
				0,
				1.25f
			);
			DrawDebugLine(
				World,
				LocalSweepStart,
				LocalSweepEnd,
				FColor::Blue,
				false,
				DebugDrawDuration,
				0,
				1.0f
			);
		}
	}

	if (!Character->HasAuthority())
	{
		Character->ServerRequestSecondAttackSweep(CurrentSpecHandle, FVector_NetQuantize(StartLocation));
		return true;
	}

	ExecutePenetratingSweepServer(StartLocation);
	return true;
}

void UGA_SecondAttack::ServerExecutePenetratingSweepFromClient(const FVector& StartLocation)
{
	AActionGameCharacter* Character = GetCharacter();
	if (!Character || !Character->HasAuthority() || !IsActive())
	{
		return;
	}

	ExecutePenetratingSweepServer(StartLocation);
}

void UGA_SecondAttack::ExecutePenetratingSweepServer(const FVector& StartLocation)
{
	AActionGameCharacter* Character = GetCharacter();
	if (!Character || !Character->HasAuthority())
	{
		return;
	}

	UAbilitySystemComponent* SourceASC = GetASC();
	if (!SourceASC)
	{
		UE_LOG(LogAbilitySystem, Warning, TEXT("[%s] Fire_PenetratingSweep failed: ASC is null"), *GetName());
		return;
	}

	if (!CommitAbilityChecked())
	{
		EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, true, true);
		return;
	}

	FVector SweepStart = FVector::ZeroVector;
	FVector SweepEnd = FVector::ZeroVector;
	float SafeRange = 0.f;
	bool bDummyBlockedByWorld = false;
	if (!ComputeActualSweepSegment(StartLocation, SweepStart, SweepEnd, SafeRange, bDummyBlockedByWorld))
	{
		UE_LOG(LogAbilitySystem, Warning, TEXT("[%s] Fire_PenetratingSweep failed: ComputeActualSweepSegment failed"), *GetName());
		return;
	}

	UWorld* World = GetWorld();
	if (!World)
	{
		return;
	}

	const float SafeRadius = FMath::Max(1.f, SweepRadius);
	if (SafeRange <= KINDA_SMALL_NUMBER)
	{
		return;
	}

	const FVector SweepDir = (SweepEnd - SweepStart).GetSafeNormal();
	const FVector SweepCenter = (SweepStart + SweepEnd) * 0.5f;
	const float HalfHeight = SafeRange * 0.5f;
	const FQuat CapsuleRot = FRotationMatrix::MakeFromZ(SweepDir).ToQuat();

	FCollisionQueryParams QueryParams(SCENE_QUERY_STAT(SecondAttack_PenetratingOverlap), false);
	QueryParams.AddIgnoredActor(Character);

	FCollisionObjectQueryParams PawnObjParams;
	PawnObjParams.AddObjectTypesToQuery(ECC_Pawn);

	TArray<FOverlapResult> OverlapResults;
	const bool bHasOverlap = World->OverlapMultiByObjectType(
		OverlapResults,
		SweepCenter,
		CapsuleRot,
		PawnObjParams,
		FCollisionShape::MakeCapsule(SafeRadius, HalfHeight),
		QueryParams
	);

	if (!bHasOverlap)
	{
		return;
	}

	TSet<AActor*> DamagedActors;
	int32 DamagedCount = 0;

	for (const FOverlapResult& Overlap : OverlapResults)
	{
		AActor* HitActor = Overlap.GetActor();
		if (!IsValid(HitActor) || HitActor == Character)
		{
			continue;
		}

		if (!HitActor->IsA<AEnemyCharacterBase>())
		{
			continue;
		}

		if (DamagedActors.Contains(HitActor))
		{
			continue;
		}
		DamagedActors.Add(HitActor);

		UAbilitySystemComponent* TargetASC = UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent(HitActor);
		if (!TargetASC)
		{
			continue;
		}

		if (bStopOnBlockingWorld)
		{
			const FVector TargetLoc = HitActor->GetActorLocation();

			FCollisionObjectQueryParams WorldBlockObjParams;
			WorldBlockObjParams.AddObjectTypesToQuery(ECC_WorldStatic);
			WorldBlockObjParams.AddObjectTypesToQuery(ECC_WorldDynamic);

			FCollisionQueryParams WorldBlockParams(SCENE_QUERY_STAT(SecondAttack_TargetWorldBlockTrace), false);
			WorldBlockParams.AddIgnoredActor(Character);
			WorldBlockParams.AddIgnoredActor(HitActor);

			FHitResult BlockHit;
			const bool bBlocked = World->LineTraceSingleByObjectType(
				BlockHit,
				SweepStart,
				TargetLoc,
				WorldBlockObjParams,
				WorldBlockParams
			);

			if (bBlocked && BlockHit.bBlockingHit)
			{
				continue;
			}
		}

		FHitResult SyntheticHit;
		SyntheticHit.Location = HitActor->GetActorLocation();
		SyntheticHit.ImpactPoint = SyntheticHit.Location;

		ApplyDamageToTargetASC(SourceASC, TargetASC, SyntheticHit);
		++DamagedCount;

		if (MaxPenetrationTargets > 0 && DamagedCount >= MaxPenetrationTargets)
		{
			break;
		}
	}
}

bool UGA_SecondAttack::ComputeActualSweepSegment(
	const FVector& StartLocation,
	FVector& OutSweepStart,
	FVector& OutSweepEnd,
	float& OutSweepLength,
	bool& bOutBlockedByWorld
) const
{
	OutSweepStart = StartLocation;
	OutSweepEnd = StartLocation;
	OutSweepLength = 0.f;
	bOutBlockedByWorld = false;

	FVector AimDir = FVector::ZeroVector;
	if (!BuildAimDirectionFromCamera(StartLocation, AimDir))
	{
		return false;
	}

	const UWorld* World = GetWorld();
	const AActionGameCharacter* Character = GetCharacter();
	if (!World || !Character)
	{
		return false;
	}

	OutSweepStart = StartLocation;
	const FVector DesiredSweepEnd = OutSweepStart + AimDir * FMath::Max(1.f, SweepRange);
	OutSweepEnd = DesiredSweepEnd;

	if (bStopOnBlockingWorld)
	{
		FCollisionObjectQueryParams WorldBlockObjParams;
		WorldBlockObjParams.AddObjectTypesToQuery(ECC_WorldStatic);
		WorldBlockObjParams.AddObjectTypesToQuery(ECC_WorldDynamic);

		FCollisionQueryParams WorldBlockParams(SCENE_QUERY_STAT(SecondAttack_ComputeWorldBlockTrace), false);
		WorldBlockParams.AddIgnoredActor(Character);

		FHitResult BlockHit;
		const bool bBlocked = World->LineTraceSingleByObjectType(
			BlockHit,
			OutSweepStart,
			DesiredSweepEnd,
			WorldBlockObjParams,
			WorldBlockParams
		);

		if (bBlocked && BlockHit.bBlockingHit)
		{
			OutSweepEnd = BlockHit.ImpactPoint;
			bOutBlockedByWorld = true;
		}
	}

	OutSweepLength = FVector::Distance(OutSweepStart, OutSweepEnd);
	return OutSweepLength > KINDA_SMALL_NUMBER;
}

void UGA_SecondAttack::NotifySecondAttackMontageFinished(bool bWasCancelled)
{
	if (!IsActive())
	{
		return;
	}

	EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, true, bWasCancelled);
}

bool UGA_SecondAttack::GetSecondAttackBeamStartEnd(FVector& OutStart, FVector& OutEnd) const
{
	OutStart = FVector::ZeroVector;
	OutEnd = FVector::ZeroVector;

	FVector MuzzleLoc = FVector::ZeroVector;
	FVector AimDir = FVector::ZeroVector;
	if (!BuildAimFromCameraAndMuzzle(MuzzleLoc, AimDir))
	{
		return false;
	}

	OutStart = MuzzleLoc;
	OutEnd = MuzzleLoc + AimDir * FMath::Max(1.f, SweepRange);
	return true;
}

bool UGA_SecondAttack::BuildAimDirectionFromCamera(const FVector& StartLocation, FVector& OutAimDir) const
{
	OutAimDir = FVector::ZeroVector;

	const AActionGameCharacter* Character = GetCharacter();
	if (!Character)
	{
		return false;
	}

	const UWorld* World = GetWorld();
	if (!World)
	{
		return false;
	}

	const APlayerController* PC = Cast<APlayerController>(Character->GetController());
	if (!PC)
	{
		return false;
	}

	FVector CamLoc = FVector::ZeroVector;
	FRotator CamRot = FRotator::ZeroRotator;
	PC->GetPlayerViewPoint(CamLoc, CamRot);

	const FVector CamEnd = CamLoc + CamRot.Vector() * FMath::Max(1.f, TraceDistance);
	FCollisionQueryParams CamParams(SCENE_QUERY_STAT(SecondAttack_CameraTrace_OverrideStart), false);
	CamParams.AddIgnoredActor(Character);

	const ECollisionChannel WeaponChannel = GetChannelByName(WeaponTraceChannelName);

	FHitResult CamHit;
	const bool bCamHit = World->LineTraceSingleByChannel(
		CamHit,
		CamLoc,
		CamEnd,
		WeaponChannel,
		CamParams
	);

	const FVector AimPoint = bCamHit ? CamHit.ImpactPoint : CamEnd;
	OutAimDir = (AimPoint - StartLocation).GetSafeNormal();
	return !OutAimDir.IsNearlyZero();
}

void UGA_SecondAttack::ApplyDamageToTargetASC(
	UAbilitySystemComponent* SourceASC,
	UAbilitySystemComponent* TargetASC,
	const FHitResult& Hit
) const
{
	if (!SourceASC || !TargetASC || !DamageEffectClass || !DamageDataTag.IsValid())
	{
		return;
	}

	const float AttackPower = SourceASC->GetNumericAttribute(UAG_AttributeSetBase::GetAttackPowerAttribute());
	const float DamageMultiplier = SourceASC->GetNumericAttribute(UAG_AttributeSetBase::GetDamageMultiplierAttribute());
	const float FinalDamage = FMath::Max(0.f, AttackPower * DamageMultiplier * DamageCoefficient);
	if (FinalDamage <= 0.f)
	{
		return;
	}

	FGameplayEffectContextHandle Context = SourceASC->MakeEffectContext();
	Context.AddSourceObject(this);
	Context.AddHitResult(Hit);

	FGameplayEffectSpecHandle SpecHandle = SourceASC->MakeOutgoingSpec(DamageEffectClass, GetAbilityLevel(), Context);
	if (!SpecHandle.IsValid())
	{
		return;
	}

	SpecHandle.Data->SetSetByCallerMagnitude(DamageDataTag, -FinalDamage);
	SourceASC->ApplyGameplayEffectSpecToTarget(*SpecHandle.Data.Get(), TargetASC);
}

bool UGA_SecondAttack::BuildAimFromCameraAndMuzzle(FVector& OutMuzzleLoc, FVector& OutAimDir) const
{
	OutMuzzleLoc = FVector::ZeroVector;
	OutAimDir = FVector::ZeroVector;

	const AActionGameCharacter* Character = GetCharacter();
	if (!Character)
	{
		return false;
	}

	const UWorld* World = GetWorld();
	if (!World)
	{
		return false;
	}

	const APlayerController* PC = Cast<APlayerController>(Character->GetController());
	if (!PC)
	{
		return false;
	}

	FVector CamLoc = FVector::ZeroVector;
	FRotator CamRot = FRotator::ZeroRotator;
	PC->GetPlayerViewPoint(CamLoc, CamRot);

	const FVector CamEnd = CamLoc + CamRot.Vector() * FMath::Max(1.f, TraceDistance);
	FCollisionQueryParams CamParams(SCENE_QUERY_STAT(SecondAttack_CameraTrace), false);
	CamParams.AddIgnoredActor(Character);

	const ECollisionChannel WeaponChannel = GetChannelByName(WeaponTraceChannelName);

	FHitResult CamHit;
	const bool bCamHit = World->LineTraceSingleByChannel(
		CamHit,
		CamLoc,
		CamEnd,
		WeaponChannel,
		CamParams
	);

	const FVector AimPoint = bCamHit ? CamHit.ImpactPoint : CamEnd;

	if (!TryGetRightWeaponMuzzleLocation(OutMuzzleLoc))
	{
		UE_LOG(LogAbilitySystem, Warning, TEXT("[%s] BuildAimFromCameraAndMuzzle failed: weapon muzzle socket '%s' not found on right weapon actor."),
			*GetName(), *MuzzleSocketName.ToString());
		return false;
	}

	OutAimDir = (AimPoint - OutMuzzleLoc).GetSafeNormal();
	return !OutAimDir.IsNearlyZero();
}

bool UGA_SecondAttack::TryGetRightWeaponMuzzleLocation(FVector& OutMuzzleLoc) const
{
	const AActionGameCharacter* Character = GetCharacter();
	if (!Character)
	{
		return false;
	}

	AActor* WeaponActor = Character->GetWeaponActor();
	if (!IsValid(WeaponActor))
	{
		return false;
	}

	TInlineComponentArray<USceneComponent*> SceneComponents;
	WeaponActor->GetComponents(SceneComponents);

	for (const USceneComponent* SceneComp : SceneComponents)
	{
		if (!SceneComp)
		{
			continue;
		}

		if (SceneComp->DoesSocketExist(MuzzleSocketName))
		{
			OutMuzzleLoc = SceneComp->GetSocketLocation(MuzzleSocketName);
			return true;
		}
	}

	if (const USceneComponent* RootComp = WeaponActor->GetRootComponent())
	{
		OutMuzzleLoc = RootComp->GetComponentLocation();
		return true;
	}

	return false;
}


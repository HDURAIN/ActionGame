#include "AbilitySystem/Abilities/GA_Ultimate.h"

#include "ActionGameCharacter.h"
#include "AbilitySystemBlueprintLibrary.h"
#include "AbilitySystemComponent.h"
#include "AbilitySystemLog.h"
#include "AbilitySystem/AttributeSets/AG_AttributeSetBase.h"
#include "ActorComponents/AG_CharacterMovementComponent.h"
#include "Characters/EnemyCharacterBase.h"
#include "DrawDebugHelpers.h"
#include "Engine/OverlapResult.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameplayEffect.h"
#include "Engine/World.h"

UGA_Ultimate::UGA_Ultimate()
{
	NetExecutionPolicy = EGameplayAbilityNetExecutionPolicy::LocalPredicted;
	InstancingPolicy = EGameplayAbilityInstancingPolicy::InstancedPerActor;

	DamageDataTag = FGameplayTag::RequestGameplayTag(TEXT("Data.Damage"));
	CooldownDataTag = FGameplayTag::RequestGameplayTag(TEXT("Data.CD.Skill4"));
	CooldownTagContainer.AddTag(FGameplayTag::RequestGameplayTag(TEXT("Cooldown.Skill4")));
}

void UGA_Ultimate::ActivateAbility(
	const FGameplayAbilitySpecHandle Handle,
	const FGameplayAbilityActorInfo* ActorInfo,
	const FGameplayAbilityActivationInfo ActivationInfo,
	const FGameplayEventData* TriggerEventData
)
{
	Super::ActivateAbility(Handle, ActorInfo, ActivationInfo, TriggerEventData);
	
	if (!ActorInfo || !ActorInfo->AvatarActor.IsValid())
	{
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
		return;
	}

	if (!CommitAbilityChecked())
	{
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
		return;
	}
	
	TriggeredWaveCount = 0;
	EnterUltimateMovementState();

	K2_OnUltimateActivated();
}

void UGA_Ultimate::EndAbility(
	const FGameplayAbilitySpecHandle Handle,
	const FGameplayAbilityActorInfo* ActorInfo,
	const FGameplayAbilityActivationInfo ActivationInfo,
	bool bReplicateEndAbility,
	bool bWasCancelled
)
{
	ExitUltimateMovementState();
	TriggeredWaveCount = 0;

	Super::EndAbility(Handle, ActorInfo, ActivationInfo, bReplicateEndAbility, bWasCancelled);
}

const FGameplayTagContainer* UGA_Ultimate::GetCooldownTags() const
{
	return &CooldownTagContainer;
}

void UGA_Ultimate::ApplyCooldown(
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

void UGA_Ultimate::TriggerUltimateWave()
{
	if (!IsActive())
	{
		return;
	}

	if (WaveCount > 0 && TriggeredWaveCount >= WaveCount)
	{
		return;
	}

	++TriggeredWaveCount;

	AActionGameCharacter* Character = GetCharacter();
	if (!Character || !Character->HasAuthority())
	{
		return;
	}

	ApplyWaveDamageAndKnockback();
}

void UGA_Ultimate::NotifyUltimateMontageFinished(bool bWasCancelled)
{
	if (!IsActive())
	{
		return;
	}

	EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, true, bWasCancelled);
}

void UGA_Ultimate::EnterUltimateMovementState()
{
	AActionGameCharacter* Character = GetCharacter();
	if (!Character)
	{
		return;
	}

	UCharacterMovementComponent* MoveComp = Character->GetCharacterMovement();
	if (!MoveComp)
	{
		return;
	}

	CachedMovementMode = MoveComp->MovementMode;
	CachedCustomMovementMode = MoveComp->CustomMovementMode;
	CachedGravityScale = MoveComp->GravityScale;

	MoveComp->SetMovementMode(MOVE_Flying);
	MoveComp->GravityScale = UltimateGravityScale;

	if (UAG_CharacterMovementComponent* AGMoveComp = Cast<UAG_CharacterMovementComponent>(MoveComp))
	{
		AGMoveComp->SetAbilityBaseMoveSpeedOverride(UltimateBaseMoveSpeed);
	}

	bMovementStateApplied = true;
}

void UGA_Ultimate::ExitUltimateMovementState()
{
	if (!bMovementStateApplied)
	{
		return;
	}

	AActionGameCharacter* Character = GetCharacter();
	if (!Character)
	{
		bMovementStateApplied = false;
		return;
	}

	UCharacterMovementComponent* MoveComp = Character->GetCharacterMovement();
	if (!MoveComp)
	{
		bMovementStateApplied = false;
		return;
	}

	MoveComp->SetMovementMode(CachedMovementMode, CachedCustomMovementMode);
	MoveComp->GravityScale = CachedGravityScale;

	if (UAG_CharacterMovementComponent* AGMoveComp = Cast<UAG_CharacterMovementComponent>(MoveComp))
	{
		AGMoveComp->ClearAbilityBaseMoveSpeedOverride();
	}

	bMovementStateApplied = false;
}

void UGA_Ultimate::ApplyWaveDamageAndKnockback()
{
	AActionGameCharacter* Character = GetCharacter();
	UAbilitySystemComponent* SourceASC = GetASC();
	UWorld* World = GetWorld();
	if (!Character || !SourceASC || !World)
	{
		return;
	}

	if (!DamageEffectClass || !DamageDataTag.IsValid())
	{
		UE_LOG(LogAbilitySystem, Warning, TEXT("[%s] Ultimate wave skipped: invalid DamageEffectClass or DamageDataTag"), *GetName());
		return;
	}

	const float AttackPower = SourceASC->GetNumericAttribute(UAG_AttributeSetBase::GetAttackPowerAttribute());
	const float DamageMultiplier = SourceASC->GetNumericAttribute(UAG_AttributeSetBase::GetDamageMultiplierAttribute());
	const float FinalDamage = FMath::Max(0.f, AttackPower * DamageMultiplier * UltimateDamage);
	if (FinalDamage <= 0.f)
	{
		return;
	}

	const FVector Origin = Character->GetActorLocation();
	const float SafeWaveRadius = FMath::Max(0.f, WaveRadius);

	if (bDebugDrawWave)
	{
		DrawDebugSphere(
			World,
			Origin,
			SafeWaveRadius,
			24,
			FColor::Cyan,
			false,
			DebugDrawDuration,
			0,
			1.25f
		);
	}

	FCollisionQueryParams QueryParams(SCENE_QUERY_STAT(UltimateWaveOverlap), false);
	QueryParams.AddIgnoredActor(Character);

	FCollisionObjectQueryParams ObjectParams;
	ObjectParams.AddObjectTypesToQuery(ECC_Pawn);

	TArray<FOverlapResult> Overlaps;
	const bool bHasOverlap = World->OverlapMultiByObjectType(
		Overlaps,
		Origin,
		FQuat::Identity,
		ObjectParams,
		FCollisionShape::MakeSphere(SafeWaveRadius),
		QueryParams
	);

	if (!bHasOverlap)
	{
		return;
	}

	TSet<AActor*> UniqueTargets;

	for (const FOverlapResult& Overlap : Overlaps)
	{
		AActor* TargetActor = Overlap.GetActor();
		if (!IsValid(TargetActor))
		{
			continue;
		}

		if (!Cast<AEnemyCharacterBase>(TargetActor))
		{
			continue;
		}

		if (UniqueTargets.Contains(TargetActor))
		{
			continue;
		}
		UniqueTargets.Add(TargetActor);

		UAbilitySystemComponent* TargetASC = UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent(TargetActor);
		if (!TargetASC)
		{
			continue;
		}

		FGameplayEffectContextHandle Context = SourceASC->MakeEffectContext();
		Context.AddSourceObject(this);

		FGameplayEffectSpecHandle SpecHandle = SourceASC->MakeOutgoingSpec(DamageEffectClass, GetAbilityLevel(), Context);
		if (SpecHandle.IsValid())
		{
			SpecHandle.Data->SetSetByCallerMagnitude(DamageDataTag, -FinalDamage);
			SourceASC->ApplyGameplayEffectSpecToTarget(*SpecHandle.Data.Get(), TargetASC);
		}

		if (bDebugDrawWave)
		{
			const FVector TargetLoc = TargetActor->GetActorLocation();
			DrawDebugLine(
				World,
				Origin,
				TargetLoc,
				FColor::Green,
				false,
				DebugDrawDuration,
				0,
				1.25f
			);
			DrawDebugSphere(
				World,
				TargetLoc,
				16.f,
				12,
				FColor::Yellow,
				false,
				DebugDrawDuration
			);
		}

		FVector KnockDir = TargetActor->GetActorLocation() - Origin;
		KnockDir.Z = 0.f;
		if (!KnockDir.Normalize())
		{
			KnockDir = Character->GetActorForwardVector().GetSafeNormal2D();
		}

		const FVector LaunchVelocity = KnockDir * FMath::Max(0.f, KnockbackStrength) + FVector::UpVector * FMath::Max(0.f, KnockbackUpward);

		if (ACharacter* TargetCharacter = Cast<ACharacter>(TargetActor))
		{
			TargetCharacter->LaunchCharacter(LaunchVelocity, true, true);
		}
	}
}

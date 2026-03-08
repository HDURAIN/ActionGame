// EnemyCharacterBase.cpp

#include "Characters/EnemyCharacterBase.h"

#include "AbilitySystemBlueprintLibrary.h"
#include "AbilitySystem/AttributeSets/AG_EnemyAttributeSet.h"

#include "Components/CapsuleComponent.h"
#include "Engine/World.h"
#include "GameFramework/PlayerController.h"
#include "GameFramework/CharacterMovementComponent.h"

#include "GameplayEffectExtension.h"
#include "GameplayEffectTypes.h"
#include <BehaviorTree/Decorators/BTDecorator_ConditionalLoop.h>

AEnemyCharacterBase::AEnemyCharacterBase()
{
	PrimaryActorTick.bCanEverTick = false;
	bReplicates = true;

	AbilitySystemComponent = CreateDefaultSubobject<UAbilitySystemComponent>(TEXT("AbilitySystemComponent"));
	if (AbilitySystemComponent)
	{
		AbilitySystemComponent->SetIsReplicated(true);
		AbilitySystemComponent->SetReplicationMode(EGameplayEffectReplicationMode::Mixed);
	}

	EnemyAttributeSet = CreateDefaultSubobject<UAG_EnemyAttributeSet>(TEXT("EnemyAttributeSet"));

	DeadTag = FGameplayTag::RequestGameplayTag(TEXT("State.Dead"));

	if (AbilitySystemComponent && EnemyAttributeSet)
	{
		AbilitySystemComponent
			->GetGameplayAttributeValueChangeDelegate(EnemyAttributeSet->GetHealthAttribute())
			.AddUObject(this, &AEnemyCharacterBase::OnHealthAttributeChanged);

		AbilitySystemComponent->RegisterGameplayTagEvent(
			FGameplayTag::RequestGameplayTag(TEXT("State.Ragdoll")),
			EGameplayTagEventType::NewOrRemoved
		).AddUObject(this, &AEnemyCharacterBase::OnRagdollStateTagChanged);
	}
}

UAbilitySystemComponent* AEnemyCharacterBase::GetAbilitySystemComponent() const
{
	return AbilitySystemComponent;
}

void AEnemyCharacterBase::PerformAttack(AActor* TargetActor)
{
	// Base default: do nothing.
}

void AEnemyCharacterBase::ApplySpawnEntryConfig(const FEnemySpawnEntry& InConfig)
{
	EnemyMovementType = InConfig.MovementType;
	TargetAcceptanceRadius = InConfig.AcceptanceRadius;
	bUsePathfinding = InConfig.bUsePathfinding;
	AttackRange = InConfig.AttackRange;
	AttackCooldown = InConfig.AttackCooldown;
	bCanAttack = InConfig.bCanAttack;

	ApplyMovementTypeConfig();
}

void AEnemyCharacterBase::BeginPlay()
{
	Super::BeginPlay();

	if (!DeadTag.IsValid())
	{
		DeadTag = FGameplayTag::RequestGameplayTag(TEXT("State.Dead"));
	}

	if (AbilitySystemComponent)
	{
		AbilitySystemComponent->InitAbilityActorInfo(this, this);
	}

	GiveDeathAbility();
	ApplyStartupEffects();
}

ACharacter* AEnemyCharacterBase::FindBestTarget() const
{
	return FindNearestAliveCharacter();
}

bool AEnemyCharacterBase::IsValidTargetCandidate(ACharacter* Candidate) const
{
	return IsValid(Candidate);
}

float AEnemyCharacterBase::ComputeTargetScoreSq(const ACharacter* Candidate) const
{
	return FVector::DistSquared(GetActorLocation(), Candidate->GetActorLocation());
}

ACharacter* AEnemyCharacterBase::FindNearestAliveCharacter() const
{
	UWorld* World = GetWorld();
	if (!World) return nullptr;

	ACharacter* Best = nullptr;
	float BestScoreSq = TNumericLimits<float>::Max();

	for (FConstPlayerControllerIterator It = World->GetPlayerControllerIterator(); It; ++It)
	{
		APlayerController* PC = It->Get();
		if (!IsValid(PC)) continue;

		ACharacter* Candidate = Cast<ACharacter>(PC->GetPawn());
		if (!IsValid(Candidate)) continue;

		if (!IsValidTargetCandidate(Candidate)) continue;
		if (IsCharacterDead(Candidate)) continue;

		const float ScoreSq = ComputeTargetScoreSq(Candidate);
		if (ScoreSq < BestScoreSq)
		{
			BestScoreSq = ScoreSq;
			Best = Candidate;
		}
	}

	return Best;
}

bool AEnemyCharacterBase::IsCharacterDead(const ACharacter* Character) const
{
	if (!IsValid(Character)) return true;

	UAbilitySystemComponent* ASC = GetASC(const_cast<ACharacter*>(Character));
	if (!ASC)
	{
		return false; // 没 ASC 就按“活着”处理
	}

	return DeadTag.IsValid() && ASC->HasMatchingGameplayTag(DeadTag);
}

UAbilitySystemComponent* AEnemyCharacterBase::GetASC(AActor* Actor) const
{
	return UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent(Actor);
}

void AEnemyCharacterBase::GiveDeathAbility()
{
	if (!HasAuthority()) return;
	if (!AbilitySystemComponent || !DeathAbilityClass) return;

	AbilitySystemComponent->GiveAbility(FGameplayAbilitySpec(DeathAbilityClass, 1));
}

void AEnemyCharacterBase::ApplyStartupEffects()
{
	if (!HasAuthority()) return;
	if (!AbilitySystemComponent) return;

	FGameplayEffectContextHandle Ctx = AbilitySystemComponent->MakeEffectContext();
	Ctx.AddSourceObject(this);

	for (const TSubclassOf<UGameplayEffect>& GEClass : StartupEffects)
	{
		if (!GEClass) continue;

		FGameplayEffectSpecHandle Spec = AbilitySystemComponent->MakeOutgoingSpec(GEClass, 1.f, Ctx);
		if (Spec.IsValid())
		{
			AbilitySystemComponent->ApplyGameplayEffectSpecToSelf(*Spec.Data.Get());
		}
	}
}

void AEnemyCharacterBase::ApplyMovementTypeConfig()
{
	UCharacterMovementComponent* MoveComp = GetCharacterMovement();
	if (!MoveComp)
	{
		return;
	}

	if (EnemyMovementType == EEnemyMovementType::Flying)
	{
		MoveComp->SetMovementMode(MOVE_Flying);
		MoveComp->GravityScale = 0.f;
	}
	else
	{
		MoveComp->SetMovementMode(MOVE_Walking);
		MoveComp->GravityScale = 1.f;
	}
}

void AEnemyCharacterBase::OnHealthAttributeChanged(const FOnAttributeChangeData& Data)
{
	if (Data.NewValue > 0.f || Data.OldValue <= 0.f)
	{
		return;
	}

	AActor* InstigatorActor = nullptr;

	if (Data.GEModData)
	{
		const FGameplayEffectContextHandle EffectContext = Data.GEModData->EffectSpec.GetEffectContext();
		InstigatorActor = EffectContext.GetInstigator();
	}

	UE_LOG(LogTemp, Log, TEXT("%s health changed to zero or below, instigator: %s"), *GetName(), InstigatorActor ? *InstigatorActor->GetName() : TEXT("None"));

	FGameplayEventData Payload;
	Payload.EventTag = ZeroHealthEventTag;
	Payload.Instigator = InstigatorActor;

	UAbilitySystemBlueprintLibrary::SendGameplayEventToActor(this, ZeroHealthEventTag, Payload);
}

void AEnemyCharacterBase::OnRagdollStateTagChanged(const FGameplayTag CallbackTag, int32 NewCount)
{
	if (NewCount > 0)
	{
		StartRagdoll();
	}
}

void AEnemyCharacterBase::StartRagdoll()
{
	if (UCharacterMovementComponent* MoveComp = GetCharacterMovement())
	{
		MoveComp->StopMovementImmediately();
		MoveComp->DisableMovement();
		MoveComp->SetMovementMode(MOVE_None);
		MoveComp->Deactivate();
	}

	USkeletalMeshComponent* SkeletalMesh = GetMesh();
	if (SkeletalMesh && !SkeletalMesh->IsSimulatingPhysics())
	{
		SkeletalMesh->SetCollisionProfileName(TEXT("Ragdoll"));
		SkeletalMesh->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
		SkeletalMesh->SetSimulatePhysics(true);
		SkeletalMesh->SetEnableGravity(true);
		SkeletalMesh->SetAllPhysicsLinearVelocity(FVector::ZeroVector);
		SkeletalMesh->SetAllPhysicsAngularVelocityInDegrees(FVector::ZeroVector);
		SkeletalMesh->WakeAllRigidBodies();

		if (UCapsuleComponent* Capsule = GetCapsuleComponent())
		{
			Capsule->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		}
	}

	SetLifeSpan(3.f);
}
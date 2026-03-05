// EnemyCharacterBase.cpp

#include "Characters/EnemyCharacterBase.h"

#include "AbilitySystemComponent.h"
#include "AbilitySystemBlueprintLibrary.h"
#include "Engine/World.h"
#include "GameFramework/PlayerController.h"
#include "TimerManager.h"
#include "Components/CapsuleComponent.h"
#include "GameplayEffectTypes.h"
#include "ActionGameTypes.h"
#include "GameplayEffectExtension.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "EnemyAIController.h"
#include "AbilitySystem/AttributeSets/AG_EnemyAttributeSet.h"
#include "ActionGameCharacter.h"

AEnemyCharacterBase::AEnemyCharacterBase()
{
	PrimaryActorTick.bCanEverTick = true;
	bReplicates = true;

	AutoPossessAI = EAutoPossessAI::PlacedInWorldOrSpawned;
	AIControllerClass = AEnemyAIController::StaticClass(); // 或你的自定义 AIController

	// =========================
	// GAS: ASC + Enemy AttributeSet
	// =========================
	AbilitySystemComponent = CreateDefaultSubobject<UAbilitySystemComponent>(TEXT("AbilitySystemComponent"));
	if (AbilitySystemComponent)
	{
		AbilitySystemComponent->SetIsReplicated(true);
		// Mixed 适合大多数角色（demo省心），也可按需改 Minimal
		AbilitySystemComponent->SetReplicationMode(EGameplayEffectReplicationMode::Mixed);
	}

	EnemyAttributeSet = CreateDefaultSubobject<UAG_EnemyAttributeSet>(TEXT("EnemyAttributeSet"));

	// =========================
	// Targeting
	// =========================
	DeadTag = FGameplayTag::RequestGameplayTag(TEXT("State.Dead"));

	AbilitySystemComponent->GetGameplayAttributeValueChangeDelegate(EnemyAttributeSet->GetHealthAttribute()).AddUObject(this, &AEnemyCharacterBase::OnHealthAttributeChanged);
	AbilitySystemComponent->RegisterGameplayTagEvent(FGameplayTag::RequestGameplayTag(TEXT("State.Ragdoll"), EGameplayTagEventType::NewOrRemoved)).AddUObject(this, &AEnemyCharacterBase::OnRagdollStateTagChanged);
}

UAbilitySystemComponent* AEnemyCharacterBase::GetAbilitySystemComponent() const
{
	return AbilitySystemComponent;
}

void AEnemyCharacterBase::ApplySpawnEntryConfig(const FEnemySpawnEntry& InConfig)
{
	EnemyMovementType = InConfig.MovementType;
	TargetAcceptanceRadius = InConfig.AcceptanceRadius;
	bUsePathfinding = InConfig.bUsePathfinding;

	// 如果当前已经有目标，配置变化后可以重下发一次 MoveTo（可选但很实用）
	if (ACharacter* Target = TargetCharacter.Get())
	{
		if (AEnemyAIController* EnemyController = Cast<AEnemyAIController>(GetController()))
		{
			EnemyController->ChaseTarget(Target);
		}
	}
}

void AEnemyCharacterBase::BeginPlay()
{
	Super::BeginPlay();

	if (!DeadTag.IsValid())
	{
		DeadTag = FGameplayTag::RequestGameplayTag(TEXT("State.Dead"));
	}

	// Init GAS actor info: for NPC, Owner/Avatar 都是自己
	if (AbilitySystemComponent)
	{
		AbilitySystemComponent->InitAbilityActorInfo(this, this);
	}

	ReacquireTarget();

	// 测试用：延迟补一次（解决 BeginPlay 时序）
	if (InitialAcquireDelay > 0.f)
	{
		FTimerHandle TmpHandle;
		GetWorldTimerManager().SetTimer(
			TmpHandle,
			this,
			&AEnemyCharacterBase::ReacquireTarget,
			InitialAcquireDelay,
			false
		);
	}

	GiveDeathAbility();
	ApplyStartupEffects();
}

void AEnemyCharacterBase::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	GetWorldTimerManager().ClearTimer(ReacquireRetryHandle);
	UnbindFromTargetDeath();
	Super::EndPlay(EndPlayReason);
}

void AEnemyCharacterBase::ReacquireTarget()
{
	ACharacter* NewTarget = FindNearestAliveCharacter();
	SetTarget(NewTarget);

	// 无目标：低频重试，避免“全死->复活后永远不再找”
	if (!IsValid(NewTarget))
	{
		if (!GetWorldTimerManager().IsTimerActive(ReacquireRetryHandle))
		{
			GetWorldTimerManager().SetTimer(
				ReacquireRetryHandle,
				this,
				&AEnemyCharacterBase::ReacquireTarget,
				0.5f,
				true
			);
		}
	}
	else
	{
		// 有目标：停止重试
		GetWorldTimerManager().ClearTimer(ReacquireRetryHandle);
	}
}

bool AEnemyCharacterBase::IsValidTargetCandidate(ACharacter* Candidate) const
{
	return IsValid(Candidate);
}

float AEnemyCharacterBase::ComputeTargetScoreSq(const ACharacter* Candidate) const
{
	return FVector::DistSquared(GetActorLocation(), Candidate->GetActorLocation());
}

void AEnemyCharacterBase::SetTarget(ACharacter* NewTarget)
{
	if (TargetCharacter.Get() == NewTarget)
	{
		return;
	}

	UnbindFromTargetDeath();
	TargetCharacter = NewTarget;

	if (IsValid(NewTarget))
	{
		BindToTargetDeath(NewTarget);
	}

	// 目标变化时驱动 AIController
	if (AEnemyAIController* EnemyController = Cast<AEnemyAIController>(GetController()))
	{
		if (IsValid(NewTarget))
		{
			EnemyController->ChaseTarget(NewTarget);
		}
		else
		{
			EnemyController->StopChasing();
		}
	}
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

		APawn* Pawn = PC->GetPawn();
		ACharacter* Candidate = Cast<ACharacter>(Pawn);
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
	if (!IsValid(Character))
	{
		return true;
	}

	UAbilitySystemComponent* ASC = GetASC(const_cast<ACharacter*>(Character));
	if (!ASC)
	{
		// 没ASC就按“活着”处理，避免误判
		return false;
	}

	return ASC->HasMatchingGameplayTag(DeadTag);
}

UAbilitySystemComponent* AEnemyCharacterBase::GetASC(AActor* Actor) const
{
	return UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent(Actor);
}

void AEnemyCharacterBase::BindToTargetDeath(ACharacter* Target)
{
	UAbilitySystemComponent* ASC = GetASC(Target);
	if (!ASC || !DeadTag.IsValid())
	{
		return;
	}

	if (ASC->HasMatchingGameplayTag(DeadTag))
	{
		OnTargetDeadTagChanged(DeadTag, 1);
		return;
	}

	DeadTagChangedHandle =
		ASC->RegisterGameplayTagEvent(DeadTag, EGameplayTagEventType::NewOrRemoved)
		.AddUObject(this, &AEnemyCharacterBase::OnTargetDeadTagChanged);
}

void AEnemyCharacterBase::UnbindFromTargetDeath()
{
	ACharacter* OldTarget = TargetCharacter.Get();
	if (!IsValid(OldTarget))
	{
		DeadTagChangedHandle.Reset();
		return;
	}

	UAbilitySystemComponent* ASC = GetASC(OldTarget);
	if (!ASC || !DeadTagChangedHandle.IsValid() || !DeadTag.IsValid())
	{
		DeadTagChangedHandle.Reset();
		return;
	}

	ASC->RegisterGameplayTagEvent(DeadTag, EGameplayTagEventType::NewOrRemoved)
		.Remove(DeadTagChangedHandle);

	DeadTagChangedHandle.Reset();
}

void AEnemyCharacterBase::OnTargetDeadTagChanged(const FGameplayTag Tag, int32 NewCount)
{
	if (Tag == DeadTag && NewCount > 0)
	{
		ReacquireTarget();
	}
}

void AEnemyCharacterBase::GiveDeathAbility()
{
	if (!AbilitySystemComponent || !DeathAbilityClass)
	{
		return;
	}
	if (!HasAuthority())
	{
		return;
	}

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

void AEnemyCharacterBase::OnHealthAttributeChanged(const FOnAttributeChangeData& Data)
{
	if (Data.NewValue <= 0.f && Data.OldValue > 0.f)
	{
		AActionGameCharacter* OtherCharacter = nullptr;

		if (Data.GEModData)
		{
			const FGameplayEffectContextHandle& EffectContext = Data.GEModData->EffectSpec.GetEffectContext();
			OtherCharacter = Cast<AActionGameCharacter>(EffectContext.GetInstigator());
		}

		FGameplayEventData EventPayload;
		EventPayload.EventTag = ZeroHealthEventTag;
		EventPayload.Instigator = OtherCharacter;
		UAbilitySystemBlueprintLibrary::SendGameplayEventToActor(this, ZeroHealthEventTag, EventPayload);
	}
}

void  AEnemyCharacterBase::OnRagdollStateTagChanged(const FGameplayTag CallbackTag, int32 NewCount)
{
	if (NewCount > 0)
	{
		StartRagdoll();
	}
}

void  AEnemyCharacterBase::StartRagdoll()
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
		SkeletalMesh->SetAllPhysicsLinearVelocity(FVector::Zero());
		SkeletalMesh->SetAllPhysicsAngularVelocityInDegrees(FVector::Zero());
		SkeletalMesh->WakeAllRigidBodies();
		GetCapsuleComponent()->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	}
	SetLifeSpan(3.f);
}

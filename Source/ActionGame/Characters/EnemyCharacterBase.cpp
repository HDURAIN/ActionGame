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
#include "ActionGameTypes.h"
#include "DataAssets/EnemyConfigDataAsset.h"
#include <BehaviorTree/Decorators/BTDecorator_ConditionalLoop.h>
#include "GameplayEffect.h"
#include "ActionGameGameState.h"

AEnemyCharacterBase::AEnemyCharacterBase()
{
	PrimaryActorTick.bCanEverTick = false;
	bReplicates = true;

	// GAS
	AbilitySystemComponent = CreateDefaultSubobject<UAbilitySystemComponent>(TEXT("AbilitySystemComponent"));
	if (AbilitySystemComponent)
	{
		AbilitySystemComponent->SetIsReplicated(true);
		AbilitySystemComponent->SetReplicationMode(EGameplayEffectReplicationMode::Mixed);
	}

	EnemyAttributeSet = CreateDefaultSubobject<UAG_EnemyAttributeSet>(TEXT("EnemyAttributeSet"));

	// 死亡
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

	// Init 
	InitializeEnemy();

	// 赋予死亡能力，确保死了之后能进 ragdoll 状态
	GiveDeathAbility();

	// 另外的一些初始效果
	ApplyStartupEffects();
}

UAbilitySystemComponent* AEnemyCharacterBase::GetAbilitySystemComponent() const
{
	return AbilitySystemComponent;
}

int32 AEnemyCharacterBase::GetCurrentDifficultyStage() const
{
	if (const AActionGameGameState* GS = GetWorld()->GetGameState<AActionGameGameState>())
	{
		return GS->GetDifficultyStage();
	}
	return 0;
}

void AEnemyCharacterBase::PerformAttack(AActor* TargetActor)
{
	// Base default: do nothing.
}

void AEnemyCharacterBase::InitFromSpawnEntry(const FEnemySpawnEntry& InEntry)
{
	EnemyConfig = InEntry.EnemyConfig;
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

void AEnemyCharacterBase::InitializeEnemy()
{
	if (!HasAuthority())
	{
		return;
	}

	if (bInitAttributesApplied)
	{
		// 你现在暂时只有这一个初始化完成标记，
		// 那就把它当作“Enemy 已初始化”的总标记来用
		return;
	}

	if (!EnemyConfig)
	{
		UE_LOG(LogTemp, Warning,
			TEXT("[%s] InitializeEnemy failed: EnemyConfig is null (did you forget Deferred InitFromSpawnEntry?)"),
			*GetName());
		return;
	}

	ApplyRuntimeConfig();
	ApplyInitAttributes();
}

void AEnemyCharacterBase::ApplyRuntimeConfig()
{
	if (!HasAuthority())
	{
		return;
	}

	if (!EnemyConfig)
	{
		UE_LOG(LogTemp, Warning,
			TEXT("[%s] ApplyRuntimeConfig failed: EnemyConfig is null"),
			*GetName());
		return;
	}

	const FEnemyConfigData& D = EnemyConfig->EnemyConfigData;

	// =========================
	// Runtime config（非 GAS）
	// =========================
	EnemyMovementType = D.MovementType;
	TargetAcceptanceRadius = D.AcceptanceRadius;
	bUsePathfinding = D.bUsePathfinding;
	AttackRange = D.AttackRange;
	AttackCooldown = D.AttackCooldown;
	bCanAttack = D.bCanAttack;
	DamageEffectClass = D.DamageEffectClass;

	UCharacterMovementComponent* MoveComp = GetCharacterMovement();
	if (!MoveComp)
	{
		UE_LOG(LogTemp, Warning,
			TEXT("[%s] ApplyRuntimeConfig warning: CharacterMovementComponent is null"),
			*GetName());
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

	UE_LOG(LogTemp, Log,
		TEXT("[%s] RuntimeConfig applied: MoveType=%d AcceptanceRadius=%.1f UsePathfinding=%s AttackRange=%.1f AttackCooldown=%.2f CanAttack=%s"),
		*GetName(),
		static_cast<int32>(EnemyMovementType),
		TargetAcceptanceRadius,
		bUsePathfinding ? TEXT("true") : TEXT("false"),
		AttackRange,
		AttackCooldown,
		bCanAttack ? TEXT("true") : TEXT("false"));
}

void AEnemyCharacterBase::ApplyInitAttributes()
{
	if (!HasAuthority())
	{
		return;
	}

	if (bInitAttributesApplied)
	{
		return;
	}

	if (!AbilitySystemComponent)
	{
		UE_LOG(LogTemp, Warning,
			TEXT("[%s] ApplyInitAttributes failed: AbilitySystemComponent is null"),
			*GetName());
		return;
	}

	if (!EnemyInitEffectClass)
	{
		UE_LOG(LogTemp, Warning,
			TEXT("[%s] ApplyInitAttributes failed: EnemyInitEffectClass is null"),
			*GetName());
		return;
	}

	if (!EnemyConfig)
	{
		UE_LOG(LogTemp, Warning,
			TEXT("[%s] ApplyInitAttributes failed: EnemyConfig is null"),
			*GetName());
		return;
	}

	// 获取当前难度阶段
	const int32 Stage = GetCurrentDifficultyStage();
	const float HealthScale = FMath::Pow(1.18f, Stage);
	const float AttackScale = FMath::Pow(1.12f, Stage);
	const float GoldScale = FMath::Pow(1.10f, Stage);

	const FEnemyConfigData& D = EnemyConfig->EnemyConfigData;

	// =========================
	// Attribute init（GAS）
	// =========================
	const float BaseMaxHealth = FMath::Max(0.01f, D.MaxHealth);
	const float BaseHealth = FMath::Max(0.01f, (D.Health > 0.f) ? D.Health : BaseMaxHealth);
	const float BaseAttackPower = FMath::Max(0.f, D.BaseAttackPower);
	const float BaseAttackMul = FMath::Max(0.f, D.AttackMultiplier);
	const float BaseBountyGold = FMath::Max(0.f, D.BountyGold);

	const float InitMaxHealth = HealthScale * BaseMaxHealth;
	const float InitHealth = HealthScale * BaseHealth;
	const float InitAttackPower = AttackScale * BaseAttackPower;
	const float InitAttackMul = BaseAttackMul;
	const float InitBountyGold = GoldScale * BaseBountyGold;

	static const FGameplayTag Tag_InitHealth =
		FGameplayTag::RequestGameplayTag(TEXT("Data.Init.Health"));
	static const FGameplayTag Tag_InitMaxHealth =
		FGameplayTag::RequestGameplayTag(TEXT("Data.Init.MaxHealth"));
	static const FGameplayTag Tag_InitAttackPower =
		FGameplayTag::RequestGameplayTag(TEXT("Data.Init.AttackPower"));
	static const FGameplayTag Tag_InitAttackMul =
		FGameplayTag::RequestGameplayTag(TEXT("Data.Init.AttackMultiplier"));
	static const FGameplayTag Tag_InitBountyGold =
		FGameplayTag::RequestGameplayTag(TEXT("Data.Init.BountyGold"));

	FGameplayEffectContextHandle Ctx = AbilitySystemComponent->MakeEffectContext();
	Ctx.AddSourceObject(this);

	FGameplayEffectSpecHandle Spec = AbilitySystemComponent->MakeOutgoingSpec(EnemyInitEffectClass, 1.f, Ctx);
	if (!Spec.IsValid() || !Spec.Data.IsValid())
	{
		UE_LOG(LogTemp, Warning,
			TEXT("[%s] ApplyInitAttributes failed: MakeOutgoingSpec returned invalid spec"),
			*GetName());
		return;
	}

	FGameplayEffectSpec* EffectSpec = Spec.Data.Get();
	if (!EffectSpec)
	{
		UE_LOG(LogTemp, Warning,
			TEXT("[%s] ApplyInitAttributes failed: EffectSpec is null"),
			*GetName());
		return;
	}

	EffectSpec->SetSetByCallerMagnitude(Tag_InitHealth, InitHealth);
	EffectSpec->SetSetByCallerMagnitude(Tag_InitMaxHealth, InitMaxHealth);
	EffectSpec->SetSetByCallerMagnitude(Tag_InitAttackPower, InitAttackPower);
	EffectSpec->SetSetByCallerMagnitude(Tag_InitAttackMul, InitAttackMul);
	EffectSpec->SetSetByCallerMagnitude(Tag_InitBountyGold, InitBountyGold);

	AbilitySystemComponent->ApplyGameplayEffectSpecToSelf(*EffectSpec);

	bInitAttributesApplied = true;

	const FString ConfigName = EnemyConfig ? EnemyConfig->GetName() : TEXT("None");
	UE_LOG(LogTemp, Log,
		TEXT("[%s] InitAttributes applied | Stage=%d | Scales: Health=%.3f Attack=%.3f Gold=%.3f | ")
		TEXT("Base: HP=%.1f/%.1f AP=%.1f Mul=%.2f Gold=%.1f | ")
		TEXT("Final: HP=%.1f/%.1f AP=%.1f Mul=%.2f Gold=%.1f"),
		*GetName(),
		Stage,
		HealthScale,
		AttackScale,
		GoldScale,
		BaseHealth,
		BaseMaxHealth,
		BaseAttackPower,
		BaseAttackMul,
		BaseBountyGold,
		InitHealth,
		InitMaxHealth,
		InitAttackPower,
		InitAttackMul,
		InitBountyGold);
}

// 死亡
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

// 死亡
void AEnemyCharacterBase::OnRagdollStateTagChanged(const FGameplayTag CallbackTag, int32 NewCount)
{
	if (NewCount > 0)
	{
		StartRagdoll();
	}
}

// 死亡
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
#include "Actors/EnemyProjectile.h"

#include "AbilitySystemBlueprintLibrary.h"
#include "AbilitySystemComponent.h"
#include "Characters/EnemyCharacterBase.h"
#include "Components/SphereComponent.h"
#include "GameFramework/ProjectileMovementComponent.h"
#include "GameplayEffect.h"

AEnemyProjectile::AEnemyProjectile()
{
	PrimaryActorTick.bCanEverTick = false;

	bReplicates = true;
	SetReplicateMovement(true);

	CollisionComp = CreateDefaultSubobject<USphereComponent>(TEXT("CollisionComp"));
	SetRootComponent(CollisionComp);

	CollisionComp->InitSphereRadius(SphereRadius);
	CollisionComp->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
	CollisionComp->SetCollisionObjectType(ECC_WorldDynamic);

	CollisionComp->SetCollisionResponseToAllChannels(ECR_Ignore);
	CollisionComp->SetCollisionResponseToChannel(ECC_WorldStatic, ECR_Block);
	CollisionComp->SetCollisionResponseToChannel(ECC_WorldDynamic, ECR_Block);
	CollisionComp->SetCollisionResponseToChannel(ECC_Pawn, ECR_Overlap);

	CollisionComp->SetNotifyRigidBodyCollision(false);
	CollisionComp->SetGenerateOverlapEvents(true);

	CollisionComp->OnComponentHit.AddDynamic(this, &AEnemyProjectile::OnProjectileHit);
	CollisionComp->OnComponentBeginOverlap.AddDynamic(this, &AEnemyProjectile::OnProjectileBeginOverlap);

	MovementComp = CreateDefaultSubobject<UProjectileMovementComponent>(TEXT("MovementComp"));
	MovementComp->UpdatedComponent = CollisionComp;
	MovementComp->InitialSpeed = 2000.f;
	MovementComp->MaxSpeed = 2000.f;
	MovementComp->bRotationFollowsVelocity = true;
	MovementComp->bShouldBounce = false;
}

void AEnemyProjectile::BeginPlay()
{
	Super::BeginPlay();

	if (CollisionComp)
	{
		if (AActor* Inst = GetInstigator())
		{
			CollisionComp->IgnoreActorWhenMoving(Inst, true);
		}
		if (AActor* Ow = GetOwner())
		{
			CollisionComp->IgnoreActorWhenMoving(Ow, true);
		}
	}

	SetLifeSpan(LifeSeconds);
}

void AEnemyProjectile::InitProjectile(
	UAbilitySystemComponent* InSourceASC,
	TSubclassOf<UGameplayEffect> InDamageGE,
	FGameplayTag InDamageTag,
	float InDamageValue,
	const FVector& Dir,
	float Speed)
{
	SourceASC = InSourceASC;
	DamageEffectClass = InDamageGE;
	DamageDataTag = InDamageTag;
	DamageValue = FMath::Max(0.f, InDamageValue);

	if (MovementComp)
	{
		const float FinalSpeed = (Speed > 0.f) ? Speed : MovementComp->InitialSpeed;
		MovementComp->Velocity = Dir.GetSafeNormal() * FinalSpeed;
		MovementComp->InitialSpeed = FinalSpeed;
		MovementComp->MaxSpeed = FinalSpeed;
	}
}

void AEnemyProjectile::OnProjectileHit(
	UPrimitiveComponent* HitComp,
	AActor* OtherActor,
	UPrimitiveComponent* OtherComp,
	FVector NormalImpulse,
	const FHitResult& Hit)
{
	if (HasAuthority())
	{
		ApplyDamageIfPossible(Hit);
	}

	if (bDestroyOnHit)
	{
		Destroy();
	}
}

void AEnemyProjectile::OnProjectileBeginOverlap(
	UPrimitiveComponent* OverlappedComp,
	AActor* OtherActor,
	UPrimitiveComponent* OtherComp,
	int32 OtherBodyIndex,
	bool bFromSweep,
	const FHitResult& SweepResult)
{
	if (!HasAuthority() || !IsValid(OtherActor) || ShouldIgnoreTargetActor(OtherActor))
	{
		return;
	}

	ApplyDamageIfPossible(SweepResult);

	if (bDestroyOnHit)
	{
		Destroy();
	}
}

void AEnemyProjectile::ApplyDamageIfPossible(const FHitResult& Hit)
{
	if (!SourceASC || !DamageEffectClass || !DamageDataTag.IsValid() || DamageValue <= 0.f)
	{
		return;
	}

	AActor* TargetActor = Hit.GetActor();
	if (!IsValid(TargetActor) || ShouldIgnoreTargetActor(TargetActor))
	{
		return;
	}

	UAbilitySystemComponent* TargetASC = UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent(TargetActor);
	if (!TargetASC)
	{
		return;
	}

	FGameplayEffectContextHandle Context = SourceASC->MakeEffectContext();
	Context.AddSourceObject(this);
	Context.AddHitResult(Hit);

	FGameplayEffectSpecHandle SpecHandle = SourceASC->MakeOutgoingSpec(DamageEffectClass, 1.f, Context);
	if (!SpecHandle.IsValid())
	{
		return;
	}

	SpecHandle.Data->SetSetByCallerMagnitude(DamageDataTag, -DamageValue);
	SourceASC->ApplyGameplayEffectSpecToTarget(*SpecHandle.Data.Get(), TargetASC);
}

bool AEnemyProjectile::ShouldIgnoreTargetActor(const AActor* TargetActor) const
{
	if (!IsValid(TargetActor))
	{
		return true;
	}

	if (TargetActor == this || TargetActor == GetOwner() || TargetActor == GetInstigator())
	{
		return true;
	}

	const AEnemyCharacterBase* TargetEnemy = Cast<AEnemyCharacterBase>(TargetActor);
	const AEnemyCharacterBase* OwnerEnemy = Cast<AEnemyCharacterBase>(GetOwner());
	if (TargetEnemy && OwnerEnemy)
	{
		return true;
	}

	return false;
}

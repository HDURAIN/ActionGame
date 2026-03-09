#include "Actors/EnemyProjectile.h"

#include "AbilitySystemBlueprintLibrary.h"
#include "AbilitySystemComponent.h"
#include "Components/SphereComponent.h"
#include "GameFramework/ProjectileMovementComponent.h"
#include "GameplayEffect.h"
#include "Engine/World.h"

AEnemyProjectile::AEnemyProjectile()
{
	PrimaryActorTick.bCanEverTick = false;

	bReplicates = true;
	SetReplicateMovement(true);

	// ===== Collision =====
	CollisionComp = CreateDefaultSubobject<USphereComponent>(TEXT("CollisionComp"));
	SetRootComponent(CollisionComp);

	CollisionComp->InitSphereRadius(SphereRadius);
	CollisionComp->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
	CollisionComp->SetCollisionObjectType(ECC_WorldDynamic);

	// Block WorldStatic/WorldDynamic，Overlap Pawn
	CollisionComp->SetCollisionResponseToAllChannels(ECR_Block);
	CollisionComp->SetCollisionResponseToChannel(ECC_Pawn, ECR_Block);

	CollisionComp->SetNotifyRigidBodyCollision(true); // 触发 Hit
	CollisionComp->SetGenerateOverlapEvents(false);

	CollisionComp->OnComponentHit.AddDynamic(this, &AEnemyProjectile::OnProjectileHit);

	// ===== Movement =====
	MovementComp = CreateDefaultSubobject<UProjectileMovementComponent>(TEXT("MovementComp"));
	MovementComp->UpdatedComponent = CollisionComp;

	MovementComp->InitialSpeed = 2000.f;
	MovementComp->MaxSpeed = 2000.f;

	MovementComp->bRotationFollowsVelocity = true;
	MovementComp->bShouldBounce = false;

	// 网络：服务器驱动即可，客户端看复制位置
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

	// 防止飞丢
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
	// 只在服务器结算伤害（权威）
	if (HasAuthority())
	{
		ApplyDamageIfPossible(Hit);
	}

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
	if (!IsValid(TargetActor))
	{
		return;
	}

	// 目标必须有 ASC 才能吃 GE
	UAbilitySystemComponent* TargetASC =
		UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent(TargetActor);

	if (!TargetASC)
	{
		return;
	}

	// 构造 Context
	FGameplayEffectContextHandle Context = SourceASC->MakeEffectContext();
	Context.AddSourceObject(this);
	Context.AddHitResult(Hit);

	// Spec
	FGameplayEffectSpecHandle SpecHandle =
		SourceASC->MakeOutgoingSpec(DamageEffectClass, 1.f, Context);

	if (!SpecHandle.IsValid())
	{
		return;
	}

	// 对齐你现在的扣血写法：SetByCaller 传负数
	SpecHandle.Data->SetSetByCallerMagnitude(DamageDataTag, -DamageValue);

	SourceASC->ApplyGameplayEffectSpecToTarget(*SpecHandle.Data.Get(), TargetASC);
}
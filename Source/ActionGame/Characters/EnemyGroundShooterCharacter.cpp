#include "Characters/EnemyGroundShooterCharacter.h"

#include "Actors/EnemyProjectile.h"
#include "AbilitySystemComponent.h"
#include "Components/CapsuleComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "Engine/World.h"
#include "AbilitySystem/AttributeSets/AG_EnemyAttributeSet.h"
#include "GameFramework/CharacterMovementComponent.h"

AEnemyGroundShooterCharacter::AEnemyGroundShooterCharacter()
{
	PrimaryActorTick.bCanEverTick = false;
	bUseControllerRotationYaw = false;

	
}

FVector AEnemyGroundShooterCharacter::GetAimPoint(AActor* TargetActor) const
{
	if (!IsValid(TargetActor))
	{
		return FVector::ZeroVector;
	}

	// Character：瞄准胶囊高度
	if (ACharacter* Char = Cast<ACharacter>(TargetActor))
	{
		if (UCapsuleComponent* Cap = Char->GetCapsuleComponent())
		{
			const float HalfH = Cap->GetScaledCapsuleHalfHeight();
			const float Z = HalfH * FMath::Clamp(AimHeightAlpha, 0.f, 1.f);
			return Char->GetActorLocation() + FVector(0.f, 0.f, Z);
		}
	}

	// 其他 Actor：瞄准 Bounds 中心
	FVector Origin, Extent;
	TargetActor->GetActorBounds(true, Origin, Extent);
	return Origin;
}

bool AEnemyGroundShooterCharacter::ComputeShotDir(AActor* TargetActor, FVector& OutDir, FVector& OutMuzzleLoc) const
{
	USkeletalMeshComponent* MeshComp = GetMesh();
	if (!MeshComp)
	{
		return false;
	}

	OutMuzzleLoc = MeshComp->GetSocketLocation(MuzzleSocketName);
	const FVector AimPoint = GetAimPoint(TargetActor);

	FVector Dir = (AimPoint - OutMuzzleLoc);
	if (!Dir.Normalize())
	{
		Dir = GetActorForwardVector();
	}
	OutDir = Dir;
	return true;
}

void AEnemyGroundShooterCharacter::PerformAttack(AActor* TargetActor)
{
	if (!HasAuthority() || !IsValid(TargetActor))
	{
		return;
	}

	// 1) ASC / Damage配置校验
	UAbilitySystemComponent* SourceASC = GetAbilitySystemComponent();
	if (!SourceASC)
	{
		UE_LOG(LogTemp, Warning, TEXT("[%s] PerformAttack failed: SourceASC is null"), *GetName());
		return;
	}

	if (!DamageEffectClass || !DamageDataTag.IsValid())
	{
		UE_LOG(LogTemp, Warning, TEXT("[%s] PerformAttack failed: Missing DamageEffectClass or DamageDataTag"), *GetName());
		return;
	}

	if (!ProjectileClass)
	{
		UE_LOG(LogTemp, Warning, TEXT("[%s] PerformAttack failed: ProjectileClass is null"), *GetName());
		return;
	}

	// 2) 计算方向 & 枪口位置
	FVector ShotDir = FVector::ZeroVector;
	FVector MuzzleLoc = FVector::ZeroVector;
	if (!ComputeShotDir(TargetActor, ShotDir, MuzzleLoc))
	{
		UE_LOG(LogTemp, Warning, TEXT("[%s] PerformAttack failed: ComputeShotDir failed for target %s"),
			*GetName(), *GetNameSafe(TargetActor));
		return;
	}

	// 3) 从 AttributeSet 读取攻击属性并计算最终伤害
	const UAG_EnemyAttributeSet* EnemyAS = SourceASC->GetSet<UAG_EnemyAttributeSet>();
	if (!EnemyAS)
	{
		UE_LOG(LogTemp, Warning, TEXT("[%s] PerformAttack failed: UAG_EnemyAttributeSet is null"), *GetName());
		return;
	}

	const float AttackPower = EnemyAS->GetAttackPower();
	const float AttackMultiplier = EnemyAS->GetAttackMultiplier();
	const float FinalDamage = FMath::Max(0.f, AttackPower * AttackMultiplier);

	if (FinalDamage <= 0.f)
	{
		UE_LOG(LogTemp, Warning,
			TEXT("[%s] PerformAttack warning: FinalDamage <= 0 (AP=%.2f Mul=%.2f)"),
			*GetName(),
			AttackPower,
			AttackMultiplier);
	}

	// 4) Spawn projectile（服务器权威）
	FActorSpawnParameters SpawnParams;
	SpawnParams.Owner = this;
	SpawnParams.Instigator = this;
	SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;

	const FRotator SpawnRot = ShotDir.Rotation();

	AEnemyProjectile* Proj = GetWorld()->SpawnActor<AEnemyProjectile>(
		ProjectileClass,
		MuzzleLoc,
		SpawnRot,
		SpawnParams
	);

	if (!Proj)
	{
		UE_LOG(LogTemp, Warning, TEXT("[%s] PerformAttack failed: Spawn projectile failed"), *GetName());
		return;
	}

	// 5) 初始化投射物（把 GE/Tag/Damage 一起传进去）
	Proj->InitProjectile(
		SourceASC,
		DamageEffectClass,
		DamageDataTag,
		FinalDamage,
		ShotDir,
		ProjectileSpeed
	);

	UE_LOG(LogTemp, Log,
		TEXT("[%s] Fire projectile -> Target=%s | AP=%.2f Mul=%.2f FinalDamage=%.2f | Muzzle=%s Dir=%s Speed=%.1f"),
		*GetName(),
		*GetNameSafe(TargetActor),
		AttackPower,
		AttackMultiplier,
		FinalDamage,
		*MuzzleLoc.ToCompactString(),
		*ShotDir.ToCompactString(),
		ProjectileSpeed);
}

void AEnemyGroundShooterCharacter::BeginPlay()
{
	Super::BeginPlay();

	if (UCharacterMovementComponent* MoveComp = GetCharacterMovement())
	{
		MoveComp->SetMovementMode(MOVE_Walking);
		MoveComp->GravityScale = 1.f;

		MoveComp->MaxWalkSpeed = WalkSpeed;
		MoveComp->BrakingDecelerationWalking = BrakingDecelerationWalking;

		MoveComp->bOrientRotationToMovement = bFaceMoveDirection;
		MoveComp->RotationRate = FRotator(0.f, TurnRateYaw, 0.f);
	}
}
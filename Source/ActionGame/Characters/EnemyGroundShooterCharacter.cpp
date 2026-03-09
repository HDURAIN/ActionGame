#include "Characters/EnemyGroundShooterCharacter.h"

#include "Actors/EnemyProjectile.h"
#include "AbilitySystemComponent.h"
#include "Components/CapsuleComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "Engine/World.h"
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
		return;
	}

	if (!DamageEffectClass || !DamageDataTag.IsValid())
	{
		UE_LOG(LogTemp, Warning, TEXT("[%s] Missing DamageEffectClass or DamageDataTag"), *GetName());
		return;
	}

	if (!ProjectileClass)
	{
		UE_LOG(LogTemp, Warning, TEXT("[%s] ProjectileClass is null (set in BP defaults)"), *GetName());
		return;
	}

	GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Red, FString::Printf(TEXT("[%s] PerformAttack to %s"), *GetName(), *GetNameSafe(TargetActor)));

	// 2) 计算方向 & 枪口位置
	FVector ShotDir = FVector::ZeroVector;
	FVector MuzzleLoc = FVector::ZeroVector;
	if (!ComputeShotDir(TargetActor, ShotDir, MuzzleLoc))
	{
		return;
	}

	// 3) 计算最终伤害（你之后可乘难度、暴击等）
	const float FinalDamage = FMath::Max(0.f, BaseAttackPower);

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

	// 可选：Debug
	UE_LOG(LogTemp, Warning, TEXT("[%s] Fire projectile to %s dmg=%.1f"), *GetName(), *GetNameSafe(TargetActor), FinalDamage);
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
#pragma once

#include "CoreMinimal.h"
#include "Characters/EnemyCharacterBase.h"
#include "EnemyGroundShooterCharacter.generated.h"


class AEnemyProjectile;
class AActor;
UCLASS()
class ACTIONGAME_API AEnemyGroundShooterCharacter : public AEnemyCharacterBase
{
	GENERATED_BODY()

public:
	AEnemyGroundShooterCharacter();

	virtual void PerformAttack(AActor* TargetActor) override;

protected:
	virtual void BeginPlay() override;

protected:
	/** 地面移动速度 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Enemy|Ground")
	float WalkSpeed = 450.f;

	/** 地面刹车减速度 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Enemy|Ground")
	float BrakingDecelerationWalking = 2048.f;

	/** 是否朝移动方向旋转 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Enemy|Ground")
	bool bFaceMoveDirection = true;

	/** 转向速度 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Enemy|Ground")
	float TurnRateYaw = 720.f;

	/** 投射物类（BP_EnemyProjectile） */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Enemy|Attack")
	TSubclassOf<AEnemyProjectile> ProjectileClass;

	/** 枪口 Socket（建议挂在 Mesh 上） */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Enemy|Attack")
	FName MuzzleSocketName = TEXT("Muzzle");

	/** 投射物速度 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Enemy|Attack", meta = (ClampMin = "0.0"))
	float ProjectileSpeed = 2000.f;

	/** Forward offset at spawn to avoid face-to-face overlap push */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Enemy|Attack", meta = (ClampMin = "0.0"))
	float ProjectileSpawnForwardOffset = 20.f;

	/** 目标瞄准高度系数：0=脚底，0.5=胸口，1=头顶附近（按胶囊半高插值） */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Enemy|Attack", meta = (ClampMin = "0.0", ClampMax = "1.0"))
	float AimHeightAlpha = 0.5f;

protected:
	FVector GetAimPoint(AActor* TargetActor) const;
	bool ComputeShotDir(AActor* TargetActor, FVector& OutDir, FVector& OutMuzzleLoc) const;
};


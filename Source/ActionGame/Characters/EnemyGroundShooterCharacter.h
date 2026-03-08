#pragma once

#include "CoreMinimal.h"
#include "Characters/EnemyCharacterBase.h"
#include "EnemyGroundShooterCharacter.generated.h"

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
};
#pragma once

#include "CoreMinimal.h"
#include "Characters/EnemyCharacterBase.h"
#include "GameplayTagContainer.h"
#include "EnemyFlyingSuiciderCharacter.generated.h"

class USphereComponent;
class UGameplayEffect;

UCLASS()
class ACTIONGAME_API AEnemyFlyingSuiciderCharacter : public AEnemyCharacterBase
{
	GENERATED_BODY()

public:
	AEnemyFlyingSuiciderCharacter();

protected:
	virtual void BeginPlay() override;
	virtual void Tick(float DeltaTime) override;

protected:
	// ========= Fly =========
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Enemy|Fly")
	float FlySpeed = 1200.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Enemy|Fly")
	float TurnRateYaw = 720.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Enemy|Fly")
	float BrakingDecel = 0.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Enemy|Fly")
	bool bFaceMoveDirection = true;

	// ========= Explode =========
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Enemy|Explode")
	TObjectPtr<USphereComponent> TriggerSphere;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Enemy|Explode")
	float TriggerRadius = 120.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Enemy|Explode")
	float ExplosionRadius = 300.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Enemy|Explode")
	TSubclassOf<UGameplayEffect> ExplosionEffect;

	TSet<TWeakObjectPtr<ACharacter>> AffectedActors;

protected:
	UFUNCTION()
	void OnTriggerBeginOverlap(UPrimitiveComponent* OverlappedComp, AActor* OtherActor,
		UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult);

private:
	bool bExploded = false;

	void ExplodeAndApply_Server();
};
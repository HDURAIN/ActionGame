#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "GameplayTagContainer.h"
#include "EnemyProjectile.generated.h"

class USphereComponent;
class UProjectileMovementComponent;
class UGameplayEffect;
class UAbilitySystemComponent;

UCLASS()
class ACTIONGAME_API AEnemyProjectile : public AActor
{
	GENERATED_BODY()

public:
	AEnemyProjectile();

	/**
	 * 初始化投射物（服务器调用）
	 * @param InSourceASC   伤害来源 ASC（一般是敌人的 ASC）
	 * @param InDamageGE    伤害 GameplayEffect（支持 SetByCaller）
	 * @param InDamageTag   SetByCaller 的 tag（例如 Data.Damage）
	 * @param InDamageValue 伤害数值（正数传入，内部会写成 -Damage）
	 * @param Dir           发射方向（单位向量）
	 * @param Speed         初速度
	 */
	void InitProjectile(
		UAbilitySystemComponent* InSourceASC,
		TSubclassOf<UGameplayEffect> InDamageGE,
		FGameplayTag InDamageTag,
		float InDamageValue,
		const FVector& Dir,
		float Speed);

protected:
	virtual void BeginPlay() override;

protected:
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Projectile")
	TObjectPtr<USphereComponent> CollisionComp;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Projectile")
	TObjectPtr<UProjectileMovementComponent> MovementComp;

	/** 碰撞半径 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Projectile")
	float SphereRadius = 8.f;

	/** 最长存活时间（防止飞丢） */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Projectile")
	float LifeSeconds = 4.f;

	/** 命中任何东西后是否立即销毁（通常是 true） */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Projectile")
	bool bDestroyOnHit = true;

protected:
	UFUNCTION()
	void OnProjectileHit(
		UPrimitiveComponent* HitComp,
		AActor* OtherActor,
		UPrimitiveComponent* OtherComp,
		FVector NormalImpulse,
		const FHitResult& Hit);

private:
	// ===== Runtime Damage Payload (server authoritative) =====
	UPROPERTY(Transient)
	TObjectPtr<UAbilitySystemComponent> SourceASC;

	UPROPERTY(Transient)
	TSubclassOf<UGameplayEffect> DamageEffectClass;

	UPROPERTY(Transient)
	FGameplayTag DamageDataTag;

	UPROPERTY(Transient)
	float DamageValue = 0.f;

private:
	void ApplyDamageIfPossible(const FHitResult& Hit);
};
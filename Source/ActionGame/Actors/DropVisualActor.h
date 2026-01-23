// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "DropVisualActor.generated.h"

class UStaticMeshComponent;

/**
 * 掉落完成回调
 * @param GroundLocation 掉落完成时的地面位置
 */
DECLARE_DELEGATE_OneParam(FOnDropFinished, const FVector&);

/**
 * ADropVisualActor
 *
 * 掉落表现体（如白色光球）
 *
 * 职责：
 * - 播放掉落表现（逻辑或物理）
 * - 判断何时“落地完成”
 * - 通知外部（Chest / DropManager）
 *
 * 不负责：
 * - 物品交互
 * - WorldItemActor
 * - 玩家交互
 */
UCLASS()
class ACTIONGAME_API ADropVisualActor : public AActor
{
	GENERATED_BODY()
	
public:	
	// Sets default values for this actor's properties
	ADropVisualActor();

	void BindOnDropFinished(FOnDropFinished InDelegate);

protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;

protected:
	// Components
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<UStaticMeshComponent> MeshComponent;

protected:
	// Drop Logic
	// 掉落完成回调 实例
	FOnDropFinished OnDropFinished;

	bool bDropFinished = false;

	UFUNCTION()
	void OnMeshHit(UPrimitiveComponent* HitComp, AActor* OtherActor, UPrimitiveComponent* OtherComp, FVector NormalImpulse, const FHitResult& Hit);

protected:
	// 冲量
	UPROPERTY(EditDefaultsOnly, Category = "Drop|Impulse")
	float ForwardImpulse = 200.f;

	UPROPERTY(EditDefaultsOnly, Category = "Drop|Impulse")
	float UpImpulse = 300.f;
};

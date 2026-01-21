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

	void StartDrop(const FVector& ForwardDirection);

	void BindOnDropFinished(FOnDropFinished InDelegate);

protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;

	virtual void Tick(float DeltaTime) override;

protected:
	// Components
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<UStaticMeshComponent> MeshComponent;

protected:
	// Drop Logic
	
	// 是否已经完成掉落
	bool bDropFinished = false;

	// 掉落完成回调 实例
	FOnDropFinished OnDropFinished;

	// 内部函数：检查是否已经落地
	void CheckDropFinished();

	// 内部函数：处理掉落完成
	void NotifyDropLanded(const FVector& GroundLocation);

protected:
	// Config

	// 掉落检测的最大向下距离
	UPROPERTY(EditDefaultsOnly, Category ="Drop")
	float GroundTraceDistance = 5000.f;

	// 冲量
	UPROPERTY(EditDefaultsOnly, Category = "Drop|Impulse")
	float ForwardImpulse = 200.f;

	UPROPERTY(EditDefaultsOnly, Category = "Drop|Impulse")
	float UpImpulse = 300.f;
};

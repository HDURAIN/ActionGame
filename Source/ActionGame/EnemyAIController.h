// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "AIController.h"
#include "EnemyAIController.generated.h"

class AActor;
class AEnemyCharacterBase;
/**
 * 
 */
UCLASS()
class ACTIONGAME_API AEnemyAIController : public AAIController
{
	GENERATED_BODY()
	
public:
	/** 开始追踪指定目标 */
	UFUNCTION(BlueprintCallable, Category = "Enemy|AI")
	void ChaseTarget(AActor* TargetActor);

	/** 停止追踪 */
	UFUNCTION(BlueprintCallable, Category = "Enemy|AI")
	void StopChasing();

private:
	/** 从当前控制的 Enemy 身上读取配置并发起 MoveTo */
	void MoveToTarget_Internal(AActor* TargetActor);
};

// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Abilities/GameplayAbility.h"
#include "ActionGameTypes.h"
#include "GA_Interact.generated.h"

class UInteractable;

/**
 * 
 */
UCLASS()
class ACTIONGAME_API UGA_Interact : public UGameplayAbility
{
	GENERATED_BODY()
	
public:

	UGA_Interact();

	// GameplayAbility
	virtual void ActivateAbility(
		const FGameplayAbilitySpecHandle Handle,
		const FGameplayAbilityActorInfo* ActorInfo,
		const FGameplayAbilityActivationInfo ActivationInfo,
		const FGameplayEventData* TriggerEventData
	) override;

protected:
	
	// 目标选择
	AActor* FindBestInteractableTarget(const FGameplayAbilityActorInfo* ActorInfo) const;


	// 对目标执行一次交互流程
	void TryInteractWithTarget(const FGameplayAbilityActorInfo* ActorInfo, AActor* TargetActor) const;


protected:
	// 可调参数

	// 交互检测距离
	UPROPERTY(EditDefaultsOnly, Category = "Interaction")
	float InteractDistance = 250.0f;

	// 射线长度
	UPROPERTY(EditDefaultsOnly, Category = "Interaction")
	float TraceDistance = 2000.0f;
};

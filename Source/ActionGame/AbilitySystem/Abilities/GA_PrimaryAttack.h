#pragma once

#include "CoreMinimal.h"
#include "AbilitySystem/Abilities/AG_GameplayAbility.h"
#include "GA_PrimaryAttack.generated.h"

struct FGameplayEventData;

UCLASS()
class ACTIONGAME_API UGA_PrimaryAttack : public UAG_GameplayAbility
{
	GENERATED_BODY()

public:
	UGA_PrimaryAttack();

	virtual void ActivateAbility(
		const FGameplayAbilitySpecHandle Handle,
		const FGameplayAbilityActorInfo* ActorInfo,
		const FGameplayAbilityActivationInfo ActivationInfo,
		const FGameplayEventData* TriggerEventData
	) override;

	virtual void EndAbility(
		const FGameplayAbilitySpecHandle Handle,
		const FGameplayAbilityActorInfo* ActorInfo,
		const FGameplayAbilityActivationInfo ActivationInfo,
		bool bReplicateEndAbility,
		bool bWasCancelled
	) override;

protected:
	/** 蓝图实现：激活后的完整流程（播放Montage、等待Shoot事件等） */
	UFUNCTION(BlueprintImplementableEvent, Category = "PrimaryAttack")
	void K2_OnActivateFromEvent();
};
// Fill out your copyright notice in the Description page of Project Settings.


#include "AbilitySystem/Abilities/GA_Death.h"
#include "AbilitySystemComponent.h"
#include "AbilitySystemBlueprintLibrary.h"
#include "AbilitySystem/AttributeSets/AG_EnemyAttributeSet.h"

void UGA_Death::ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData)
{
	if (!ActorInfo || !ActorInfo->AvatarActor.IsValid() || !TriggerEventData)
	{
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
		return;
	}

	if (!HasAuthority(&ActivationInfo))
	{
		EndAbility(Handle, ActorInfo, ActivationInfo, true, false);
		return;
	}

	AActor* Killer = const_cast<AActor*>(TriggerEventData->Instigator.Get());
	UAbilitySystemComponent* SelfASC = ActorInfo->AbilitySystemComponent.Get();

	// 1) Apply DeathEffect to self
	{
		FGameplayEffectContextHandle Ctx = SelfASC->MakeEffectContext();
		FGameplayEffectSpecHandle DeathSpec = SelfASC->MakeOutgoingSpec(DeathEffect, 1.f, Ctx);
		if (DeathSpec.IsValid())
		{
			SelfASC->ApplyGameplayEffectSpecToSelf(*DeathSpec.Data.Get());
		}
	}

	// 2) Reward killer based on victim attribute
	if (Killer && RewardEffect)
	{
		UAbilitySystemComponent* KillerASC = UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent(Killer);
		if (KillerASC)
		{
			// ´ÓËÀÍöÕß AttributeSet ¶ÁÉÍ½ð
			const UAG_EnemyAttributeSet* EnemyAS = SelfASC->GetSet<UAG_EnemyAttributeSet>();
			const float Bounty = EnemyAS ? EnemyAS->GetBountyGold() : 0.f;

			FGameplayEffectContextHandle RewardCtx = KillerASC->MakeEffectContext();
			RewardCtx.AddSourceObject(ActorInfo->AvatarActor.Get());

			FGameplayEffectSpecHandle RewardSpec = KillerASC->MakeOutgoingSpec(RewardEffect, 1.f, RewardCtx);
			if (RewardSpec.IsValid())
			{
				static const FGameplayTag TAG_Data_Reward_Gold =
					FGameplayTag::RequestGameplayTag(TEXT("Data.Reward.Gold"));

				RewardSpec.Data->SetSetByCallerMagnitude(TAG_Data_Reward_Gold, Bounty);
				KillerASC->ApplyGameplayEffectSpecToSelf(*RewardSpec.Data.Get());
			}
		}
	}

	EndAbility(Handle, ActorInfo, ActivationInfo, false, false);
}

void UGA_Death::EndAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, bool bReplicateEndAbility, bool bWasCancelled)
{

	Super::EndAbility(Handle, ActorInfo, ActivationInfo, bReplicateEndAbility, bWasCancelled);
}

// AG_AbilitySystemComponentBase.cpp

#include "AG_AbilitySystemComponentBase.h"
#include "DataAssets/DA_Item.h"

#include "AbilitySystem/Abilities/AG_GameplayAbility.h"
#include "GameplayEffect.h"
#include "ActionGameCharacter.h"
#include "ActorComponents/ItemContainerComponent.h"

/*
 */

void UAG_AbilitySystemComponentBase::ApplyItem(const UDA_Item* Item, int32 OldCount, int32 NewCount)
{
	if (!Item)
	{
		return;
	}

	// 仅在首次获得该物品时授予 Effect / Ability
	if (NewCount <= 0)
	{
		return;
	}

	
	const int32 Delta = NewCount - OldCount;
	for (int32 i = 0; i < Delta; i++)
	{
		/* ===============================
		* 授予 GameplayEffect
		* =============================== */
		for (TSubclassOf<UGameplayEffect> EffectClass : Item->GrantedEffects)
		{
			if (!EffectClass)
			{
				continue;
			}

			// 直接对自身应用 GameplayEffect
			ApplyGameplayEffectToSelf(
				EffectClass.GetDefaultObject(),
				1.0f,
				MakeEffectContext()
			);
		}
	}

	/* ===============================
	 * 授予 GameplayAbility
	 * =============================== */
	if (OldCount == 0)
	{
		for (TSubclassOf<UGameplayAbility> AbilityClass : Item->GrantedAbilities)
		{
			if (!AbilityClass)
			{
				continue;
			}

			GiveAbility(FGameplayAbilitySpec(AbilityClass, 1));
		}
	}
}

void UAG_AbilitySystemComponentBase::RemoveItem(const UDA_Item* Item)
{
	if (!Item)
	{
		return;
	}
}

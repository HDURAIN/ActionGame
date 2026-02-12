// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "AbilitySystemComponent.h"
#include "AttributeSet.h"
#include "AG_AttributeSetBase.generated.h"


#define ATTRIBUTE_ACCESSORS(ClassName, PropertyName) \
	GAMEPLAYATTRIBUTE_PROPERTY_GETTER(ClassName, PropertyName) \
	GAMEPLAYATTRIBUTE_VALUE_GETTER(PropertyName) \
	GAMEPLAYATTRIBUTE_VALUE_SETTER(PropertyName) \
	GAMEPLAYATTRIBUTE_VALUE_INITTER(PropertyName)

// 定义数值与规则
UCLASS()
class ACTIONGAME_API UAG_AttributeSetBase : public UAttributeSet
{
	GENERATED_BODY()
	

public:

	UPROPERTY(BlueprintReadOnly, Category = "Health", ReplicatedUsing = OnRep_Health)
	FGameplayAttributeData Health;
	ATTRIBUTE_ACCESSORS(UAG_AttributeSetBase, Health)

	UPROPERTY(BlueprintReadOnly, Category = "Health", ReplicatedUsing = OnRep_MaxHealth)
	FGameplayAttributeData MaxHealth;
	ATTRIBUTE_ACCESSORS(UAG_AttributeSetBase, MaxHealth)

	UPROPERTY(BlueprintReadOnly, Category = "Stamina", ReplicatedUsing = OnRep_Stamina)
	FGameplayAttributeData Stamina;
	ATTRIBUTE_ACCESSORS(UAG_AttributeSetBase, Stamina)

	UPROPERTY(BlueprintReadOnly, Category = "Stamina", ReplicatedUsing = OnRep_MaxStamina)
	FGameplayAttributeData MaxStamina;
	ATTRIBUTE_ACCESSORS(UAG_AttributeSetBase, MaxStamina)

	UPROPERTY(BlueprintReadOnly, Category = "Movement", ReplicatedUsing = OnRep_BaseMoveSpeed)
	FGameplayAttributeData BaseMoveSpeed;
	ATTRIBUTE_ACCESSORS(UAG_AttributeSetBase, BaseMoveSpeed)

	UPROPERTY(BlueprintReadOnly, Category = "Movement", ReplicatedUsing = OnRep_MoveSpeedMultiplier)
	FGameplayAttributeData MoveSpeedMultiplier;
	ATTRIBUTE_ACCESSORS(UAG_AttributeSetBase, MoveSpeedMultiplier)

	UPROPERTY(BlueprintReadOnly, Category = "Jump", ReplicatedUsing = OnRep_MaxJumpCount)
	FGameplayAttributeData MaxJumpCount;
	ATTRIBUTE_ACCESSORS(UAG_AttributeSetBase, MaxJumpCount)

	UPROPERTY(BlueprintReadOnly, Category = "Money", ReplicatedUsing = OnRep_Gold)
	FGameplayAttributeData Gold;
	ATTRIBUTE_ACCESSORS(UAG_AttributeSetBase, Gold)

protected:
	// 修改Attribute后的纠错函数
	virtual void PostGameplayEffectExecute(const struct FGameplayEffectModCallbackData& Data) override;
	

	/**
	* 当属性完成网络复制后调用的通知函数（客户端）
	*/
	UFUNCTION()
	virtual void OnRep_Health(const FGameplayAttributeData& OldHealth);

	UFUNCTION()
	virtual void OnRep_MaxHealth(const FGameplayAttributeData& OldMaxHealth);

	UFUNCTION()
	virtual void OnRep_Stamina(const FGameplayAttributeData& OldStamina);

	UFUNCTION()
	virtual void OnRep_MaxStamina(const FGameplayAttributeData& OldMaxStamina);

	UFUNCTION()
	virtual void OnRep_BaseMoveSpeed(const FGameplayAttributeData& OldBaseMoveSpeed);

	UFUNCTION()
	virtual void OnRep_MoveSpeedMultiplier(const FGameplayAttributeData& OldMoveSpeedMultipiler);

	UFUNCTION()
	virtual void OnRep_MaxJumpCount(const FGameplayAttributeData& OldMaxJumpCount);

	UFUNCTION()
	virtual void OnRep_Gold(const FGameplayAttributeData& OldGold);
};

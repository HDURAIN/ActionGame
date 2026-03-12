// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "AG_CharacterMovementComponent.generated.h"


class UAbilitySystemComponent;
class UAG_AttributeSetBase;

UCLASS()
class ACTIONGAME_API UAG_CharacterMovementComponent : public UCharacterMovementComponent
{
	GENERATED_BODY()
	
public:
	UAG_CharacterMovementComponent();

	virtual float GetMaxSpeed() const override;

	void SetAbilityBaseMoveSpeedOverride(float InBaseMoveSpeed);
	void ClearAbilityBaseMoveSpeedOverride();
	bool HasAbilityBaseMoveSpeedOverride() const { return bHasAbilityBaseMoveSpeedOverride; }

protected:
	virtual void BeginPlay()override;

	UPROPERTY()
	UAbilitySystemComponent* CachedASC;

	UPROPERTY()
	const UAG_AttributeSetBase* CachedAttributeSet;

	UPROPERTY(Transient)
	bool bHasAbilityBaseMoveSpeedOverride = false;

	UPROPERTY(Transient)
	float AbilityBaseMoveSpeedOverride = 0.f;

	void CachedAbilitySystem();
};

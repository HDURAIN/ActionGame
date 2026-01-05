// Fill out your copyright notice in the Description page of Project Settings.


#include "ActorComponents/AG_CharacterMovementComponent.h"

#include "AbilitySystemInterface.h"
#include "GameFramework/Character.h"
#include "AbilitySystemComponent.h"
#include "AbilitySystem/AttributeSets/AG_AttributeSetBase.h"

UAG_CharacterMovementComponent::UAG_CharacterMovementComponent()
{
}

void UAG_CharacterMovementComponent::BeginPlay()
{
	Super::BeginPlay();

	CachedAbilitySystem();
}

float UAG_CharacterMovementComponent::GetMaxSpeed() const
{
	float BaseSpeed = Super::GetMaxSpeed();

	if (!CachedAttributeSet)
	{
		return BaseSpeed;
	}

	const float BaseMoveSpeed = CachedAttributeSet->GetBaseMoveSpeed();

	const float SpeedMultiplier = CachedAttributeSet->GetMoveSpeedMultiplier();

	return BaseMoveSpeed * SpeedMultiplier;

}

void UAG_CharacterMovementComponent::CachedAbilitySystem()
{
	CachedASC = nullptr;
	CachedAttributeSet = nullptr;

	ACharacter* OwnerCharacter = Cast<ACharacter>(GetOwner());
	if (!OwnerCharacter) return;

	if (IAbilitySystemInterface* ASI = Cast<IAbilitySystemInterface>(OwnerCharacter))
	{
		CachedASC = ASI->GetAbilitySystemComponent();
		if (CachedASC)
		{
			CachedAttributeSet = CachedASC->GetSet<UAG_AttributeSetBase>();
		}
	}
}

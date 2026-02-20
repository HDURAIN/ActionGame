// Fill out your copyright notice in the Description page of Project Settings.


#include "PlayerHUDWidget.h"

#include "AbilitySystemComponent.h"
#include "AbilitySystem/AttributeSets/AG_AttributeSetBase.h"

#include "Components/ProgressBar.h"
#include "Components/TextBlock.h"

void UPlayerHUDWidget::InitWithASC(UAbilitySystemComponent* InASC)
{
	if (!InASC) return;

	if (ASC == InASC) return;

	// 如果重复绑定先解绑
	UnbindAttributeDelegates();

	ASC = InASC;

	// ===== 初始数值 =====
	const float Health =
		ASC->GetNumericAttribute(UAG_AttributeSetBase::GetHealthAttribute());

	CachedMaxHealth =
		ASC->GetNumericAttribute(UAG_AttributeSetBase::GetMaxHealthAttribute());

	const float Gold =
		ASC->GetNumericAttribute(UAG_AttributeSetBase::GetGoldAttribute());

	RefreshHealthBar(Health);
	RefreshGold(Gold);

	// ===== 绑定监听 =====
	BindAttributeDelegates();
}

void UPlayerHUDWidget::BindAttributeDelegates()
{
	if (!ASC) return;

	ASC->GetGameplayAttributeValueChangeDelegate(
		UAG_AttributeSetBase::GetHealthAttribute()
	).AddUObject(this, &UPlayerHUDWidget::OnHealthChanged);

	ASC->GetGameplayAttributeValueChangeDelegate(
		UAG_AttributeSetBase::GetMaxHealthAttribute()
	).AddUObject(this, &UPlayerHUDWidget::OnMaxHealthChanged);

	ASC->GetGameplayAttributeValueChangeDelegate(
		UAG_AttributeSetBase::GetGoldAttribute()
	).AddUObject(this, &UPlayerHUDWidget::OnGoldChanged);
}

void UPlayerHUDWidget::UnbindAttributeDelegates()
{
	if (!ASC) return;

	ASC->GetGameplayAttributeValueChangeDelegate(
		UAG_AttributeSetBase::GetHealthAttribute()
	).RemoveAll(this);

	ASC->GetGameplayAttributeValueChangeDelegate(
		UAG_AttributeSetBase::GetMaxHealthAttribute()
	).RemoveAll(this);

	ASC->GetGameplayAttributeValueChangeDelegate(
		UAG_AttributeSetBase::GetGoldAttribute()
	).RemoveAll(this);
}

void UPlayerHUDWidget::NativeDestruct()
{
	UnbindAttributeDelegates();
	Super::NativeDestruct();
}

void UPlayerHUDWidget::OnHealthChanged(const FOnAttributeChangeData& Data)
{
	RefreshHealthBar(Data.NewValue);
}

void UPlayerHUDWidget::OnMaxHealthChanged(const FOnAttributeChangeData& Data)
{
	CachedMaxHealth = Data.NewValue;

	const float CurrentHealth =
		ASC->GetNumericAttribute(UAG_AttributeSetBase::GetHealthAttribute());

	RefreshHealthBar(CurrentHealth);
}

void UPlayerHUDWidget::OnGoldChanged(const FOnAttributeChangeData& Data)
{
	RefreshGold(Data.NewValue);
}

void UPlayerHUDWidget::RefreshHealthBar(float Health)
{
	if (!ProgressBar_Health) return;

	const float Percent =
		CachedMaxHealth > KINDA_SMALL_NUMBER
		? Health / CachedMaxHealth
		: 0.f;

	ProgressBar_Health->SetPercent(Percent);
}

void UPlayerHUDWidget::RefreshGold(float Gold)
{
	if (!Text_Gold) return;

	Text_Gold->SetText(FText::FromString(FString::Printf(TEXT("$%d"), FMath::RoundToInt(Gold))));
}
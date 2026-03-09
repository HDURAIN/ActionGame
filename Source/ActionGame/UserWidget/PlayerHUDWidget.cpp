#include "PlayerHUDWidget.h"

#include "AbilitySystemComponent.h"
#include "AbilitySystem/AttributeSets/AG_AttributeSetBase.h"
#include "Components/ProgressBar.h"
#include "Components/TextBlock.h"
#include "Engine/World.h"
#include "ActionGameGameState.h"

void UPlayerHUDWidget::InitWithASC(UAbilitySystemComponent* InASC)
{
	if (!InASC)
	{
		return;
	}

	if (ASC == InASC)
	{
		return;
	}

	// 如果重复初始化，先解绑旧 ASC
	UnbindAttributeDelegates();

	ASC = InASC;

	// 初始化一次玩家属性 UI
	RefreshInitialAttributeUI();

	// 初始化一次 GameState UI
	RefreshGameStateSection();

	// 绑定属性变化监听
	BindAttributeDelegates();
}

void UPlayerHUDWidget::NativeTick(const FGeometry& MyGeometry, float InDeltaTime)
{
	Super::NativeTick(MyGeometry, InDeltaTime);

	// GameState 这类全局数据直接轻量刷新即可
	RefreshGameStateSection();
}

void UPlayerHUDWidget::NativeDestruct()
{
	UnbindAttributeDelegates();

	Super::NativeDestruct();
}

//
// ASC Binding
//

void UPlayerHUDWidget::BindAttributeDelegates()
{
	if (!ASC)
	{
		return;
	}

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
	if (!ASC)
	{
		return;
	}

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

//
// Attribute Change Callbacks
//

void UPlayerHUDWidget::OnHealthChanged(const FOnAttributeChangeData& Data)
{
	RefreshHealthBar(Data.NewValue);
}

void UPlayerHUDWidget::OnMaxHealthChanged(const FOnAttributeChangeData& Data)
{
	CachedMaxHealth = Data.NewValue;
	RefreshHealthBar(GetCurrentHealth());
}

void UPlayerHUDWidget::OnGoldChanged(const FOnAttributeChangeData& Data)
{
	RefreshGold(Data.NewValue);
}

//
// UI Refresh - Player Attributes
//

void UPlayerHUDWidget::RefreshInitialAttributeUI()
{
	CachedMaxHealth = GetCurrentMaxHealth();

	RefreshHealthBar(GetCurrentHealth());
	RefreshGold(GetCurrentGold());
}

void UPlayerHUDWidget::RefreshHealthBar(float Health)
{
	if (!ProgressBar_Health)
	{
		return;
	}

	const float Percent =
		CachedMaxHealth > KINDA_SMALL_NUMBER
		? Health / CachedMaxHealth
		: 0.f;

	ProgressBar_Health->SetPercent(Percent);
}

void UPlayerHUDWidget::RefreshGold(float Gold)
{
	if (!Text_Gold)
	{
		return;
	}

	Text_Gold->SetText(
		FText::FromString(FString::Printf(TEXT("$%d"), FMath::RoundToInt(Gold)))
	);
}

//
// UI Refresh - GameState
//

void UPlayerHUDWidget::RefreshGameStateSection()
{
	const AActionGameGameState* GS = GetActionGameGameState();
	if (!GS)
	{
		return;
	}

	RefreshGameTime(GS->GetElapsedSurvivalTime());
	RefreshStage(GS->GetDifficultyStage());
}

void UPlayerHUDWidget::RefreshGameTime(float ElapsedSeconds)
{
	if (!Text_GameTime)
	{
		return;
	}

	const int32 TotalSeconds = FMath::Max(0, FMath::FloorToInt(ElapsedSeconds));
	const int32 Minutes = TotalSeconds / 60;
	const int32 Seconds = TotalSeconds % 60;

	Text_GameTime->SetText(
		FText::FromString(FString::Printf(TEXT("%02d:%02d"), Minutes, Seconds))
	);
}

void UPlayerHUDWidget::RefreshStage(int32 Stage)
{
	if (!Text_Stage)
	{
		return;
	}

	// UI 给玩家看时通常从 1 开始更自然
	Text_Stage->SetText(
		FText::FromString(FString::Printf(TEXT("Stage %d"), Stage + 1))
	);
}

//
// Helpers
//

AActionGameGameState* UPlayerHUDWidget::GetActionGameGameState() const
{
	UWorld* World = GetWorld();
	if (!World)
	{
		return nullptr;
	}

	return World->GetGameState<AActionGameGameState>();
}

float UPlayerHUDWidget::GetCurrentHealth() const
{
	if (!ASC)
	{
		return 0.f;
	}

	return ASC->GetNumericAttribute(UAG_AttributeSetBase::GetHealthAttribute());
}

float UPlayerHUDWidget::GetCurrentMaxHealth() const
{
	if (!ASC)
	{
		return 0.f;
	}

	return ASC->GetNumericAttribute(UAG_AttributeSetBase::GetMaxHealthAttribute());
}

float UPlayerHUDWidget::GetCurrentGold() const
{
	if (!ASC)
	{
		return 0.f;
	}

	return ASC->GetNumericAttribute(UAG_AttributeSetBase::GetGoldAttribute());
}
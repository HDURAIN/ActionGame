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
	// If re-initialized, unbind delegates from previous ASC first.
	UnbindAttributeDelegates();

	ASC = InASC;
	// Initialize player attribute UI once.
	RefreshInitialAttributeUI();
	// Initialize GameState UI once.
	RefreshGameStateSection();
	// Bind attribute change listeners.
	BindAttributeDelegates();
}

void UPlayerHUDWidget::NativeTick(const FGeometry& MyGeometry, float InDeltaTime)
{
	Super::NativeTick(MyGeometry, InDeltaTime);
	// Lightweight refresh for global GameState values.
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
	const float Percent =
		CachedMaxHealth > KINDA_SMALL_NUMBER
		? Health / CachedMaxHealth
		: 0.f;

	if (ProgressBar_Health)
	{
		ProgressBar_Health->SetPercent(Percent);
	}

	RefreshHealthText(Health);
}

void UPlayerHUDWidget::RefreshHealthText(float Health)
{
	if (!Text_HealthOverMax)
	{
		return;
	}

	Text_HealthOverMax->SetText(
		FText::FromString(
			FString::Printf(
				TEXT("%d / %d"),
				FMath::RoundToInt(Health),
				FMath::RoundToInt(CachedMaxHealth)
			)
		)
	);
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
	// Show stage as 1-based index for players.
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

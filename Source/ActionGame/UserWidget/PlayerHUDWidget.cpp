#include "PlayerHUDWidget.h"

#include "AbilitySystemComponent.h"
#include "AbilitySystem/AttributeSets/AG_AttributeSetBase.h"
#include "Components/Image.h"
#include "Components/ProgressBar.h"
#include "Components/TextBlock.h"
#include "Engine/World.h"
#include "ActionGameGameState.h"
#include "GameplayEffectTypes.h"

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
	// Initialize skill icon and cooldown UI once.
	RefreshSkillCooldownSection();
	// Bind attribute change listeners.
	BindAttributeDelegates();
}

void UPlayerHUDWidget::NativeTick(const FGeometry& MyGeometry, float InDeltaTime)
{
	Super::NativeTick(MyGeometry, InDeltaTime);
	// Lightweight refresh for global GameState values.
	RefreshGameStateSection();
	RefreshSkillCooldownSection();
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

void UPlayerHUDWidget::RefreshSkillCooldownSection()
{
	for (int32 SlotIndex = 0; SlotIndex < 4; ++SlotIndex)
	{
		UImage* IconWidget = GetSkillIconWidgetBySlot(SlotIndex);
		UTextBlock* CooldownTextWidget = GetSkillCooldownTextWidgetBySlot(SlotIndex);
		if (!IconWidget && !CooldownTextWidget)
		{
			continue;
		}

		if (!SkillSlotConfigs.IsValidIndex(SlotIndex))
		{
			ApplySkillSlotVisual(SlotIndex, 0.f);
			continue;
		}

		const FSkillSlotUIConfig& Config = SkillSlotConfigs[SlotIndex];
		if (IconWidget && Config.Icon)
		{
			IconWidget->SetBrushFromTexture(Config.Icon);
		}

		const float RemainingSeconds = GetCooldownRemainingByTag(Config.CooldownTag);
		ApplySkillSlotVisual(SlotIndex, RemainingSeconds);
	}
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

float UPlayerHUDWidget::GetCooldownRemainingByTag(const FGameplayTag& CooldownTag) const
{
	if (!ASC || !CooldownTag.IsValid())
	{
		return 0.f;
	}

	FGameplayTagContainer CooldownTags;
	CooldownTags.AddTag(CooldownTag);

	const FGameplayEffectQuery Query = FGameplayEffectQuery::MakeQuery_MatchAnyOwningTags(CooldownTags);
	const TArray<float> RemainingTimes = ASC->GetActiveEffectsTimeRemaining(Query);

	float MaxRemaining = 0.f;
	for (const float TimeRemaining : RemainingTimes)
	{
		MaxRemaining = FMath::Max(MaxRemaining, TimeRemaining);
	}

	return FMath::Max(0.f, MaxRemaining);
}

void UPlayerHUDWidget::ApplySkillSlotVisual(int32 SlotIndex, float RemainingSeconds)
{
	UImage* IconWidget = GetSkillIconWidgetBySlot(SlotIndex);
	UTextBlock* CooldownTextWidget = GetSkillCooldownTextWidgetBySlot(SlotIndex);

	const bool bCoolingDown = RemainingSeconds > KINDA_SMALL_NUMBER;

	if (IconWidget)
	{
		IconWidget->SetColorAndOpacity(
			bCoolingDown ? FLinearColor(0.35f, 0.35f, 0.35f, 1.f) : FLinearColor::White
		);
	}

	if (!CooldownTextWidget)
	{
		return;
	}

	if (!bCoolingDown)
	{
		CooldownTextWidget->SetVisibility(ESlateVisibility::Hidden);
		return;
	}

	const float DisplayValue = RemainingSeconds >= 1.f
		? static_cast<float>(FMath::CeilToInt(RemainingSeconds))
		: FMath::RoundToFloat(RemainingSeconds * 10.f) / 10.f;

	const bool bShowDecimal = DisplayValue < 1.f;
	CooldownTextWidget->SetText(
		bShowDecimal
		? FText::FromString(FString::Printf(TEXT("%.1f"), DisplayValue))
		: FText::AsNumber(FMath::RoundToInt(DisplayValue))
	);
	CooldownTextWidget->SetVisibility(ESlateVisibility::SelfHitTestInvisible);
}

UImage* UPlayerHUDWidget::GetSkillIconWidgetBySlot(int32 SlotIndex) const
{
	switch (SlotIndex)
	{
	case 0: return Image_SkillIcon_1;
	case 1: return Image_SkillIcon_2;
	case 2: return Image_SkillIcon_3;
	case 3: return Image_SkillIcon_4;
	default: return nullptr;
	}
}

UTextBlock* UPlayerHUDWidget::GetSkillCooldownTextWidgetBySlot(int32 SlotIndex) const
{
	switch (SlotIndex)
	{
	case 0: return Text_SkillCD_1;
	case 1: return Text_SkillCD_2;
	case 2: return Text_SkillCD_3;
	case 3: return Text_SkillCD_4;
	default: return nullptr;
	}
}

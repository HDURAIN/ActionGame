#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "GameplayTagContainer.h"
#include "PlayerHUDWidget.generated.h"

class UAbilitySystemComponent;
class UProgressBar;
class UTextBlock;
class UImage;
class UTexture2D;
class AActionGameGameState;
struct FOnAttributeChangeData;

USTRUCT(BlueprintType)
struct FSkillSlotUIConfig
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SkillUI")
	FGameplayTag CooldownTag;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SkillUI")
	TObjectPtr<UTexture2D> Icon = nullptr;
};

UCLASS()
class ACTIONGAME_API UPlayerHUDWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	/** Called by PlayerController after possess */
	void InitWithASC(UAbilitySystemComponent* InASC);

protected:
	// =========================
	// Widget Lifecycle
	// =========================
	virtual void NativeTick(const FGeometry& MyGeometry, float InDeltaTime) override;
	virtual void NativeDestruct() override;

protected:
	// =========================
	// BindWidget
	// =========================
	UPROPERTY(BlueprintReadWrite, meta = (BindWidget))
	TObjectPtr<UProgressBar> ProgressBar_Health = nullptr;
	
	/** Optional single-line health text, e.g. "75 / 100" */
	UPROPERTY(BlueprintReadWrite, meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> Text_HealthOverMax = nullptr;

	UPROPERTY(BlueprintReadWrite, meta = (BindWidget))
	TObjectPtr<UTextBlock> Text_Gold = nullptr;

	UPROPERTY(BlueprintReadWrite, meta = (BindWidget))
	TObjectPtr<UTextBlock> Text_GameTime = nullptr;

	UPROPERTY(BlueprintReadWrite, meta = (BindWidget))
	TObjectPtr<UTextBlock> Text_Stage = nullptr;

	UPROPERTY(BlueprintReadWrite, meta = (BindWidgetOptional))
	TObjectPtr<UImage> Image_SkillIcon_1 = nullptr;

	UPROPERTY(BlueprintReadWrite, meta = (BindWidgetOptional))
	TObjectPtr<UImage> Image_SkillIcon_2 = nullptr;

	UPROPERTY(BlueprintReadWrite, meta = (BindWidgetOptional))
	TObjectPtr<UImage> Image_SkillIcon_3 = nullptr;

	UPROPERTY(BlueprintReadWrite, meta = (BindWidgetOptional))
	TObjectPtr<UImage> Image_SkillIcon_4 = nullptr;

	UPROPERTY(BlueprintReadWrite, meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> Text_SkillCD_1 = nullptr;

	UPROPERTY(BlueprintReadWrite, meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> Text_SkillCD_2 = nullptr;

	UPROPERTY(BlueprintReadWrite, meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> Text_SkillCD_3 = nullptr;

	UPROPERTY(BlueprintReadWrite, meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> Text_SkillCD_4 = nullptr;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "SkillUI")
	TArray<FSkillSlotUIConfig> SkillSlotConfigs;

private:
	// =========================
	// Cached References / State
	// =========================
	UPROPERTY()
	TObjectPtr<UAbilitySystemComponent> ASC = nullptr;

	float CachedMaxHealth = 0.f;

private:
	// =========================
	// ASC Binding
	// =========================
	void BindAttributeDelegates();
	void UnbindAttributeDelegates();

private:
	// =========================
	// Attribute Change Callbacks
	// =========================
	void OnHealthChanged(const FOnAttributeChangeData& Data);
	void OnMaxHealthChanged(const FOnAttributeChangeData& Data);
	void OnGoldChanged(const FOnAttributeChangeData& Data);

private:
	// =========================
	// UI Refresh - Player Attributes
	// =========================
	void RefreshInitialAttributeUI();
	void RefreshHealthBar(float Health);
	void RefreshHealthText(float Health);
	void RefreshGold(float Gold);

private:
	// =========================
	// UI Refresh - GameState
	// =========================
	void RefreshGameTime(float ElapsedSeconds);
	void RefreshStage(int32 Stage);
	void RefreshGameStateSection();
	void RefreshSkillCooldownSection();

private:
	// =========================
	// Helpers
	// =========================
	AActionGameGameState* GetActionGameGameState() const;
	float GetCurrentHealth() const;
	float GetCurrentMaxHealth() const;
	float GetCurrentGold() const;
	float GetCooldownRemainingByTag(const FGameplayTag& CooldownTag) const;
	void ApplySkillSlotVisual(int32 SlotIndex, float RemainingSeconds);
	UImage* GetSkillIconWidgetBySlot(int32 SlotIndex) const;
	UTextBlock* GetSkillCooldownTextWidgetBySlot(int32 SlotIndex) const;
};

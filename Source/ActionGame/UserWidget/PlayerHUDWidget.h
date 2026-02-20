// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "ActionGameTypes.h"
#include "PlayerHUDWidget.generated.h"

class UAbilitySystemComponent;
class UProgressBar;
class UTextBlock;
struct FOnAttributeChangeData;
/**
 * 
 */
UCLASS()
class ACTIONGAME_API UPlayerHUDWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	// Called by PlayerController after possess
	void InitWithASC(UAbilitySystemComponent* InASC);

protected:

	virtual void NativeDestruct() override;

	UPROPERTY(BlueprintReadWrite, meta = (BindWidget))
	TObjectPtr<UProgressBar> ProgressBar_Health;

	UPROPERTY(BlueprintReadWrite, meta = (BindWidget))
	TObjectPtr<UTextBlock> Text_Gold;

private: // GAS

	UPROPERTY()
	UAbilitySystemComponent* ASC = nullptr;

	float CachedMaxHealth = 0.f;
	
	/* ===== Attribute Callbacks ===== */
	void OnHealthChanged(const FOnAttributeChangeData& Data);
	void OnMaxHealthChanged(const FOnAttributeChangeData& Data);
	void OnGoldChanged(const FOnAttributeChangeData& Data);

	void RefreshHealthBar(float Health);
	void RefreshGold(float Gold);

	void BindAttributeDelegates();
	void UnbindAttributeDelegates();
};

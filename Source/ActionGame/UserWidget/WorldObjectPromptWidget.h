// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "WorldObjectPromptWidget.generated.h"


class UWorldObjectDataAsset;
class UTextBlock;
/**
 * 
 */
UCLASS()
class ACTIONGAME_API UWorldObjectPromptWidget : public UUserWidget
{
	GENERATED_BODY()

public:

	UPROPERTY(meta = (BindWidget), BlueprintReadWrite)
	UTextBlock* TextBlock_Name;

	UPROPERTY(meta = (BindWidget), BlueprintReadWrite)
	UTextBlock* TextBlock_Cost;

	UFUNCTION(BlueprintImplementableEvent)
	void InitWithData(UWorldObjectDataAsset* InData);
};

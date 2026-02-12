// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "ActionGameTypes.h"

#include "WorldObjectDataAsset.generated.h"

/**
 * 
 */
UCLASS(BlueprintType, Blueprintable)
class ACTIONGAME_API UWorldObjectDataAsset : public UDataAsset
{
	GENERATED_BODY()
	
public:

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly)
	FInteractDisplayData InteractDisplayData;
};

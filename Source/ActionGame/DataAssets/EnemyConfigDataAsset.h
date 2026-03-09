// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "ActionGameTypes.h"
#include "EnemyConfigDataAsset.generated.h"

UCLASS()
class ACTIONGAME_API UEnemyConfigDataAsset : public UDataAsset
{
	GENERATED_BODY()

public:
	/** µ–»À≈‰÷√ ˝æ› */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Config")
	FEnemyConfigData EnemyConfigData;
};

// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameModeBase.h"
#include "ActionGameGameMode.generated.h"

class AActionGamePlayerController;

/**
 *  Simple GameMode for a third person game
 */
UCLASS(abstract)
class AActionGameGameMode : public AGameModeBase
{
	GENERATED_BODY()

public:
	
	/** Constructor */
	AActionGameGameMode();

	void NotifyPlayerDied(AActionGamePlayerController* PlayerController);
};




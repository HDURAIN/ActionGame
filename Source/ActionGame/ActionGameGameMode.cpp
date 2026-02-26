// Copyright Epic Games, Inc. All Rights Reserved.

#include "ActionGameGameMode.h"
#include "ActionGamePlayerController.h"

AActionGameGameMode::AActionGameGameMode()
{
	// stub
}

void AActionGameGameMode::NotifyPlayerDied(AActionGamePlayerController* PlayerController)
{
	if (PlayerController)
	{
		PlayerController->RestartPlayerIn(3.f);
	}
}

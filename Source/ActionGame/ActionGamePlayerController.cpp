// Copyright Epic Games, Inc. All Rights Reserved.


#include "ActionGamePlayerController.h"
#include "EnhancedInputSubsystems.h"
#include "Engine/LocalPlayer.h"
#include "InputMappingContext.h"
#include "ActionGame.h"

void AActionGamePlayerController::ApplyDefaultMappings()
{
	ULocalPlayer* LP = GetLocalPlayer();
	if (!LP) return;

	UEnhancedInputLocalPlayerSubsystem* Subsystem =
		ULocalPlayer::GetSubsystem<UEnhancedInputLocalPlayerSubsystem>(LP);
	if (!Subsystem) return;

	Subsystem->ClearAllMappings();

	for (UInputMappingContext* Context : DefaultMappingContexts)
	{
		if (Context)
		{
			Subsystem->AddMappingContext(Context, 0);
		}
	}
}


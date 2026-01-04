// Copyright Epic Games, Inc. All Rights Reserved.


#include "ActionGamePlayerController.h"
#include "EnhancedInputSubsystems.h"
#include "Engine/LocalPlayer.h"
#include "InputMappingContext.h"
#include "Blueprint/UserWidget.h"
#include "ActionGame.h"
#include "Widgets/Input/SVirtualJoystick.h"

void AActionGamePlayerController::BeginPlay()
{
	Super::BeginPlay();

	// only spawn touch controls on local player controllers
	if (SVirtualJoystick::ShouldDisplayTouchInterface() && IsLocalPlayerController())
	{
		// spawn the mobile controls widget
		MobileControlsWidget = CreateWidget<UUserWidget>(this, MobileControlsWidgetClass);

		if (MobileControlsWidget)
		{
			// add the controls to the player screen
			MobileControlsWidget->AddToPlayerScreen(0);

		} else {

			UE_LOG(LogActionGame, Error, TEXT("Could not spawn mobile controls widget."));

		}

	}
}

void AActionGamePlayerController::SetupInputComponent()
{
	Super::SetupInputComponent();

}

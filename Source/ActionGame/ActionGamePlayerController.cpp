// Copyright Epic Games, Inc. All Rights Reserved.


#include "ActionGamePlayerController.h"
#include "EnhancedInputSubsystems.h"
#include "Engine/LocalPlayer.h"
#include "InputMappingContext.h"
#include "UserWidget/PlayerHUDWidget.h"
#include "AbilitySystemComponent.h"
#include "AbilitySystemInterface.h"
#include "ActionGame.h"

void AActionGamePlayerController::BeginPlay()
{
	Super::BeginPlay();

	if (!IsLocalController()) return;
	if (!HUDWidgetClass) return;

	HUDWidget = CreateWidget<UPlayerHUDWidget>(this, HUDWidgetClass);

	if (HUDWidget)
	{
		HUDWidget->AddToViewport();
	}

	InitHUDWithPawn(GetPawn());
}

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

void AActionGamePlayerController::OnPossess(APawn* InPawn)
{
	Super::OnPossess(InPawn);

	InitHUDWithPawn(InPawn);
}

void AActionGamePlayerController::InitHUDWithPawn(APawn* InPawn)
{
	if (!HUDWidget || !InPawn) return;

	if (InPawn->GetClass()->ImplementsInterface(UAbilitySystemInterface::StaticClass()))
	{
		IAbilitySystemInterface* ASCInterface =
			Cast<IAbilitySystemInterface>(InPawn);

		if (ASCInterface)
		{
			UAbilitySystemComponent* ASC =
				ASCInterface->GetAbilitySystemComponent();

			HUDWidget->InitWithASC(ASC);
		}
	}
}


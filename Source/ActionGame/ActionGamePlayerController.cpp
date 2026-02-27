// Copyright Epic Games, Inc. All Rights Reserved.


#include "ActionGamePlayerController.h"
#include "EnhancedInputSubsystems.h"
#include "Engine/LocalPlayer.h"
#include "InputMappingContext.h"
#include "UserWidget/PlayerHUDWidget.h"
#include "AbilitySystemComponent.h"
#include "AbilitySystemInterface.h"
#include "AbilitySystemBlueprintLibrary.h"
#include "GameFramework/SpectatorPawn.h"
#include "GameFramework/PawnMovementComponent.h"

#include "ActionGameGameMode.h"

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

void AActionGamePlayerController::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);
}

void AActionGamePlayerController::RestartPlayerIn(float InTime)
{
	ChangeState(NAME_Spectating);

	GetWorld()->GetTimerManager().SetTimer(RestartPlayerTimerHandle, this, &AActionGamePlayerController::RestartPlayer, InTime, false);
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

	if (UAbilitySystemComponent* AbilityComponent = UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent(InPawn))
	{
		DeathStateTagDelegate = AbilityComponent->RegisterGameplayTagEvent(FGameplayTag::RequestGameplayTag(TEXT("State.Dead")), EGameplayTagEventType::NewOrRemoved).AddUObject(this, &AActionGamePlayerController::OnPawnDeathStateChanged);
	}
}

void AActionGamePlayerController::OnUnPossess()
{
	Super::OnUnPossess();

	if (DeathStateTagDelegate.IsValid())
	{
		if (UAbilitySystemComponent* AbilityComponent = UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent(GetPawn()))
		{
			AbilityComponent->UnregisterGameplayTagEvent(DeathStateTagDelegate, FGameplayTag::RequestGameplayTag(TEXT("State.Dead")), EGameplayTagEventType::NewOrRemoved);
		}
	}
}

void AActionGamePlayerController::OnRep_Pawn()
{
	Super::OnRep_Pawn();

	// 客户端重生后会走这里：重新把HUD绑定到新Pawn的ASC
	InitHUDWithPawn(GetPawn());
}

void AActionGamePlayerController::OnPawnDeathStateChanged(const FGameplayTag CallbackTag, int32 NewCount)
{
	if (NewCount > 0)
	{
		UWorld* World = GetWorld();

		AActionGameGameMode* GameMode = World ? Cast<AActionGameGameMode>(World->GetAuthGameMode()) : nullptr;

		if (GameMode)
		{
			GameMode->NotifyPlayerDied(this);
		}

		if (DeathStateTagDelegate.IsValid())
		{
			if (UAbilitySystemComponent* AbilityComponent = UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent(GetPawn()))
			{
				AbilityComponent->UnregisterGameplayTagEvent(DeathStateTagDelegate, FGameplayTag::RequestGameplayTag(TEXT("State.Dead")), EGameplayTagEventType::NewOrRemoved);
			}
		}
	}
}

void AActionGamePlayerController::BeginSpectatingState()
{
	Super::BeginSpectatingState();

	// 你想“完全不能动不能转”
	SetIgnoreMoveInput(true);
	SetIgnoreLookInput(true);

	// 或者你想“能转镜头但不能飞”
	// SetIgnoreMoveInput(true);

	if (ASpectatorPawn* SP = GetSpectatorPawn())
	{
		if (UPawnMovementComponent* PMC = SP->GetMovementComponent())
		{
			PMC->StopMovementImmediately();
			PMC->Deactivate();
		}
	}
}

void AActionGamePlayerController::RestartPlayer()
{

	UWorld* World = GetWorld();
	AActionGameGameMode* GameMode = World ? Cast<AActionGameGameMode>(World->GetAuthGameMode()) : nullptr;

	if (GameMode)
	{
		GameMode->RestartPlayer(this);
	}
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
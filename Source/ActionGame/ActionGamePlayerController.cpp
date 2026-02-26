// Copyright Epic Games, Inc. All Rights Reserved.


#include "ActionGamePlayerController.h"
#include "EnhancedInputSubsystems.h"
#include "Engine/LocalPlayer.h"
#include "InputMappingContext.h"
#include "UserWidget/PlayerHUDWidget.h"
#include "AbilitySystemComponent.h"
#include "AbilitySystemInterface.h"
#include "AbilitySystemBlueprintLibrary.h"

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

void AActionGamePlayerController::RestartPlayerIn(float InTime)
{
	ChangeState(NAME_Spectating); // 修正：补全参数，假设需要传递状态名

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
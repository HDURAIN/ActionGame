// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerController.h"
#include "ActionGamePlayerController.generated.h"

class UInputMappingContext;
class UPlayerHUDWidget;

/**
 * PlayerController
 * - Owns input mapping contexts
 * - Exposes simple requests for Pawns to switch input states
 */
UCLASS()
class AActionGamePlayerController : public APlayerController
{
	GENERATED_BODY()

public:
	/** Apply default gameplay input mappings (called by locally controlled Pawn) */
	UFUNCTION(BlueprintCallable, Category = "Input")
	void ApplyDefaultMappings();

	virtual void BeginPlay() override;

	void RestartPlayerIn(float InTime);

protected:

	/** Default gameplay input mappings */
	UPROPERTY(EditDefaultsOnly, Category = "Input|Mapping")
	TArray<TObjectPtr<UInputMappingContext>> DefaultMappingContexts;

	UPROPERTY(EditDefaultsOnly, Category = "UI")
	TSubclassOf<UPlayerHUDWidget> HUDWidgetClass;

	virtual void OnPossess(APawn* InPawn) override;

	virtual void OnUnPossess() override;

	virtual void OnRep_Pawn() override;

	UFUNCTION()
	void OnPawnDeathStateChanged(const FGameplayTag CallbackTag, int32 NewCount);

	void RestartPlayer();

	FTimerHandle RestartPlayerTimerHandle;

	FDelegateHandle DeathStateTagDelegate;

private:

	UPROPERTY()
	UPlayerHUDWidget* HUDWidget;

	void InitHUDWithPawn(APawn* InPawn);
};

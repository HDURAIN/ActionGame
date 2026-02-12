// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerController.h"
#include "ActionGamePlayerController.generated.h"

class UInputMappingContext;

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

protected:

	/** Default gameplay input mappings */
	UPROPERTY(EditDefaultsOnly, Category = "Input|Mapping")
	TArray<TObjectPtr<UInputMappingContext>> DefaultMappingContexts;
};

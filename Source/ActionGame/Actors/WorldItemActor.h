// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Interfaces/Interactable.h"
#include "WorldItemActor.generated.h"

class UDA_Item;
class UStaticMeshComponent;
class USphereComponent;

UCLASS()
class ACTIONGAME_API AWorldItemActor : public AActor, public IInteractable
{
	GENERATED_BODY()
	
public:	
	// Sets default values for this actor's properties
	AWorldItemActor();

	// Interactable
	virtual bool CanInteract_Implementation(AActor* Interactor) const override;
	virtual void ExecuteInteract_Implementation(AActor* Interactor) override;

	UFUNCTION(BlueprintCallable, Category = "Item")
	UDA_Item* GetItemDef() const { return ItemDef; }

protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;

	// Components
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<UStaticMeshComponent> MeshComponent;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<USphereComponent> CollisionComponent;

	// Definition
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Item")
	TObjectPtr<UDA_Item> ItemDef;

	UPROPERTY(VisibleInstanceOnly, Category = "Item")
	bool bConsumed = false;

protected:
	bool GiveItemTo(AActor* Interactor);

	void ConsumeAndDestroy();
};

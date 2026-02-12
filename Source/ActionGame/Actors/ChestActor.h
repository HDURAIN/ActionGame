// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Interfaces/Interactable.h"
#include "ActionGameTypes.h"
#include "DataAssets/DA_Item.h"
#include "DropVisualActor.h"
#include "ChestActor.generated.h"

class ADropVisualActor;
class UStaticMeshComponent;
class USphereComponent;
class UBoxComponent;
class AWorldItemActor;
class UInteractableComponent;
class UWorldObjectDataAsset;
class UWidgetComponent;
class UUserWidget;

/**
 * AChestActor
 *
 * 世界中的宝箱实体
 *
 * 职责：
 * - 作为 IInteractable，被 GA_Interact 触发
 * - 校验是否允许打开
 * - 打开后触发一次“掉落流程”
 *
 * 不负责：
 * - 掉落表现细节
 * - 物品交互逻辑
 * - 物理/下落
 */
UCLASS()
class ACTIONGAME_API AChestActor : public AActor, public IInteractable
{
	GENERATED_BODY()
	
public:	
	// Sets default values for this actor's properties
	AChestActor();

	// IInteractable
	virtual bool CanInteract_Implementation(AActor* Interactor) const override;
	virtual void ExecuteInteract_Implementation(AActor* Interactor) override;
	virtual EInteractType GetInteractType_Implementation() const override;

protected:
	// Components
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<USceneComponent> SceneRootComponent;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<UStaticMeshComponent> MeshComponent;

	// 当角色进入互动范围时 给角色传递指针
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Interact")
	TObjectPtr<UInteractableComponent> InteractableComponent;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<UBoxComponent> InteractTargetBox;


protected:

	// Called when the game starts or when spawned
	virtual void BeginPlay() override;

	virtual void Tick(float DeltaTime) override;

protected: // Interact

	// 打开宝箱后触发掉落Drop visual actor
	void StartDropFlow();

	// Drop visual actor 的生成位置，默认设置为保险的上方
	FVector GetDropSpawnLocation() const;

	UFUNCTION(BlueprintImplementableEvent, Category = "Chest")
	void OnChestOpened();

	UPROPERTY(EditDefaultsOnly, Category = "Interaction")
	EInteractType InteractType = EInteractType::Use;

	void BindOnDropFinished(FOnDropFinished InDegegate);

	// 掉落回调函数
	void HandleDropLanded(const FVector& GroundLocation);

	UFUNCTION()
	void OnRep_Opened();

public:	

	// 掉落表现体的 class
	UPROPERTY(EditDefaultsOnly, Category = "Drop")
	TSubclassOf<ADropVisualActor> DropVisualClass;

	UPROPERTY(EditDefaultsOnly, Category = "Drop")
	TSubclassOf<AWorldItemActor> WorldItemClass;

	UPROPERTY(EditDefaultsOnly, Category = "Drop")
	TArray<TObjectPtr<UDA_Item>> DropItems;

	UPROPERTY(EditDefaultsOnly, Category = "UI")
	TObjectPtr<UWorldObjectDataAsset> WorldObjectDataAsset;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "UI")
	TObjectPtr<UWidgetComponent> InteractWidgetComponent;

	// 宝箱是否已经打开
	UPROPERTY(VisibleInstanceOnly, ReplicatedUsing = OnRep_Opened, Category = "State")
	bool bOpened = false;

	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	void SetInteractUIVisible(bool bVisible);
};

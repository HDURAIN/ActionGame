// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "Delegates/Delegate.h"
#include "InteractCandidateComponent.generated.h"

DECLARE_MULTICAST_DELEGATE_TwoParams(FOnInteractCandidateChanged, AActor* /*Actor*/, bool /*bAdded*/);

UCLASS( ClassGroup=(Custom), meta=(BlueprintSpawnableComponent) )
class ACTIONGAME_API UInteractCandidateComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	// === 查询接口（只读） ===

	// 是否仍是交互候选（范围内）
	UFUNCTION(BlueprintCallable, Category = "Interact")
	bool IsInteractCandidate(AActor* Actor) const;

	// === 维护接口（由 Overlap / Scanner 调用） ===
	void AddCandidate(AActor* Actor);
	void RemoveCandidate(AActor* Actor);

	FOnInteractCandidateChanged OnInteractCandidateChanged;

	TSet<TWeakObjectPtr<AActor>> GetAllCandidates();

protected:
	// 当前交互候选集合（弱引用，非拥有）
	UPROPERTY()
	TSet<TWeakObjectPtr<AActor>> InteractCandidates;
};

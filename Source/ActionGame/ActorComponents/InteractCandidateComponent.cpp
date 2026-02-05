// Fill out your copyright notice in the Description page of Project Settings.


#include "ActorComponents/InteractCandidateComponent.h"

bool UInteractCandidateComponent::IsInteractCandidate(AActor* Actor) const
{
	if (!Actor)
	{
		return false;
	}

	return InteractCandidates.Contains(Actor);
}

void UInteractCandidateComponent::AddCandidate(AActor* Actor)
{
	if (!IsValid(Actor))
	{
		return;
	}

	InteractCandidates.Add(Actor);
}

void UInteractCandidateComponent::RemoveCandidate(AActor* Actor)
{
	if (!Actor)
	{
		return;
	}

	InteractCandidates.Remove(Actor);
}

// Fill out your copyright notice in the Description page of Project Settings.


#include "AnimNotifies/AnimNotify_Step.h"

#include "ActionGameCharacter.h"
#include "ActorComponents/FootstepsComponent.h"

void UAnimNotify_Step::Notify(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation)
{
	Super::Notify(MeshComp, Animation);
	if (!MeshComp) return;

	AActionGameCharacter* Character = Cast<AActionGameCharacter>(MeshComp->GetOwner());
	if (!Character) return;

	if (UAbilitySystemComponent* ASC = Character->GetAbilitySystemComponent())
	{
		static const FGameplayTag DeadTag = FGameplayTag::RequestGameplayTag(TEXT("State.Dead"));
		static const FGameplayTag RagdollTag = FGameplayTag::RequestGameplayTag(TEXT("State.Ragdoll"));

		if (ASC->HasMatchingGameplayTag(DeadTag) || ASC->HasMatchingGameplayTag(RagdollTag))
		{
			return;
		}
	}

	if (UFootstepsComponent* FootstepsComponent = Character->GetFootstepsComponent())
	{
		FootstepsComponent->HandleFootstep(Foot);
	}
}

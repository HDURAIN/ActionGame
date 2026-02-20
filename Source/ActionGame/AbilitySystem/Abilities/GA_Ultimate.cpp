// Fill out your copyright notice in the Description page of Project Settings.


#include "AbilitySystem/Abilities/GA_Ultimate.h"

void UGA_Ultimate::OnAbilityActivated()
{
	UE_LOG(LogTemp, Log, TEXT("Ultimate Activated!"));
	GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Red, TEXT("Ultimate Activated!"));
}

void UGA_Ultimate::OnAbilityEnded(bool bWasCancelled)
{
	UE_LOG(LogTemp, Log, TEXT("Ultimate Ended!"));
	GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Red, TEXT("Ultimate Ended!"));
}

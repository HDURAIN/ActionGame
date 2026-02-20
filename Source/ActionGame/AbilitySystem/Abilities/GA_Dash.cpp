// Fill out your copyright notice in the Description page of Project Settings.


#include "AbilitySystem/Abilities/GA_Dash.h"

void UGA_Dash::OnAbilityActivated()
{
	UE_LOG(LogTemp, Log, TEXT("Dash Activated!"));
	GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Red, TEXT("Dash Activated!"));
}

void UGA_Dash::OnAbilityEnded(bool bWasCancelled)
{
	UE_LOG(LogTemp, Log, TEXT("Dash Ended!"));
	GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Red, TEXT("Dash Ended!"));
}

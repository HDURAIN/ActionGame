// Fill out your copyright notice in the Description page of Project Settings.


#include "AbilitySystem/Abilities/GA_PrimaryAttack.h"

void UGA_PrimaryAttack::OnAbilityActivated()
{
	UE_LOG(LogTemp, Log, TEXT("Primary Attack Activated!"));
	GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Red, TEXT("Primary Attack Activated!"));
}

void UGA_PrimaryAttack::OnAbilityEnded(bool bWasCancelled)
{
	UE_LOG(LogTemp, Log, TEXT("Primary Attack Ended!"));
	GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Red, TEXT("Primary Attack Ended!"));
}

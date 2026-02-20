// Fill out your copyright notice in the Description page of Project Settings.


#include "AbilitySystem/Abilities/GA_SecondAttack.h"

void UGA_SecondAttack::OnAbilityActivated()
{
	UE_LOG(LogTemp, Log, TEXT("SecondAttack Activated!"));
	GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Red, TEXT("SecondAttack Activated!"));
}

void UGA_SecondAttack::OnAbilityEnded(bool bWasCancelled)
{
	UE_LOG(LogTemp, Log, TEXT("SecondAttack Ended!"));
	GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Red, TEXT("SecondAttack Ended!"));
}

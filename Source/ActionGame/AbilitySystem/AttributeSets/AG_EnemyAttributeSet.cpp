// Fill out your copyright notice in the Description page of Project Settings.


#include "AbilitySystem/AttributeSets/AG_EnemyAttributeSet.h"
#include "GameplayEffectExtension.h"
#include "GameFramework/Character.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Net/UnrealNetwork.h"

void UAG_EnemyAttributeSet::PostGameplayEffectExecute(const FGameplayEffectModCallbackData& Data)
{
	Super::PostGameplayEffectExecute(Data);
}

void UAG_EnemyAttributeSet::OnRep_Health(const FGameplayAttributeData& OldHealth)
{
	// 通知GAS这个值最近同步过了一次 自动调用
	GAMEPLAYATTRIBUTE_REPNOTIFY(UAG_EnemyAttributeSet, Health, OldHealth);
}

void UAG_EnemyAttributeSet::OnRep_MaxHealth(const FGameplayAttributeData& OldMaxHealth)
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(UAG_EnemyAttributeSet, MaxHealth, OldMaxHealth);
}

void UAG_EnemyAttributeSet::GetLifetimeReplicatedProps(TArray<class FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	// 告诉UE网络成员变量的同步方式
	// 类  成员变量  同步条件 (COND_None)无限制  同步后是否调用OnRep
	DOREPLIFETIME_CONDITION_NOTIFY(UAG_EnemyAttributeSet, Health, COND_None, REPNOTIFY_Always);
	DOREPLIFETIME_CONDITION_NOTIFY(UAG_EnemyAttributeSet, MaxHealth, COND_None, REPNOTIFY_Always);
}
// Fill out your copyright notice in the Description page of Project Settings.


#include "GameInstances/AG_GameInstance.h"
#include "AbilitySystemGlobals.h"

void UAG_GameInstance::Init()
{
	// 初始化GAS系统 全局配置/反射 Tag、Attribute、Effect 注册 
	// 不涉及Ability System Component实例以及具体的Ability和Attribute值
	UAbilitySystemGlobals::Get().InitGlobalData();
}

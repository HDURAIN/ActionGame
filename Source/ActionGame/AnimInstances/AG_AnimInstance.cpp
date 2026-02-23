// Fill out your copyright notice in the Description page of Project Settings.

#include "AnimInstances/AG_AnimInstance.h"
#include "ActionGameCharacter.h"
#include "ActionGameTypes.h"
#include "Animation/AnimSequenceBase.h"
#include "Animation/BlendSpace.h"
#include "DataAssets/CharacterDataAsset.h"
#include "DataAssets/CharacterAnimDataAsset.h"

UBlendSpace* UAG_AnimInstance::GetLocomotionBlendspace() const
{
	// 编辑器模式下返回默认的 BlendSpace，避免崩溃和空白预览
	if (GIsEditor && !GIsPlayInEditorWorld)
	{
		return IsValid(DefaultCharacterAnimDataAsset)
			? DefaultCharacterAnimDataAsset->CharacterAnimationData.MovementBlendspace
			: nullptr;
	}

	// 游戏运行时逻辑
	if (AActionGameCharacter* ActionGameCharacter = Cast<AActionGameCharacter>(GetOwningActor()))
	{
		FCharacterData CharacterData = ActionGameCharacter->GetCharacterData();

		if (IsValid(CharacterData.CharacterAnimDataAsset))
		{
			return CharacterData.CharacterAnimDataAsset->CharacterAnimationData.MovementBlendspace;
		}
	}

	// 回退到默认 BlendSpace
	return IsValid(DefaultCharacterAnimDataAsset)
		? DefaultCharacterAnimDataAsset->CharacterAnimationData.MovementBlendspace
		: nullptr;
}

UAnimSequenceBase* UAG_AnimInstance::GetIdleAnimation() const
{
	// 编辑器模式下返回默认动画，避免崩溃和空白预览
	if (GIsEditor && !GIsPlayInEditorWorld)
	{
		return IsValid(DefaultCharacterAnimDataAsset)
			? DefaultCharacterAnimDataAsset->CharacterAnimationData.IdleAnimationAsset
			: nullptr;
	}

	// 游戏运行时逻辑
	if (AActionGameCharacter* Character = Cast<AActionGameCharacter>(GetOwningActor()))
	{
		FCharacterData CharData = Character->GetCharacterData();
		if (IsValid(CharData.CharacterAnimDataAsset))
		{
			return CharData.CharacterAnimDataAsset->CharacterAnimationData.IdleAnimationAsset;
		}
	}

	// 回退到默认动画
	return IsValid(DefaultCharacterAnimDataAsset)
		? DefaultCharacterAnimDataAsset->CharacterAnimationData.IdleAnimationAsset
		: nullptr;
}

UBlendSpace* UAG_AnimInstance::GetCrouchLocomotionBlendspace() const
{
	// 编辑器模式下返回默认的 BlendSpace，避免崩溃和空白预览
	if (GIsEditor && !GIsPlayInEditorWorld)
	{
		return IsValid(DefaultCharacterAnimDataAsset)
			? DefaultCharacterAnimDataAsset->CharacterAnimationData.CrouchMovementBlendspace
			: nullptr;
	}

	// 游戏运行时逻辑
	if (AActionGameCharacter* ActionGameCharacter = Cast<AActionGameCharacter>(GetOwningActor()))
	{
		FCharacterData CharacterData = ActionGameCharacter->GetCharacterData();

		if (IsValid(CharacterData.CharacterAnimDataAsset))
		{
			return CharacterData.CharacterAnimDataAsset->CharacterAnimationData.CrouchMovementBlendspace;
		}
	}

	// 回退到默认 BlendSpace
	return IsValid(DefaultCharacterAnimDataAsset)
		? DefaultCharacterAnimDataAsset->CharacterAnimationData.CrouchMovementBlendspace
		: nullptr;
}

UAnimSequenceBase* UAG_AnimInstance::GetCrouchIdleAnimation() const
{
	// 编辑器模式下返回默认动画，避免崩溃和空白预览
	if (GIsEditor && !GIsPlayInEditorWorld)
	{
		return IsValid(DefaultCharacterAnimDataAsset)
			? DefaultCharacterAnimDataAsset->CharacterAnimationData.CrouchIdleAnimationAsset
			: nullptr;
	}

	// 游戏运行时逻辑
	if (AActionGameCharacter* Character = Cast<AActionGameCharacter>(GetOwningActor()))
	{
		FCharacterData CharData = Character->GetCharacterData();
		if (IsValid(CharData.CharacterAnimDataAsset))
		{
			return CharData.CharacterAnimDataAsset->CharacterAnimationData.CrouchIdleAnimationAsset;
		}
	}

	// 回退到默认动画
	return IsValid(DefaultCharacterAnimDataAsset)
		? DefaultCharacterAnimDataAsset->CharacterAnimationData.CrouchIdleAnimationAsset
		: nullptr;
}

UBlendSpace* UAG_AnimInstance::GetFireLocomotionBlendspace() const
{
	// 编辑器模式下返回默认的 BlendSpace，避免崩溃和空白预览
	if (GIsEditor && !GIsPlayInEditorWorld)
	{
		return IsValid(DefaultCharacterAnimDataAsset)
			? DefaultCharacterAnimDataAsset->CharacterAnimationData.FireMovementBlendSpace
			: nullptr;
	}

	// 游戏运行时逻辑
	if (AActionGameCharacter* ActionGameCharacter = Cast<AActionGameCharacter>(GetOwningActor()))
	{
		FCharacterData CharacterData = ActionGameCharacter->GetCharacterData();

		if (IsValid(CharacterData.CharacterAnimDataAsset))
		{
			return CharacterData.CharacterAnimDataAsset->CharacterAnimationData.FireMovementBlendSpace;
		}
	}

	// 回退到默认 BlendSpace
	return IsValid(DefaultCharacterAnimDataAsset)
		? DefaultCharacterAnimDataAsset->CharacterAnimationData.FireMovementBlendSpace
		: nullptr;
}

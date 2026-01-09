// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "Abilities/GameplayAbility.h"
#include "AbilitySystemInterface.h"
#include "Logging/LogMacros.h"
#include "ActionGameTypes.h"
#include "ActionGameCharacter.generated.h"

class UAG_AbilitySystemComponentBase;
class UAG_AttributeSetBase;

class UGameplayEffect;
class UGameplayAbility;

class USpringArmComponent;
class UCameraComponent;
class UFootstepsComponent;
class UInputAction;
class UInputMappingContext;
class UInputAction;
class UItemContainerComponent;
class UDA_Item;
struct FInputActionValue;


DECLARE_LOG_CATEGORY_EXTERN(LogTemplateCharacter, Log, All);

/**
 *  A simple player-controllable third person character
 *  Implements a controllable orbiting camera
 */
UCLASS(abstract)
class AActionGameCharacter : public ACharacter, public IAbilitySystemInterface
{
	GENERATED_BODY()

public:
	/** Constructor */
	AActionGameCharacter(const FObjectInitializer& ObjectInitializer);

	void BeginPlay() override;

private: // Camera
	/** Camera boom positioning the camera behind the character */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Components", meta = (AllowPrivateAccess = "true"))
	USpringArmComponent* CameraBoom;

	/** Follow camera */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Components", meta = (AllowPrivateAccess = "true"))
	UCameraComponent* FollowCamera;

protected: // Input

	/** Jump Input Action */
	UPROPERTY(EditAnywhere, Category="Input")
	UInputAction* JumpAction;

	/** Move Input Action */
	UPROPERTY(EditAnywhere, Category="Input")
	UInputAction* MoveAction;

	/** Look Input Action */
	UPROPERTY(EditAnywhere, Category="Input")
	UInputAction* LookAction;

	/** Mouse Look Input Action */
	UPROPERTY(EditAnywhere, Category="Input")
	UInputAction* MouseLookAction;

	UPROPERTY(EditAnywhere, Category="Input")
	UInputAction* CrouchAction;

	UPROPERTY(EditAnywhere, Category = "Input")
	UInputAction* SprintAction;

protected: // Input

	/** Initialize input action bindings */
	virtual void SetupPlayerInputComponent(class UInputComponent* PlayerInputComponent) override;

public: // Input

	/** Handles move inputs from either controls or UI interfaces */
	UFUNCTION(BlueprintCallable, Category="Input")
	virtual void DoMove(const FInputActionValue& Value);

	/** Handles look inputs from either controls or UI interfaces */
	UFUNCTION(BlueprintCallable, Category="Input")
	virtual void DoLook(const FInputActionValue& Value);

	/** Handles jump pressed inputs from either controls or UI interfaces */
	UFUNCTION(BlueprintCallable, Category="Input")
	virtual void DoJumpStart();

	/** Handles jump pressed inputs from either controls or UI interfaces */
	UFUNCTION(BlueprintCallable, Category="Input")
	virtual void DoJumpEnd();

	UFUNCTION(BlueprintCallable, Category="Input")
	virtual void DoCrouchActivate();

	UFUNCTION(BlueprintCallable, Category = "Input")
	virtual void DoCrouchCancel();

	UFUNCTION(BlueprintCallable, Category = "Input")
	virtual void DoSprintActivate();

	UFUNCTION(BlueprintCallable, Category = "Input")
	virtual void DoSprintCancel();

public: // Camera

	/** Returns CameraBoom subobject **/
	FORCEINLINE class USpringArmComponent* GetCameraBoom() const { return CameraBoom; }

	/** Returns FollowCamera subobject **/
	FORCEINLINE class UCameraComponent* GetFollowCamera() const { return FollowCamera; }

public: // Ability System

	virtual void PostInitializeComponents() override;

	bool ApplyGameplayEffectToSelf(TSubclassOf<UGameplayEffect> Effect, FGameplayEffectContextHandle InEffectContext);

	virtual UAbilitySystemComponent* GetAbilitySystemComponent() const override;

	virtual void PawnClientRestart() override;

	virtual void Landed(const FHitResult& Hit) override;

protected: // Ability System
	
	void GiveAbilities();

	void ApplyStartupEffects();

	// UE 网络/控制器系统自动调用的两个回调
	// 服务端初始化
	virtual void PossessedBy(AController* NewController) override;
	// 客户端初始化
	virtual void OnRep_PlayerState() override;

	UPROPERTY(EditDefaultsOnly)
	UAG_AbilitySystemComponentBase* AbilitySystemComponent;
	
	UPROPERTY(Transient)
	UAG_AttributeSetBase* AttributeSet;

	UPROPERTY(EditDefaultsOnly, Category = "Abilities|Tags")
	FGameplayTag CrouchAbilityTag;

	UPROPERTY(EditDefaultsOnly, Category = "Abilities|Tags")
	FGameplayTag SprintAbilityTag;

public: // Data Assets
	
	UFUNCTION(BlueprintCallable)
	FCharacterData GetCharacterData() const;

	UFUNCTION(BlueprintCallable)
	void SetCharacterData(const FCharacterData& InCharacterData);

	UFootstepsComponent* GetFootstepsComponent() const;

protected: // Data Assets

	// 动态的实时的CharacterData
	UPROPERTY(ReplicatedUsing = OnRep_CharacterData)
	FCharacterData CharacterData;

	// 客户端调用
	UFUNCTION()
	void OnRep_CharacterData();

	virtual void InitFromCharacterData(const FCharacterData& InCharacterData, bool bFromReplication = false);

	// 静态的，在蓝图中设置的
	UPROPERTY(EditDefaultsOnly)
	class UCharacterDataAsset* CharacterDataAsset;

	UPROPERTY(BlueprintReadOnly)
	TObjectPtr<UFootstepsComponent> FootstepsComponent;

protected:	// Gameplay Events

	UPROPERTY(EditDefaultsOnly)
	FGameplayTag JumpEventTag;

protected:	// Gameplay Tags

	UPROPERTY(EditDefaultsOnly)
	FGameplayTagContainer InAirTags;

protected: // Item
	TObjectPtr<UItemContainerComponent> ItemContainerComponent;

	UPROPERTY(EditDefaultsOnly, Category="Test")
	TObjectPtr<UDA_Item> TestStartupItem;
};


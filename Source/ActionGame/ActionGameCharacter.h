// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "Abilities/GameplayAbility.h"
#include "AbilitySystemInterface.h"
#include "Logging/LogMacros.h"
#include "ActionGameTypes.h"
#include "AbilitySystemComponent.h"
#include "ActionGameCharacter.generated.h"

class UAG_AbilitySystemComponentBase;
class UAG_AttributeSetBase;
class UGameplayEffect;
class UGameplayAbility;
class UInputAction;
class UInputMappingContext;
class UDA_Item;
class UCameraComponent;
class USpringArmComponent;
class UFootstepsComponent;
class UItemContainerComponent;
class UInteractCandidateComponent;
class UChildActorComponent;
class UCharacterDataAsset;
struct FInputActionValue;

DECLARE_LOG_CATEGORY_EXTERN(LogTemplateCharacter, Log, All);

/**
 * A simple player-controllable third person character.
 * - Third-person camera
 * - GAS integration
 * - Interaction / movement / startup initialization
 */
UCLASS(Abstract)
class AActionGameCharacter : public ACharacter, public IAbilitySystemInterface
{
	GENERATED_BODY()

public:
	// =========================================================================
	// Construction
	// =========================================================================

	/** Constructor */
	AActionGameCharacter(const FObjectInitializer& ObjectInitializer);

	// =========================================================================
	// Engine Lifecycle
	// =========================================================================

	virtual void BeginPlay() override;
	virtual void PostInitializeComponents() override;
	virtual void PawnClientRestart() override;
	virtual void Landed(const FHitResult& Hit) override;

protected:
	/** Initializes input bindings */
	virtual void SetupPlayerInputComponent(class UInputComponent* PlayerInputComponent) override;

	/** Server-side possession initialization */
	virtual void PossessedBy(AController* NewController) override;

	/** Client-side PlayerState replication initialization */
	virtual void OnRep_PlayerState() override;

public:
	// =========================================================================
	// Input API
	// =========================================================================

	/** Handles move input from player controls or UI */
	UFUNCTION(BlueprintCallable, Category = "Input")
	virtual void DoMove(const FInputActionValue& Value);

	/** Handles look input from player controls or UI */
	UFUNCTION(BlueprintCallable, Category = "Input")
	virtual void DoLook(const FInputActionValue& Value);

	/** Handles jump pressed */
	UFUNCTION(BlueprintCallable, Category = "Input")
	virtual void DoJumpStart();

	/** Handles jump released */
	UFUNCTION(BlueprintCallable, Category = "Input")
	virtual void DoJumpEnd();

	UFUNCTION(BlueprintCallable, Category = "Input")
	virtual void DoCrouchActivate();

	UFUNCTION(BlueprintCallable, Category = "Input")
	virtual void DoCrouchCancel();

	UFUNCTION(BlueprintCallable, Category = "Input")
	virtual void DoSprintActivate();

	UFUNCTION(BlueprintCallable, Category = "Input")
	virtual void DoSprintCancel();

	UFUNCTION(BlueprintCallable, Category = "Input")
	virtual void DoInteract();

	/** Debug hook before forwarding skill input to GAS */
	UFUNCTION(BlueprintImplementableEvent, Category = "Debug|Input")
	void BP_DebugBeforeSkillInput(EAbilityInputID InputID);

	void Input_Skill_1();
	void Input_Skill_2();
	void Input_Skill_3();
	void Input_Skill_4();

	/** Grants skill abilities */
	void GrantSkillAbilities();

	UFUNCTION(BlueprintCallable, Category = "Input")
	FVector GetMuzzleLocation() const;

public:
	// =========================================================================
	// Camera Accessors
	// =========================================================================

	/** Returns camera boom subobject */
	FORCEINLINE USpringArmComponent* GetCameraBoom() const { return CameraBoom; }

	/** Returns follow camera subobject */
	FORCEINLINE UCameraComponent* GetFollowCamera() const { return FollowCamera; }

public:
	// =========================================================================
	// Ability System
	// =========================================================================

	/** Applies a GameplayEffect to self */
	bool ApplyGameplayEffectToSelf(
		TSubclassOf<UGameplayEffect> Effect,
		FGameplayEffectContextHandle InEffectContext
	);

	/** IAbilitySystemInterface */
	virtual UAbilitySystemComponent* GetAbilitySystemComponent() const override;

	/** 向自身发送技能输入Gameplay Event（后续供 Skill1~4 输入函数复用） */
	void SendSkillInputEvent(const FGameplayTag& EventTag);

protected:
	/** Grants startup abilities */
	void GiveAbilities();

	/** Applies startup effects */
	void ApplyStartupEffects();

	/** GAS tag change callback */
	UFUNCTION()
	void OnFiringTagChanged(const FGameplayTag Tag, int32 Count);

public:
	// =========================================================================
	// Character Data
	// =========================================================================

	UFUNCTION(BlueprintCallable, Category = "Character|Data")
	FCharacterData GetCharacterData() const;

	UFUNCTION(BlueprintCallable, Category = "Character|Data")
	void SetCharacterData(const FCharacterData& InCharacterData);

	UFUNCTION(BlueprintCallable, Category = "Character|Data")
	UFootstepsComponent* GetFootstepsComponent() const;

protected:
	/** RepNotify for runtime character data */
	UFUNCTION()
	void OnRep_CharacterData();

	/** Initializes runtime state from character data */
	virtual void InitFromCharacterData(const FCharacterData& InCharacterData, bool bFromReplication = false);

public:
	// =========================================================================
	// Attributes / ASC Delegates
	// =========================================================================

	virtual void OnMaxJumpCountChanged(const FOnAttributeChangeData& Data);
	void OnHealthAttributeChanged(const FOnAttributeChangeData& Data);

	void BindASCAttributeDelegates();
	void UnbindASCAttributeDelegates();

	UFUNCTION(BlueprintCallable, Category = "Components|Weapon")
	AActor* GetWeaponActor() const;

public:
	// =========================================================================
	// Runtime State Queries
	// =========================================================================

	UFUNCTION(BlueprintPure, Category = "State")
	bool IsFiring() const { return bFiring; }

protected:
	// =========================================================================
	// Input Assets
	// =========================================================================

	/** Jump input action */
	UPROPERTY(EditAnywhere, Category = "Input")
	TObjectPtr<UInputAction> JumpAction;

	/** Move input action */
	UPROPERTY(EditAnywhere, Category = "Input")
	TObjectPtr<UInputAction> MoveAction;

	/** Look input action (gamepad / stick) */
	UPROPERTY(EditAnywhere, Category = "Input")
	TObjectPtr<UInputAction> LookAction;

	/** Look input action (mouse) */
	UPROPERTY(EditAnywhere, Category = "Input")
	TObjectPtr<UInputAction> MouseLookAction;

	UPROPERTY(EditAnywhere, Category = "Input")
	TObjectPtr<UInputAction> CrouchAction;

	UPROPERTY(EditAnywhere, Category = "Input")
	TObjectPtr<UInputAction> SprintAction;

	UPROPERTY(EditAnywhere, Category = "Input")
	TObjectPtr<UInputAction> InteractAction;

	UPROPERTY(EditAnywhere, Category = "Input")
	TObjectPtr<UInputAction> Skill_1_Action;

	UPROPERTY(EditAnywhere, Category = "Input")
	TObjectPtr<UInputAction> Skill_2_Action;

	UPROPERTY(EditAnywhere, Category = "Input")
	TObjectPtr<UInputAction> Skill_3_Action;

	UPROPERTY(EditAnywhere, Category = "Input")
	TObjectPtr<UInputAction> Skill_4_Action;

protected:
	// =========================================================================
	// Character Data (Runtime / Static)
	// =========================================================================

	/** Runtime replicated character data */
	UPROPERTY(ReplicatedUsing = OnRep_CharacterData)
	FCharacterData CharacterData;

	/** Static character config set in Blueprint */
	UPROPERTY(EditDefaultsOnly, Category = "Character|Data")
	TObjectPtr<UCharacterDataAsset> CharacterDataAsset;

	UPROPERTY(BlueprintReadOnly, Category = "Character|Data")
	TObjectPtr<UFootstepsComponent> FootstepsComponent;

protected:
	// =========================================================================
	// Gameplay Events / Tags / Components
	// =========================================================================

	UPROPERTY(EditDefaultsOnly, Category = "Abilities|Events")
	FGameplayTag JumpEventTag;

	// 技能输入事件（固定4槽位，Pressed）
	UPROPERTY(EditDefaultsOnly, Category = "Abilities|Events")
	FGameplayTag Skill1PressedEventTag;

	UPROPERTY(EditDefaultsOnly, Category = "Abilities|Events")
	FGameplayTag Skill2PressedEventTag;

	UPROPERTY(EditDefaultsOnly, Category = "Abilities|Events")
	FGameplayTag Skill3PressedEventTag;

	UPROPERTY(EditDefaultsOnly, Category = "Abilities|Events")
	FGameplayTag Skill4PressedEventTag;

	UPROPERTY(EditDefaultsOnly, Category = "Abilities|Events")
	FGameplayTag ZeroHealthEventTag;

	UPROPERTY(EditDefaultsOnly, Category = "Abilities|Tags")
	FGameplayTagContainer InAirTags;

	UPROPERTY(EditDefaultsOnly, Category = "Abilities|Tags")
	FGameplayTag CrouchAbilityTag;

	UPROPERTY(EditDefaultsOnly, Category = "Abilities|Tags")
	FGameplayTag SprintAbilityTag;

	UPROPERTY(EditDefaultsOnly, Category = "Abilities|Tags")
	FGameplayTag InteractAbilityTag;

	UPROPERTY(EditDefaultsOnly, Category = "Abilities|Tags")
	FGameplayTag RagdollStateTag;

	UPROPERTY(EditDefaultsOnly, Category = "Abilities")
	TObjectPtr<UAG_AbilitySystemComponentBase> AbilitySystemComponent;

	UPROPERTY(Transient)
	TObjectPtr<UAG_AttributeSetBase> AttributeSet;

protected:
	// =========================================================================
	// Item / Inventory
	// =========================================================================

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components|Inventory")
	TObjectPtr<UItemContainerComponent> ItemContainerComponent;

protected:
	// =========================================================================
	// Interaction
	// =========================================================================

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components|Interact")
	TObjectPtr<UInteractCandidateComponent> InteractCandidateComponent;

	void HandleInteractCandidateChanged(AActor* Actor, bool bAdded);

protected:
	// =========================================================================
	// Runtime State
	// =========================================================================

	bool bFiring = false;

protected:
	// =========================================================================
	// Delegate Handles
	// =========================================================================

	FDelegateHandle MaxJumpCountChangedHandle;

private:
	// =========================================================================
	// Components - Camera
	// =========================================================================

	/** Camera boom positioning the camera behind the character */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components|Camera", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<USpringArmComponent> CameraBoom;

	/** Follow camera */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components|Camera", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UCameraComponent> FollowCamera;

private:
	// =========================================================================
	// Components - Weapon
	// =========================================================================

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components|Weapon", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UChildActorComponent> WeaponComponentRight;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components|Weapon", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UChildActorComponent> WeaponComponentLeft;

public:
	// =========================================================================
	// Death
	// =========================================================================
	void StartRagdoll();

protected:
	// =========================================================================
	// Death
	// =========================================================================

	UFUNCTION()
	void OnRagdollStateTagChanged(const FGameplayTag CallbackTag, int32 NewCount);
};
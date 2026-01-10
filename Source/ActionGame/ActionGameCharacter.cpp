// Copyright Epic Games, Inc. All Rights Reserved.

#include "ActionGameCharacter.h"
#include "Engine/LocalPlayer.h"
#include "Camera/CameraComponent.h"
#include "Components/CapsuleComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/SpringArmComponent.h"
#include "GameFramework/Controller.h"
#include "EnhancedInputComponent.h"
#include "EnhancedInputSubsystems.h"
#include "InputActionValue.h"
#include "InputMappingContext.h"
#include "ActionGame.h"

#include "AbilitySystemComponent.h"
#include "AbilitySystemBlueprintLibrary.h"
#include "AbilitySystem/AttributeSets/AG_AttributeSetBase.h"
#include "AbilitySystem/Components/AG_AbilitySystemComponentBase.h"

#include "ActorComponents/ItemContainerComponent.h"
#include "DataAssets/DA_Item.h"

#include "ActorComponents/AG_CharacterMovementComponent.h"
#include "ActorComponents/FootstepsComponent.h"
#include "DataAssets/CharacterDataAsset.h"
#include "DataAssets/CharacterAnimDataAsset.h"
#include "Net/UnrealNetwork.h"
#include "ActionGamePlayerController.h"

// 加上自己的移动组件的构造函数
// const FObjectInitializer& ObjectInitializer - UE 用来集中管理“子对象创建规则”的对象
AActionGameCharacter::AActionGameCharacter(const FObjectInitializer& ObjectInitializer) :
	Super(ObjectInitializer.SetDefaultSubobjectClass<UAG_CharacterMovementComponent>(ACharacter::CharacterMovementComponentName))
{
	PrimaryActorTick.bCanEverTick = false;

	// Set size for collision capsule
	GetCapsuleComponent()->InitCapsuleSize(42.f, 96.0f);

	// Don't rotate when the controller rotates. Let that just affect the camera.
	bUseControllerRotationPitch = false;
	bUseControllerRotationYaw = false;
	bUseControllerRotationRoll = false;

	// Configure character movement
	GetCharacterMovement()->bOrientRotationToMovement = true;
	GetCharacterMovement()->RotationRate = FRotator(0.0f, 500.0f, 0.0f);

	// Note: For faster iteration times these variables, and many more, can be tweaked in the Character Blueprint
	// instead of recompiling to adjust them
	GetCharacterMovement()->JumpZVelocity = 500.f;
	GetCharacterMovement()->AirControl = 0.35f;
	/*GetCharacterMovement()->MaxWalkSpeed = 500.f;*/
	GetCharacterMovement()->MinAnalogWalkSpeed = 20.f;
	GetCharacterMovement()->BrakingDecelerationWalking = 2000.f;
	GetCharacterMovement()->BrakingDecelerationFalling = 1500.0f;

	// Create a camera boom (pulls in towards the player if there is a collision)
	CameraBoom = CreateDefaultSubobject<USpringArmComponent>(TEXT("CameraBoom"));
	CameraBoom->SetupAttachment(RootComponent);
	CameraBoom->TargetArmLength = 400.0f;
	CameraBoom->bUsePawnControlRotation = true;

	// Create a follow camera
	FollowCamera = CreateDefaultSubobject<UCameraComponent>(TEXT("FollowCamera"));
	FollowCamera->SetupAttachment(CameraBoom, USpringArmComponent::SocketName);
	FollowCamera->bUsePawnControlRotation = false;

	// Note: The skeletal mesh and anim blueprint references on the Mesh component (inherited from Character) 
	// are set in the derived blueprint asset named ThirdPersonCharacter (to avoid direct content references in C++)

	// Ability System

	AbilitySystemComponent = CreateDefaultSubobject<UAG_AbilitySystemComponentBase>(TEXT("AbilitySystemComponent"));
	AbilitySystemComponent->SetIsReplicated(true);
	AbilitySystemComponent->SetReplicationMode(EGameplayEffectReplicationMode::Mixed);

	AttributeSet = CreateDefaultSubobject<UAG_AttributeSetBase>(TEXT("AttributeSet"));

	FootstepsComponent = CreateDefaultSubobject<UFootstepsComponent>(TEXT("FootstepsComponent"));

	ItemContainerComponent =CreateDefaultSubobject<UItemContainerComponent>(TEXT("ItemContainerComponent"));
}

void AActionGameCharacter::BeginPlay()
{
	Super::BeginPlay();
}

void AActionGameCharacter::PostInitializeComponents()
{
	// 初始化CharacterData
	Super::PostInitializeComponents();

	if (GetWorld() && GetWorld()->IsGameWorld())
	{
		// 只在真正游戏世界里初始化
		if (IsValid(CharacterDataAsset))
		{
			SetCharacterData(CharacterDataAsset->CharacterData);
		}
	}
}

void AActionGameCharacter::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	// 如果AActionGameCharacter的实例参与复制，那么CharacterData需要被复制给客户端
	DOREPLIFETIME(AActionGameCharacter, CharacterData);
}

void AActionGameCharacter::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	// Set up action bindings
	if (UEnhancedInputComponent* EnhancedInputComponent = Cast<UEnhancedInputComponent>(PlayerInputComponent)) {
		
		// Jumping
		EnhancedInputComponent->BindAction(JumpAction, ETriggerEvent::Started, this, &AActionGameCharacter::DoJumpStart);
		EnhancedInputComponent->BindAction(JumpAction, ETriggerEvent::Completed, this, &AActionGameCharacter::DoJumpEnd);

		// Moving
		EnhancedInputComponent->BindAction(MoveAction, ETriggerEvent::Triggered, this, &AActionGameCharacter::DoMove);

		// Looking
		EnhancedInputComponent->BindAction(MouseLookAction, ETriggerEvent::Triggered, this, &AActionGameCharacter::DoLook);
		EnhancedInputComponent->BindAction(LookAction, ETriggerEvent::Triggered, this, &AActionGameCharacter::DoLook);

		// Crouching
		EnhancedInputComponent->BindAction(CrouchAction, ETriggerEvent::Started, this, &AActionGameCharacter::DoCrouchActivate);
		EnhancedInputComponent->BindAction(CrouchAction, ETriggerEvent::Completed, this, &AActionGameCharacter::DoCrouchCancel);

		// Sprinting
		EnhancedInputComponent->BindAction(SprintAction, ETriggerEvent::Started, this, &AActionGameCharacter::DoSprintActivate);
		EnhancedInputComponent->BindAction(SprintAction, ETriggerEvent::Completed, this, &AActionGameCharacter::DoSprintCancel);
	}
	else
	{
		UE_LOG(LogActionGame, Error, TEXT("'%s' Failed to find an Enhanced Input component! This template is built to use the Enhanced Input system. If you intend to use the legacy system, then you will need to update this C++ file."), *GetNameSafe(this));
	}
}

void AActionGameCharacter::PawnClientRestart()
{
	Super::PawnClientRestart();

	// 确保对象是客户端本地的Pawn而不是服务器和其他客户端上的
	if (!IsLocallyControlled()) return;

	if (AActionGamePlayerController* PC = Cast<AActionGamePlayerController>(GetController()))
	{
		PC->ApplyDefaultMappings();
	}
}

void AActionGameCharacter::Landed(const FHitResult& Hit)
{
	Super::Landed(Hit);

	if (!AbilitySystemComponent) return;

	if (HasAuthority() && AbilitySystemComponent)
	{
		AbilitySystemComponent->RemoveActiveEffectsWithTags(InAirTags);
	}
}

void AActionGameCharacter::DoMove(const FInputActionValue& Value)
{
	// input is a Vector2D
	const FVector2D MoveInput = Value.Get<FVector2D>();

	if (!Controller) return;

	const FRotator YawRot(0.f, Controller->GetControlRotation().Yaw, 0.f);

	const FVector Forward = FRotationMatrix(YawRot).GetUnitAxis(EAxis::X);
	const FVector Right = FRotationMatrix(YawRot).GetUnitAxis(EAxis::Y);

	AddMovementInput(Forward, MoveInput.Y);
	AddMovementInput(Right, MoveInput.X);
}

void AActionGameCharacter::DoLook(const FInputActionValue& Value)
{
	// input is a Vector2D
	const FVector2D LookInput = Value.Get<FVector2D>();

	AddControllerYawInput(LookInput.X);
	AddControllerPitchInput(LookInput.Y);
}

void AActionGameCharacter::DoJumpStart()
{
	// 向 AbilitySystemComponent 发送一个 Gameplay Event
	FGameplayEventData Payload;
	Payload.Instigator = this;
	Payload.EventTag = JumpEventTag;

	UAbilitySystemBlueprintLibrary::SendGameplayEventToActor(this, JumpEventTag, Payload);
}

void AActionGameCharacter::DoJumpEnd()
{
	// signal the character to stop jumping
	StopJumping();
}

void AActionGameCharacter::DoCrouchActivate()
{
	if (!AbilitySystemComponent) return;

	FGameplayTagContainer TempTags;
	TempTags.AddTag(CrouchAbilityTag);
	AbilitySystemComponent->TryActivateAbilitiesByTag(FGameplayTagContainer(TempTags));
}

void AActionGameCharacter::DoCrouchCancel()
{
	if (!AbilitySystemComponent) return;

	FGameplayTagContainer TempTags;
	TempTags.AddTag(CrouchAbilityTag);
	AbilitySystemComponent->CancelAbilities(&TempTags);
}

void AActionGameCharacter::DoSprintActivate()
{
	if (!AbilitySystemComponent) return;

	FGameplayTagContainer TempTags;
	TempTags.AddTag(SprintAbilityTag);
	AbilitySystemComponent->TryActivateAbilitiesByTag(FGameplayTagContainer(TempTags));
}

void AActionGameCharacter::DoSprintCancel()
{
	if (!AbilitySystemComponent) return;

	FGameplayTagContainer TempTags;
	TempTags.AddTag(SprintAbilityTag);
	AbilitySystemComponent->CancelAbilities(&TempTags);
}

void AActionGameCharacter::OnMaxJumpCountChanged(const FOnAttributeChangeData& Data)
{
	const int32 NewJumpCount = FMath::Max(1, FMath::FloorToInt(Data.NewValue));
	JumpMaxCount = NewJumpCount;
}

void AActionGameCharacter::BindASCAttributeDelegates()
{
	if (!AbilitySystemComponent)
	{
		return;
	}

	// 先解绑，保证幂等（不会重复绑定）
	UnbindASCAttributeDelegates();

	MaxJumpCountChangedHandle =
		AbilitySystemComponent
		->GetGameplayAttributeValueChangeDelegate(
			UAG_AttributeSetBase::GetMaxJumpCountAttribute()
		)
		.AddUObject(this, &AActionGameCharacter::OnMaxJumpCountChanged);
}

void AActionGameCharacter::UnbindASCAttributeDelegates()
{
	if (!AbilitySystemComponent)
	{
		return;
	}

	if (MaxJumpCountChangedHandle.IsValid())
	{
		AbilitySystemComponent
			->GetGameplayAttributeValueChangeDelegate(
				UAG_AttributeSetBase::GetMaxJumpCountAttribute()
			)
			.Remove(MaxJumpCountChangedHandle);

		MaxJumpCountChangedHandle.Reset();
	}
}

bool AActionGameCharacter::ApplyGameplayEffectToSelf(TSubclassOf<UGameplayEffect> Effect, FGameplayEffectContextHandle InEffectContext)
{
	if (!Effect) return false;

	FGameplayEffectSpecHandle SpecHandle = AbilitySystemComponent->MakeOutgoingSpec(Effect, 1, InEffectContext);
	if (SpecHandle.IsValid())
	{
		FActiveGameplayEffectHandle ActiveGEHandle = AbilitySystemComponent->ApplyGameplayEffectSpecToSelf(*SpecHandle.Data);

		return ActiveGEHandle.WasSuccessfullyApplied();
	}

	return false;
}

UAbilitySystemComponent* AActionGameCharacter::GetAbilitySystemComponent() const
{
	return AbilitySystemComponent;
}

void AActionGameCharacter::GiveAbilities()
{
	if (HasAuthority() && AbilitySystemComponent)
	{
		for (auto DefaultAbiltiy : CharacterData.Abilities)
		{
			AbilitySystemComponent->GiveAbility(FGameplayAbilitySpec(DefaultAbiltiy));
		}
	}
}

void AActionGameCharacter::ApplyStartupEffects()
{
	if (GetLocalRole() == ROLE_Authority)
	{
		FGameplayEffectContextHandle EffectContext = AbilitySystemComponent->MakeEffectContext();
		EffectContext.AddSourceObject(this);

		for (auto CharacterEffect : CharacterData.Effects)
		{
			ApplyGameplayEffectToSelf(CharacterEffect, EffectContext);
		}
	}
}

void AActionGameCharacter::PossessedBy(AController* NewController)
{
	Super::PossessedBy(NewController);

	AbilitySystemComponent->InitAbilityActorInfo(this, this);
	UE_LOG(LogTemp, Warning, TEXT("PossessedBy InitAbilityActorInfo Completed"));

	GiveAbilities();
	ApplyStartupEffects();

	if (TestStartupItem_SpeedUp)
	{
		if (UItemContainerComponent* ItemContainer =
			FindComponentByClass<UItemContainerComponent>())
		{
			ItemContainer->AddItem(TestStartupItem_SpeedUp, 3);
		}
	}

	BindASCAttributeDelegates();

	if (TestStartupItem_MoreJump)
	{	
		if (UItemContainerComponent* ItemContainer =
			FindComponentByClass<UItemContainerComponent>())
		{
			ItemContainer->AddItem(TestStartupItem_MoreJump, 3);
		}
	}
}

void AActionGameCharacter::OnRep_PlayerState()
{
	Super::OnRep_PlayerState();

	// 告诉ASC谁拥有它，作用到谁身上
	AbilitySystemComponent->InitAbilityActorInfo(this, this);
	UE_LOG(LogTemp, Warning, TEXT("OnRep_PlayerState InitAbilityActorInfo Completed"));

}

FCharacterData AActionGameCharacter::GetCharacterData() const
{
	return CharacterData;
}

void AActionGameCharacter::SetCharacterData(const FCharacterData& InCharacterData)
{
	CharacterData = InCharacterData;

	InitFromCharacterData(CharacterData);
}

UFootstepsComponent* AActionGameCharacter::GetFootstepsComponent() const
{
	return FootstepsComponent;
}

void AActionGameCharacter::OnRep_CharacterData()
{
	InitFromCharacterData(CharacterData, true);
}

void AActionGameCharacter::InitFromCharacterData(const FCharacterData& InCharacterData, bool bFromReplication)
{
	// 纯客户端的表现
}
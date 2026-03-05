#include "Characters/EnemyGroundShooterCharacter.h"

#include "GameFramework/CharacterMovementComponent.h"

AEnemyGroundShooterCharacter::AEnemyGroundShooterCharacter()
{
	PrimaryActorTick.bCanEverTick = false;
	bUseControllerRotationYaw = false;
}

void AEnemyGroundShooterCharacter::BeginPlay()
{
	Super::BeginPlay();

	if (UCharacterMovementComponent* MoveComp = GetCharacterMovement())
	{
		MoveComp->SetMovementMode(MOVE_Walking);
		MoveComp->GravityScale = 1.f;

		MoveComp->MaxWalkSpeed = WalkSpeed;
		MoveComp->BrakingDecelerationWalking = BrakingDecelerationWalking;

		MoveComp->bOrientRotationToMovement = bFaceMoveDirection;
		MoveComp->RotationRate = FRotator(0.f, TurnRateYaw, 0.f);
	}
}
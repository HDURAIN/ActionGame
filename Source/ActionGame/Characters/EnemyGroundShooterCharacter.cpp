#include "Characters/EnemyGroundShooterCharacter.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "AbilitySystem/AttributeSets/AG_EnemyAttributeSet.h"

AEnemyGroundShooterCharacter::AEnemyGroundShooterCharacter()
{
	PrimaryActorTick.bCanEverTick = false;
	bUseControllerRotationYaw = false;
}

void AEnemyGroundShooterCharacter::PerformAttack(AActor* TargetActor)
{
	if (!HasAuthority() || !IsValid(TargetActor)) return;

}

void AEnemyGroundShooterCharacter::BeginPlay()
{
	Super::BeginPlay();

	UE_LOG(LogTemp, Warning, TEXT("Enemy %s Health=%.1f"),
		*GetName(), EnemyAttributeSet ? EnemyAttributeSet->GetHealth() : -1.f);

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
#include "EnemyAIController.h"
#include "AITypes.h"
#include "Navigation/PathFollowingComponent.h"
#include "Characters/EnemyCharacterBase.h"

void AEnemyAIController::ChaseTarget(AActor* TargetActor)
{
	if (!IsValid(TargetActor))
	{
		StopChasing();
		return;
	}

	MoveToTarget_Internal(TargetActor);
}

void AEnemyAIController::StopChasing()
{
	StopMovement();
}

void AEnemyAIController::MoveToTarget_Internal(AActor* TargetActor)
{
	if (!IsValid(TargetActor))
	{
		return;
	}

	const AEnemyCharacterBase* Enemy = Cast<AEnemyCharacterBase>(GetPawn());
	if (!IsValid(Enemy))
	{
		UE_LOG(LogTemp, Warning, TEXT("EnemyAIController: Controlled Pawn is not AEnemyCharacterBase."));
		return;
	}

	FAIMoveRequest MoveRequest;
	MoveRequest.SetGoalActor(TargetActor);
	MoveRequest.SetAcceptanceRadius(Enemy->GetTargetAcceptanceRadius());
	MoveRequest.SetUsePathfinding(Enemy->ShouldUsePathfinding());
	MoveRequest.SetAllowPartialPath(true);
	MoveRequest.SetProjectGoalLocation(Enemy->ShouldUsePathfinding());

	EPathFollowingRequestResult::Type Result = MoveTo(MoveRequest);
	UE_LOG(LogTemp, Verbose, TEXT("EnemyAIController MoveTo result=%d UsePath=%d Radius=%.1f Target=%s"),
		(int32)Result,
		Enemy->ShouldUsePathfinding() ? 1 : 0,
		Enemy->GetTargetAcceptanceRadius(),
		*GetNameSafe(TargetActor));
}
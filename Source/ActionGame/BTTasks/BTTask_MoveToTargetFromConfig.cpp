#include "BTTask_MoveToTargetFromConfig.h"

#include "AIController.h"
#include "BehaviorTree/BlackboardComponent.h"
#include "Navigation/PathFollowingComponent.h"
#include "Characters/EnemyCharacterBase.h"

UBTTask_MoveToTargetFromConfig::UBTTask_MoveToTargetFromConfig()
{
	NodeName = TEXT("Move To Target (Enemy Config)");
	bNotifyTick = true; // 让 TickTask 生效
}

EBTNodeResult::Type UBTTask_MoveToTargetFromConfig::ExecuteTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory)
{
	AAIController* AIC = OwnerComp.GetAIOwner();
	if (!IsValid(AIC))
	{
		return EBTNodeResult::Failed;
	}

	UBlackboardComponent* BB = OwnerComp.GetBlackboardComponent();
	if (!BB)
	{
		return EBTNodeResult::Failed;
	}

	AActor* TargetActor = Cast<AActor>(BB->GetValueAsObject(GetSelectedBlackboardKey()));
	if (!IsValid(TargetActor))
	{
		return EBTNodeResult::Failed;
	}

	AEnemyCharacterBase* Enemy = Cast<AEnemyCharacterBase>(AIC->GetPawn());
	if (!IsValid(Enemy))
	{
		return EBTNodeResult::Failed;
	}

	const bool bUsePath = Enemy->ShouldUsePathfinding();

	FAIMoveRequest Req;
	Req.SetGoalActor(TargetActor);
	Req.SetAcceptanceRadius(Enemy->GetTargetAcceptanceRadius());
	Req.SetUsePathfinding(bUsePath);
	Req.SetProjectGoalLocation(bUsePath);   // 飞行怪 false，地面怪 true
	Req.SetAllowPartialPath(true);

	const EPathFollowingRequestResult::Type Result = AIC->MoveTo(Req);
	if (Result == EPathFollowingRequestResult::Failed)
	{
		return EBTNodeResult::Failed;
	}

	// InProgress：后续在 TickTask 里判断是否到达/失败
	return EBTNodeResult::InProgress;
}

void UBTTask_MoveToTargetFromConfig::TickTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory, float DeltaSeconds)
{
	AAIController* AIC = OwnerComp.GetAIOwner();
	AEnemyCharacterBase* Enemy = IsValid(AIC) ? Cast<AEnemyCharacterBase>(AIC->GetPawn()) : nullptr;
	UBlackboardComponent* BB = OwnerComp.GetBlackboardComponent();
	AActor* TargetActor = BB ? Cast<AActor>(BB->GetValueAsObject(GetSelectedBlackboardKey())) : nullptr;

	if (!IsValid(AIC) || !IsValid(Enemy) || !IsValid(TargetActor))
	{
		FinishLatentTask(OwnerComp, EBTNodeResult::Failed);
		return;
	}

	// 1) 如果 PathFollowing 已经结束（成功/失败），直接收尾
	UPathFollowingComponent* PFC = AIC->GetPathFollowingComponent();
	if (!PFC)
	{
		FinishLatentTask(OwnerComp, EBTNodeResult::Failed);
		return;
	}

	const EPathFollowingStatus::Type Status = PFC->GetStatus();
	if (Status == EPathFollowingStatus::Idle)
	{
		const bool bReached = PFC->DidMoveReachGoal();
		FinishLatentTask(OwnerComp, bReached ? EBTNodeResult::Succeeded : EBTNodeResult::Failed);
		return;
	}

	// 2) （可选）当目标离开“接受半径+滞回”时，重新发一次 MoveTo，保证持续追踪
	//    这能解决“追到就停住、目标又跑远”的情况，且不用 BT Decorator 抖动。
	const float R = FMath::Max(0.f, Enemy->GetTargetAcceptanceRadius());
	const float LeaveRadius = R + ExtraLeaveRadius;

	const float DistSq = FVector::DistSquared(Enemy->GetActorLocation(), TargetActor->GetActorLocation());
	if (DistSq > FMath::Square(LeaveRadius))
	{
		// 重新下发一次（不会每帧发，因为只有离开阈值才发）
		const bool bUsePath = Enemy->ShouldUsePathfinding();

		FAIMoveRequest Req;
		Req.SetGoalActor(TargetActor);
		Req.SetAcceptanceRadius(R);
		Req.SetUsePathfinding(bUsePath);
		Req.SetProjectGoalLocation(bUsePath);
		Req.SetAllowPartialPath(true);

		AIC->MoveTo(Req);
	}
}

EBTNodeResult::Type UBTTask_MoveToTargetFromConfig::AbortTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory)
{
	AAIController* AIC = OwnerComp.GetAIOwner();
	if (IsValid(AIC))
	{
		AIC->StopMovement();
	}
	return EBTNodeResult::Aborted;
}
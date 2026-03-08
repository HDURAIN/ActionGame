#include "BTTask_Attack.h"

#include "AIController.h"
#include "BehaviorTree/BlackboardComponent.h"
#include "Characters/EnemyCharacterBase.h"

UBTTask_Attack::UBTTask_Attack()
{
	NodeName = TEXT("Attack (Enemy)");
	// BlackboardBaseKey：在 BT 里选 TargetActor
}

EBTNodeResult::Type UBTTask_Attack::ExecuteTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory)
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

	// 触发一次攻击（由子类实现：地面射击/近战/自爆等）
	Enemy->PerformAttack(TargetActor);

	return EBTNodeResult::Succeeded;
}
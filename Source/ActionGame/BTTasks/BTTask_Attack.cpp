#include "BTTask_Attack.h"

#include "AIController.h"
#include "BehaviorTree/BlackboardComponent.h"
#include "Characters/EnemyCharacterBase.h"

UBTTask_Attack::UBTTask_Attack()
{
	NodeName = TEXT("Attack (Enemy)");
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

	if (!BB->GetValueAsBool(TEXT("IsAlive")))
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

	AIC->SetFocus(TargetActor, EAIFocusPriority::Gameplay);
	Enemy->PerformAttack(TargetActor);

	return EBTNodeResult::Succeeded;
}

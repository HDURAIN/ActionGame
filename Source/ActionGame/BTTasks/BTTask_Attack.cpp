#include "BTTask_Attack.h"

#include "AIController.h"
#include "BehaviorTree/BlackboardComponent.h"
#include "Characters/EnemyCharacterBase.h"

UBTTask_Attack::UBTTask_Attack()
{
	NodeName = TEXT("Attack (Enemy)");
	// BlackboardBaseKey���� BT ��ѡ TargetActor
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

	// ���빥����Ϊʱ�����ý��㣻����ά���� Service ����
	AIC->SetFocus(TargetActor, EAIFocusPriority::Gameplay);

	// ����һ�ι�����������ʵ�֣��������/��ս/�Ա��ȣ�
	Enemy->PerformAttack(TargetActor);

	return EBTNodeResult::Succeeded;
}
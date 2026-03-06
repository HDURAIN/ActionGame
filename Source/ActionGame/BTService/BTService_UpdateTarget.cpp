// BTService_UpdateTarget.cpp

#include "BTService/BTService_UpdateTarget.h"

#include "AIController.h"
#include "BehaviorTree/BlackboardComponent.h"
#include "Characters/EnemyCharacterBase.h"

UBTService_UpdateTarget::UBTService_UpdateTarget()
{
	NodeName = TEXT("Update Target Actor");

	// Service tick interval（避免每帧）
	Interval = 0.3f;
	RandomDeviation = 0.1f;
}

void UBTService_UpdateTarget::TickNode(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory, float DeltaSeconds)
{
	Super::TickNode(OwnerComp, NodeMemory, DeltaSeconds);

	AAIController* AIC = OwnerComp.GetAIOwner();
	if (!IsValid(AIC))
	{
		return;
	}

	AEnemyCharacterBase* Enemy = Cast<AEnemyCharacterBase>(AIC->GetPawn());
	if (!IsValid(Enemy))
	{
		return;
	}

	UBlackboardComponent* BB = OwnerComp.GetBlackboardComponent();
	if (!BB)
	{
		return;
	}

	// BT 版 EnemyBase：用公开接口 FindBestTarget（内部会选最近存活玩家）
	AActor* TargetActor = Enemy->FindBestTarget();

	BB->SetValueAsObject(GetSelectedBlackboardKey(), TargetActor);
}
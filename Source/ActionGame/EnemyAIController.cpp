#include "EnemyAIController.h"

#include "BehaviorTree/BehaviorTree.h"
#include "BehaviorTree/BehaviorTreeComponent.h"
#include "BehaviorTree/BlackboardComponent.h"

AEnemyAIController::AEnemyAIController()
{
	BlackboardComp = CreateDefaultSubobject<UBlackboardComponent>(TEXT("BlackboardComp"));
	BehaviorComp = CreateDefaultSubobject<UBehaviorTreeComponent>(TEXT("BehaviorComp"));
}

void AEnemyAIController::OnPossess(APawn* InPawn)
{
	Super::OnPossess(InPawn);

	if (!DefaultBehaviorTree)
	{
		UE_LOG(LogTemp, Warning, TEXT("EnemyAIController: DefaultBehaviorTree is null (set BT_Enemy in blueprint/defaults)."));
		return;
	}

	UE_LOG(LogTemp, Warning, TEXT("EnemyAIController OnPossess: %s  BT=%s"),
		*GetNameSafe(InPawn),
		*GetNameSafe(DefaultBehaviorTree));

	UBlackboardData* BBAsset = DefaultBehaviorTree->BlackboardAsset;
	if (!BBAsset)
	{
		UE_LOG(LogTemp, Warning, TEXT("EnemyAIController: BehaviorTree has no BlackboardAsset."));
		return;
	}

	// UseBlackboard 需要 UBlackboardComponent*&
	UBlackboardComponent* BBPtr = BlackboardComp;
	if (!UseBlackboard(BBAsset, BBPtr))
	{
		UE_LOG(LogTemp, Warning, TEXT("EnemyAIController: UseBlackboard failed."));
		return;
	}
	BlackboardComp = BBPtr;

	// 运行 BT
	if (!RunBehaviorTree(DefaultBehaviorTree))
	{
		UE_LOG(LogTemp, Warning, TEXT("EnemyAIController: RunBehaviorTree failed."));
		return;
	}

	UE_LOG(LogTemp, Log, TEXT("EnemyAIController: Running BT %s"), *DefaultBehaviorTree->GetName());
}

void AEnemyAIController::OnUnPossess()
{
	if (BehaviorComp)
	{
		BehaviorComp->StopTree(EBTStopMode::Safe);
	}

	Super::OnUnPossess();
}
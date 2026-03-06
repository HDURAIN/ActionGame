#pragma once

#include "CoreMinimal.h"
#include "BehaviorTree/Tasks/BTTask_BlackboardBase.h"
#include "BTTask_MoveToTargetFromConfig.generated.h"

UCLASS()
class ACTIONGAME_API UBTTask_MoveToTargetFromConfig : public UBTTask_BlackboardBase
{
	GENERATED_BODY()

public:
	UBTTask_MoveToTargetFromConfig();

protected:
	virtual EBTNodeResult::Type ExecuteTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory) override;
	virtual void TickTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory, float DeltaSeconds) override;
	virtual EBTNodeResult::Type AbortTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory) override;

private:
	/** 用于判断“离开范围后继续追”的滞回，避免抖动（可调） */
	UPROPERTY(EditAnywhere, Category = "AI")
	float ExtraLeaveRadius = 50.f;
};
#pragma once

#include "CoreMinimal.h"
#include "AIController.h"
#include "EnemyAIController.generated.h"

class UBehaviorTree;
class UBlackboardComponent;
class UBehaviorTreeComponent;

UCLASS()
class ACTIONGAME_API AEnemyAIController : public AAIController
{
	GENERATED_BODY()

public:
	AEnemyAIController();

protected:
	virtual void OnPossess(APawn* InPawn) override;
	virtual void OnUnPossess() override;

private:
	/** 默认行为树（在蓝图/默认值里指定 BT_Enemy） */
	UPROPERTY(EditDefaultsOnly, Category = "AI")
	TObjectPtr<UBehaviorTree> DefaultBehaviorTree = nullptr;

	/** 运行时黑板（注意：UseBlackboard 需要 UBlackboardComponent*&，这里用原生指针） */
	UPROPERTY(Transient)
	UBlackboardComponent* BlackboardComp = nullptr;

	/** 运行时行为树组件 */
	UPROPERTY(Transient)
	UBehaviorTreeComponent* BehaviorComp = nullptr;
};
// BTService_UpdateTarget.cpp

#include "BTService/BTService_UpdateTarget.h"

#include "AbilitySystemBlueprintLibrary.h"
#include "AbilitySystemComponent.h"
#include "AIController.h"
#include "BehaviorTree/BlackboardComponent.h"
#include "Characters/EnemyCharacterBase.h"
#include "GameFramework/CharacterMovementComponent.h"

UBTService_UpdateTarget::UBTService_UpdateTarget()
{
	NodeName = TEXT("Update Target Actor");

	// Service tick interval
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

	UCharacterMovementComponent* MoveComp = Enemy->GetCharacterMovement();
	if (!MoveComp)
	{
		return;
	}

	const FGameplayTag DeadTag = FGameplayTag::RequestGameplayTag(TEXT("State.Dead"));
	const UAbilitySystemComponent* EnemyASC = UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent(Enemy);
	const bool bIsAlive = !EnemyASC || !EnemyASC->HasMatchingGameplayTag(DeadTag);
	BB->SetValueAsBool(TEXT("IsAlive"), bIsAlive);

	if (!bIsAlive)
	{
		BB->SetValueAsObject(GetSelectedBlackboardKey(), nullptr);
		BB->SetValueAsBool(TEXT("CanAttack"), false);
		BB->SetValueAsBool(TEXT("InAttackRange"), false);
		BB->SetValueAsFloat(TEXT("AttackCooldown"), FMath::Max(0.05f, Enemy->GetAttackCooldown()));

		AIC->ClearFocus(EAIFocusPriority::Gameplay);
		MoveComp->bUseControllerDesiredRotation = false;
		MoveComp->bOrientRotationToMovement = true;
		return;
	}

	// BT EnemyBase: use public API FindBestTarget (nearest alive player)
	AActor* TargetActor = Enemy->FindBestTarget();

	BB->SetValueAsObject(GetSelectedBlackboardKey(), TargetActor);
	BB->SetValueAsBool(TEXT("CanAttack"), Enemy->CanAttack());

	bool bInRange = false;
	if (IsValid(TargetActor))
	{
		const float DistSq = FVector::DistSquared(Enemy->GetActorLocation(), TargetActor->GetActorLocation());
		const float R = FMath::Max(0.f, Enemy->GetAttackRange());
		bInRange = DistSq <= FMath::Square(R);
	}
	BB->SetValueAsBool(TEXT("InAttackRange"), bInRange);

	BB->SetValueAsFloat(TEXT("AttackCooldown"), FMath::Max(0.05f, Enemy->GetAttackCooldown()));

	if (IsValid(TargetActor) && bInRange)
	{
		// Attack mode: face target via controller focus.
		MoveComp->bOrientRotationToMovement = false;
		MoveComp->bUseControllerDesiredRotation = true;
		AIC->SetFocus(TargetActor, EAIFocusPriority::Gameplay);
	}
	else
	{
		// Chase/non-attack mode: orient to movement direction.
		AIC->ClearFocus(EAIFocusPriority::Gameplay);
		MoveComp->bUseControllerDesiredRotation = false;
		MoveComp->bOrientRotationToMovement = true;
	}
}

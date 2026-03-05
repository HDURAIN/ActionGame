#include "Characters/EnemyFlyingSuiciderCharacter.h"

#include "AbilitySystemBlueprintLibrary.h"
#include "AbilitySystemComponent.h"
#include "ActionGameCharacter.h"
#include "Components/SphereComponent.h"
#include "Engine/OverlapResult.h"
#include "Engine/World.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameplayEffect.h"

AEnemyFlyingSuiciderCharacter::AEnemyFlyingSuiciderCharacter()
{
	PrimaryActorTick.bCanEverTick = false;
	bUseControllerRotationYaw = false;

	TriggerSphere = CreateDefaultSubobject<USphereComponent>(TEXT("TriggerSphere"));
	TriggerSphere->SetupAttachment(RootComponent);

	TriggerSphere->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
	TriggerSphere->SetCollisionObjectType(ECC_WorldDynamic);
	TriggerSphere->SetCollisionResponseToAllChannels(ECR_Ignore);
	TriggerSphere->SetCollisionResponseToChannel(ECC_Pawn, ECR_Overlap);
	TriggerSphere->SetGenerateOverlapEvents(true);

	TriggerSphere->OnComponentBeginOverlap.AddDynamic(
		this,
		&AEnemyFlyingSuiciderCharacter::OnTriggerBeginOverlap
	);
}

void AEnemyFlyingSuiciderCharacter::BeginPlay()
{
	Super::BeginPlay();

	if (UCharacterMovementComponent* MoveComp = GetCharacterMovement())
	{
		MoveComp->SetMovementMode(MOVE_Flying);
		MoveComp->GravityScale = 0.f;

		MoveComp->MaxFlySpeed = FlySpeed;
		MoveComp->BrakingDecelerationFlying = BrakingDecel;

		MoveComp->bOrientRotationToMovement = bFaceMoveDirection;
		MoveComp->RotationRate = FRotator(0.f, TurnRateYaw, 0.f);
	}

	if (TriggerSphere)
	{
		TriggerSphere->SetSphereRadius(TriggerRadius, true);
	}
}

void AEnemyFlyingSuiciderCharacter::OnTriggerBeginOverlap(
	UPrimitiveComponent* OverlappedComp,
	AActor* OtherActor,
	UPrimitiveComponent* OtherComp,
	int32 OtherBodyIndex,
	bool bFromSweep,
	const FHitResult& SweepResult)
{
	if (bExploded)
	{
		return;
	}

	// 死亡不能爆
	if (AbilitySystemComponent && DeadTag.IsValid() &&
		AbilitySystemComponent->HasMatchingGameplayTag(DeadTag))
	{
		return;
	}

	if (!IsValid(OtherActor) || OtherActor == this)
	{
		return;
	}

	// 这里只炸玩家角色
	if (!OtherActor->IsA(AActionGameCharacter::StaticClass()))
	{
		return;
	}

	// 服务器权威
	if (!HasAuthority())
	{
		return;
	}

	ExplodeAndApply_Server();
}

void AEnemyFlyingSuiciderCharacter::ExplodeAndApply_Server()
{
	if (bExploded)
	{
		return;
	}

	bExploded = true;

	const FVector Origin = GetActorLocation();

	if (ExplosionEffect)
	{
		UWorld* World = GetWorld();
		if (World)
		{
			FCollisionObjectQueryParams ObjParams;
			ObjParams.AddObjectTypesToQuery(ECC_Pawn);

			const FCollisionShape Sphere = FCollisionShape::MakeSphere(ExplosionRadius);

			TArray<FOverlapResult> Hits;
			World->OverlapMultiByObjectType(
				Hits,
				Origin,
				FQuat::Identity,
				ObjParams,
				Sphere
			);

			UAbilitySystemComponent* SourceASC = GetAbilitySystemComponent();

			for (const FOverlapResult& Result : Hits)
			{
				ACharacter* Character = Cast<ACharacter>(Result.GetActor());
				if (!IsValid(Character) || Character == this)
				{
					continue;
				}

				if (AffectedActors.Contains(Character))
				{
					continue;
				}
				AffectedActors.Add(Character);

				UAbilitySystemComponent* TargetASC =
					UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent(Character);
				if (!TargetASC)
				{
					continue;
				}

				// 不对已死亡目标施加效果
				if (DeadTag.IsValid() && TargetASC->HasMatchingGameplayTag(DeadTag))
				{
					continue;
				}

				if (SourceASC)
				{
					FGameplayEffectContextHandle Context = SourceASC->MakeEffectContext();
					Context.AddInstigator(this, this);
					Context.AddSourceObject(this);
					Context.AddOrigin(Origin);

					FGameplayEffectSpecHandle Spec =
						SourceASC->MakeOutgoingSpec(ExplosionEffect, 1.f, Context);

					if (Spec.IsValid())
					{
						SourceASC->ApplyGameplayEffectSpecToTarget(*Spec.Data.Get(), TargetASC);
					}
				}
			}
		}
	}

	Destroy();
}
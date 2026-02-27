#include "Characters/EnemyFlyingSuiciderCharacter.h"

#include "Components/SphereComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "AbilitySystemComponent.h"
#include "AbilitySystemBlueprintLibrary.h"
#include "GameplayEffect.h"
#include "Engine/World.h"
#include "Engine/OverlapResult.h"

AEnemyFlyingSuiciderCharacter::AEnemyFlyingSuiciderCharacter()
{
	PrimaryActorTick.bCanEverTick = true;
	bUseControllerRotationYaw = false;

	TriggerSphere = CreateDefaultSubobject<USphereComponent>(TEXT("TriggerSphere"));
	TriggerSphere->SetupAttachment(RootComponent);

	TriggerSphere->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
	TriggerSphere->SetCollisionObjectType(ECC_WorldDynamic);
	TriggerSphere->SetCollisionResponseToAllChannels(ECR_Ignore);
	TriggerSphere->SetCollisionResponseToChannel(ECC_Pawn, ECR_Overlap);
	TriggerSphere->SetGenerateOverlapEvents(true);

	TriggerSphere->OnComponentBeginOverlap.AddDynamic(this, &AEnemyFlyingSuiciderCharacter::OnTriggerBeginOverlap);
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

void AEnemyFlyingSuiciderCharacter::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	if (bExploded) return;

	ACharacter* Target = GetTargetCharacter();
	if (!IsValid(Target)) return;

	const FVector Dir = (Target->GetActorLocation() - GetActorLocation()).GetSafeNormal();
	if (!Dir.IsNearlyZero())
	{
		AddMovementInput(Dir, 1.f);
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
	if (bExploded) return;
	if (!OtherActor || OtherActor == this) return;

	// 碰到任意 Character 就爆（你也可以改成只爆玩家阵营）
	if (!OtherActor->IsA(ACharacter::StaticClass())) return;

	// 服务器权威
	if (!HasAuthority()) return;

	ExplodeAndApply_Server();
}

void AEnemyFlyingSuiciderCharacter::ExplodeAndApply_Server()
{
	if (bExploded) return;
	bExploded = true;

	const FVector Origin = GetActorLocation();

	// 2) AoE Apply GE
	if (ExplosionEffect)
	{
		UWorld* World = GetWorld();
		if (World)
		{
			FCollisionObjectQueryParams ObjParams;
			ObjParams.AddObjectTypesToQuery(ECC_Pawn);

			FCollisionShape Sphere = FCollisionShape::MakeSphere(ExplosionRadius);

			TArray<FOverlapResult> Hits;
			World->OverlapMultiByObjectType(
				Hits,
				Origin,
				FQuat::Identity,
				ObjParams,
				Sphere
			);

			UAbilitySystemComponent* SourceASC = GetAbilitySystemComponent();

			for (const FOverlapResult& R : Hits)
			{
				ACharacter* Char = Cast<ACharacter>(R.GetActor());
				if (!IsValid(Char) || Char == this) continue;


				if (AffectedActors.Contains(Char)) continue;
				AffectedActors.Add(Char);
				// 过滤掉已死亡的角色（避免给尸体加效果）
				// 这里复用你基类的 DeadTag 逻辑：从目标ASC查 DeadTag
				UAbilitySystemComponent* TargetASC = UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent(Char);
				if (!TargetASC) continue;
				if (DeadTag.IsValid() && TargetASC->HasMatchingGameplayTag(DeadTag)) continue;

				if (SourceASC)
				{
					FGameplayEffectContextHandle Ctx = SourceASC->MakeEffectContext();
					Ctx.AddInstigator(this, this);
					Ctx.AddSourceObject(this);
					Ctx.AddOrigin(Origin);

					FGameplayEffectSpecHandle Spec = SourceASC->MakeOutgoingSpec(ExplosionEffect, 1.f, Ctx);
					if (Spec.IsValid())
					{
						SourceASC->ApplyGameplayEffectSpecToTarget(*Spec.Data.Get(), TargetASC);
					}
				}
			}
		}
	}

	// 3) Destroy
	Destroy();
}
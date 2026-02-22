// Fill out your copyright notice in the Description page of Project Settings.


#include "AbilitySystem/Abilities/GA_PrimaryAttack.h"
#include "ActionGame/ActionGameCharacter.h"

FORCEINLINE ECollisionChannel GetChannel(FName Name)
{
	const UCollisionProfile* Profile = UCollisionProfile::Get();

	for (int32 i = 0; i < ECC_MAX; i++)
	{
		if (Profile->ReturnChannelNameFromContainerIndex(i) == Name)
		{
			return (ECollisionChannel)i;
		}
	}

	UE_LOG(LogTemp, Error, TEXT("Collision Channel not found: %s"), *Name.ToString());
	return ECC_Visibility;
}
namespace CollisionChannels
{
	FORCEINLINE ECollisionChannel Weapon()
	{
		static ECollisionChannel Channel = GetChannel("WeaponTrace");
		return Channel;
	}
}

void UGA_PrimaryAttack::OnAbilityActivated()
{
	FHitResult CameraHit;
	FVector AimPoint;

	if (!DoCameraTrace(CameraHit, AimPoint))
		return;

	DoWeaponTrace(AimPoint);
}

void UGA_PrimaryAttack::OnAbilityEnded(bool bWasCancelled)
{
	
}

bool UGA_PrimaryAttack::DoCameraTrace(FHitResult& OutHit, FVector& OutAimPoint)
{
	ACharacter* Avatar = Cast<ACharacter>(GetAvatarActorFromActorInfo());
	if (!Avatar) return false;

	APlayerController* PC = Cast<APlayerController>(Avatar->GetController());
	if (!PC) return false;

	FVector CamLoc;
	FRotator CamRot;
	PC->GetPlayerViewPoint(CamLoc, CamRot);

	FVector End = CamLoc + CamRot.Vector() * 10000.f;

	FCollisionQueryParams Params;
	Params.AddIgnoredActor(Avatar);

	ECollisionChannel WeaponChannel = CollisionChannels::Weapon();

	bool bHit = GetWorld()->LineTraceSingleByChannel(
		OutHit,
		CamLoc,
		End,
		WeaponChannel,
		Params
	);

	OutAimPoint = bHit ? OutHit.ImpactPoint : End;

	DrawDebugLine(
		GetWorld(),
		CamLoc,
		OutAimPoint,
		FColor::Green,
		false,
		1.0f,
		0,
		1.0f
	);

	return true;
}

void UGA_PrimaryAttack::DoWeaponTrace(const FVector& AimPoint)
{
	ACharacter* Avatar = Cast<ACharacter>(GetAvatarActorFromActorInfo());
	if (!Avatar) return;

	USkeletalMeshComponent* Mesh = Avatar->GetMesh();
	FVector MuzzleLoc = Mesh->GetSocketLocation("Muzzle");

	FVector Dir = (AimPoint - MuzzleLoc).GetSafeNormal();
	FVector End = MuzzleLoc + Dir * 10000.f;

	FHitResult Hit;

	FCollisionQueryParams Params;
	Params.AddIgnoredActor(Avatar);

	ECollisionChannel WeaponChannel = CollisionChannels::Weapon();

	bool bHit = GetWorld()->LineTraceSingleByChannel(
		Hit,
		MuzzleLoc,
		End,
		WeaponChannel,
		Params
	);

	DrawDebugLine(
		GetWorld(),
		MuzzleLoc,
		bHit ? Hit.ImpactPoint : End,
		FColor::Red,
		false,
		1.0f,
		0,
		1.5f
	);
}

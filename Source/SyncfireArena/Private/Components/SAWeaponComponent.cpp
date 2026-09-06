// Copyright Epic Games, Inc. All Rights Reserved.

#include "Components/SAWeaponComponent.h"

#include "Animation/AnimInstance.h"
#include "Character/SACharacter.h"
#include "Components/SkeletalMeshComponent.h"

USAWeaponComponent::USAWeaponComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
}

void USAWeaponComponent::BeginPlay()
{
	Super::BeginPlay();

	OwnerCharacter = Cast<ASACharacter>(GetOwner());
}

void USAWeaponComponent::PlayLocalFire()
{
	PlayFireMontage();
}

void USAWeaponComponent::PlayRemoteFire()
{
	PlayFireMontage();
}


bool USAWeaponComponent::GetMuzzleLocation(FVector& OutLocation) const
{
	if (!OwnerCharacter)
	{
		return false;
	}

	USkeletalMeshComponent* Mesh = OwnerCharacter->GetMesh();
	if (!Mesh || MuzzleSocketName.IsNone() || !Mesh->DoesSocketExist(MuzzleSocketName))
	{
		return false;
	}

	OutLocation = Mesh->GetSocketLocation(MuzzleSocketName);
	return true;
}

void USAWeaponComponent::PlayFireMontage() const
{
	if (!OwnerCharacter || !FireMontage)
	{
		return;
	}

	USkeletalMeshComponent* Mesh = OwnerCharacter->GetMesh();
	if (!Mesh)
	{
		return;
	}

	UAnimInstance* AnimInstance = Mesh->GetAnimInstance();
	if (!AnimInstance)
	{
		return;
	}

	if (bSkipIfFireMontageIsPlaying && AnimInstance->Montage_IsPlaying(FireMontage))
	{
		return;
	}

	AnimInstance->Montage_Play(FireMontage, FireMontagePlayRate);
}

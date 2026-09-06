// Copyright Epic Games, Inc. All Rights Reserved.

#include "Weapons/SAWeaponActor.h"

#include "Animation/AnimInstance.h"
#include "Character/SACharacter.h"
#include "Components/SkeletalMeshComponent.h"
#include "GameFramework/Pawn.h"
#include "Kismet/GameplayStatics.h"
#include "Effects/BulletTrajectoryEffect.h"
#include "Net/UnrealNetwork.h"
#include "NiagaraFunctionLibrary.h"

ASAWeaponActor::ASAWeaponActor()
{
	PrimaryActorTick.bCanEverTick = false;

	// 武器是独立 Actor，CurrentWeapon 指针复制到客户端后，客户端才能看到同一把被装备的武器。
	bReplicates = true;
	bNetUseOwnerRelevancy = true;
	SetReplicateMovement(false);

	BulletTrajectoryEffectClass = ABulletTrajectoryEffect::StaticClass();

	WeaponMesh = CreateDefaultSubobject<USkeletalMeshComponent>(TEXT("WeaponMesh"));
	SetRootComponent(WeaponMesh);

	WeaponMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	WeaponMesh->SetGenerateOverlapEvents(false);
}

void ASAWeaponActor::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	// 弹量和换弹状态都由服务器决定，再复制给客户端用于表现和后续 UI。
	DOREPLIFETIME(ASAWeaponActor, MagazineAmmo);
	DOREPLIFETIME(ASAWeaponActor, ReserveAmmo);
	DOREPLIFETIME(ASAWeaponActor, bIsReloading);
}

bool ASAWeaponActor::GetMuzzleLocation(FVector& OutLocation) const
{
	if (!WeaponMesh || MuzzleSocketName.IsNone() || !WeaponMesh->DoesSocketExist(MuzzleSocketName))
	{
		return false;
	}

	OutLocation = WeaponMesh->GetSocketLocation(MuzzleSocketName);
	return true;
}

USkeletalMesh* ASAWeaponActor::GetWeaponSkeletalMeshAsset() const
{
	return WeaponMesh ? WeaponMesh->GetSkeletalMeshAsset() : nullptr;
}

bool ASAWeaponActor::CanFire() const
{
	return !bIsReloading && MagazineAmmo > 0;
}

bool ASAWeaponActor::CanReload() const
{
	return !bIsReloading && MagazineAmmo < MagazineSize && ReserveAmmo > 0;
}

void ASAWeaponActor::InitializeAmmo(int32 InMagazineAmmo, int32 InReserveAmmo)
{
	MagazineAmmo = FMath::Clamp(InMagazineAmmo, 0, MagazineSize);
	ReserveAmmo = FMath::Max(0, InReserveAmmo);
	bIsReloading = false;
}

void ASAWeaponActor::AddReserveAmmo(int32 InReserveAmmo)
{
	ReserveAmmo = FMath::Max(0, ReserveAmmo + InReserveAmmo);
}

bool ASAWeaponActor::ConsumeAmmo(int32 Amount)
{
	if (Amount <= 0 || MagazineAmmo < Amount)
	{
		return false;
	}

	MagazineAmmo -= Amount;
	return true;
}

bool ASAWeaponActor::StartReload()
{
	if (!CanReload())
	{
		return false;
	}

	bIsReloading = true;
	return true;
}

void ASAWeaponActor::CancelReload()
{
	bIsReloading = false;
}

void ASAWeaponActor::FinishReload()
{
	if (!bIsReloading)
	{
		return;
	}

	const int32 AmmoNeeded = FMath::Max(0, MagazineSize - MagazineAmmo);
	const int32 AmmoToLoad = FMath::Min(AmmoNeeded, ReserveAmmo);

	MagazineAmmo += AmmoToLoad;
	ReserveAmmo -= AmmoToLoad;
	bIsReloading = false;
}

void ASAWeaponActor::PlayLocalFire(ASACharacter* Character) const
{
	PlayCharacterMontage(Character, FireMontage, FireMontagePlayRate);
	PlayMuzzleEffects();
}

void ASAWeaponActor::PlayRemoteFire(ASACharacter* Character) const
{
	PlayCharacterMontage(Character, FireMontage, FireMontagePlayRate);
	PlayMuzzleEffects();
}

void ASAWeaponActor::PlayLocalReload(ASACharacter* Character) const
{
	PlayCharacterMontage(Character, ReloadMontage, ReloadMontagePlayRate);
}

void ASAWeaponActor::PlayRemoteReload(ASACharacter* Character) const
{
	PlayCharacterMontage(Character, ReloadMontage, ReloadMontagePlayRate);
}

void ASAWeaponActor::StopReloadMontage(ASACharacter* Character) const
{
	if (!Character || !ReloadMontage)
	{
		return;
	}

	if (USkeletalMeshComponent* CharacterMesh = Character->GetMesh())
	{
		if (UAnimInstance* AnimInstance = CharacterMesh->GetAnimInstance())
		{
			AnimInstance->Montage_Stop(0.1f, ReloadMontage);
		}
	}
}

void ASAWeaponActor::PlayEquip(ASACharacter* Character) const
{
	PlayCharacterMontage(Character, EquipMontage, EquipMontagePlayRate);
}

void ASAWeaponActor::PlayMuzzleEffects() const
{
	if (!WeaponMesh)
	{
		return;
	}

	if (MuzzleFlashEffect && !MuzzleSocketName.IsNone())
	{
		UNiagaraFunctionLibrary::SpawnSystemAttached(
			MuzzleFlashEffect,
			WeaponMesh,
			MuzzleSocketName,
			FVector::ZeroVector,
			FRotator::ZeroRotator,
			EAttachLocation::SnapToTarget,
			true);
	}

	FVector MuzzleLocation;
	if (FireSound && GetMuzzleLocation(MuzzleLocation))
	{
		UGameplayStatics::PlaySoundAtLocation(this, FireSound, MuzzleLocation);
	}
}

void ASAWeaponActor::PlayImpactEffects(const FVector& ImpactLocation, const FVector& ImpactNormal) const
{
	const FRotator ImpactRotation = ImpactNormal.IsNearlyZero() ? FRotator::ZeroRotator : ImpactNormal.Rotation();

	if (ImpactEffect)
	{
		UNiagaraFunctionLibrary::SpawnSystemAtLocation(this, ImpactEffect, ImpactLocation, ImpactRotation);
	}

	if (ImpactSound)
	{
		UGameplayStatics::PlaySoundAtLocation(this, ImpactSound, ImpactLocation);
	}
	
}

void ASAWeaponActor::SpawnBulletTrajectoryEffect(const FVector& StartLocation, const FVector& EndLocation, bool bHit, bool bHitEnemy, const FVector& HitLocation, const FVector& HitNormal) const
{
	if (!GetWorld() || GetNetMode() == NM_DedicatedServer || !BulletTrajectoryEffectClass)
	{
		return;
	}

	FActorSpawnParameters SpawnParams;
	SpawnParams.Owner = GetOwner();
	SpawnParams.Instigator = GetOwner() ? Cast<APawn>(GetOwner()) : nullptr;
	SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;

	const FRotator SpawnRotation = (EndLocation - StartLocation).IsNearlyZero() ? FRotator::ZeroRotator : (EndLocation - StartLocation).Rotation();
	AActor* TrajectoryActor = GetWorld()->SpawnActor<AActor>(BulletTrajectoryEffectClass, StartLocation, SpawnRotation, SpawnParams);
	if (!TrajectoryActor)
	{
		return;
	}

	if (ABulletTrajectoryEffect* TrajectoryEffect = Cast<ABulletTrajectoryEffect>(TrajectoryActor))
	{
		/* 我们自己的轨迹类会用服务器确认的起点/终点计算飞行时间，
		   并把命中特效延迟到视觉子弹到达命中点时播放。 */
		UNiagaraSystem* SelectedImpactEffect = bHitEnemy ? EnemyImpactEffect.Get() : ImpactEffect.Get();
		TrajectoryEffect->InitializeTrajectory(StartLocation, EndLocation, bHit, HitLocation, HitNormal, BulletTrajectoryNiagara.Get(), SelectedImpactEffect, ImpactSound.Get(), BulletTrajectorySpeed);
		return;
	}

	TrajectoryActor->SetLifeSpan(1.0f);
}
void ASAWeaponActor::PlayCharacterMontage(ASACharacter* Character, UAnimMontage* Montage, float PlayRate) const
{
	if (!Character || !Montage)
	{
		UE_LOG(LogTemp, Warning, TEXT("武器 Montage 未播放：Character 或 Montage 为空。Weapon=%s"), *GetName());
		return;
	}

	USkeletalMeshComponent* CharacterMesh = Character->GetMesh();
	if (!CharacterMesh)
	{
		UE_LOG(LogTemp, Warning, TEXT("武器 Montage 未播放：角色 Mesh 为空。Character=%s Weapon=%s"), *GetNameSafe(Character), *GetName());
		return;
	}

	UAnimInstance* AnimInstance = CharacterMesh->GetAnimInstance();
	if (!AnimInstance)
	{
		UE_LOG(LogTemp, Warning, TEXT("武器 Montage 未播放：角色 Mesh 没有 AnimInstance。Character=%s Weapon=%s"), *GetNameSafe(Character), *GetName());
		return;
	}

	if (bSkipIfMontageIsPlaying && AnimInstance->Montage_IsPlaying(Montage))
	{
		return;
	}

	const float PlayedLength = AnimInstance->Montage_Play(Montage, PlayRate);
	if (PlayedLength <= 0.0f)
	{
		UE_LOG(LogTemp, Warning, TEXT("武器 Montage 播放失败：请检查 Montage 骨架是否匹配角色 Mesh，并确认 AnimBP 已接入对应 Slot。Character=%s Weapon=%s Montage=%s"),
			*GetNameSafe(Character),
			*GetName(),
			*GetNameSafe(Montage));
	}
}

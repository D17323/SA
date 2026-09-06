// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Net/UnrealNetwork.h"
#include "SAWeaponActor.generated.h"

class ASACharacter;
class UAnimMontage;
class UNiagaraSystem;
class USkeletalMesh;
class USoundBase;
class USkeletalMeshComponent;

UENUM(BlueprintType)
enum class ESAWeaponType : uint8
{
	Pistol,
	Rifle
};

UCLASS()
class SYNCFIREARENA_API ASAWeaponActor : public AActor
{
	GENERATED_BODY()

public:
	ASAWeaponActor();
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	bool GetMuzzleLocation(FVector& OutLocation) const;
	USkeletalMesh* GetWeaponSkeletalMeshAsset() const;

	float GetFireRange() const { return FireRange; }
	float GetFireCooldown() const { return FireCooldown; }
	float GetDamage() const { return Damage; }
	ESAWeaponType GetWeaponType() const { return WeaponType; }
	bool IsAutomaticFire() const { return bAutomaticFire; }
	FName GetAttachSocketName() const { return AttachSocketName; }
	float GetEquipWeaponVisibleDelay() const { return EquipWeaponVisibleDelay; }
	int32 GetMagazineSize() const { return MagazineSize; }
	int32 GetMagazineAmmo() const { return MagazineAmmo; }
	int32 GetReserveAmmo() const { return ReserveAmmo; }
	bool CanFire() const;
	bool CanReload() const;
	bool IsReloading() const { return bIsReloading; }
	float GetReloadDuration() const { return ReloadDuration; }

	void InitializeAmmo(int32 InMagazineAmmo, int32 InReserveAmmo);
	void AddReserveAmmo(int32 InReserveAmmo);
	bool ConsumeAmmo(int32 Amount = 1);
	bool StartReload();
	void CancelReload();
	void FinishReload();

	void PlayLocalFire(ASACharacter* Character) const;
	void PlayRemoteFire(ASACharacter* Character) const;
	void PlayLocalReload(ASACharacter* Character) const;
	void PlayRemoteReload(ASACharacter* Character) const;
	void StopReloadMontage(ASACharacter* Character) const;
	void PlayEquip(ASACharacter* Character) const;
	void PlayMuzzleEffects() const;
	void PlayImpactEffects(const FVector& ImpactLocation, const FVector& ImpactNormal) const;
	void SpawnBulletTrajectoryEffect(const FVector& StartLocation, const FVector& EndLocation, bool bHit, bool bHitEnemy, const FVector& HitLocation, const FVector& HitNormal) const;

protected:
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Syncfire|Weapon")
	TObjectPtr<USkeletalMeshComponent> WeaponMesh;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Syncfire|Weapon")
	ESAWeaponType WeaponType = ESAWeaponType::Pistol;

	// 自动射击武器会在按住开火键时持续出枪，手枪这类半自动武器保持关闭。
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Syncfire|Weapon")
	bool bAutomaticFire = false;

	// 不同武器可能使用不同的挂点，比如手枪和步枪通常不会共用同一个 Socket。
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Syncfire|Weapon")
	FName AttachSocketName = TEXT("WeaponSocket");

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Syncfire|Weapon")
	FName MuzzleSocketName = TEXT("Muzzle");

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Syncfire|Weapon")
	float FireRange = 3000.0f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Syncfire|Weapon")
	float FireCooldown = 0.18f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Syncfire|Weapon")
	float Damage = 25.0f;

	// 弹匣容量是武器设计上限，当前弹量和备弹量会在服务器上同步给所有客户端。
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Syncfire|Weapon", meta = (ClampMin = "0"))
	int32 MagazineSize = 12;

	UPROPERTY(Replicated, EditDefaultsOnly, BlueprintReadOnly, Category = "Syncfire|Weapon", meta = (ClampMin = "0"))
	int32 MagazineAmmo = 12;

	UPROPERTY(Replicated, EditDefaultsOnly, BlueprintReadOnly, Category = "Syncfire|Weapon", meta = (ClampMin = "0"))
	int32 ReserveAmmo = 36;

	UPROPERTY(Replicated, EditDefaultsOnly, BlueprintReadOnly, Category = "Syncfire|Weapon")
	bool bIsReloading = false;

	// 换弹时长最好和蒙太奇大致一致，后续如果严格对齐动画，可以改成用 AnimNotify 驱动。
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Syncfire|Weapon", meta = (ClampMin = "0.0"))
	float ReloadDuration = 1.35f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Syncfire|Animation")
	TObjectPtr<UAnimMontage> FireMontage;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Syncfire|Animation")
	TObjectPtr<UAnimMontage> EquipMontage;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Syncfire|Animation")
	TObjectPtr<UAnimMontage> ReloadMontage;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Syncfire|Effects")
	TObjectPtr<UNiagaraSystem> MuzzleFlashEffect;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Syncfire|Effects")
	TObjectPtr<UNiagaraSystem> ImpactEffect;

	/* 敌人命中使用血液类特效，非敌人或环境命中继续使用普通火花。 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Syncfire|Effects")
	TObjectPtr<UNiagaraSystem> EnemyImpactEffect;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Syncfire|Effects")
	TObjectPtr<USoundBase> FireSound;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Syncfire|Effects")
	TObjectPtr<USoundBase> ImpactSound;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Syncfire|Effects")
	TSubclassOf<AActor> BulletTrajectoryEffectClass;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Syncfire|Effects")
	TObjectPtr<UNiagaraSystem> BulletTrajectoryNiagara;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Syncfire|Effects")
	float BulletTrajectorySpeed = 50000.0f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Syncfire|Animation")
	float FireMontagePlayRate = 1.0f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Syncfire|Animation")
	float EquipMontagePlayRate = 1.0f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Syncfire|Animation")
	float ReloadMontagePlayRate = 1.0f;

	// 装备动画开始后，延迟多久把旧武器隐藏并显示新武器，后续可用 AnimNotify 精确替代。
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Syncfire|Animation")
	float EquipWeaponVisibleDelay = 0.15f;

	// 连续触发同一个 Montage 时跳过重播，避免动画被不断重置到起始帧。
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Syncfire|Animation")
	bool bSkipIfMontageIsPlaying = true;

private:
	void PlayCharacterMontage(ASACharacter* Character, UAnimMontage* Montage, float PlayRate) const;
};

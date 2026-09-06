// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "TimerManager.h"
#include "Weapons/SAWeaponActor.h"
#include "SACharacter.generated.h"

class USAStatComponent;
class ASAWeaponPickupActor;
class UAnimMontage;
class UCameraComponent;
class USACombatComponent;
class USAHealthComponent;
class USpringArmComponent;

UCLASS()
class SYNCFIREARENA_API ASACharacter : public ACharacter
{
	GENERATED_BODY()

public:
	ASACharacter();

	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
	virtual void Tick(float DeltaSeconds) override;
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	void MoveForwardInput(float Value);
	void MoveRightInput(float Value);
	void FireInput();
	void StopFireInput();
	void ReloadWeaponInput();
	void PickupWeaponInput();
	void EquipWeaponByIndexInput(int32 WeaponIndex);

	bool IsDead() const;
	ASAWeaponActor* GetCurrentWeapon() const { return CurrentWeapon; }
	USAStatComponent* GetStatComponent() const { return StatComponent; }
	ASAWeaponActor* FindWeaponByType(ESAWeaponType WeaponType) const;

	bool GrantWeaponFromPickup(TSubclassOf<ASAWeaponActor> WeaponClass, int32 MagazineAmmo, int32 ReserveAmmo, bool bEquipIfPossible);
	bool GrantAmmoPickup(int32 AmmoAmount);
	void NotifyPickupInRange(ASAWeaponPickupActor* PickupActor);
	void ClearPickupInRange(ASAWeaponPickupActor* PickupActor);

	UFUNCTION(BlueprintPure, Category = "Syncfire|Weapon")
	ESAWeaponType GetCurrentWeaponType() const;

protected:
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Syncfire|Camera")
	TObjectPtr<USpringArmComponent> CameraBoom;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Syncfire|Camera")
	TObjectPtr<UCameraComponent> TopDownCamera;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Syncfire|Combat")
	TObjectPtr<USACombatComponent> CombatComponent;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Syncfire|Health")
	TObjectPtr<USAHealthComponent> HealthComponent;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Syncfire|Stats")
	TObjectPtr<USAStatComponent> StatComponent;

	// 玩家死亡表现由服务器确认后的死亡状态驱动；Health 复制仍负责血条数值同步。
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Syncfire|Animation")
	TObjectPtr<UAnimMontage> DeathMontage;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Syncfire|Animation")
	float DeathMontagePlayRate = 1.0f;

	/* 玩家死亡后是否切入布娃娃。死亡蒙太奇先播，随后再交给物理。 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Syncfire|Animation")
	bool bEnableRagdollAfterDeath = true;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Syncfire|Animation", meta = (EditCondition = "bEnableRagdollAfterDeath", ClampMin = "0.0"))
	float RagdollDelay = 0.35f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Syncfire|Animation")
	bool bClearRagdollVelocityOnStart = true;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Syncfire|Animation", meta = (ClampMin = "0.0"))
	float RagdollLinearDamping = 8.0f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Syncfire|Animation", meta = (ClampMin = "0.0"))
	float RagdollAngularDamping = 12.0f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Syncfire|Weapon")
	TArray<TSubclassOf<ASAWeaponActor>> DefaultWeaponClasses;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Syncfire|Weapon")
	TSubclassOf<ASAWeaponPickupActor> WeaponPickupClass;

	// 四个武器槽：0 步枪，1 手枪，2 近战预留，3 投掷物预留。
	UPROPERTY(ReplicatedUsing = OnRep_EquippedWeapons, BlueprintReadOnly, Category = "Syncfire|Weapon")
	TArray<TObjectPtr<ASAWeaponActor>> EquippedWeapons;

	// 当前服务器确认装备的武器。客户端通过 OnRep 接收后更新本地表现。
	UPROPERTY(ReplicatedUsing = OnRep_CurrentWeapon, BlueprintReadOnly, Category = "Syncfire|Weapon")
	TObjectPtr<ASAWeaponActor> CurrentWeapon;

	// 当前真正显示在角色手上的武器。切枪动画延迟显隐时会暂时晚于 CurrentWeapon。
	UPROPERTY(BlueprintReadOnly, Category = "Syncfire|Weapon")
	TObjectPtr<ASAWeaponActor> VisibleWeapon;

	// 瞄准朝向需要同步给其他客户端，用于显示远端玩家朝向。
	UPROPERTY(ReplicatedUsing = OnRep_AimYaw, BlueprintReadOnly, Category = "Syncfire|Aim")
	float AimYaw = 0.0f;

	// 死亡是关键玩法状态，单独复制一份给远端客户端，避免只依赖 HealthComponent 的 RepNotify 驱动表现。
	UPROPERTY(ReplicatedUsing = OnRep_DeathState)
	bool bReplicatedIsDead = false;

	UFUNCTION()
	void OnRep_AimYaw();

	UFUNCTION()
	void OnRep_CurrentWeapon();

	UFUNCTION()
	void OnRep_EquippedWeapons();

	UFUNCTION()
	void OnRep_DeathState();

	UFUNCTION(BlueprintImplementableEvent, Category = "Syncfire|Health")
	void BP_OnHealthChanged(float NewHealth, float HealthDelta);

	UFUNCTION(BlueprintImplementableEvent, Category = "Syncfire|Health")
	void BP_OnDeath();

private:
	float LastSentAimYaw = 0.0f;
	bool bLocalDeathHandled = false;
	bool bRagdollStarted = false;
	FTimerHandle RagdollTimerHandle;

	void SpawnDefaultWeapons();
	void EnsureWeaponSlots();
	int32 GetSlotIndexForWeaponType(ESAWeaponType WeaponType) const;
	void AttachWeapon(ASAWeaponActor* Weapon) const;
	void AttachAllWeapons();
	void RefreshWeaponVisibility();
	void BeginEquipWeaponVisual(ASAWeaponActor* EquippedWeapon);
	void CompleteEquipWeaponVisibility();
	void EquipWeaponByIndex(int32 WeaponIndex);
	void DropWeaponToGround(ASAWeaponActor* WeaponToDrop);
	ASAWeaponPickupActor* FindBestPickupCandidate() const;
	void UpdateAimFromMouse();
	void ApplyAimYaw(float NewAimYaw);
	void ApplyStatModifiers();
	void HandleHealthChanged(float NewHealth, float HealthDelta);
	void HandleDeath();
	void EnterRagdoll();
	void PlayMontage(UAnimMontage* Montage, float PlayRate) const;

	float GetAimPlaneZ() const;
	bool GetMouseAimPoint(FVector& OutAimPoint) const;

	// 客户端到服务器的瞄准 RPC。瞄准角度更新频繁，偶尔丢包会被后续更新覆盖。
	UFUNCTION(Server, Unreliable)
	void ServerSetAimYaw(float NewAimYaw);

	// 客户端只提交“想切到哪个槽位”，真正切换由服务器执行并复制给所有客户端。
	UFUNCTION(Server, Reliable)
	void ServerEquipWeaponByIndex(int32 WeaponIndex);

	// 客户端只提交“想拾取哪个 Actor”，服务器验证距离和配置后再真正发放武器。
	UFUNCTION(Server, Reliable)
	void ServerPickupWeapon(ASAWeaponPickupActor* PickupActor);

	// 切枪动画是一类一次性表现事件，用 Multicast 广播给所有客户端播放。
	UFUNCTION(NetMulticast, Reliable)
	void MulticastWeaponEquipped(ASAWeaponActor* EquippedWeapon);

	UPROPERTY()
	TObjectPtr<ASAWeaponPickupActor> CurrentPickupCandidate;

	UPROPERTY()
	TObjectPtr<ASAWeaponActor> PendingVisibleWeapon;

	FTimerHandle EquipVisibilityTimerHandle;
	bool bHasInitializedWeaponVisibility = false;
};

// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "Engine/NetSerialization.h"
#include "TimerManager.h"
#include "SACombatComponent.generated.h"

class ASACharacter;
class ASAWeaponActor;
class USAStatComponent;

UCLASS(ClassGroup=(Syncfire), meta=(BlueprintSpawnableComponent))
class SYNCFIREARENA_API USACombatComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	USACombatComponent();

	void StartFire(const FVector& AimPoint);
	void StopFire();
	void UpdateFireAimPoint(const FVector& AimPoint);
	void Fire(const FVector& AimPoint);
	void Reload();

	// 切枪或拾取导致手持武器变化时调用；服务器计时未结束就不会给旧武器补弹。
	void CancelReload();

	// 空弹匣时自动走普通换弹流程，最终仍由 ServerReload 决定是否成立。
	void TryAutoReload();

protected:
	virtual void BeginPlay() override;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Syncfire|Combat")
	float FireRange = 3000.0f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Syncfire|Combat")
	float FireCooldown = 0.18f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Syncfire|Combat")
	float TraceStartHeight = 50.0f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Syncfire|Combat")
	float Damage = 25.0f;

	// 仅用于开发调试：开火时是否画出服务器与本地射线。
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Syncfire|Debug")
	bool bDrawFireTrace = false;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Syncfire|Debug")
	float FireTraceDebugLifeTime = 1.0f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Syncfire|Debug")
	float FireTraceDebugThickness = 3.0f;

private:
	UPROPERTY()
	TObjectPtr<ASACharacter> OwnerCharacter;

	// 本地冷却只负责输入手感，真正的射速校验仍然由服务器执行。
	float LastLocalFireTime = -1000.0f;

	// 服务器冷却用于权威校验，避免客户端通过高频 RPC 伪造射速。
	float LastServerFireTime = -1000.0f;

	bool bLocalReloadInProgress = false;
	bool bWantsToFire = false;
	TWeakObjectPtr<ASAWeaponActor> LocalReloadingWeapon;
	TWeakObjectPtr<ASAWeaponActor> ServerReloadingWeapon;
	FVector PendingFireAimPoint = FVector::ZeroVector;

	FTimerHandle LocalReloadTimerHandle;
	FTimerHandle ServerReloadTimerHandle;
	FTimerHandle AutoFireTimerHandle;

	void HandleAutoFire();
	void ScheduleNextAutoFire();
	bool CanAutoFireCurrentWeapon() const;
	bool CanFireByCooldown(float LastFireTime, float CurrentTime) const;
	bool CanServerFire(float CurrentTime) const;
	bool CanLocalFire(float CurrentTime) const;
	ASAWeaponActor* GetCurrentWeapon() const;
	USAStatComponent* GetOwnerStatComponent() const;
	float GetCurrentFireRange() const;
	float GetCurrentFireCooldown() const;
	float GetCurrentDamage() const;
	float GetReloadDurationForWeapon(const ASAWeaponActor* Weapon) const;
	FVector GetTraceStart() const;
	FVector BuildFallbackAimPoint() const;
	void BuildFireTrace(const FVector& AimPoint, FVector& OutTraceStart, FVector& OutTraceEnd) const;
	void PlayFireDebug(const FVector& TraceStart, const FVector& TraceEnd, bool bHit, const FVector& HitLocation, bool bServerConfirmed) const;
	void ClearLocalReloadState();
	void CancelServerReload();
	void FinishServerReload();

	// 客户端只提交开火意图；服务器会重新构建射线并执行最终命中判定。
	UFUNCTION(Server, Reliable)
	void ServerFire(FVector_NetQuantize AimPoint);

	// 换弹同样由服务器决定是否成立，客户端只做请求和本地表现。
	UFUNCTION(Server, Reliable)
	void ServerReload(ASAWeaponActor* RequestedWeapon);

	UFUNCTION(Server, Reliable)
	void ServerCancelReload(ASAWeaponActor* RequestedWeapon);

	// 短生命周期表现使用 Multicast 即可，丢失一次不会影响服务器权威结果。
	UFUNCTION(NetMulticast, Unreliable)
	void MulticastFireConfirmed(ASAWeaponActor* FiredWeapon, FVector_NetQuantize TraceStart, FVector_NetQuantize TraceEnd, bool bHit, bool bHitEnemy, FVector_NetQuantize HitLocation, FVector_NetQuantizeNormal HitNormal);

	// 换弹不是高频事件，可靠广播可以让切枪后的换弹表现更稳定地到达各客户端。
	UFUNCTION(NetMulticast, Reliable)
	void MulticastReloadConfirmed(ASAWeaponActor* ReloadWeapon);

	UFUNCTION(NetMulticast, Reliable)
	void MulticastReloadCanceled(ASAWeaponActor* ReloadWeapon);
};

// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "TimerManager.h"
#include "SAEnemyCharacter.generated.h"

class UAnimMontage;
class USAHealthComponent;

UCLASS()
class SYNCFIREARENA_API ASAEnemyCharacter : public ACharacter
{
	GENERATED_BODY()

public:
	ASAEnemyCharacter();

	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
	virtual void GetActorEyesViewPoint(FVector& OutLocation, FRotator& OutRotation) const override;

	UFUNCTION(BlueprintPure, Category = "Syncfire|Enemy")
	bool IsDead() const;

	void PlayAttackAnimation(AActor* AttackTarget);
	void SetDeathImpulseDirection(const FVector& ImpulseDirection);

	UFUNCTION(BlueprintCallable, Category = "Syncfire|Enemy|Animation")
	void EnterRagdoll();

protected:
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Syncfire|Enemy")
	TObjectPtr<USAHealthComponent> HealthComponent;

	// 攻击蒙太奇由服务器确认攻击后通过 Multicast 播放，避免客户端自己播放导致表现和伤害时机不一致。
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Syncfire|Enemy|Animation")
	TObjectPtr<UAnimMontage> AttackMontage;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Syncfire|Enemy|Animation")
	float AttackMontagePlayRate = 1.0f;

	// 受击动画同样由 Health 变化驱动：服务器扣血和客户端 OnRep_Health 都会在本地播放一次。
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Syncfire|Enemy|Animation")
	TObjectPtr<UAnimMontage> HitReactMontage;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Syncfire|Enemy|Animation")
	float HitReactMontagePlayRate = 1.0f;

	// 死亡由 Health 复制结果驱动：服务器扣到 0，客户端收到 OnRep_Health 后各自本地播放死亡蒙太奇。
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Syncfire|Enemy|Animation")
	TObjectPtr<UAnimMontage> DeathMontage;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Syncfire|Enemy|Animation")
	float DeathMontagePlayRate = 1.0f;

	// 死亡 Montage 播放一小段后再交给物理，避免布娃娃瞬间盖掉死亡动画。
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Syncfire|Enemy|Animation")
	bool bEnableRagdollAfterDeath = true;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Syncfire|Enemy|Animation", meta = (EditCondition = "bEnableRagdollAfterDeath", ClampMin = "0.0"))
	float RagdollDelay = 0.35f;

	// 清掉角色移动残留速度，避免布娃娃刚开启就沿地面滑行。
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Syncfire|Enemy|Ragdoll")
	bool bClearRagdollVelocityOnStart = true;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Syncfire|Enemy|Ragdoll", meta = (ClampMin = "0.0"))
	float RagdollLinearDamping = 8.0f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Syncfire|Enemy|Ragdoll", meta = (ClampMin = "0.0"))
	float RagdollAngularDamping = 12.0f;

	// 后续需要更强的受击反馈时可以打开；第一版默认关闭，避免尸体被推得太远。
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Syncfire|Enemy|Ragdoll")
	bool bApplyDeathImpulse = false;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Syncfire|Enemy|Ragdoll", meta = (EditCondition = "bApplyDeathImpulse", ClampMin = "0.0"))
	float DeathImpulseStrength = 3000.0f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Syncfire|Enemy|Ragdoll", meta = (EditCondition = "bApplyDeathImpulse", ClampMin = "0.0"))
	float DeathImpulseUpwardStrength = 400.0f;

	/* 死亡后尸体在场景里保留多久。服务端销毁 Actor 后，客户端也会收到销毁同步。 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Syncfire|Enemy|Death", meta = (ClampMin = "0.0"))
	float CorpseLifeSpan = 8.0f;

	// 血量变化会在服务器扣血时触发，也会在客户端收到 Health RepNotify 后触发。
	UFUNCTION(BlueprintImplementableEvent, Category = "Syncfire|Enemy")
	void BP_OnHealthChanged(float NewHealth, float HealthDelta);

	// 死亡的附加表现交给蓝图，例如粒子、音效或延迟隐藏 Mesh。
	UFUNCTION(BlueprintImplementableEvent, Category = "Syncfire|Enemy")
	void BP_OnDeath();

private:
	bool bLocalDeathHandled = false;
	bool bRagdollStarted = false;
	FVector LastDeathImpulseDirection = FVector::ZeroVector;
	FTimerHandle RagdollTimerHandle;

	void HandleHealthChanged(float NewHealth, float HealthDelta);
	void HandleDeath();
	bool CanPlayHitReactAnimation() const;
	void PlayHitReactAnimation();
	void PlayMontage(UAnimMontage* Montage, float PlayRate) const;

	// 攻击是一次性表现事件，不适合用属性复制；服务器确认攻击后广播给所有机器播放。
	UFUNCTION(NetMulticast, Reliable)
	void MulticastPlayAttackMontage();

	// 死亡冲击方向是短生命周期表现数据，用 Multicast 让各客户端的布娃娃朝同一方向受力。
	UFUNCTION(NetMulticast, Reliable)
	void MulticastSetDeathImpulseDirection(FVector_NetQuantizeNormal ImpulseDirection);
};

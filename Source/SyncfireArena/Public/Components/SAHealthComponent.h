// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "SAHealthComponent.generated.h"

DECLARE_MULTICAST_DELEGATE_TwoParams(FSAOnHealthChanged, float /*NewHealth*/, float /*HealthDelta*/);

UCLASS(ClassGroup=(Syncfire), meta=(BlueprintSpawnableComponent))
class SYNCFIREARENA_API USAHealthComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	USAHealthComponent();

	virtual void BeginPlay() override;
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	float GetHealth() const { return Health; }
	float GetMaxHealth() const { return MaxHealth; }
	bool IsDead() const { return Health <= 0.0f; }
	AActor* GetLastDamageCauser() const { return LastDamageCauser.Get(); }

	// 本地玩法通知，不是网络同步本身；服务器改血和客户端收到复制结果时都会触发。
	FSAOnHealthChanged OnHealthChanged;

	// 血量修改由服务器权威执行，客户端直接调用会被实现层忽略。
	UFUNCTION(BlueprintCallable, Category = "Syncfire|Health")
	void ApplyDamage(float DamageAmount, AActor* DamageCauser = nullptr);

	// 最大生命同样只由服务器修改并复制；属性组件会用它应用 MaxHealthBonus。
	UFUNCTION(BlueprintCallable, Category = "Syncfire|Health")
	void SetMaxHealth(float NewMaxHealth, bool bKeepHealthPercent = true);

protected:
	UPROPERTY(ReplicatedUsing = OnRep_MaxHealth, EditDefaultsOnly, BlueprintReadOnly, Category = "Syncfire|Health")
	float MaxHealth = 100.0f;

	UPROPERTY(ReplicatedUsing = OnRep_Health, VisibleInstanceOnly, BlueprintReadOnly, Category = "Syncfire|Health")
	float Health = 100.0f;

	UFUNCTION()
	void OnRep_Health();

	UFUNCTION()
	void OnRep_MaxHealth();

private:
	UPROPERTY()
	TObjectPtr<AActor> LastDamageCauser;

	float LastKnownHealth = 100.0f;

	void SetHealthInternal(float NewHealth);
};

// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "BulletTrajectoryEffect.generated.h"

class UProjectileMovementComponent;
class UNiagaraComponent;
class UNiagaraSystem;
class USceneComponent;
class USoundBase;

UCLASS()
class SYNCFIREARENA_API ABulletTrajectoryEffect : public AActor
{
	GENERATED_BODY()

public:
	ABulletTrajectoryEffect();

	void InitializeTrajectory(
		const FVector& StartLocation,
		const FVector& EndLocation,
		bool bInHit,
		const FVector& InHitLocation,
		const FVector& InHitNormal,
		UNiagaraSystem* InBulletSystem,
		UNiagaraSystem* InImpactEffect,
		USoundBase* InImpactSound,
		float InBulletSpeed);

protected:
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Syncfire|Bullet")
	TObjectPtr<USceneComponent> SceneRoot;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Syncfire|Bullet")
	TObjectPtr<UNiagaraComponent> BulletNiagaraComponent;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Syncfire|Bullet")
	TObjectPtr<UProjectileMovementComponent> ProjectileMovementComponent;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Syncfire|Bullet")
	TObjectPtr<UNiagaraSystem> BulletTrajectorySystem;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Syncfire|Bullet")
	TObjectPtr<UNiagaraSystem> ImpactEffect;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Syncfire|Bullet")
	TObjectPtr<USoundBase> ImpactSound;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Syncfire|Bullet")
	float ProjectileSpeed = 50000.0f;

	// 防止没有命中点或速度配置异常时，视觉子弹一直留在场景里。
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Syncfire|Bullet")
	float MaxLifeTime = 1.0f;

private:
	FTimerHandle FinishTimerHandle;
	FVector TrajectoryEndLocation = FVector::ZeroVector;
	FVector HitLocation = FVector::ZeroVector;
	FVector HitNormal = FVector::UpVector;
	bool bHasImpact = false;

	void FinishTrajectory();
	void PlayImpactEffects() const;
};

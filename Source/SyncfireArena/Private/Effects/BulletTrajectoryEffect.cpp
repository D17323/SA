// Copyright Epic Games, Inc. All Rights Reserved.

#include "Effects/BulletTrajectoryEffect.h"

#include "Components/SceneComponent.h"
#include "GameFramework/ProjectileMovementComponent.h"
#include "Kismet/GameplayStatics.h"
#include "NiagaraComponent.h"
#include "NiagaraFunctionLibrary.h"
#include "TimerManager.h"

ABulletTrajectoryEffect::ABulletTrajectoryEffect()
{
	PrimaryActorTick.bCanEverTick = false;

	SetReplicates(false);
	SetActorEnableCollision(false);

	SceneRoot = CreateDefaultSubobject<USceneComponent>(TEXT("SceneRoot"));
	SetRootComponent(SceneRoot);

	BulletNiagaraComponent = CreateDefaultSubobject<UNiagaraComponent>(TEXT("BulletNiagaraComponent"));
	BulletNiagaraComponent->SetupAttachment(SceneRoot);
	BulletNiagaraComponent->SetAutoActivate(false);

	ProjectileMovementComponent = CreateDefaultSubobject<UProjectileMovementComponent>(TEXT("ProjectileMovementComponent"));
	ProjectileMovementComponent->UpdatedComponent = SceneRoot;
	ProjectileMovementComponent->InitialSpeed = ProjectileSpeed;
	ProjectileMovementComponent->MaxSpeed = ProjectileSpeed;
	ProjectileMovementComponent->ProjectileGravityScale = 0.0f;
	ProjectileMovementComponent->bRotationFollowsVelocity = true;
	ProjectileMovementComponent->bAutoActivate = false;
}

void ABulletTrajectoryEffect::InitializeTrajectory(
	const FVector& StartLocation,
	const FVector& EndLocation,
	bool bInHit,
	const FVector& InHitLocation,
	const FVector& InHitNormal,
	UNiagaraSystem* InBulletSystem,
	UNiagaraSystem* InImpactEffect,
	USoundBase* InImpactSound,
	float InBulletSpeed)
{
	const FVector Trajectory = EndLocation - StartLocation;
	const FVector Direction = Trajectory.IsNearlyZero() ? FVector::ForwardVector : Trajectory.GetSafeNormal();
	const float Distance = FMath::Max(1.0f, Trajectory.Size());

	bHasImpact = bInHit;
	TrajectoryEndLocation = EndLocation;
	HitLocation = bInHit ? InHitLocation : EndLocation;
	HitNormal = InHitNormal.IsNearlyZero() ? FVector::UpVector : InHitNormal.GetSafeNormal();

	if (InBulletSystem)
	{
		BulletNiagaraComponent->SetAsset(InBulletSystem);
	}
	else if (BulletTrajectorySystem)
	{
		BulletNiagaraComponent->SetAsset(BulletTrajectorySystem.Get());
	}

	if (InImpactEffect)
	{
		ImpactEffect = InImpactEffect;
	}

	if (InImpactSound)
	{
		ImpactSound = InImpactSound;
	}

	ProjectileSpeed = FMath::Max(1.0f, InBulletSpeed);

	SetActorLocation(StartLocation);
	SetActorRotation(Direction.Rotation());

	// 这里不再手动拉伸或翻转 Niagara，让资源按自己的 Projectile 逻辑播放。
	BulletNiagaraComponent->Activate(true);

	ProjectileMovementComponent->InitialSpeed = ProjectileSpeed;
	ProjectileMovementComponent->MaxSpeed = ProjectileSpeed;
	ProjectileMovementComponent->Velocity = Direction * ProjectileSpeed;
	ProjectileMovementComponent->Activate(true);

	// 视觉子弹的到达时间只由服务器确认的射线距离和本地表现速度决定。
	// 命中特效在 FinishTrajectory 中播放，因此会和视觉弹体到达命中点保持同步。
	const float TravelTime = FMath::Max(0.01f, Distance / ProjectileSpeed);
	GetWorldTimerManager().SetTimer(FinishTimerHandle, this, &ABulletTrajectoryEffect::FinishTrajectory, TravelTime, false);
	SetLifeSpan(TravelTime + MaxLifeTime);
}

void ABulletTrajectoryEffect::FinishTrajectory()
{
	ProjectileMovementComponent->StopMovementImmediately();
	SetActorLocation(TrajectoryEndLocation);
	PlayImpactEffects();
	Destroy();
}

void ABulletTrajectoryEffect::PlayImpactEffects() const
{
	if (!bHasImpact)
	{
		return;
	}

	const FRotator ImpactRotation = HitNormal.Rotation();
	if (ImpactEffect)
	{
		UNiagaraFunctionLibrary::SpawnSystemAtLocation(this, ImpactEffect, HitLocation, ImpactRotation);
	}

	if (ImpactSound)
	{
		UGameplayStatics::PlaySoundAtLocation(this, ImpactSound, HitLocation);
	}
}

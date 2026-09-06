// Copyright Epic Games, Inc. All Rights Reserved.

#include "Components/SACombatComponent.h"

#include "AI/SAEnemyCharacter.h"
#include "Character/SACharacter.h"
#include "Components/SAHealthComponent.h"
#include "Components/SAStatComponent.h"
#include "DrawDebugHelpers.h"
#include "Weapons/SAWeaponActor.h"

USACombatComponent::USACombatComponent()
{
	PrimaryComponentTick.bCanEverTick = false;

	// 组件上的 Server RPC 依赖拥有者 Actor 的复制，角色本身必须是可复制的。
	SetIsReplicatedByDefault(true);
}

void USACombatComponent::BeginPlay()
{
	Super::BeginPlay();

	OwnerCharacter = Cast<ASACharacter>(GetOwner());
}

void USACombatComponent::StartFire(const FVector& AimPoint)
{
	if (!OwnerCharacter)
	{
		return;
	}

	bWantsToFire = true;
	PendingFireAimPoint = AimPoint;

	// 无论是半自动还是全自动，按下时都先立即发射一发。
	Fire(PendingFireAimPoint);

	if (CanAutoFireCurrentWeapon())
	{
		ScheduleNextAutoFire();
	}
}

void USACombatComponent::StopFire()
{
	bWantsToFire = false;
	PendingFireAimPoint = FVector::ZeroVector;

	if (GetWorld())
	{
		GetWorld()->GetTimerManager().ClearTimer(AutoFireTimerHandle);
	}
}

void USACombatComponent::UpdateFireAimPoint(const FVector& AimPoint)
{
	if (bWantsToFire)
	{
		PendingFireAimPoint = AimPoint;
	}
}

void USACombatComponent::Fire(const FVector& AimPoint)
{
	if (!OwnerCharacter)
	{
		return;
	}

	const float CurrentTime = GetWorld() ? GetWorld()->GetTimeSeconds() : 0.0f;
	ASAWeaponActor* CurrentWeapon = GetCurrentWeapon();
	if (CurrentWeapon && !CurrentWeapon->CanFire())
	{
		if (CurrentWeapon->GetMagazineAmmo() <= 0)
		{
			TryAutoReload();
		}

		return;
	}

	if (!CanLocalFire(CurrentTime))
	{
		return;
	}

	LastLocalFireTime = CurrentTime;

	const FVector FinalAimPoint = AimPoint.IsNearlyZero() ? BuildFallbackAimPoint() : AimPoint;
	FVector TraceStart;
	FVector TraceEnd;
	BuildFireTrace(FinalAimPoint, TraceStart, TraceEnd);

	// 本地射线只用于即时反馈，不参与最终伤害判定。
	FHitResult LocalHit;
	FCollisionQueryParams LocalParams(SCENE_QUERY_STAT(SyncfireLocalFireTrace), false, OwnerCharacter);
	const bool bLocalHit = GetWorld()->LineTraceSingleByChannel(LocalHit, TraceStart, TraceEnd, ECC_Visibility, LocalParams);
	const FVector LocalEnd = bLocalHit ? LocalHit.ImpactPoint : TraceEnd;
	PlayFireDebug(TraceStart, LocalEnd, bLocalHit, LocalEnd, false);

	if (CurrentWeapon)
	{
		// 本地玩家先播放开火表现，减少输入到表现之间的延迟。
		CurrentWeapon->PlayLocalFire(OwnerCharacter);

		// 纯客户端提前扣除本地弹量，避免服务器回应前快速多点出一枪。
		if (!OwnerCharacter->HasAuthority())
		{
			CurrentWeapon->ConsumeAmmo();
		}
	}

	// 客户端只发送瞄准点，不能直接发送伤害、命中目标或扣血结果。
	ServerFire(FinalAimPoint);

	if (CurrentWeapon && CurrentWeapon->GetMagazineAmmo() <= 0)
	{
		// 最后一发打空后自动尝试换弹；服务器仍会再次校验备弹和换弹状态。
		TryAutoReload();
	}
}

void USACombatComponent::Reload()
{
	if (!OwnerCharacter || bLocalReloadInProgress)
	{
		return;
	}

	ASAWeaponActor* CurrentWeapon = GetCurrentWeapon();
	if (!CurrentWeapon || !CurrentWeapon->CanReload())
	{
		return;
	}

	// 换弹会结束当前自动射击，避免换弹计时和自动开火同时运行。
	StopFire();

	bLocalReloadInProgress = true;
	LocalReloadingWeapon = CurrentWeapon;

	if (GetWorld())
	{
		GetWorld()->GetTimerManager().SetTimer(
			LocalReloadTimerHandle,
			this,
			&USACombatComponent::ClearLocalReloadState,
			GetReloadDurationForWeapon(CurrentWeapon),
			false);
	}

	// 本地先播放换弹动画，服务器随后验证这次换弹是否合法。
	CurrentWeapon->PlayLocalReload(OwnerCharacter);
	ServerReload(CurrentWeapon);
}

void USACombatComponent::CancelReload()
{
	ASAWeaponActor* ReloadWeapon = LocalReloadingWeapon.Get();

	if (GetWorld())
	{
		GetWorld()->GetTimerManager().ClearTimer(LocalReloadTimerHandle);
	}

	bLocalReloadInProgress = false;
	LocalReloadingWeapon.Reset();

	if (OwnerCharacter && OwnerCharacter->HasAuthority())
	{
		if (ASAWeaponActor* ServerReloadWeapon = ServerReloadingWeapon.Get())
		{
			CancelServerReload();
			MulticastReloadCanceled(ServerReloadWeapon);
		}

		return;
	}

	if (ReloadWeapon)
	{
		ReloadWeapon->CancelReload();
		ReloadWeapon->StopReloadMontage(OwnerCharacter);

		if (OwnerCharacter)
		{
			ServerCancelReload(ReloadWeapon);
		}
	}
}

void USACombatComponent::TryAutoReload()
{
	if (!OwnerCharacter || bLocalReloadInProgress)
	{
		return;
	}

	ASAWeaponActor* CurrentWeapon = GetCurrentWeapon();
	if (CurrentWeapon && CurrentWeapon->GetMagazineAmmo() <= 0 && CurrentWeapon->CanReload())
	{
		Reload();
	}
}

bool USACombatComponent::CanFireByCooldown(float LastFireTime, float CurrentTime) const
{
	return CurrentTime - LastFireTime >= GetCurrentFireCooldown();
}

bool USACombatComponent::CanLocalFire(float CurrentTime) const
{
	if (bLocalReloadInProgress)
	{
		return false;
	}

	const ASAWeaponActor* CurrentWeapon = GetCurrentWeapon();
	return CurrentWeapon
		&& CurrentWeapon->CanFire()
		&& CanFireByCooldown(LastLocalFireTime, CurrentTime);
}

bool USACombatComponent::CanServerFire(float CurrentTime) const
{
	if (!OwnerCharacter)
	{
		return false;
	}

	const ASAWeaponActor* CurrentWeapon = GetCurrentWeapon();
	return CurrentWeapon
		&& CurrentWeapon->CanFire()
		&& CanFireByCooldown(LastServerFireTime, CurrentTime);
}

bool USACombatComponent::CanAutoFireCurrentWeapon() const
{
	const ASAWeaponActor* CurrentWeapon = GetCurrentWeapon();
	return CurrentWeapon && CurrentWeapon->IsAutomaticFire() && CurrentWeapon->CanFire();
}

ASAWeaponActor* USACombatComponent::GetCurrentWeapon() const
{
	return OwnerCharacter ? OwnerCharacter->GetCurrentWeapon() : nullptr;
}

USAStatComponent* USACombatComponent::GetOwnerStatComponent() const
{
	return OwnerCharacter ? OwnerCharacter->GetStatComponent() : nullptr;
}

float USACombatComponent::GetCurrentFireRange() const
{
	const ASAWeaponActor* CurrentWeapon = GetCurrentWeapon();
	return CurrentWeapon ? CurrentWeapon->GetFireRange() : FireRange;
}

float USACombatComponent::GetCurrentFireCooldown() const
{
	const ASAWeaponActor* CurrentWeapon = GetCurrentWeapon();
	const float BaseFireCooldown = CurrentWeapon ? CurrentWeapon->GetFireCooldown() : FireCooldown;

	if (const USAStatComponent* StatComponent = GetOwnerStatComponent())
	{
		return StatComponent->GetFinalFireCooldown(BaseFireCooldown);
	}

	return BaseFireCooldown;
}

float USACombatComponent::GetCurrentDamage() const
{
	const ASAWeaponActor* CurrentWeapon = GetCurrentWeapon();
	const float BaseDamage = CurrentWeapon ? CurrentWeapon->GetDamage() : Damage;

	if (const USAStatComponent* StatComponent = GetOwnerStatComponent())
	{
		return StatComponent->GetFinalDamage(BaseDamage);
	}

	return BaseDamage;
}

float USACombatComponent::GetReloadDurationForWeapon(const ASAWeaponActor* Weapon) const
{
	if (!Weapon)
	{
		return 0.0f;
	}

	const float BaseReloadDuration = Weapon->GetReloadDuration();
	if (const USAStatComponent* StatComponent = GetOwnerStatComponent())
	{
		return StatComponent->GetFinalReloadDuration(BaseReloadDuration);
	}

	return FMath::Max(0.01f, BaseReloadDuration);
}

FVector USACombatComponent::GetTraceStart() const
{
	FVector MuzzleLocation;
	const ASAWeaponActor* CurrentWeapon = GetCurrentWeapon();
	if (CurrentWeapon && CurrentWeapon->GetMuzzleLocation(MuzzleLocation))
	{
		return MuzzleLocation;
	}

	return OwnerCharacter
		? OwnerCharacter->GetActorLocation() + FVector(0.0f, 0.0f, TraceStartHeight)
		: FVector::ZeroVector;
}

FVector USACombatComponent::BuildFallbackAimPoint() const
{
	if (!OwnerCharacter)
	{
		return FVector::ZeroVector;
	}

	return OwnerCharacter->GetActorLocation() + OwnerCharacter->GetActorForwardVector() * GetCurrentFireRange();
}

void USACombatComponent::BuildFireTrace(const FVector& AimPoint, FVector& OutTraceStart, FVector& OutTraceEnd) const
{
	OutTraceStart = GetTraceStart();

	if (!OwnerCharacter)
	{
		OutTraceEnd = OutTraceStart;
		return;
	}

	// 鼠标点只决定角色在 XY 平面上的瞄准方向，射线始终从枪口发出。
	const FVector AimOrigin = OwnerCharacter->GetActorLocation();
	const FVector ToAimPoint(AimPoint.X - AimOrigin.X, AimPoint.Y - AimOrigin.Y, 0.0f);

	const FVector FallbackDirection = OwnerCharacter->GetActorForwardVector().GetSafeNormal2D();
	const FVector FireDirection = ToAimPoint.IsNearlyZero() ? FallbackDirection : ToAimPoint.GetSafeNormal();
	OutTraceEnd = OutTraceStart + FireDirection * GetCurrentFireRange();
}

void USACombatComponent::PlayFireDebug(
	const FVector& TraceStart,
	const FVector& TraceEnd,
	bool bHit,
	const FVector& HitLocation,
	bool bServerConfirmed) const
{
	if (!bDrawFireTrace || !GetWorld())
	{
		return;
	}

	const FColor LineColor = bServerConfirmed ? FColor::Green : (bHit ? FColor::Orange : FColor::Cyan);
	const FColor HitColor = bServerConfirmed ? FColor::Yellow : FColor::Red;
	const float LifeTime = bServerConfirmed ? FireTraceDebugLifeTime : FMath::Max(0.25f, FireTraceDebugLifeTime * 0.35f);
	const float Thickness = bServerConfirmed ? FireTraceDebugThickness : FMath::Max(1.5f, FireTraceDebugThickness * 0.75f);

	DrawDebugSphere(GetWorld(), TraceStart, 8.0f, 8, LineColor, false, LifeTime);
	DrawDebugLine(GetWorld(), TraceStart, TraceEnd, LineColor, false, LifeTime, 0, Thickness);
	DrawDebugDirectionalArrow(GetWorld(), TraceStart, TraceEnd, 18.0f, LineColor, false, LifeTime, 0, Thickness);

	if (bHit)
	{
		DrawDebugSphere(GetWorld(), HitLocation, bServerConfirmed ? 16.0f : 10.0f, 12, HitColor, false, LifeTime);
	}
}

void USACombatComponent::HandleAutoFire()
{
	if (!bWantsToFire || !OwnerCharacter)
	{
		return;
	}

	ASAWeaponActor* CurrentWeapon = GetCurrentWeapon();
	if (!CurrentWeapon || !CurrentWeapon->IsAutomaticFire())
	{
		StopFire();
		return;
	}

	if (!CurrentWeapon->CanFire())
	{
		TryAutoReload();
		StopFire();
		return;
	}

	Fire(PendingFireAimPoint);

	if (bWantsToFire && CanAutoFireCurrentWeapon())
	{
		ScheduleNextAutoFire();
	}
}

void USACombatComponent::ScheduleNextAutoFire()
{
	if (!GetWorld() || !bWantsToFire)
	{
		return;
	}

	ASAWeaponActor* CurrentWeapon = GetCurrentWeapon();
	if (!CurrentWeapon || !CurrentWeapon->IsAutomaticFire())
	{
		return;
	}

	GetWorld()->GetTimerManager().SetTimer(
		AutoFireTimerHandle,
		this,
		&USACombatComponent::HandleAutoFire,
		GetCurrentFireCooldown(),
		false);
}

void USACombatComponent::ClearLocalReloadState()
{
	bLocalReloadInProgress = false;
	LocalReloadingWeapon.Reset();
}

void USACombatComponent::CancelServerReload()
{
	if (GetWorld())
	{
		GetWorld()->GetTimerManager().ClearTimer(ServerReloadTimerHandle);
	}

	if (ASAWeaponActor* ReloadWeapon = ServerReloadingWeapon.Get())
	{
		ReloadWeapon->CancelReload();
	}

	ServerReloadingWeapon.Reset();
}

void USACombatComponent::FinishServerReload()
{
	if (!OwnerCharacter || !OwnerCharacter->HasAuthority())
	{
		return;
	}

	if (ASAWeaponActor* ReloadWeapon = ServerReloadingWeapon.Get())
	{
		ReloadWeapon->FinishReload();
	}

	ServerReloadingWeapon.Reset();
}

void USACombatComponent::ServerFire_Implementation(FVector_NetQuantize AimPoint)
{
	if (!OwnerCharacter || !OwnerCharacter->HasAuthority())
	{
		return;
	}

	const float CurrentTime = GetWorld() ? GetWorld()->GetTimeSeconds() : 0.0f;
	if (!CanServerFire(CurrentTime))
	{
		return;
	}

	ASAWeaponActor* CurrentWeapon = GetCurrentWeapon();
	if (!CurrentWeapon || !CurrentWeapon->ConsumeAmmo())
	{
		return;
	}

	LastServerFireTime = CurrentTime;

	FVector TraceStart;
	FVector TraceEnd;
	BuildFireTrace(FVector(AimPoint), TraceStart, TraceEnd);

	FHitResult Hit;
	FCollisionQueryParams Params(SCENE_QUERY_STAT(SyncfireServerFireTrace), false, OwnerCharacter);
	GetWorld()->LineTraceSingleByChannel(Hit, TraceStart, TraceEnd, ECC_Visibility, Params);

	const FVector DebugEnd = Hit.bBlockingHit ? Hit.ImpactPoint : TraceEnd;

	// 服务器把最终射线结果广播给客户端，客户端只负责播放视觉弹体和命中特效。
	const bool bHitEnemy = Hit.GetActor() && Hit.GetActor()->IsA(ASAEnemyCharacter::StaticClass());
	MulticastFireConfirmed(CurrentWeapon, TraceStart, DebugEnd, Hit.bBlockingHit, bHitEnemy, DebugEnd, Hit.ImpactNormal);

	if (!Hit.bBlockingHit)
	{
		return;
	}

	AActor* HitActor = Hit.GetActor();
	if (!HitActor)
	{
		return;
	}

	if (USAHealthComponent* HealthComponent = HitActor->FindComponentByClass<USAHealthComponent>())
	{

		if (ASAEnemyCharacter* HitEnemy = Cast<ASAEnemyCharacter>(HitActor))
		{
			HitEnemy->SetDeathImpulseDirection((TraceEnd - TraceStart).GetSafeNormal());
			// 布娃娃冲击方向来自服务器确认的射线方向，而不是敌人自身朝向。
		}

		// 伤害只在服务器执行，客户端通过 Health 的复制结果更新表现。
		HealthComponent->ApplyDamage(GetCurrentDamage(), OwnerCharacter);
	}
}

void USACombatComponent::ServerReload_Implementation(ASAWeaponActor* RequestedWeapon)
{
	if (!OwnerCharacter || !OwnerCharacter->HasAuthority())
	{
		return;
	}

	if (ServerReloadingWeapon.IsValid()
		|| !RequestedWeapon
		|| RequestedWeapon->GetOwner() != OwnerCharacter
		|| !RequestedWeapon->StartReload())
	{
		return;
	}

	ServerReloadingWeapon = RequestedWeapon;

	if (GetWorld())
	{
		GetWorld()->GetTimerManager().SetTimer(
			ServerReloadTimerHandle,
			this,
			&USACombatComponent::FinishServerReload,
			GetReloadDurationForWeapon(RequestedWeapon),
			false);
	}

	// 服务器确认换弹后，再让其他客户端播放远端角色的换弹动作。
	MulticastReloadConfirmed(RequestedWeapon);
}

void USACombatComponent::ServerCancelReload_Implementation(ASAWeaponActor* RequestedWeapon)
{
	if (!OwnerCharacter || !OwnerCharacter->HasAuthority() || !RequestedWeapon)
	{
		return;
	}

	if (RequestedWeapon->GetOwner() != OwnerCharacter || ServerReloadingWeapon.Get() != RequestedWeapon)
	{
		return;
	}

	CancelServerReload();
	MulticastReloadCanceled(RequestedWeapon);
}

void USACombatComponent::MulticastFireConfirmed_Implementation(
	ASAWeaponActor* FiredWeapon,
	FVector_NetQuantize TraceStart,
	FVector_NetQuantize TraceEnd,
	bool bHit,
	bool bHitEnemy,
	FVector_NetQuantize HitLocation,
	FVector_NetQuantizeNormal HitNormal)
{
	PlayFireDebug(FVector(TraceStart), FVector(TraceEnd), bHit, FVector(HitLocation), true);

	if (!FiredWeapon)
	{
		return;
	}

	// 服务器确认后的轨迹只播放一次，避免客户端和服务器各生成一条视觉子弹。
	FiredWeapon->SpawnBulletTrajectoryEffect(
		FVector(TraceStart),
		FVector(TraceEnd),
		bHit,
		bHitEnemy,
		FVector(HitLocation),
		FVector(HitNormal));

	if (OwnerCharacter && !OwnerCharacter->IsLocallyControlled())
	{
		FiredWeapon->PlayRemoteFire(OwnerCharacter);
	}
}

void USACombatComponent::MulticastReloadConfirmed_Implementation(ASAWeaponActor* ReloadWeapon)
{
	if (OwnerCharacter && !OwnerCharacter->IsLocallyControlled() && ReloadWeapon)
	{
		ReloadWeapon->PlayRemoteReload(OwnerCharacter);
	}
}

void USACombatComponent::MulticastReloadCanceled_Implementation(ASAWeaponActor* ReloadWeapon)
{
	if (OwnerCharacter && ReloadWeapon)
	{
		ReloadWeapon->CancelReload();
		ReloadWeapon->StopReloadMontage(OwnerCharacter);
	}
}

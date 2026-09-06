// Copyright Epic Games, Inc. All Rights Reserved.

#include "Components/SAHealthComponent.h"

#include "Net/UnrealNetwork.h"
#include "Perception/AISense_Damage.h"

USAHealthComponent::USAHealthComponent()
{
	// 组件复制依赖拥有它的 Actor 开启复制；角色/敌人已经负责复制自身。
	SetIsReplicatedByDefault(true);
	PrimaryComponentTick.bCanEverTick = false;
}

void USAHealthComponent::BeginPlay()
{
	Super::BeginPlay();

	LastKnownHealth = Health;
}

void USAHealthComponent::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(USAHealthComponent, MaxHealth);
	DOREPLIFETIME(USAHealthComponent, Health);
}

void USAHealthComponent::ApplyDamage(float DamageAmount, AActor* DamageCauser)
{
	if (DamageAmount <= 0.0f || GetOwner() == nullptr || !GetOwner()->HasAuthority())
	{
		return;
	}

	const float OldHealth = LastKnownHealth;
	SetHealthInternal(Health - DamageAmount);
	LastKnownHealth = Health;

	if (FMath::IsNearlyEqual(OldHealth, Health))
	{
		return;
	}

	if (AActor* OwnerActor = GetOwner())
	{
		OwnerActor->ForceNetUpdate();
	}

	// 最后伤害来源只在服务器记录，GameMode 用它结算击杀分数，不需要复制给客户端。
	LastDamageCauser = (DamageCauser && DamageCauser != GetOwner()) ? DamageCauser : nullptr;

	if (DamageCauser && DamageCauser != GetOwner() && GetWorld())
	{
		// AI Damage Sense 只在服务器上报，避免各客户端重复刺激 AI 感知。
		UAISense_Damage::ReportDamageEvent(
			GetWorld(),
			GetOwner(),
			DamageCauser,
			DamageAmount,
			DamageCauser->GetActorLocation(),
			GetOwner()->GetActorLocation());
	}

	OnHealthChanged.Broadcast(Health, Health - OldHealth);
}

void USAHealthComponent::SetMaxHealth(float NewMaxHealth, bool bKeepHealthPercent)
{
	if (GetOwner() == nullptr || !GetOwner()->HasAuthority())
	{
		return;
	}

	const float OldHealth = Health;
	const float OldMaxHealth = FMath::Max(1.0f, MaxHealth);
	const float NewSafeMaxHealth = FMath::Max(1.0f, NewMaxHealth);
	const float HealthPercent = FMath::Clamp(Health / OldMaxHealth, 0.0f, 1.0f);

	MaxHealth = NewSafeMaxHealth;
	SetHealthInternal(bKeepHealthPercent ? MaxHealth * HealthPercent : FMath::Min(Health, MaxHealth));
	LastKnownHealth = Health;

	// 最大生命由服务器修改并复制，客户端只接收结果。
	if (!FMath::IsNearlyEqual(OldHealth, Health))
	{
		OnHealthChanged.Broadcast(Health, Health - OldHealth);
	}

	if (AActor* OwnerActor = GetOwner())
	{
		OwnerActor->ForceNetUpdate();
	}
}

void USAHealthComponent::OnRep_Health()
{
	const float OldHealth = LastKnownHealth;
	LastKnownHealth = Health;
	OnHealthChanged.Broadcast(Health, Health - OldHealth);
}

void USAHealthComponent::OnRep_MaxHealth()
{
	if (Health > MaxHealth)
	{
		Health = MaxHealth;
		LastKnownHealth = Health;
	}
}

void USAHealthComponent::SetHealthInternal(float NewHealth)
{
	const float ClampedHealth = FMath::Clamp(NewHealth, 0.0f, MaxHealth);
	if (FMath::IsNearlyEqual(Health, ClampedHealth))
	{
		return;
	}

	Health = ClampedHealth;
}

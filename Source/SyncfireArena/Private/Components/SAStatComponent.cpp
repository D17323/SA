
#include "Components/SAStatComponent.h"

#include "Character/SACharacter.h"
#include "Components/SAHealthComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Net/UnrealNetwork.h"

USAStatComponent::USAStatComponent()
{
	SetIsReplicatedByDefault(true);
	PrimaryComponentTick.bCanEverTick = false;

}

void USAStatComponent::BeginPlay()
{
	Super::BeginPlay();
	CacheOwnerBaselineStats();
}

void USAStatComponent::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	
	DOREPLIFETIME(USAStatComponent, MoveSpeedMultiplier);
	DOREPLIFETIME(USAStatComponent, DamageMultiplier);
	DOREPLIFETIME(USAStatComponent, FireRateMultiplier);
	DOREPLIFETIME(USAStatComponent, ReloadSpeedMultiplier);
	DOREPLIFETIME(USAStatComponent, MaxHealthBonus);
	DOREPLIFETIME(USAStatComponent, ComboWindowBonus);
}

void USAStatComponent::ResetStats()
{
	if (!GetOwner() || !GetOwner()->HasAuthority())
	{
		return;
	}

	MoveSpeedMultiplier = 1.0f;
	DamageMultiplier = 1.0f;
	FireRateMultiplier = 1.0f;
	ReloadSpeedMultiplier = 1.0f;
	MaxHealthBonus = 0.0f;
	ComboWindowBonus = 0.0f;

	NotifyStatChanged();
}

void USAStatComponent::SetStatValue(ESAStatType StatType, float NewValue)
{
	if (!GetOwner() || !GetOwner()->HasAuthority())
	{
		return;
	}
	ApplyStatValue(StatType, NewValue);
}

void USAStatComponent::AddStatValue(ESAStatType StatType, float DeltaValue)
{
	if (!GetOwner() || !GetOwner()->HasAuthority())
	{
		return;
	}
	
	const float CurrentValue = GetStatValue(StatType);
	ApplyStatValue(StatType, CurrentValue+DeltaValue);
}

float USAStatComponent::GetStatValue(ESAStatType StatType) const
{
	switch (StatType)
	{
	case ESAStatType::MoveSpeedMultiplier:
		return MoveSpeedMultiplier;
	case ESAStatType::DamageMultiplier:
		return DamageMultiplier;
	case ESAStatType::FireRateMultiplier:
		return FireRateMultiplier;
	case ESAStatType::ReloadSpeedMultiplier:
		return ReloadSpeedMultiplier;
	case ESAStatType::MaxHealthBonus:
		return MaxHealthBonus;
	case ESAStatType::ComboWindowBonus:
		return ComboWindowBonus;
	default:
		return 0.0f;
	}
}

float USAStatComponent::GetFinalMoveSpeed() const
{
	return FMath::Max(1.0f,CachedBaseMoveSpeed)*ClampMultiplier(MoveSpeedMultiplier);
}

float USAStatComponent::GetFinalMaxHealth() const
{
	return FMath::Max(1.0f,CachedBaseMaxHealth) + ClampBonus(MaxHealthBonus);
}

float USAStatComponent::GetFinalDamage(float BaseDamage) const
{
	return FMath::Max(0.0f,BaseDamage)*ClampMultiplier(DamageMultiplier);
}

float USAStatComponent::GetFinalFireCooldown(float BaseFireCooldown) const
{
	return FMath::Max(0.01f,BaseFireCooldown/ClampMultiplier(FireRateMultiplier));
}

float USAStatComponent::GetFinalReloadDuration(float BaseReloadDuration) const
{
	return FMath::Max(0.01f, BaseReloadDuration / ClampMultiplier(ReloadSpeedMultiplier));
}

float USAStatComponent::GetFinalComboWindow(float BaseComboWindow) const
{
	return FMath::Max(0.1f, BaseComboWindow + ClampBonus(ComboWindowBonus));
}

void USAStatComponent::OnRep_Stats()
{
	// 属性复制到客户端后，只刷新本地表现。真正的伤害、射速、换弹等权威结算仍由服务器读取同一套数值。
	NotifyStatChanged();
}

void USAStatComponent::CacheOwnerBaselineStats()
{
	CachedOwnerCharacter = Cast<ASACharacter>(GetOwner());
	CachedHealthComponent = CachedOwnerCharacter ? CachedOwnerCharacter->FindComponentByClass<USAHealthComponent>() : nullptr;

	if (CachedOwnerCharacter)
	{
		if (UCharacterMovementComponent* MovementComponent = CachedOwnerCharacter->GetCharacterMovement())
		{
			CachedBaseMoveSpeed = FMath::Max(1.0f, MovementComponent->MaxWalkSpeed);
		}
	}

	if (CachedHealthComponent)
	{
		CachedBaseMaxHealth = FMath::Max(1.0f, CachedHealthComponent->GetMaxHealth());
	}
}

void USAStatComponent::NotifyStatChanged()
{
	if (AActor* OwnerActor = GetOwner())
	{
		if (OwnerActor->HasAuthority())
		{
			OwnerActor->ForceNetUpdate();
		}
	}

	OnStatChanged.Broadcast();
}

void USAStatComponent::ApplyStatValue(ESAStatType StatType, float NewValue)
{
	float* TargetValue = nullptr;

	switch (StatType)
	{
	case ESAStatType::MoveSpeedMultiplier:
		TargetValue = &MoveSpeedMultiplier;
		NewValue = ClampMultiplier(NewValue);
		break;
	case ESAStatType::DamageMultiplier:
		TargetValue = &DamageMultiplier;
		NewValue = ClampMultiplier(NewValue);
		break;
	case ESAStatType::FireRateMultiplier:
		TargetValue = &FireRateMultiplier;
		NewValue = ClampMultiplier(NewValue);
		break;
	case ESAStatType::ReloadSpeedMultiplier:
		TargetValue = &ReloadSpeedMultiplier;
		NewValue = ClampMultiplier(NewValue);
		break;
	case ESAStatType::MaxHealthBonus:
		TargetValue = &MaxHealthBonus;
		NewValue = ClampBonus(NewValue);
		break;
	case ESAStatType::ComboWindowBonus:
		TargetValue = &ComboWindowBonus;
		NewValue = ClampBonus(NewValue);
		break;
	default:
		return;
	}

	if (!TargetValue || FMath::IsNearlyEqual(*TargetValue, NewValue))
	{
		return;
	}

	*TargetValue = NewValue;
	NotifyStatChanged();
}

float USAStatComponent::ClampMultiplier(float Value) const
{
	return FMath::Max(0.01f, Value);
}

float USAStatComponent::ClampBonus(float Value) const
{
	return FMath::Max(0.0f, Value);
}


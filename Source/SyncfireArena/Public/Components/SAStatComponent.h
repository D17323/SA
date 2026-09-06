// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "SAStatComponent.generated.h"

class USAHealthComponent;
class ASACharacter;

UENUM(BlueprintType)
enum class ESAStatType: uint8
{
	MoveSpeedMultiplier,
	DamageMultiplier,
	FireRateMultiplier,
	ReloadSpeedMultiplier,
	MaxHealthBonus,
	ComboWindowBonus,
};

DECLARE_MULTICAST_DELEGATE(FSAOnStatChanged);

UCLASS(ClassGroup=(Syncfire), meta=(BlueprintSpawnableComponent))
class SYNCFIREARENA_API USAStatComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	USAStatComponent();
	
	virtual void BeginPlay() override;
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;
	
	UFUNCTION(BlueprintCallable, Category = "Syncfire|Stat")
	void ResetStats();
	
	UFUNCTION(BlueprintCallable, Category = "Syncfire|Stat")
	void SetStatValue(ESAStatType StatType,float NewValue);
	
	UFUNCTION(BlueprintCallable, Category = "Syncfire|Stat")
	void AddStatValue(ESAStatType StatType,float DeltaValue);
	
	UFUNCTION(BlueprintPure, Category = "Syncfire|Stat")
	float GetStatValue(ESAStatType StatType) const;
	
	UFUNCTION(BlueprintPure, Category = "Syncfire|Stat")
	float GetMoveSpeedMultiplier() const {return MoveSpeedMultiplier;}
	
	UFUNCTION(BlueprintPure, Category = "Syncfire|Stat")
	float GetDamageMultiplier() const {return DamageMultiplier;}
	
	UFUNCTION(BlueprintPure, Category = "Syncfire|Stat")
	float GetFireRateMultiplier() const {return FireRateMultiplier;}
	
	UFUNCTION(BlueprintPure, Category = "Syncfire|Stat")
	float GetReloadSpeedMultiplier() const {return ReloadSpeedMultiplier;}
	
	UFUNCTION(BlueprintPure, Category = "Syncfire|Stat")
	float GetMaxHealthBonus() const {return MaxHealthBonus;}
	
	UFUNCTION(BlueprintPure, Category = "Syncfire|Stat")
	float GetComboWindowBonus() const {return ComboWindowBonus;}
	
	UFUNCTION(BlueprintPure, Category = "Syncfire|Stat")
	float GetFinalMoveSpeed() const;

	UFUNCTION(BlueprintPure, Category = "Syncfire|Stat")
	float GetFinalMaxHealth() const;

	UFUNCTION(BlueprintPure, Category = "Syncfire|Stat")
	float GetFinalDamage(float BaseDamage) const;

	UFUNCTION(BlueprintPure, Category = "Syncfire|Stat")
	float GetFinalFireCooldown(float BaseFireCooldown) const;

	UFUNCTION(BlueprintPure, Category = "Syncfire|Stat")
	float GetFinalReloadDuration(float BaseReloadDuration) const;

	UFUNCTION(BlueprintPure, Category = "Syncfire|Stat")
	float GetFinalComboWindow(float BaseComboWindow) const;

	FSAOnStatChanged OnStatChanged;
protected:
	UPROPERTY(EditDefaultsOnly, ReplicatedUsing=OnRep_Stats, BlueprintReadOnly, Category = "Syncfire|Stat", meta = (ClampMin = "0.01", AllowPrivateAccess = "true"))
	float MoveSpeedMultiplier = 1.0f;
	
	UPROPERTY(EditDefaultsOnly, ReplicatedUsing=OnRep_Stats, BlueprintReadOnly, Category = "Syncfire|Stat", meta = (ClampMin = "0.01", AllowPrivateAccess = "true"))
	float DamageMultiplier = 1.0f;
	
	UPROPERTY(EditDefaultsOnly, ReplicatedUsing=OnRep_Stats, BlueprintReadOnly, Category = "Syncfire|Stat", meta = (ClampMin = "0.01", AllowPrivateAccess = "true"))
	float FireRateMultiplier = 1.0f;
	
	UPROPERTY(EditDefaultsOnly, ReplicatedUsing=OnRep_Stats, BlueprintReadOnly, Category = "Syncfire|Stat", meta = (ClampMin = "0.01", AllowPrivateAccess = "true"))
	float ReloadSpeedMultiplier = 1.0f;
	
	UPROPERTY(EditDefaultsOnly, ReplicatedUsing=OnRep_Stats, BlueprintReadOnly, Category = "Syncfire|Stat", meta = (ClampMin = "0.0", AllowPrivateAccess = "true"))
	float MaxHealthBonus = 0.0f;
	
	UPROPERTY(EditDefaultsOnly, ReplicatedUsing=OnRep_Stats, BlueprintReadOnly, Category = "Syncfire|Stat", meta = (ClampMin = "0.0", AllowPrivateAccess = "true"))
	float ComboWindowBonus = 0.0f;
	
private:
	UPROPERTY()
	TObjectPtr<ASACharacter> CachedOwnerCharacter;
	UPROPERTY()
	TObjectPtr<USAHealthComponent> CachedHealthComponent;
	
	float CachedBaseMoveSpeed = 600.0f;
	float CachedBaseMaxHealth = 100.0f;
	
	UFUNCTION()
	void OnRep_Stats();
	
	void CacheOwnerBaselineStats();
	void NotifyStatChanged();
	void ApplyStatValue(ESAStatType StatType, float NewValue);
	float ClampMultiplier(float Value) const;
	float ClampBonus(float Value) const;
};

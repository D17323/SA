// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "SAWeaponPickupActor.generated.h"

class ASACharacter;
class ASAWeaponActor;
class UPrimitiveComponent;
class USkeletalMeshComponent;
class USphereComponent;
struct FHitResult;

UCLASS()
class SYNCFIREARENA_API ASAWeaponPickupActor : public AActor
{
	GENERATED_BODY()

public:
	ASAWeaponPickupActor();

	virtual void BeginPlay() override;
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	void ConfigurePickup(TSubclassOf<ASAWeaponActor> InWeaponClass, int32 InMagazineAmmo, int32 InReserveAmmo, bool bInEquipOnPickup);

	TSubclassOf<ASAWeaponActor> GetWeaponClass() const { return WeaponClass; }
	int32 GetMagazineAmmo() const { return MagazineAmmo; }
	int32 GetReserveAmmo() const { return ReserveAmmo; }
	bool ShouldEquipOnPickup() const { return bEquipOnPickup; }

protected:
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Syncfire|Pickup")
	TObjectPtr<USphereComponent> PickupSphere;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Syncfire|Pickup")
	TObjectPtr<USkeletalMeshComponent> PickupMesh;

	// 服务器配置拾取物代表哪把武器，并复制给客户端，避免客户端只看到蓝图默认武器。
	UPROPERTY(ReplicatedUsing = OnRep_PickupData, EditAnywhere, BlueprintReadOnly, Category = "Syncfire|Pickup")
	TSubclassOf<ASAWeaponActor> WeaponClass;

	UPROPERTY(ReplicatedUsing = OnRep_PickupData, EditAnywhere, BlueprintReadOnly, Category = "Syncfire|Pickup", meta = (ClampMin = "0"))
	int32 MagazineAmmo = 12;

	UPROPERTY(ReplicatedUsing = OnRep_PickupData, EditAnywhere, BlueprintReadOnly, Category = "Syncfire|Pickup", meta = (ClampMin = "0"))
	int32 ReserveAmmo = 36;

	UPROPERTY(ReplicatedUsing = OnRep_PickupData, EditAnywhere, BlueprintReadOnly, Category = "Syncfire|Pickup")
	bool bEquipOnPickup = true;

	UFUNCTION()
	void OnRep_PickupData();

	void RefreshPickupVisual();

	UFUNCTION()
	void HandlePickupOverlapBegin(
		UPrimitiveComponent* OverlappedComponent,
		AActor* OtherActor,
		UPrimitiveComponent* OtherComp,
		int32 OtherBodyIndex,
		bool bFromSweep,
		const FHitResult& SweepResult);

	UFUNCTION()
	void HandlePickupOverlapEnd(
		UPrimitiveComponent* OverlappedComponent,
		AActor* OtherActor,
		UPrimitiveComponent* OtherComp,
		int32 OtherBodyIndex);
};

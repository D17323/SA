#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "SAAmmoPickupActor.generated.h"

class ASACharacter;
class UPrimitiveComponent;
class USceneComponent;
class USphereComponent;
class UStaticMeshComponent;
struct FHitResult;

UCLASS()
class SYNCFIREARENA_API ASAAmmoPickupActor : public AActor
{
	GENERATED_BODY()

public:
	ASAAmmoPickupActor();

	virtual void BeginPlay() override;
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	void ConfigurePickup(int32 InAmmoAmount);

protected:
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Syncfire|Drop")
	TObjectPtr<USceneComponent> SceneRoot;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Syncfire|Drop")
	TObjectPtr<USphereComponent> PickupSphere;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Syncfire|Drop")
	TObjectPtr<UStaticMeshComponent> PickupMesh;

	UPROPERTY(Replicated, EditAnywhere, BlueprintReadOnly, Category = "Syncfire|Drop", meta = (ClampMin = "1"))
	int32 AmmoAmount = 24;

	UFUNCTION()
	void HandlePickupOverlapBegin(
		UPrimitiveComponent* OverlappedComponent,
		AActor* OtherActor,
		UPrimitiveComponent* OtherComp,
		int32 OtherBodyIndex,
		bool bFromSweep,
		const FHitResult& SweepResult);
};

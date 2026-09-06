#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "SAAmmoSupplyActor.generated.h"

class USceneComponent;
class UStaticMeshComponent;
class UWidgetComponent;

UCLASS()
class SYNCFIREARENA_API ASAAmmoSupplyActor : public AActor
{
	GENERATED_BODY()

public:
	ASAAmmoSupplyActor();

	virtual void BeginPlay() override;

protected:
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Syncfire|Supply")
	TObjectPtr<USceneComponent> SceneRoot;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Syncfire|Supply")
	TObjectPtr<UStaticMeshComponent> SupplyMesh;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Syncfire|Supply")
	TObjectPtr<UWidgetComponent> PromptWidget;
};

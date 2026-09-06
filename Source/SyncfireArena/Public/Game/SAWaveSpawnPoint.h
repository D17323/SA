#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "SAWaveSpawnPoint.generated.h"

class USceneComponent;

UCLASS()
class SYNCFIREARENA_API ASAWaveSpawnPoint : public AActor
{
	GENERATED_BODY()

public:
	ASAWaveSpawnPoint();

protected:
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Syncfire|Wave")
	TObjectPtr<USceneComponent> SceneRoot;
};

// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "SATargetDummy.generated.h"

class UBoxComponent;
class UStaticMeshComponent;
class USceneComponent;
class USAHealthComponent;

UCLASS()
class SYNCFIREARENA_API ASATargetDummy : public AActor
{
	GENERATED_BODY()

public:
	ASATargetDummy();

protected:
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Syncfire|Dummy")
	TObjectPtr<USceneComponent> SceneRoot;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Syncfire|Dummy")
	TObjectPtr<UBoxComponent> CollisionBox;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Syncfire|Dummy")
	TObjectPtr<UStaticMeshComponent> Mesh;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Syncfire|Dummy")
	TObjectPtr<USAHealthComponent> HealthComponent;

	virtual void BeginPlay() override;

private:
	void HandleHealthChanged(float NewHealth, float HealthDelta);
};

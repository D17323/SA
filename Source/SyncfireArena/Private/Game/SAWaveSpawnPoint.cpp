#include "Game/SAWaveSpawnPoint.h"

#include "Components/SceneComponent.h"

ASAWaveSpawnPoint::ASAWaveSpawnPoint()
{
	PrimaryActorTick.bCanEverTick = false;

	SceneRoot = CreateDefaultSubobject<USceneComponent>(TEXT("SceneRoot"));
	SetRootComponent(SceneRoot);
}

#include "Game/SAWaveGameState.h"

#include "Net/UnrealNetwork.h"

ASAWaveGameState::ASAWaveGameState()
{
	PrimaryActorTick.bCanEverTick = false;
}

void ASAWaveGameState::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(ASAWaveGameState, WaveState);
	DOREPLIFETIME(ASAWaveGameState, CurrentWaveNumber);
	DOREPLIFETIME(ASAWaveGameState, CurrentWaveEnemyCount);
	DOREPLIFETIME(ASAWaveGameState, SpawnedEnemyCount);
	DOREPLIFETIME(ASAWaveGameState, AliveEnemyCount);
	DOREPLIFETIME(ASAWaveGameState, TimeUntilNextWave);
}

void ASAWaveGameState::SetWaveState(ESAWaveState NewWaveState)
{
	WaveState = NewWaveState;
}

void ASAWaveGameState::SetCurrentWaveNumber(int32 NewWaveNumber)
{
	CurrentWaveNumber = FMath::Max(0, NewWaveNumber);
}

void ASAWaveGameState::SetCurrentWaveEnemyCount(int32 NewEnemyCount)
{
	CurrentWaveEnemyCount = FMath::Max(0, NewEnemyCount);
}

void ASAWaveGameState::SetSpawnedEnemyCount(int32 NewSpawnedEnemyCount)
{
	SpawnedEnemyCount = FMath::Max(0, NewSpawnedEnemyCount);
}

void ASAWaveGameState::SetAliveEnemyCount(int32 NewAliveEnemyCount)
{
	AliveEnemyCount = FMath::Max(0, NewAliveEnemyCount);
}

void ASAWaveGameState::SetTimeUntilNextWave(float NewTimeUntilNextWave)
{
	TimeUntilNextWave = FMath::Max(0.0f, NewTimeUntilNextWave);
}

void ASAWaveGameState::OnRep_WaveState()
{
}

void ASAWaveGameState::OnRep_WaveCounters()
{
}

void ASAWaveGameState::OnRep_TimeUntilNextWave()
{
}

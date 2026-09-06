#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameStateBase.h"
#include "SAWaveGameState.generated.h"

UENUM(BlueprintType)
enum class ESAWaveState : uint8
{
	WaitingToStart,
	InWave,
	Intermission,
	Completed,
	Failed
};

UCLASS()
class SYNCFIREARENA_API ASAWaveGameState : public AGameStateBase
{
	GENERATED_BODY()

public:
	ASAWaveGameState();

	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	UFUNCTION(BlueprintPure, Category = "Syncfire|Wave")
	ESAWaveState GetWaveState() const { return WaveState; }

	UFUNCTION(BlueprintPure, Category = "Syncfire|Wave")
	int32 GetCurrentWaveNumber() const { return CurrentWaveNumber; }

	UFUNCTION(BlueprintPure, Category = "Syncfire|Wave")
	int32 GetCurrentWaveEnemyCount() const { return CurrentWaveEnemyCount; }

	UFUNCTION(BlueprintPure, Category = "Syncfire|Wave")
	int32 GetSpawnedEnemyCount() const { return SpawnedEnemyCount; }

	UFUNCTION(BlueprintPure, Category = "Syncfire|Wave")
	int32 GetAliveEnemyCount() const { return AliveEnemyCount; }

	UFUNCTION(BlueprintPure, Category = "Syncfire|Wave")
	float GetTimeUntilNextWave() const { return TimeUntilNextWave; }

	void SetWaveState(ESAWaveState NewWaveState);
	void SetCurrentWaveNumber(int32 NewWaveNumber);
	void SetCurrentWaveEnemyCount(int32 NewEnemyCount);
	void SetSpawnedEnemyCount(int32 NewSpawnedEnemyCount);
	void SetAliveEnemyCount(int32 NewAliveEnemyCount);
	void SetTimeUntilNextWave(float NewTimeUntilNextWave);

protected:
	UPROPERTY(ReplicatedUsing = OnRep_WaveState, BlueprintReadOnly, Category = "Syncfire|Wave")
	ESAWaveState WaveState = ESAWaveState::WaitingToStart;

	UPROPERTY(ReplicatedUsing = OnRep_WaveCounters, BlueprintReadOnly, Category = "Syncfire|Wave")
	int32 CurrentWaveNumber = 0;

	UPROPERTY(ReplicatedUsing = OnRep_WaveCounters, BlueprintReadOnly, Category = "Syncfire|Wave")
	int32 CurrentWaveEnemyCount = 0;

	UPROPERTY(ReplicatedUsing = OnRep_WaveCounters, BlueprintReadOnly, Category = "Syncfire|Wave")
	int32 SpawnedEnemyCount = 0;

	UPROPERTY(ReplicatedUsing = OnRep_WaveCounters, BlueprintReadOnly, Category = "Syncfire|Wave")
	int32 AliveEnemyCount = 0;

	UPROPERTY(ReplicatedUsing = OnRep_TimeUntilNextWave, BlueprintReadOnly, Category = "Syncfire|Wave")
	float TimeUntilNextWave = 0.0f;

	UFUNCTION()
	void OnRep_WaveState();

	UFUNCTION()
	void OnRep_WaveCounters();

	UFUNCTION()
	void OnRep_TimeUntilNextWave();
};

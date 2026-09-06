#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameModeBase.h"
#include "TimerManager.h"
#include "SAGameMode.generated.h"

struct FSAWaveRewardOption;
class ASACharacter;
class ASAEnemyCharacter;
class ASAWaveGameState;
class ASAWaveSpawnPoint;
class ASAAmmoPickupActor;
class ASAPlayerState;
class ASAPlayerController;

USTRUCT(BlueprintType)
struct FSAWaveDefinition
{
	GENERATED_BODY()

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Syncfire|Wave", meta = (ClampMin = "1"))
	int32 EnemyCount = 5;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Syncfire|Wave", meta = (ClampMin = "0.01"))
	float SpawnInterval = 1.0f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Syncfire|Wave", meta = (ClampMin = "1"))
	int32 SpawnGroupCount = 1;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Syncfire|Wave", meta = (ClampMin = "0.0"))
	float IntermissionDuration = 5.0f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Syncfire|Wave")
	TSubclassOf<ASAEnemyCharacter> EnemyClass;
};

UCLASS()
class SYNCFIREARENA_API ASAGameMode : public AGameModeBase
{
	GENERATED_BODY()

public:
	ASAGameMode();

	virtual void BeginPlay() override;

	void NotifyEnemyKilled(ASAEnemyCharacter* Enemy);
	void NotifyPlayerDied(ASACharacter* PlayerCharacter);
	void HandlePlayerSelectedWaveReward(ASAPlayerController* PlayerController, int32 RewardIndex);
protected:
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Syncfire|Wave", meta = (ClampMin = "0.0"))
	float InitialWaveDelay = 3.0f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Syncfire|Wave")
	bool bEndlessMode = false;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Syncfire|Wave")
	TSubclassOf<ASAEnemyCharacter> DefaultEnemyClass;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Syncfire|Wave")
	TArray<FSAWaveDefinition> WaveDefinitions;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Syncfire|Drop")
	TSubclassOf<ASAAmmoPickupActor> AmmoDropClass;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Syncfire|Drop", meta = (ClampMin = "0.0"))
	float BaseAmmoDropChance = 0.05f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Syncfire|Drop", meta = (ClampMin = "0.0"))
	float AmmoDropChancePerKill = 0.04f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Syncfire|Drop", meta = (ClampMin = "0.0"))
	float MaxAmmoDropChance = 0.35f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Syncfire|Drop", meta = (ClampMin = "1"))
	int32 AmmoDropAmount = 24;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Syncfire|Score", meta = (ClampMin = "0"))
	int32 BaseEnemyKillScore = 100;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Syncfire|Reward")
	bool bEnableWaveRewards = true;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Syncfire|Reward", meta = (ClampMin = "1"))
	int32 FirstRewardWaveNumber = 3;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Syncfire|Reward", meta = (ClampMin = "1"))
	int32 RewardWaveInterval = 3;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Syncfire|Reward", meta = (ClampMin = "1"))
	int32 RewardOptionCount = 3;
private:
	UPROPERTY()
	TObjectPtr<ASAWaveGameState> CachedWaveGameState;

	UPROPERTY()
	TArray<TObjectPtr<ASAWaveSpawnPoint>> CachedSpawnPoints;

	UPROPERTY()
	TArray<TObjectPtr<ASAWaveSpawnPoint>> ActiveWaveSpawnPoints;

	int32 CurrentWaveIndex = INDEX_NONE;
	int32 EnemiesToSpawnThisWave = 0;
	int32 SpawnedEnemiesThisWave = 0;
	int32 AliveEnemiesThisWave = 0;
	int32 NextSpawnPointIndex = 0;
	TArray<TWeakObjectPtr<ASAEnemyCharacter>> AliveWaveEnemies;
	FSAWaveDefinition ActiveWaveDefinition;
	float CurrentAmmoDropChance = 0.0f;

	FTimerHandle StartWaveTimerHandle;
	FTimerHandle SpawnEnemyTimerHandle;
	FTimerHandle IntermissionCountdownTimerHandle;

	void CacheWaveSpawnPoints();
	FSAWaveDefinition BuildWaveDefinitionForIndex(int32 WaveIndex) const;
	void ResetEnemyDropChance();
	void TrySpawnEnemyAmmoDrop(const ASAEnemyCharacter* Enemy);
	void AwardEnemyKillScore(const ASAEnemyCharacter* Enemy);
	ASACharacter* ResolveScoringCharacter(const ASAEnemyCharacter* Enemy) const;
	ASAPlayerState* ResolveScoringPlayerState(const ASAEnemyCharacter* Enemy) const;
	void StartNextWave();
	void SpawnOneEnemy();
	void FinishCurrentWave();
	void FinishRunAsFailed();
	void BeginIntermission(float Duration);
	void UpdateIntermissionCountdown();
	void SyncWaveStateToGameState();
	bool SelectSpawnPointsForWave();
	bool TryGetWaveSpawnTransform(FTransform& OutSpawnTransform);
	bool HasAnyAlivePlayer() const;
	bool IsPlayerStateAlive(const ASAPlayerState* PlayerState) const;
	
	void BeginWaveRewardSelection();
	void GenerateRewardOptionsForPlayer(ASAPlayerState* PlayerState) const;
	bool AreAllRewardChoicesComplete() const;
	void ApplyWaveReward(ASACharacter* Character, const FSAWaveRewardOption& RewardOption) const;
	
	// 奖池构建
	TArray<FSAWaveRewardOption> BuildRewardPool() const;
};

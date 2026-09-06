// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerState.h"
#include "TimerManager.h"
#include "Game/SAWaveRewardTypes.h"
#include "SAPlayerState.generated.h"


DECLARE_MULTICAST_DELEGATE(FSAOnWaveRewardStateChanged);

UCLASS()
class SYNCFIREARENA_API ASAPlayerState : public APlayerState
{
	GENERATED_BODY()

public:
	ASAPlayerState();

	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	void RegisterEnemyKill(int32 BaseKillScore, float OverrideComboWindow = -1.0f);
	void ResetCombo();

	UFUNCTION(BlueprintPure, Category = "Syncfire|Score")
	int32 GetTotalScore() const { return TotalScore; }

	UFUNCTION(BlueprintPure, Category = "Syncfire|Score")
	int32 GetComboCount() const { return ComboCount; }

	UFUNCTION(BlueprintPure, Category = "Syncfire|Score")
	int32 GetBestComboCount() const { return BestComboCount; }

	UFUNCTION(BlueprintPure, Category = "Syncfire|Score")
	float GetComboMultiplier() const { return ComboMultiplier; }

	UFUNCTION(BlueprintPure, Category = "Syncfire|Score")
	float GetBaseComboWindow() const { return ComboWindow; }

	void SetWaveRewardOptions(const TArray<FSAWaveRewardOption>& InOptions);
	void ClearWaveRewardOptions();
	void MarkWaveRewardSelected();

	bool IsWaitingForWaveReward() const { return bWaitingForWaveReward; }
	bool HasSelectedWaveReward() const { return bHasSelectedWaveReward; }

	const TArray<FSAWaveRewardOption>& GetWaveRewardOptions() const { return WaveRewardOptions; }

	FSAOnWaveRewardStateChanged OnWaveRewardStateChanged;
protected:
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Syncfire|Score", meta = (ClampMin = "0.1"))
	float ComboWindow = 3.0f;

	UPROPERTY(Replicated, VisibleInstanceOnly, BlueprintReadOnly, Category = "Syncfire|Score")
	int32 TotalScore = 0;

	UPROPERTY(Replicated, VisibleInstanceOnly, BlueprintReadOnly, Category = "Syncfire|Score")
	int32 ComboCount = 0;

	UPROPERTY(Replicated, VisibleInstanceOnly, BlueprintReadOnly, Category = "Syncfire|Score")
	int32 BestComboCount = 0;

	UPROPERTY(Replicated, VisibleInstanceOnly, BlueprintReadOnly, Category = "Syncfire|Score")
	float ComboMultiplier = 1.0f;

	UPROPERTY(ReplicatedUsing = OnRep_WaveRewardState, VisibleInstanceOnly, BlueprintReadOnly, Category = "Syncfire|Reward")
	TArray<FSAWaveRewardOption> WaveRewardOptions;

	UPROPERTY(ReplicatedUsing = OnRep_WaveRewardState, VisibleInstanceOnly, BlueprintReadOnly, Category = "Syncfire|Reward")
	bool bWaitingForWaveReward = false;

	UPROPERTY(ReplicatedUsing = OnRep_WaveRewardState, VisibleInstanceOnly, BlueprintReadOnly, Category = "Syncfire|Reward")
	bool bHasSelectedWaveReward = false;

	UFUNCTION()
	void OnRep_WaveRewardState();
private:
	FTimerHandle ComboResetTimerHandle;

	float CalculateComboMultiplier() const;
	void RestartComboTimer(float OverrideComboWindow);
};

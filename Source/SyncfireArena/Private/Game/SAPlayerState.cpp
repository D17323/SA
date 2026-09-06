// Copyright Epic Games, Inc. All Rights Reserved.

#include "Game/SAPlayerState.h"

#include "Engine/World.h"
#include "Net/UnrealNetwork.h"

ASAPlayerState::ASAPlayerState()
{
	bReplicates = true;
}

void ASAPlayerState::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	if (GetWorld())
	{
		GetWorld()->GetTimerManager().ClearTimer(ComboResetTimerHandle);
	}

	Super::EndPlay(EndPlayReason);
}

void ASAPlayerState::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(ASAPlayerState, TotalScore);
	DOREPLIFETIME(ASAPlayerState, ComboCount);
	DOREPLIFETIME(ASAPlayerState, BestComboCount);
	DOREPLIFETIME(ASAPlayerState, ComboMultiplier);
	DOREPLIFETIME(ASAPlayerState, WaveRewardOptions);
	DOREPLIFETIME(ASAPlayerState, bWaitingForWaveReward);
	DOREPLIFETIME(ASAPlayerState, bHasSelectedWaveReward);
}

void ASAPlayerState::RegisterEnemyKill(int32 BaseKillScore, float OverrideComboWindow)
{
	if (!HasAuthority())
	{
		return;
	}

	ComboCount = FMath::Max(1, ComboCount + 1);
	BestComboCount = FMath::Max(BestComboCount, ComboCount);
	ComboMultiplier = CalculateComboMultiplier();

	const int32 ScoreToAdd = FMath::RoundToInt(FMath::Max(0, BaseKillScore) * ComboMultiplier);
	TotalScore += ScoreToAdd;
	SetScore(static_cast<float>(TotalScore));

	RestartComboTimer(OverrideComboWindow);
	ForceNetUpdate();
}

void ASAPlayerState::ResetCombo()
{
	if (!HasAuthority())
	{
		return;
	}

	ComboCount = 0;
	ComboMultiplier = 1.0f;

	if (GetWorld())
	{
		GetWorld()->GetTimerManager().ClearTimer(ComboResetTimerHandle);
	}

	ForceNetUpdate();
}

void ASAPlayerState::SetWaveRewardOptions(const TArray<FSAWaveRewardOption>& InOptions)
{	
	if (!HasAuthority())
	{
		return;
	}
	WaveRewardOptions = InOptions;
	bWaitingForWaveReward = true;
	bHasSelectedWaveReward = false;
	
	// 奖励选项由服务器生成并复制给对应玩家，客户端只负责展示。
	OnWaveRewardStateChanged.Broadcast();
	ForceNetUpdate();
}

void ASAPlayerState::ClearWaveRewardOptions()
{
	if (!HasAuthority())
	{
		return;
	}

	WaveRewardOptions.Reset();
	bWaitingForWaveReward = false;
	bHasSelectedWaveReward = false;

	OnWaveRewardStateChanged.Broadcast();
	ForceNetUpdate();
}

void ASAPlayerState::MarkWaveRewardSelected()
{
	if (!HasAuthority())
	{
		return;
	}

	bWaitingForWaveReward = false;
	bHasSelectedWaveReward = true;

	OnWaveRewardStateChanged.Broadcast();
	ForceNetUpdate();
}

void ASAPlayerState::OnRep_WaveRewardState()
{
	OnWaveRewardStateChanged.Broadcast();
}

float ASAPlayerState::CalculateComboMultiplier() const
{
	if (ComboCount >= 10)
	{
		return 3.0f;
	}

	if (ComboCount >= 6)
	{
		return 2.0f;
	}

	if (ComboCount >= 3)
	{
		return 1.5f;
	}

	return 1.0f;
}

void ASAPlayerState::RestartComboTimer(float OverrideComboWindow)
{
	if (!GetWorld())
	{
		return;
	}

	// 连杀窗口由服务器计时；属性增益只影响这里传入的最终窗口，客户端只接收复制结果。
	const float EffectiveComboWindow = OverrideComboWindow > 0.0f ? OverrideComboWindow : ComboWindow;
	GetWorld()->GetTimerManager().SetTimer(
		ComboResetTimerHandle,
		this,
		&ASAPlayerState::ResetCombo,
		FMath::Max(0.1f, EffectiveComboWindow),
		false);
}

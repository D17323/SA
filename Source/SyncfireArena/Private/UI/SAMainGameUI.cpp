#include "UI/SAMainGameUI.h"

#include "Character/SACharacter.h"
#include "Components/ProgressBar.h"
#include "Components/SAHealthComponent.h"
#include "Components/TextBlock.h"
#include "Engine/World.h"
#include "Game/SAPlayerState.h"
#include "Game/SAWaveGameState.h"
#include "GameFramework/PlayerController.h"
#include "UI/SAAmmoWidget.h"

void USAMainGameUI::NativeConstruct()
{
	Super::NativeConstruct();

	RefreshFromCharacter(nullptr);
}

void USAMainGameUI::RefreshFromCharacter(ASACharacter* InCharacter)
{
	CachedCharacter = InCharacter;

	USAHealthComponent* HealthComponent = CachedCharacter
		? CachedCharacter->FindComponentByClass<USAHealthComponent>()
		: nullptr;

	if (!HealthComponent)
	{
		if (HealthProgressBar)
		{
			HealthProgressBar->SetPercent(0.0f);
		}

		if (HealthText)
		{
			HealthText->SetText(FText::FromString(TEXT("--")));
		}
	}
	else
	{
		const float MaxHealth = FMath::Max(1.0f, HealthComponent->GetMaxHealth());
		const float HealthPercent = FMath::Clamp(HealthComponent->GetHealth() / MaxHealth, 0.0f, 1.0f);

		if (HealthProgressBar)
		{
			HealthProgressBar->SetPercent(HealthPercent);
		}

		if (HealthText)
		{
			HealthText->SetText(FText::AsNumber(FMath::RoundToInt(HealthComponent->GetHealth())));
		}
	}

	if (AmmoWidget)
	{
		AmmoWidget->RefreshFromCharacter(CachedCharacter);
	}

	RefreshWaveDisplay();
	RefreshScoreDisplay();
}

void USAMainGameUI::RefreshWaveDisplay()
{
	const ASAWaveGameState* WaveGameState = GetWorld()
		? GetWorld()->GetGameState<ASAWaveGameState>()
		: nullptr;

	if (!WaveGameState)
	{
		if (WaveStateText)
		{
			WaveStateText->SetText(FText::FromString(TEXT("--")));
		}

		if (WaveNumberText)
		{
			WaveNumberText->SetText(FText::FromString(TEXT("--")));
		}

		if (WaveEnemyText)
		{
			WaveEnemyText->SetText(FText::FromString(TEXT("--")));
		}

		if (WaveCountdownText)
		{
			WaveCountdownText->SetText(FText::GetEmpty());
		}

		return;
	}

	if (WaveStateText)
	{
		WaveStateText->SetText(BuildWaveStateText(WaveGameState));
	}

	if (WaveNumberText)
	{
		const int32 DisplayWaveNumber = FMath::Max(1, WaveGameState->GetCurrentWaveNumber());
		WaveNumberText->SetText(FText::FromString(FString::Printf(TEXT("第 %d 波"), DisplayWaveNumber)));
	}

	if (WaveEnemyText)
	{
		switch (WaveGameState->GetWaveState())
		{
		case ESAWaveState::WaitingToStart:
			WaveEnemyText->SetText(FText::FromString(TEXT("等待刷怪")));
			break;
		case ESAWaveState::Intermission:
			WaveEnemyText->SetText(FText::FromString(TEXT("短暂整备")));
			break;
		case ESAWaveState::Completed:
			WaveEnemyText->SetText(FText::FromString(TEXT("敌人已清除")));
			break;
		case ESAWaveState::Failed:
			WaveEnemyText->SetText(FText::FromString(TEXT("全员阵亡")));
			break;
		case ESAWaveState::InWave:
		default:
			{
				const int32 AliveEnemies = WaveGameState->GetAliveEnemyCount();
				const int32 SpawnedEnemies = WaveGameState->GetSpawnedEnemyCount();
				const int32 TotalEnemies = WaveGameState->GetCurrentWaveEnemyCount();
				WaveEnemyText->SetText(FText::FromString(FString::Printf(TEXT("剩余 %d / %d  已生成 %d"), AliveEnemies, TotalEnemies, SpawnedEnemies)));
			}
			break;
		}
	}

	if (WaveCountdownText)
	{
		const ESAWaveState WaveState = WaveGameState->GetWaveState();
		if (WaveState == ESAWaveState::WaitingToStart || WaveState == ESAWaveState::Intermission)
		{
			const int32 RemainingSeconds = FMath::CeilToInt(WaveGameState->GetTimeUntilNextWave());
			WaveCountdownText->SetText(FText::FromString(FString::Printf(TEXT("下一波 %d 秒"), RemainingSeconds)));
		}
		else
		{
			WaveCountdownText->SetText(FText::GetEmpty());
		}
	}
}

void USAMainGameUI::RefreshScoreDisplay()
{
	const ASAPlayerState* SAPlayerState = GetDisplayPlayerState();
	if (!SAPlayerState)
	{
		if (ScoreText)
		{
			ScoreText->SetText(FText::FromString(TEXT("--")));
		}

		if (ComboText)
		{
			ComboText->SetText(FText::FromString(TEXT("--")));
		}

		if (ComboMultiplierText)
		{
			ComboMultiplierText->SetText(FText::GetEmpty());
		}

		if (BestComboText)
		{
			BestComboText->SetText(FText::GetEmpty());
		}

		return;
	}

	if (ScoreText)
	{
		ScoreText->SetText(FText::FromString(FString::Printf(TEXT("分数 %d"), SAPlayerState->GetTotalScore())));
	}

	if (ComboText)
	{
		ComboText->SetText(FText::FromString(FString::Printf(TEXT("连杀 %d"), SAPlayerState->GetComboCount())));
	}

	if (ComboMultiplierText)
	{
		ComboMultiplierText->SetText(FText::FromString(FString::Printf(TEXT("倍率 x%.1f"), SAPlayerState->GetComboMultiplier())));
	}

	if (BestComboText)
	{
		BestComboText->SetText(FText::FromString(FString::Printf(TEXT("最高连杀 %d"), SAPlayerState->GetBestComboCount())));
	}
}

const ASAPlayerState* USAMainGameUI::GetDisplayPlayerState() const
{
	if (CachedCharacter)
	{
		if (const ASAPlayerState* SAPlayerState = CachedCharacter->GetPlayerState<ASAPlayerState>())
		{
			return SAPlayerState;
		}
	}

	const APlayerController* OwningPlayer = GetOwningPlayer();
	return OwningPlayer ? OwningPlayer->GetPlayerState<ASAPlayerState>() : nullptr;
}

FText USAMainGameUI::BuildWaveStateText(const ASAWaveGameState* WaveGameState) const
{
	if (!WaveGameState)
	{
		return FText::FromString(TEXT("--"));
	}

	switch (WaveGameState->GetWaveState())
	{
	case ESAWaveState::WaitingToStart:
		return FText::FromString(TEXT("准备迎战"));
	case ESAWaveState::InWave:
		return FText::FromString(TEXT("清理敌人"));
	case ESAWaveState::Intermission:
		return FText::FromString(TEXT("短暂整备"));
	case ESAWaveState::Completed:
		return FText::FromString(TEXT("全部波次清除"));
	case ESAWaveState::Failed:
		return FText::FromString(TEXT("行动失败"));
	default:
		return FText::FromString(TEXT("--"));
	}
}

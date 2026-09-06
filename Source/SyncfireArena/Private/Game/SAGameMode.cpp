#include "Game/SAGameMode.h"

#include "AI/SAEnemyCharacter.h"
#include "Character/SACharacter.h"
#include "Components/SAHealthComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "Components/SAStatComponent.h"
#include "Game/SAPlayerController.h"
#include "Game/SAPlayerState.h"
#include "Game/SAWaveGameState.h"
#include "Game/SAWaveSpawnPoint.h"
#include "Pickups/SAAmmoPickupActor.h"
#include "GameFramework/Controller.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/PlayerController.h"
#include "Kismet/GameplayStatics.h"
#include "TimerManager.h"

ASAGameMode::ASAGameMode()
{
	DefaultPawnClass = ASACharacter::StaticClass();
	PlayerControllerClass = ASAPlayerController::StaticClass();
	PlayerStateClass = ASAPlayerState::StaticClass();
	GameStateClass = ASAWaveGameState::StaticClass();

	DefaultEnemyClass = ASAEnemyCharacter::StaticClass();
	AmmoDropClass = ASAAmmoPickupActor::StaticClass();

	FSAWaveDefinition WaveOne;
	WaveOne.EnemyCount = 3;
	WaveOne.SpawnInterval = 1.0f;
	WaveOne.SpawnGroupCount = 1;
	WaveOne.IntermissionDuration = 5.0f;
	WaveDefinitions.Add(WaveOne);

	FSAWaveDefinition WaveTwo;
	WaveTwo.EnemyCount = 5;
	WaveTwo.SpawnInterval = 0.9f;
	WaveTwo.SpawnGroupCount = 2;
	WaveTwo.IntermissionDuration = 6.0f;
	WaveDefinitions.Add(WaveTwo);

	FSAWaveDefinition WaveThree;
	WaveThree.EnemyCount = 8;
	WaveThree.SpawnInterval = 0.75f;
	WaveThree.SpawnGroupCount = 3;
	WaveThree.IntermissionDuration = 0.0f;
	WaveDefinitions.Add(WaveThree);
}

void ASAGameMode::BeginPlay()
{
	Super::BeginPlay();

	CachedWaveGameState = GetWorld() ? GetWorld()->GetGameState<ASAWaveGameState>() : nullptr;
	CacheWaveSpawnPoints();

	CurrentWaveIndex = INDEX_NONE;
	EnemiesToSpawnThisWave = 0;
	SpawnedEnemiesThisWave = 0;
	AliveEnemiesThisWave = 0;
	ResetEnemyDropChance();

	if (CachedWaveGameState)
	{
		CachedWaveGameState->SetWaveState(ESAWaveState::WaitingToStart);
		CachedWaveGameState->SetCurrentWaveNumber(0);
		CachedWaveGameState->SetCurrentWaveEnemyCount(0);
		CachedWaveGameState->SetSpawnedEnemyCount(0);
		CachedWaveGameState->SetAliveEnemyCount(0);
		CachedWaveGameState->SetTimeUntilNextWave(InitialWaveDelay);
	}

	if (WaveDefinitions.IsEmpty() && !bEndlessMode)
	{
		UE_LOG(LogTemp, Warning, TEXT("Wave system stopped: WaveDefinitions is empty."));
		return;
	}

	if (InitialWaveDelay <= 0.0f)
	{
		StartNextWave();
		return;
	}

	GetWorldTimerManager().SetTimer(StartWaveTimerHandle, this, &ASAGameMode::StartNextWave, InitialWaveDelay, false);
	GetWorldTimerManager().SetTimer(IntermissionCountdownTimerHandle, this, &ASAGameMode::UpdateIntermissionCountdown, 0.25f, true);
}

void ASAGameMode::NotifyEnemyKilled(ASAEnemyCharacter* Enemy)
{
	if (!Enemy)
	{
		return;
	}

	const int32 RemovedCount = AliveWaveEnemies.RemoveAll([Enemy](const TWeakObjectPtr<ASAEnemyCharacter>& TrackedEnemy)
	{
		return TrackedEnemy.Get() == Enemy;
	});

	if (RemovedCount <= 0)
	{
		return;
	}

	AliveEnemiesThisWave = AliveWaveEnemies.Num();
	SyncWaveStateToGameState();
	AwardEnemyKillScore(Enemy);

	if (SpawnedEnemiesThisWave >= EnemiesToSpawnThisWave && AliveEnemiesThisWave <= 0)
	{
		FinishCurrentWave();
	}
}

void ASAGameMode::NotifyPlayerDied(ASACharacter* PlayerCharacter)
{
	if (!HasAuthority() || !PlayerCharacter || !PlayerCharacter->IsPlayerControlled())
	{
		return;
	}

	ASAPlayerState* DeadPlayerState = PlayerCharacter->GetPlayerState<ASAPlayerState>();
	const bool bWasWaitingForWaveReward = DeadPlayerState && DeadPlayerState->IsWaitingForWaveReward();
	if (bWasWaitingForWaveReward)
	{
		// 死亡玩家不能继续获得/选择波间奖励，否则多人局会一直等待这个 PlayerState 完成选择。
		DeadPlayerState->MarkWaveRewardSelected();
	}

	if (!HasAnyAlivePlayer())
	{
		FinishRunAsFailed();
		return;
	}

	if (bWasWaitingForWaveReward && AreAllRewardChoicesComplete())
	{
		BeginIntermission(ActiveWaveDefinition.IntermissionDuration);
	}
}

void ASAGameMode::HandlePlayerSelectedWaveReward(ASAPlayerController* PlayerController, int32 RewardIndex)
{
	if (!PlayerController || !PlayerController->HasAuthority())
	{
		return;
	}
	ASAPlayerState* SAPlayerState = PlayerController->GetPlayerState<ASAPlayerState>();
	if (!SAPlayerState || !SAPlayerState->IsWaitingForWaveReward())
	{
		return;
	}

	const TArray<FSAWaveRewardOption>& Options = SAPlayerState->GetWaveRewardOptions();
	if (!Options.IsValidIndex(RewardIndex))
	{
		return;
	}

	ASACharacter* Character = Cast<ASACharacter>(PlayerController->GetPawn());
	if (!Character || Character->IsDead())
	{
		// 死亡玩家不应用奖励，但要结束自己的等待状态，避免房主或客户端死亡后卡住下一波。
		SAPlayerState->MarkWaveRewardSelected();
		if (AreAllRewardChoicesComplete())
		{
			BeginIntermission(ActiveWaveDefinition.IntermissionDuration);
		}
		return;
	}
	
	ApplyWaveReward(Character,Options[RewardIndex]);
	SAPlayerState->MarkWaveRewardSelected();

	if (AreAllRewardChoicesComplete())
	{
		BeginIntermission(ActiveWaveDefinition.IntermissionDuration);
	}
}

FSAWaveDefinition ASAGameMode::BuildWaveDefinitionForIndex(int32 WaveIndex) const
{
	if (WaveDefinitions.IsValidIndex(WaveIndex))
	{
		return WaveDefinitions[WaveIndex];
	}

	FSAWaveDefinition BaseDefinition;
	if (!WaveDefinitions.IsEmpty())
	{
		BaseDefinition = WaveDefinitions.Last();
	}
	else
	{
		BaseDefinition.EnemyCount = 3;
		BaseDefinition.SpawnInterval = 1.0f;
		BaseDefinition.SpawnGroupCount = 1;
		BaseDefinition.IntermissionDuration = 4.0f;
		BaseDefinition.EnemyClass = DefaultEnemyClass;
	}

	const int32 DifficultyStep = FMath::Max(1, WaveIndex - WaveDefinitions.Num() + 1);

	FSAWaveDefinition EndlessDefinition = BaseDefinition;
	EndlessDefinition.EnemyClass = BaseDefinition.EnemyClass ? BaseDefinition.EnemyClass : DefaultEnemyClass;
	EndlessDefinition.EnemyCount = FMath::Max(1, BaseDefinition.EnemyCount + DifficultyStep * 2);
	EndlessDefinition.SpawnInterval = FMath::Max(0.25f, BaseDefinition.SpawnInterval - DifficultyStep * 0.05f);
	EndlessDefinition.SpawnGroupCount = FMath::Max(1, BaseDefinition.SpawnGroupCount);
	EndlessDefinition.IntermissionDuration = FMath::Max(2.5f, BaseDefinition.IntermissionDuration > 0.0f ? BaseDefinition.IntermissionDuration : 4.0f);

	return EndlessDefinition;
}

void ASAGameMode::CacheWaveSpawnPoints()
{
	CachedSpawnPoints.Reset();

	TArray<AActor*> FoundActors;
	UGameplayStatics::GetAllActorsOfClass(this, ASAWaveSpawnPoint::StaticClass(), FoundActors);
	for (AActor* FoundActor : FoundActors)
	{
		if (ASAWaveSpawnPoint* SpawnPoint = Cast<ASAWaveSpawnPoint>(FoundActor))
		{
			CachedSpawnPoints.Add(SpawnPoint);
		}
	}
}

void ASAGameMode::ResetEnemyDropChance()
{
	CurrentAmmoDropChance = FMath::Max(0.0f, BaseAmmoDropChance);
}

void ASAGameMode::TrySpawnEnemyAmmoDrop(const ASAEnemyCharacter* Enemy)
{
	if (!GetWorld() || !Enemy || !AmmoDropClass)
	{
		return;
	}

	const float RollChance = FMath::Clamp(CurrentAmmoDropChance, 0.0f, MaxAmmoDropChance);
	if (RollChance <= 0.0f || FMath::FRand() > RollChance)
	{
		CurrentAmmoDropChance = FMath::Min(MaxAmmoDropChance, CurrentAmmoDropChance + AmmoDropChancePerKill);
		return;
	}

	FActorSpawnParameters SpawnParams;
	SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButAlwaysSpawn;

	// 掉落点只在服务器生成；位置直接取敌人网格组件，避免跟随角色胶囊体中心导致看起来浮高或偏移。
	FVector SpawnLocation = Enemy->GetActorLocation();
	if (USkeletalMeshComponent* EnemyMesh = Enemy->GetMesh())
	{
		SpawnLocation = EnemyMesh->GetComponentLocation();
	}

	ASAAmmoPickupActor* SpawnedPickup = GetWorld()->SpawnActor<ASAAmmoPickupActor>(
		AmmoDropClass,
		SpawnLocation,
		FRotator::ZeroRotator,
		SpawnParams);
	if (SpawnedPickup)
	{
		SpawnedPickup->ConfigurePickup(AmmoDropAmount);
	}

	ResetEnemyDropChance();
}

void ASAGameMode::AwardEnemyKillScore(const ASAEnemyCharacter* Enemy)
{
	ASAPlayerState* ScoringPlayerState = ResolveScoringPlayerState(Enemy);
	if (!ScoringPlayerState)
	{
		return;
	}

	float EffectiveComboWindow = ScoringPlayerState->GetBaseComboWindow();
	if (const ASACharacter* ScoringCharacter = ResolveScoringCharacter(Enemy))
	{
		if (const USAStatComponent* StatComponent = ScoringCharacter->GetStatComponent())
		{
			EffectiveComboWindow = StatComponent->GetFinalComboWindow(EffectiveComboWindow);
		}
	}

	ScoringPlayerState->RegisterEnemyKill(BaseEnemyKillScore, EffectiveComboWindow);
}

ASACharacter* ASAGameMode::ResolveScoringCharacter(const ASAEnemyCharacter* Enemy) const
{
	if (!Enemy)
	{
		return nullptr;
	}

	const USAHealthComponent* EnemyHealth = Enemy->FindComponentByClass<USAHealthComponent>();
	AActor* DamageCauser = EnemyHealth ? EnemyHealth->GetLastDamageCauser() : nullptr;
	if (!DamageCauser)
	{
		return nullptr;
	}

	ASACharacter* KillerCharacter = Cast<ASACharacter>(DamageCauser);
	if (!KillerCharacter)
	{
		KillerCharacter = Cast<ASACharacter>(DamageCauser->GetOwner());
	}

	if (!KillerCharacter)
	{
		if (const AController* KillerController = Cast<AController>(DamageCauser))
		{
			KillerCharacter = Cast<ASACharacter>(KillerController->GetPawn());
		}
	}

	return KillerCharacter && KillerCharacter->IsPlayerControlled() ? KillerCharacter : nullptr;
}

ASAPlayerState* ASAGameMode::ResolveScoringPlayerState(const ASAEnemyCharacter* Enemy) const
{
	if (!Enemy)
	{
		return nullptr;
	}

	const USAHealthComponent* EnemyHealth = Enemy->FindComponentByClass<USAHealthComponent>();
	AActor* DamageCauser = EnemyHealth ? EnemyHealth->GetLastDamageCauser() : nullptr;
	if (!DamageCauser)
	{
		return nullptr;
	}

	AController* KillerController = Cast<AController>(DamageCauser);
	APawn* KillerPawn = Cast<APawn>(DamageCauser);
	if (!KillerPawn)
	{
		KillerPawn = Cast<APawn>(DamageCauser->GetOwner());
	}

	if (!KillerController && KillerPawn)
	{
		KillerController = KillerPawn->GetController();
	}

	if (KillerPawn && !KillerPawn->IsPlayerControlled())
	{
		return nullptr;
	}

	// 击杀归属只由服务器根据最后伤害来源判定，客户端显示复制后的 PlayerState 分数即可。
	if (KillerController)
	{
		return KillerController->GetPlayerState<ASAPlayerState>();
	}

	return KillerPawn ? KillerPawn->GetPlayerState<ASAPlayerState>() : nullptr;
}

void ASAGameMode::StartNextWave()
{
	GetWorldTimerManager().ClearTimer(IntermissionCountdownTimerHandle);
	GetWorldTimerManager().ClearTimer(SpawnEnemyTimerHandle);

	const int32 NextWaveIndex = CurrentWaveIndex + 1;
	if (!bEndlessMode && !WaveDefinitions.IsValidIndex(NextWaveIndex))
	{
		if (CachedWaveGameState)
		{
			CachedWaveGameState->SetWaveState(ESAWaveState::Completed);
			CachedWaveGameState->SetTimeUntilNextWave(0.0f);
		}
		return;
	}

	CurrentWaveIndex = NextWaveIndex;
	ActiveWaveDefinition = BuildWaveDefinitionForIndex(CurrentWaveIndex);
	ResetEnemyDropChance();

	EnemiesToSpawnThisWave = FMath::Max(1, ActiveWaveDefinition.EnemyCount);
	SpawnedEnemiesThisWave = 0;
	AliveEnemiesThisWave = 0;
	AliveWaveEnemies.Reset();
	ActiveWaveSpawnPoints.Reset();
	NextSpawnPointIndex = 0;

	if (!SelectSpawnPointsForWave())
	{
		UE_LOG(LogTemp, Warning, TEXT("Wave %d cannot start: no ASAWaveSpawnPoint placed in the level."), CurrentWaveIndex + 1);

		EnemiesToSpawnThisWave = 0;

		if (CachedWaveGameState)
		{
			CachedWaveGameState->SetWaveState(ESAWaveState::WaitingToStart);
			CachedWaveGameState->SetCurrentWaveNumber(CurrentWaveIndex + 1);
			CachedWaveGameState->SetCurrentWaveEnemyCount(0);
			CachedWaveGameState->SetSpawnedEnemyCount(0);
			CachedWaveGameState->SetAliveEnemyCount(0);
			CachedWaveGameState->SetTimeUntilNextWave(0.0f);
		}

		return;
	}

	if (CachedWaveGameState)
	{
		CachedWaveGameState->SetWaveState(ESAWaveState::InWave);
		CachedWaveGameState->SetCurrentWaveNumber(CurrentWaveIndex + 1);
		CachedWaveGameState->SetCurrentWaveEnemyCount(EnemiesToSpawnThisWave);
		CachedWaveGameState->SetTimeUntilNextWave(0.0f);
	}

	SyncWaveStateToGameState();

	SpawnOneEnemy();

	if (SpawnedEnemiesThisWave < EnemiesToSpawnThisWave)
	{
		GetWorldTimerManager().SetTimer(
			SpawnEnemyTimerHandle,
			this,
			&ASAGameMode::SpawnOneEnemy,
			FMath::Max(0.01f, ActiveWaveDefinition.SpawnInterval),
			true);
	}
}

void ASAGameMode::SpawnOneEnemy()
{
	if (SpawnedEnemiesThisWave >= EnemiesToSpawnThisWave)
	{
		GetWorldTimerManager().ClearTimer(SpawnEnemyTimerHandle);

		if (AliveEnemiesThisWave <= 0)
		{
			FinishCurrentWave();
		}

		return;
	}

	TSubclassOf<ASAEnemyCharacter> EnemyClass = ActiveWaveDefinition.EnemyClass ? ActiveWaveDefinition.EnemyClass : DefaultEnemyClass;
	if (!EnemyClass)
	{
		UE_LOG(LogTemp, Warning, TEXT("Wave %d failed to spawn: EnemyClass is empty."), CurrentWaveIndex + 1);
		++SpawnedEnemiesThisWave;
		SyncWaveStateToGameState();

		if (SpawnedEnemiesThisWave >= EnemiesToSpawnThisWave && AliveEnemiesThisWave <= 0)
		{
			FinishCurrentWave();
		}

		return;
	}

	FActorSpawnParameters SpawnParams;
	SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButAlwaysSpawn;

	FTransform SpawnTransform;
	if (!TryGetWaveSpawnTransform(SpawnTransform))
	{
		UE_LOG(LogTemp, Warning, TEXT("Wave %d stopped spawning: active spawn point is invalid."), CurrentWaveIndex + 1);
		GetWorldTimerManager().ClearTimer(SpawnEnemyTimerHandle);
		return;
	}

	ASAEnemyCharacter* SpawnedEnemy = GetWorld()->SpawnActor<ASAEnemyCharacter>(
		EnemyClass,
		SpawnTransform,
		SpawnParams);

	++SpawnedEnemiesThisWave;

	if (SpawnedEnemy)
	{
		AliveWaveEnemies.Add(SpawnedEnemy);
		AliveEnemiesThisWave = AliveWaveEnemies.Num();
	}
	else
	{
		UE_LOG(LogTemp, Warning, TEXT("Wave %d failed to spawn enemy actor."), CurrentWaveIndex + 1);
	}

	SyncWaveStateToGameState();

	if (SpawnedEnemiesThisWave >= EnemiesToSpawnThisWave)
	{
		GetWorldTimerManager().ClearTimer(SpawnEnemyTimerHandle);

		if (AliveEnemiesThisWave <= 0)
		{
			FinishCurrentWave();
		}
	}
}

void ASAGameMode::FinishCurrentWave()
{
	GetWorldTimerManager().ClearTimer(SpawnEnemyTimerHandle);

	if (!bEndlessMode && !WaveDefinitions.IsValidIndex(CurrentWaveIndex + 1))
	{
		if (CachedWaveGameState)
		{
			CachedWaveGameState->SetWaveState(ESAWaveState::Completed);
			CachedWaveGameState->SetAliveEnemyCount(0);
			CachedWaveGameState->SetTimeUntilNextWave(0.0f);
		}
		return;
	}

	const int32 CompletedWaveNumber = CurrentWaveIndex + 1;
	const int32 SafeFirstRewardWaveNumber = FMath::Max(1, FirstRewardWaveNumber);
	const int32 SafeRewardWaveInterval = FMath::Max(1, RewardWaveInterval);
	const bool bShouldOfferReward =
		bEnableWaveRewards
		&& CompletedWaveNumber >= SafeFirstRewardWaveNumber
		&& ((CompletedWaveNumber - SafeFirstRewardWaveNumber) % SafeRewardWaveInterval == 0);

	if (bShouldOfferReward)
	{
		// 波间奖励只由服务器根据已完成波数决定，再把每个玩家的卡牌选项复制给客户端显示。
		BeginWaveRewardSelection();
		return;
	}
	
	BeginIntermission(ActiveWaveDefinition.IntermissionDuration);
}

void ASAGameMode::FinishRunAsFailed()
{
	GetWorldTimerManager().ClearTimer(StartWaveTimerHandle);
	GetWorldTimerManager().ClearTimer(SpawnEnemyTimerHandle);
	GetWorldTimerManager().ClearTimer(IntermissionCountdownTimerHandle);

	AliveEnemiesThisWave = 0;
	SyncWaveStateToGameState();

	if (GameState)
	{
		for (APlayerState* BasePlayerState : GameState->PlayerArray)
		{
			if (ASAPlayerState* SAPlayerState = Cast<ASAPlayerState>(BasePlayerState))
			{
				SAPlayerState->ClearWaveRewardOptions();
			}
		}
	}

	if (CachedWaveGameState)
	{
		// 全部玩家死亡时由服务器结束波次流程；客户端只通过 GameState 复制更新失败表现。
		CachedWaveGameState->SetWaveState(ESAWaveState::Failed);
		CachedWaveGameState->SetTimeUntilNextWave(0.0f);
	}
}

void ASAGameMode::BeginIntermission(float Duration)
{
	const float SafeDuration = FMath::Max(0.0f, Duration);

	if (CachedWaveGameState)
	{
		CachedWaveGameState->SetWaveState(ESAWaveState::Intermission);
		CachedWaveGameState->SetAliveEnemyCount(0);
		CachedWaveGameState->SetTimeUntilNextWave(SafeDuration);
	}

	if (SafeDuration <= 0.0f)
	{
		StartNextWave();
		return;
	}

	GetWorldTimerManager().SetTimer(StartWaveTimerHandle, this, &ASAGameMode::StartNextWave, SafeDuration, false);
	GetWorldTimerManager().SetTimer(IntermissionCountdownTimerHandle, this, &ASAGameMode::UpdateIntermissionCountdown, 0.25f, true);
}

void ASAGameMode::UpdateIntermissionCountdown()
{
	if (!CachedWaveGameState)
	{
		return;
	}

	const float RemainingTime = GetWorldTimerManager().GetTimerRemaining(StartWaveTimerHandle);
	CachedWaveGameState->SetTimeUntilNextWave(FMath::Max(0.0f, RemainingTime));
}

void ASAGameMode::SyncWaveStateToGameState()
{
	if (!CachedWaveGameState)
	{
		return;
	}

	CachedWaveGameState->SetCurrentWaveEnemyCount(EnemiesToSpawnThisWave);
	CachedWaveGameState->SetSpawnedEnemyCount(SpawnedEnemiesThisWave);
	CachedWaveGameState->SetAliveEnemyCount(AliveEnemiesThisWave);
}

bool ASAGameMode::SelectSpawnPointsForWave()
{
	TArray<ASAWaveSpawnPoint*> ValidSpawnPoints;
	for (const TObjectPtr<ASAWaveSpawnPoint>& SpawnPoint : CachedSpawnPoints)
	{
		if (IsValid(SpawnPoint.Get()))
		{
			ValidSpawnPoints.Add(SpawnPoint.Get());
		}
	}

	if (ValidSpawnPoints.IsEmpty())
	{
		CacheWaveSpawnPoints();

		for (const TObjectPtr<ASAWaveSpawnPoint>& SpawnPoint : CachedSpawnPoints)
		{
			if (IsValid(SpawnPoint.Get()))
			{
				ValidSpawnPoints.Add(SpawnPoint.Get());
			}
		}
	}

	if (ValidSpawnPoints.IsEmpty())
	{
		ActiveWaveSpawnPoints.Reset();
		return false;
	}

	const int32 DesiredGroupCount = FMath::Clamp(ActiveWaveDefinition.SpawnGroupCount, 1, ValidSpawnPoints.Num());
	ActiveWaveSpawnPoints.Reset(DesiredGroupCount);

	while (!ValidSpawnPoints.IsEmpty() && ActiveWaveSpawnPoints.Num() < DesiredGroupCount)
	{
		const int32 SpawnIndex = FMath::RandRange(0, ValidSpawnPoints.Num() - 1);
		ActiveWaveSpawnPoints.Add(ValidSpawnPoints[SpawnIndex]);
		ValidSpawnPoints.RemoveAtSwap(SpawnIndex);
	}

	NextSpawnPointIndex = 0;

	// 刷怪只在服务器执行；本波随机抽取多个出生点后轮流使用，保证多人会话里所有客户端看到一致的出怪节奏。
	UE_LOG(LogTemp, Log, TEXT("Wave %d selected %d spawn point group(s)."), CurrentWaveIndex + 1, ActiveWaveSpawnPoints.Num());
	return true;
}

bool ASAGameMode::TryGetWaveSpawnTransform(FTransform& OutSpawnTransform)
{
	if (ActiveWaveSpawnPoints.IsEmpty())
	{
		return false;
	}

	int32 CheckedCount = 0;
	while (CheckedCount < ActiveWaveSpawnPoints.Num())
	{
		const int32 SpawnPointIndex = NextSpawnPointIndex % ActiveWaveSpawnPoints.Num();
		NextSpawnPointIndex = (NextSpawnPointIndex + 1) % ActiveWaveSpawnPoints.Num();
		++CheckedCount;

		if (ASAWaveSpawnPoint* SpawnPoint = ActiveWaveSpawnPoints[SpawnPointIndex].Get())
		{
			if (IsValid(SpawnPoint))
			{
				OutSpawnTransform = SpawnPoint->GetActorTransform();
				return true;
			}
		}
	}

	return false;
}

bool ASAGameMode::HasAnyAlivePlayer() const
{
	if (!GameState)
	{
		return false;
	}

	for (APlayerState* BasePlayerState : GameState->PlayerArray)
	{
		if (IsPlayerStateAlive(Cast<ASAPlayerState>(BasePlayerState)))
		{
			return true;
		}
	}

	return false;
}

bool ASAGameMode::IsPlayerStateAlive(const ASAPlayerState* PlayerState) const
{
	if (!PlayerState)
	{
		return false;
	}

	if (const UWorld* World = GetWorld())
	{
		for (FConstPlayerControllerIterator It = World->GetPlayerControllerIterator(); It; ++It)
		{
			const APlayerController* PlayerController = It->Get();
			if (!PlayerController || PlayerController->PlayerState != PlayerState)
			{
				continue;
			}

			const ASACharacter* Character = Cast<ASACharacter>(PlayerController->GetPawn());
			return Character && !Character->IsDead();
		}
	}

	const AController* OwnerController = Cast<AController>(PlayerState->GetOwner());
	const ASACharacter* Character = OwnerController ? Cast<ASACharacter>(OwnerController->GetPawn()) : nullptr;
	return Character && !Character->IsDead();
}

void ASAGameMode::BeginWaveRewardSelection()
{
	if (!GameState)
	{
		BeginIntermission(ActiveWaveDefinition.IntermissionDuration);
		return;
	}

	bool bAnyPlayerNeedsReward = false;

	for (APlayerState* BasePlayerState : GameState->PlayerArray)
	{
		ASAPlayerState* SAPlayerState = Cast<ASAPlayerState>(BasePlayerState);
		if (!SAPlayerState)
		{
			continue;
		}

		if (!IsPlayerStateAlive(SAPlayerState))
		{
			// 死亡玩家跳过本次选卡，避免奖励界面等待无法操作的 Pawn。
			SAPlayerState->ClearWaveRewardOptions();
			continue;
		}

		GenerateRewardOptionsForPlayer(SAPlayerState);
		bAnyPlayerNeedsReward = true;
	}

	if (CachedWaveGameState)
	{
		CachedWaveGameState->SetWaveState(ESAWaveState::Intermission);
		CachedWaveGameState->SetAliveEnemyCount(0);
		CachedWaveGameState->SetTimeUntilNextWave(0.0f);
	}

	if (!bAnyPlayerNeedsReward)
	{
		BeginIntermission(ActiveWaveDefinition.IntermissionDuration);
	}
}

void ASAGameMode::GenerateRewardOptionsForPlayer(ASAPlayerState* PlayerState) const
{
	if (!PlayerState)
	{
		return;
	}
	TArray<FSAWaveRewardOption> Pool = BuildRewardPool();
	TArray<FSAWaveRewardOption> Options;
	
	// 保证最小选项也至少有1个
	const int32 SafeOptionCount = FMath::Max(1, RewardOptionCount);
	while (!Pool.IsEmpty() && Options.Num() < SafeOptionCount)
	{
		// 奖池中随机抽卡
		const int32 PickIndex = FMath::RandRange(0, Pool.Num() - 1);
		Options.Add(Pool[PickIndex]);
		Pool.RemoveAtSwap(PickIndex);
	}

	PlayerState->SetWaveRewardOptions(Options);
}

bool ASAGameMode::AreAllRewardChoicesComplete() const
{
	if (!GameState)
	{
		return true;
	}

	for (APlayerState* BasePlayerState : GameState->PlayerArray)
	{
		const ASAPlayerState* SAPlayerState = Cast<ASAPlayerState>(BasePlayerState);
		if (SAPlayerState && IsPlayerStateAlive(SAPlayerState) && SAPlayerState->IsWaitingForWaveReward())
		{
			return false;
		}
	}

	return true;
}

void ASAGameMode::ApplyWaveReward(ASACharacter* Character, const FSAWaveRewardOption& RewardOption) const
{
	if (!Character)
	{
		return;
	}

	USAStatComponent* StatComponent = Character->GetStatComponent();
	if (!StatComponent)
	{
		return;
	}

	// 增益实际应用只在服务器发生；StatComponent 复制最终数值给客户端用于显示和表现。
	StatComponent->AddStatValue(RewardOption.StatType, RewardOption.DeltaValue);
}

TArray<FSAWaveRewardOption> ASAGameMode::BuildRewardPool() const
{
	TArray<FSAWaveRewardOption> Rewards;

	Rewards.Add({ ESAStatType::DamageMultiplier, 0.10f });
	Rewards.Add({ ESAStatType::FireRateMultiplier, 0.10f });
	Rewards.Add({ ESAStatType::ReloadSpeedMultiplier, 0.15f });
	Rewards.Add({ ESAStatType::MoveSpeedMultiplier, 0.08f });
	Rewards.Add({ ESAStatType::MaxHealthBonus, 20.0f });
	Rewards.Add({ ESAStatType::ComboWindowBonus, 1.0f });

	return Rewards;
}

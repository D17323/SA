// Copyright Epic Games, Inc. All Rights Reserved.

#include "AI/SAEnemyAIController.h"

#include "AI/SAEnemyCharacter.h"
#include "Character/SACharacter.h"
#include "Components/SAHealthComponent.h"
#include "EngineUtils.h"
#include "Navigation/PathFollowingComponent.h"
#include "Perception/AISense.h"
#include "Perception/AISense_Damage.h"
#include "Perception/AISense_Hearing.h"
#include "Perception/AISense_Sight.h"
#include "BehaviorTree/BlackboardComponent.h"
#include "TimerManager.h"
#include "BehaviorTree/BehaviorTree.h"

ASAEnemyAIController::ASAEnemyAIController()
{
	PrimaryActorTick.bCanEverTick = false;

	// 当前 Demo 阶段敌人只通过行为树服务选择最近玩家，不再创建视觉/听觉/伤害感知组件。
	// HandleTargetPerceptionUpdated 保留在代码中，后续如果恢复感知系统可以重新接回。
}

void ASAEnemyAIController::OnPossess(APawn* InPawn)
{
	Super::OnPossess(InPawn);

	ControlledEnemy = Cast<ASAEnemyCharacter>(InPawn);
	if (!HasAuthority() || !ControlledEnemy)
	{
		return;
	}

	// AI 决策只在服务器执行；客户端只接收服务器复制来的移动结果。
	if (BehaviorTreeAsset)
	{
		// 配置行为树后，服务器由 BT 接管寻敌、追击和攻击。
		RunBehaviorTree(BehaviorTreeAsset);

		if (UBlackboardComponent* BlackboardComponent = GetBlackboardComponent())
		{
			// 简化 AI 后不再使用“最后目击点/听觉点”分支，进入行为树时先清空旧状态。
			BlackboardComponent->ClearValue(TargetActorKeyName);
			BlackboardComponent->ClearValue(LastKnownLocationKeyName);
			BlackboardComponent->ClearValue(HeardLocationKeyName);
			BlackboardComponent->SetValueAsBool(HasVisualTargetKeyName, false);
			BlackboardComponent->SetValueAsBool(HasLastKnownLocationKeyName, false);
			BlackboardComponent->SetValueAsBool(HasHeardLocationKeyName, false);
		}

		return;
	}

	UpdateTarget();
	GetWorldTimerManager().SetTimer(TargetSearchTimerHandle, this, &ASAEnemyAIController::UpdateTarget, TargetSearchInterval, true);
}

void ASAEnemyAIController::OnUnPossess()
{
	GetWorldTimerManager().ClearTimer(TargetSearchTimerHandle);
	ResetMoveRequestCache();
	ControlledEnemy = nullptr;
	CurrentTarget = nullptr;

	Super::OnUnPossess();
}

void ASAEnemyAIController::HandleTargetPerceptionUpdated(AActor* Actor, FAIStimulus Stimulus)
{
	if (!HasAuthority())
	{
		return;
	}

	UBlackboardComponent* BlackboardComponent = GetBlackboardComponent();
	if (!BlackboardComponent)
	{
		return;
	}

	const FAISenseID SightSenseID = UAISense::GetSenseID(UAISense_Sight::StaticClass());
	const FAISenseID HearingSenseID = UAISense::GetSenseID(UAISense_Hearing::StaticClass());
	const FAISenseID DamageSenseID = UAISense::GetSenseID(UAISense_Damage::StaticClass());
	const bool bSuccessfullySensed = Stimulus.WasSuccessfullySensed();

	if (Stimulus.Type == SightSenseID)
	{
		ASACharacter* PlayerCharacter = Cast<ASACharacter>(Actor);
		if (!PlayerCharacter || PlayerCharacter->IsDead())
		{
			return;
		}

		if (bSuccessfullySensed)
		{
			// 视觉优先级最高：看到玩家后直接进入追击/攻击分支，并清掉旧的调查位置。
			BlackboardComponent->SetValueAsObject(TargetActorKeyName, PlayerCharacter);
			BlackboardComponent->SetValueAsVector(LastKnownLocationKeyName, PlayerCharacter->GetActorLocation());
			BlackboardComponent->SetValueAsBool(HasVisualTargetKeyName, true);
			BlackboardComponent->SetValueAsBool(HasLastKnownLocationKeyName, false);
			BlackboardComponent->SetValueAsBool(HasHeardLocationKeyName, false);
		}
		else
		{
			// 失去视野后不再保留直接攻击目标，但记录最后一次看到的位置。
			if (BlackboardComponent->GetValueAsObject(TargetActorKeyName) == PlayerCharacter)
			{
				BlackboardComponent->ClearValue(TargetActorKeyName);
			}

			BlackboardComponent->SetValueAsBool(HasVisualTargetKeyName, false);
			BlackboardComponent->SetValueAsVector(LastKnownLocationKeyName, Stimulus.StimulusLocation);
			BlackboardComponent->SetValueAsBool(HasLastKnownLocationKeyName, true);
		}

		return;
	}

	if (Stimulus.Type == HearingSenseID)
	{
		if (bSuccessfullySensed)
		{
			// 听到声音只得到位置，不直接得到玩家 Actor，避免丧尸隔墙精准锁定目标。
			BlackboardComponent->SetValueAsVector(HeardLocationKeyName, Stimulus.StimulusLocation);
			BlackboardComponent->SetValueAsBool(HasHeardLocationKeyName, true);
		}

		return;
	}

	if (Stimulus.Type == DamageSenseID && bSuccessfullySensed)
	{
		// 受伤后立刻前往受击位置；如果感知事件带有攻击者，则暂存为目标，之后由 Sight 决定是否进入攻击分支。
		if (ASACharacter* Attacker = Cast<ASACharacter>(Actor))
		{
			if (!Attacker->IsDead())
			{
				BlackboardComponent->SetValueAsObject(TargetActorKeyName, Attacker);
			}
		}

		BlackboardComponent->SetValueAsVector(LastKnownLocationKeyName, Stimulus.StimulusLocation);
		BlackboardComponent->SetValueAsBool(HasLastKnownLocationKeyName, true);
		BlackboardComponent->SetValueAsBool(HasHeardLocationKeyName, false);
	}
}

void ASAEnemyAIController::UpdateTarget()
{
	if (!HasAuthority() || !ControlledEnemy || ControlledEnemy->IsDead())
	{
		StopMovement();
		return;
	}

	CurrentTarget = FindNearestPlayerCharacter();
	if (!CurrentTarget)
	{
		StopMovement();
		ResetMoveRequestCache();
		return;
	}

	if (IsTargetInAttackRange(CurrentTarget))
	{
		StopMovement();
		ResetMoveRequestCache();
		TryAttackTarget();
		return;
	}

	RequestMoveToTarget(CurrentTarget);
}

ASACharacter* ASAEnemyAIController::FindNearestPlayerCharacter() const
{
	if (!ControlledEnemy || !GetWorld())
	{
		return nullptr;
	}

	ASACharacter* NearestCharacter = nullptr;
	float NearestDistanceSq = TNumericLimits<float>::Max();
	const FVector EnemyLocation = ControlledEnemy->GetActorLocation();

	auto ConsiderCandidate = [&](ASACharacter* Candidate)
	{
		if (!Candidate || Candidate->IsDead())
		{
			return;
		}

		const float DistanceSq = FVector::DistSquared(EnemyLocation, Candidate->GetActorLocation());
		if (TargetSearchRadius > 0.0f && DistanceSq > FMath::Square(TargetSearchRadius))
		{
			return;
		}

		if (DistanceSq < NearestDistanceSq)
		{
			NearestDistanceSq = DistanceSq;
			NearestCharacter = Candidate;
		}
	};

	for (TActorIterator<ASACharacter> It(GetWorld()); It; ++It)
	{
		ConsiderCandidate(*It);
	}

	return NearestCharacter;
}

bool ASAEnemyAIController::IsTargetInAttackRange(const ASACharacter* Target) const
{
	if (!ControlledEnemy || !Target)
	{
		return false;
	}

	const float AttackRangeSq = FMath::Square(AttackRange);
	return FVector::DistSquared2D(ControlledEnemy->GetActorLocation(), Target->GetActorLocation()) <= AttackRangeSq;
}

bool ASAEnemyAIController::ShouldRequestMoveToTarget(const ASACharacter* Target) const
{
	if (!Target)
	{
		return false;
	}

	const float CurrentTime = GetWorld() ? GetWorld()->GetTimeSeconds() : 0.0f;
	if (LastMoveTarget.Get() != Target)
	{
		return true;
	}

	if (CurrentTime - LastMoveRequestTime < MinMoveRequestInterval)
	{
		return false;
	}

	const float RepathDistanceSq = FMath::Square(MoveRepathDistance);
	return FVector::DistSquared2D(LastMoveGoalLocation, Target->GetActorLocation()) > RepathDistanceSq;
}

void ASAEnemyAIController::RequestMoveToTarget(ASACharacter* Target)
{
	if (!Target || !ShouldRequestMoveToTarget(Target))
	{
		return;
	}

	const FVector TargetLocation = Target->GetActorLocation();

	/* 使用位置快照而不是 MoveToActor 持续跟踪移动目标；目标移动超过阈值后才重新寻路。 */
	const EPathFollowingRequestResult::Type RequestResult = MoveToLocation(
		TargetLocation,
		AcceptanceRadius,
		true,
		true,
		true,
		true,
		nullptr,
		true);
	if (RequestResult != EPathFollowingRequestResult::Failed)
	{
		LastMoveTarget = Target;
		LastMoveGoalLocation = TargetLocation;
		LastMoveRequestTime = GetWorld() ? GetWorld()->GetTimeSeconds() : 0.0f;
	}
}

void ASAEnemyAIController::ResetMoveRequestCache()
{
	LastMoveTarget = nullptr;
	LastMoveGoalLocation = FVector::ZeroVector;
	LastMoveRequestTime = -1000.0f;
}

void ASAEnemyAIController::TryAttackTarget()
{
	if (!HasAuthority() || !ControlledEnemy || ControlledEnemy->IsDead() || !CurrentTarget || CurrentTarget->IsDead())
	{
		return;
	}

	const float CurrentTime = GetWorld() ? GetWorld()->GetTimeSeconds() : 0.0f;
	if (CurrentTime - LastAttackTime < AttackInterval)
	{
		return;
	}

	if (bRequireLineOfSight && !LineOfSightTo(CurrentTarget))
	{
		return;
	}

	LastAttackTime = CurrentTime;
	ControlledEnemy->PlayAttackAnimation(CurrentTarget);

	if (USAHealthComponent* TargetHealth = CurrentTarget->FindComponentByClass<USAHealthComponent>())
	{
		// AI 伤害只在服务器执行；玩家血量变化通过 Health RepNotify 同步到各客户端。
		TargetHealth->ApplyDamage(AttackDamage, ControlledEnemy);
	}
}

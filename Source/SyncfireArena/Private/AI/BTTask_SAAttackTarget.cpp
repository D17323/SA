// Copyright Epic Games, Inc. All Rights Reserved.

#include "AI/BTTask_SAAttackTarget.h"

#include "AIController.h"
#include "AI/SAEnemyCharacter.h"
#include "Character/SACharacter.h"
#include "Components/SAHealthComponent.h"
#include "BehaviorTree/BehaviorTreeComponent.h"
#include "BehaviorTree/BlackboardComponent.h"

UBTTask_SAAttackTarget::UBTTask_SAAttackTarget()
{
	NodeName = TEXT("Attack Target");

	// 行为树节点默认可能被多个 AI 共享；这里需要每个 AI 有自己的攻击冷却时间。
	bCreateNodeInstance = true;

	TargetActorKey.AddObjectFilter(this, GET_MEMBER_NAME_CHECKED(UBTTask_SAAttackTarget, TargetActorKey), AActor::StaticClass());
}

EBTNodeResult::Type UBTTask_SAAttackTarget::ExecuteTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory)
{
	AAIController* AIController = OwnerComp.GetAIOwner();
	APawn* ControlledPawn = AIController ? AIController->GetPawn() : nullptr;
	UBlackboardComponent* BlackboardComponent = OwnerComp.GetBlackboardComponent();
	if (!AIController || !AIController->HasAuthority() || !ControlledPawn || !BlackboardComponent)
	{
		return EBTNodeResult::Failed;
	}

	ASACharacter* TargetCharacter = Cast<ASACharacter>(BlackboardComponent->GetValueAsObject(TargetActorKey.SelectedKeyName));
	if (!TargetCharacter || TargetCharacter->IsDead())
	{
		return EBTNodeResult::Failed;
	}

	const float AttackRangeSq = FMath::Square(AttackRange);
	if (FVector::DistSquared2D(ControlledPawn->GetActorLocation(), TargetCharacter->GetActorLocation()) > AttackRangeSq)
	{
		return EBTNodeResult::Failed;
	}

	if (bRequireLineOfSight && !AIController->LineOfSightTo(TargetCharacter))
	{
		// 目标虽然在攻击距离内，但被墙体等阻挡时不能直接造成伤害。
		return EBTNodeResult::Failed;
	}

	const float CurrentTime = ControlledPawn->GetWorld() ? ControlledPawn->GetWorld()->GetTimeSeconds() : 0.0f;
	if (CurrentTime - LastAttackTime < AttackInterval)
	{
		// 冷却中返回成功，避免 Selector 立刻切到 MoveTo 导致近距离抖动。
		return EBTNodeResult::Succeeded;
	}

	LastAttackTime = CurrentTime;
	AIController->StopMovement();

	if (ASAEnemyCharacter* EnemyCharacter = Cast<ASAEnemyCharacter>(ControlledPawn))
	{
		// 行为树只在服务器运行；攻击表现由服务器确认后 Multicast 给所有客户端播放。
		EnemyCharacter->PlayAttackAnimation(TargetCharacter);
	}

	if (USAHealthComponent* TargetHealth = TargetCharacter->FindComponentByClass<USAHealthComponent>())
	{
		// 行为树只发起攻击意图；真正血量修改仍然只在服务器执行。
		TargetHealth->ApplyDamage(AttackDamage, ControlledPawn);
		return EBTNodeResult::Succeeded;
	}

	return EBTNodeResult::Failed;
}

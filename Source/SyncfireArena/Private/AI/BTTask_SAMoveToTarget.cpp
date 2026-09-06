// Copyright Epic Games, Inc. All Rights Reserved.

#include "AI/BTTask_SAMoveToTarget.h"

#include "AIController.h"
#include "BehaviorTree/BehaviorTreeComponent.h"
#include "BehaviorTree/BlackboardComponent.h"
#include "Navigation/PathFollowingComponent.h"

UBTTask_SAMoveToTarget::UBTTask_SAMoveToTarget()
{
	NodeName = TEXT("Move To Target Throttled");

	/* 任务节点默认可能被多个 AI 共享；这里需要每个 AI 独立记录上一次 MoveTo 请求。 */
	bCreateNodeInstance = true;

	TargetActorKey.AddObjectFilter(this, GET_MEMBER_NAME_CHECKED(UBTTask_SAMoveToTarget, TargetActorKey), AActor::StaticClass());
}

EBTNodeResult::Type UBTTask_SAMoveToTarget::ExecuteTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory)
{
	AAIController* AIController = OwnerComp.GetAIOwner();
	APawn* ControlledPawn = AIController ? AIController->GetPawn() : nullptr;
	UBlackboardComponent* BlackboardComponent = OwnerComp.GetBlackboardComponent();
	if (!AIController || !AIController->HasAuthority() || !ControlledPawn || !BlackboardComponent)
	{
		return EBTNodeResult::Failed;
	}

	AActor* TargetActor = Cast<AActor>(BlackboardComponent->GetValueAsObject(TargetActorKey.SelectedKeyName));
	if (!TargetActor)
	{
		AIController->StopMovement();
		LastMoveTarget = nullptr;
		return EBTNodeResult::Failed;
	}

	const FVector TargetLocation = TargetActor->GetActorLocation();
	const float AcceptanceRadiusSq = FMath::Square(AcceptanceRadius);
	if (FVector::DistSquared2D(ControlledPawn->GetActorLocation(), TargetLocation) <= AcceptanceRadiusSq)
	{
		AIController->StopMovement();
		return EBTNodeResult::Succeeded;
	}

	const float CurrentTime = ControlledPawn->GetWorld() ? ControlledPawn->GetWorld()->GetTimeSeconds() : 0.0f;
	const bool bSameTarget = LastMoveTarget.Get() == TargetActor;
	const bool bTargetMovedEnough = FVector::DistSquared2D(LastMoveGoalLocation, TargetLocation) > FMath::Square(RepathDistance);
	const bool bCanRequestByInterval = CurrentTime - LastMoveRequestTime >= MinMoveRequestInterval;

	if (bSameTarget && !bTargetMovedEnough)
	{
		return EBTNodeResult::Succeeded;
	}

	if (bSameTarget && !bCanRequestByInterval)
	{
		return EBTNodeResult::Succeeded;
	}

	/* 使用位置快照而不是持续跟踪 Actor；玩家移动超过阈值后再由本任务发起新的 MoveTo。 */
	const EPathFollowingRequestResult::Type RequestResult = AIController->MoveToLocation(
		TargetLocation,
		AcceptanceRadius,
		true,
		true,
		true,
		true,
		nullptr,
		true);

	if (RequestResult == EPathFollowingRequestResult::Failed)
	{
		return EBTNodeResult::Failed;
	}

	LastMoveTarget = TargetActor;
	LastMoveGoalLocation = TargetLocation;
	LastMoveRequestTime = CurrentTime;

	return EBTNodeResult::Succeeded;
}

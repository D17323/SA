// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "BehaviorTree/BTTaskNode.h"
#include "BehaviorTree/Blackboard/BlackboardKeyType_Object.h"
#include "BTTask_SAMoveToTarget.generated.h"

UCLASS()
class SYNCFIREARENA_API UBTTask_SAMoveToTarget : public UBTTaskNode
{
	GENERATED_BODY()

public:
	UBTTask_SAMoveToTarget();

protected:
	virtual EBTNodeResult::Type ExecuteTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory) override;

	UPROPERTY(EditAnywhere, Category = "Blackboard")
	FBlackboardKeySelector TargetActorKey;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Syncfire|Move", meta = (ClampMin = "0.0"))
	float AcceptanceRadius = 120.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Syncfire|Move", meta = (ClampMin = "0.0"))
	float RepathDistance = 250.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Syncfire|Move", meta = (ClampMin = "0.0"))
	float MinMoveRequestInterval = 0.25f;

private:
	UPROPERTY()
	TObjectPtr<AActor> LastMoveTarget;

	FVector LastMoveGoalLocation = FVector::ZeroVector;
	float LastMoveRequestTime = -1000.0f;
};

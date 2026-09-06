// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "BehaviorTree/BTService.h"
#include "BehaviorTree/Blackboard/BlackboardKeyType_Object.h"
#include "BTService_SAFindNearestPlayer.generated.h"

UCLASS()
class SYNCFIREARENA_API UBTService_SAFindNearestPlayer : public UBTService
{
	GENERATED_BODY()

public:
	UBTService_SAFindNearestPlayer();

protected:
	virtual void OnBecomeRelevant(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory) override;
	virtual void TickNode(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory, float DeltaSeconds) override;

	// 黑板里保存当前目标玩家的 Object Key，建议命名为 TargetActor。
	UPROPERTY(EditAnywhere, Category = "Blackboard")
	FBlackboardKeySelector TargetActorKey;

	UPROPERTY(EditAnywhere, Category = "Blackboard")
	FBlackboardKeySelector LastKnownLocationKey;

	UPROPERTY(EditAnywhere, Category = "Blackboard")
	FBlackboardKeySelector HasVisualTargetKey;
};

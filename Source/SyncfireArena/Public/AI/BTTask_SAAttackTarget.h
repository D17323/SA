// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "BehaviorTree/BTTaskNode.h"
#include "BehaviorTree/Blackboard/BlackboardKeyType_Object.h"
#include "BTTask_SAAttackTarget.generated.h"

UCLASS()
class SYNCFIREARENA_API UBTTask_SAAttackTarget : public UBTTaskNode
{
	GENERATED_BODY()

public:
	UBTTask_SAAttackTarget();

protected:
	virtual EBTNodeResult::Type ExecuteTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory) override;

	UPROPERTY(EditAnywhere, Category = "Blackboard")
	FBlackboardKeySelector TargetActorKey;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Syncfire|Attack")
	float AttackRange = 160.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Syncfire|Attack")
	float AttackDamage = 10.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Syncfire|Attack")
	float AttackInterval = 1.0f;

	// 开启后，服务端只有在敌人能直接看到玩家时才会造成近战攻击伤害。
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Syncfire|Attack")
	bool bRequireLineOfSight = true;

private:
	float LastAttackTime = -1000.0f;
};

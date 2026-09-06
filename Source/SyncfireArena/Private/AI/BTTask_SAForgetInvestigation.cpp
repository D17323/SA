// Copyright Epic Games, Inc. All Rights Reserved.

#include "AI/BTTask_SAForgetInvestigation.h"

#include "AIController.h"
#include "BehaviorTree/BehaviorTreeComponent.h"
#include "BehaviorTree/BlackboardComponent.h"

UBTTask_SAForgetInvestigation::UBTTask_SAForgetInvestigation()
{
	NodeName = TEXT("Forget Investigation");

	TargetActorKey.AddObjectFilter(this, GET_MEMBER_NAME_CHECKED(UBTTask_SAForgetInvestigation, TargetActorKey), AActor::StaticClass());
	HeardLocationKey.AddVectorFilter(this, GET_MEMBER_NAME_CHECKED(UBTTask_SAForgetInvestigation, HeardLocationKey));
	LastKnownLocationKey.AddVectorFilter(this, GET_MEMBER_NAME_CHECKED(UBTTask_SAForgetInvestigation, LastKnownLocationKey));
	HasHeardLocationKey.AddBoolFilter(this, GET_MEMBER_NAME_CHECKED(UBTTask_SAForgetInvestigation, HasHeardLocationKey));
	HasLastKnownLocationKey.AddBoolFilter(this, GET_MEMBER_NAME_CHECKED(UBTTask_SAForgetInvestigation, HasLastKnownLocationKey));
}

EBTNodeResult::Type UBTTask_SAForgetInvestigation::ExecuteTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory)
{
	AAIController* AIController = OwnerComp.GetAIOwner();
	UBlackboardComponent* BlackboardComponent = OwnerComp.GetBlackboardComponent();
	if (!AIController || !AIController->HasAuthority() || !BlackboardComponent)
	{
		return EBTNodeResult::Failed;
	}

	// 调查结束后清理临时感知信息；新的视觉、听觉或伤害事件仍然可以重新写入这些 Key。
	BlackboardComponent->ClearValue(TargetActorKey.SelectedKeyName);
	BlackboardComponent->ClearValue(HeardLocationKey.SelectedKeyName);
	BlackboardComponent->ClearValue(LastKnownLocationKey.SelectedKeyName);
	BlackboardComponent->SetValueAsBool(HasHeardLocationKey.SelectedKeyName, false);
	BlackboardComponent->SetValueAsBool(HasLastKnownLocationKey.SelectedKeyName, false);

	return EBTNodeResult::Succeeded;
}

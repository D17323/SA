// Copyright Epic Games, Inc. All Rights Reserved.

#include "AI/BTService_SAFindNearestPlayer.h"

#include "AIController.h"
#include "Character/SACharacter.h"
#include "BehaviorTree/BehaviorTreeComponent.h"
#include "BehaviorTree/BlackboardComponent.h"
#include "EngineUtils.h"

UBTService_SAFindNearestPlayer::UBTService_SAFindNearestPlayer()
{
	NodeName = TEXT("Find Nearest Player");
	Interval = 0.5f;
	RandomDeviation = 0.1f;
	bNotifyBecomeRelevant = true;

	TargetActorKey.AddObjectFilter(this, GET_MEMBER_NAME_CHECKED(UBTService_SAFindNearestPlayer, TargetActorKey), AActor::StaticClass());
	LastKnownLocationKey.AddVectorFilter(this, GET_MEMBER_NAME_CHECKED(UBTService_SAFindNearestPlayer, LastKnownLocationKey));
	HasVisualTargetKey.AddBoolFilter(this, GET_MEMBER_NAME_CHECKED(UBTService_SAFindNearestPlayer, HasVisualTargetKey));
}

void UBTService_SAFindNearestPlayer::OnBecomeRelevant(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory)
{
	Super::OnBecomeRelevant(OwnerComp, NodeMemory);

	TickNode(OwnerComp, NodeMemory, 0.0f);
}

void UBTService_SAFindNearestPlayer::TickNode(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory, float DeltaSeconds)
{
	Super::TickNode(OwnerComp, NodeMemory, DeltaSeconds);

	AAIController* AIController = OwnerComp.GetAIOwner();
	APawn* ControlledPawn = AIController ? AIController->GetPawn() : nullptr;
	UBlackboardComponent* BlackboardComponent = OwnerComp.GetBlackboardComponent();
	if (!AIController || !AIController->HasAuthority() || !ControlledPawn || !BlackboardComponent)
	{
		return;
	}

	ASACharacter* NearestCharacter = nullptr;
	float NearestDistanceSq = TNumericLimits<float>::Max();
	const FVector EnemyLocation = ControlledPawn->GetActorLocation();

	auto ConsiderCandidate = [&](ASACharacter* Candidate)
	{
		if (!Candidate || Candidate->IsDead() || !Candidate->IsPlayerControlled())
		{
			return;
		}

		const float DistanceSq = FVector::DistSquared(EnemyLocation, Candidate->GetActorLocation());
		if (DistanceSq < NearestDistanceSq)
		{
			NearestDistanceSq = DistanceSq;
			NearestCharacter = Candidate;
		}
	};

	if (UWorld* World = ControlledPawn->GetWorld())
	{
		for (TActorIterator<ASACharacter> It(World); It; ++It)
		{
			ConsiderCandidate(*It);
		}
	}

	if (NearestCharacter)
	{
		// 行为树只在服务器选择目标；客户端接收服务器复制的移动和攻击结果。
		// 这里保留 HasVisualTarget Key，是为了兼容旧行为树装饰器，当前语义等价于 HasTarget。
		BlackboardComponent->SetValueAsObject(TargetActorKey.SelectedKeyName, NearestCharacter);
		BlackboardComponent->SetValueAsVector(LastKnownLocationKey.SelectedKeyName, NearestCharacter->GetActorLocation());
		BlackboardComponent->SetValueAsBool(HasVisualTargetKey.SelectedKeyName, true);
	}
	else
	{
		// 没有活着的玩家时清空目标，避免敌人继续追逐已经死亡或失效的旧 Actor。
		BlackboardComponent->ClearValue(TargetActorKey.SelectedKeyName);
		BlackboardComponent->ClearValue(LastKnownLocationKey.SelectedKeyName);
		BlackboardComponent->SetValueAsBool(HasVisualTargetKey.SelectedKeyName, false);
	}
}

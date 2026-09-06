// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "AIController.h"
#include "Perception/AIPerceptionTypes.h"
#include "SAEnemyAIController.generated.h"

class ASACharacter;
class ASAEnemyCharacter;
class UBehaviorTree;
class UAIPerceptionComponent;

UCLASS()
class SYNCFIREARENA_API ASAEnemyAIController : public AAIController
{
	GENERATED_BODY()

public:
	ASAEnemyAIController();

	// 当前 Demo 阶段不启用 AI Perception；保留接口，避免旧蓝图或调试代码直接失效。
	UAIPerceptionComponent* GetEnemyPerceptionComponent() const { return nullptr; }

protected:
	virtual void OnPossess(APawn* InPawn) override;
	virtual void OnUnPossess() override;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Syncfire|AI|Blackboard")
	FName TargetActorKeyName = TEXT("TargetActor");

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Syncfire|AI|Blackboard")
	FName LastKnownLocationKeyName = TEXT("LastKnownLocation");

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Syncfire|AI|Blackboard")
	FName HeardLocationKeyName = TEXT("HeardLocation");

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Syncfire|AI|Blackboard")
	FName HasVisualTargetKeyName = TEXT("HasVisualTarget");

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Syncfire|AI|Blackboard")
	FName HasLastKnownLocationKeyName = TEXT("HasLastKnownLocation");

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Syncfire|AI|Blackboard")
	FName HasHeardLocationKeyName = TEXT("HasHeardLocation");

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Syncfire|AI")
	float TargetSearchInterval = 0.5f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Syncfire|AI")
	float AcceptanceRadius = 120.0f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Syncfire|AI", meta = (ClampMin = "0.0"))
	float MoveRepathDistance = 250.0f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Syncfire|AI", meta = (ClampMin = "0.0"))
	float MinMoveRequestInterval = 0.25f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Syncfire|AI")
	float TargetSearchRadius = 3000.0f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Syncfire|AI|Attack")
	float AttackRange = 160.0f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Syncfire|AI|Attack")
	float AttackDamage = 10.0f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Syncfire|AI|Attack")
	float AttackInterval = 1.0f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Syncfire|AI|Attack")
	bool bRequireLineOfSight = true;

	// 配置后使用行为树驱动 AI；未配置时继续使用 C++ 定时器逻辑，方便逐步迁移。
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Syncfire|AI|Behavior")
	TObjectPtr<UBehaviorTree> BehaviorTreeAsset;

private:
	UPROPERTY()
	TObjectPtr<ASAEnemyCharacter> ControlledEnemy;

	UPROPERTY()
	TObjectPtr<ASACharacter> CurrentTarget;

	UPROPERTY()
	TObjectPtr<ASACharacter> LastMoveTarget;

	FTimerHandle TargetSearchTimerHandle;
	float LastAttackTime = -1000.0f;
	float LastMoveRequestTime = -1000.0f;
	FVector LastMoveGoalLocation = FVector::ZeroVector;

	void UpdateTarget();
	ASACharacter* FindNearestPlayerCharacter() const;
	bool IsTargetInAttackRange(const ASACharacter* Target) const;
	bool ShouldRequestMoveToTarget(const ASACharacter* Target) const;
	void RequestMoveToTarget(ASACharacter* Target);
	void ResetMoveRequestCache();
	void TryAttackTarget();

	UFUNCTION()
	void HandleTargetPerceptionUpdated(AActor* Actor, FAIStimulus Stimulus);
};

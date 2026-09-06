#pragma once

#include "CoreMinimal.h"
#include "Components/SAStatComponent.h"
#include "SAWaveRewardTypes.generated.h"

// 奖励数据
USTRUCT(BlueprintType)
struct FSAWaveRewardOption
{
	GENERATED_BODY()

	/*
	 * 奖励类型、增值
	 */
	UPROPERTY(BlueprintReadOnly, Category = "Syncfire|Reward")
	ESAStatType StatType = ESAStatType::DamageMultiplier;

	UPROPERTY(BlueprintReadOnly, Category = "Syncfire|Reward")
	float DeltaValue = 0.0f;

	bool operator==(const FSAWaveRewardOption& Other) const
	{
		return StatType == Other.StatType && FMath::IsNearlyEqual(DeltaValue, Other.DeltaValue);
	}
};

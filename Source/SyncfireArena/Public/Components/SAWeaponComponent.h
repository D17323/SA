// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "SAWeaponComponent.generated.h"

class ASACharacter;
class UAnimMontage;

UCLASS(ClassGroup=(Syncfire), meta=(BlueprintSpawnableComponent))
class SYNCFIREARENA_API USAWeaponComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	USAWeaponComponent();

	// 本地玩家开火时立即播放，减少输入到表现之间的延迟。
	void PlayLocalFire();

	// 服务器通过 Multicast 确认开火后，远端客户端播放对应表现。
	void PlayRemoteFire();

	bool GetMuzzleLocation(FVector& OutLocation) const;

protected:
	virtual void BeginPlay() override;

	// 当前阶段先从角色 Mesh 上找枪口 Socket；后续切成武器 Actor 时，这里可以改为武器 Mesh 的 Socket。
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Syncfire|Weapon")
	FName MuzzleSocketName = TEXT("Muzzle");

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Syncfire|Weapon")
	TObjectPtr<UAnimMontage> FireMontage;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Syncfire|Weapon")
	float FireMontagePlayRate = 1.0f;

	// 连续开火时如果反复从头播放 Montage，容易把移动状态机一直盖住。
	// 默认跳过正在播放中的射击 Montage，让移动动画有机会继续更新。
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Syncfire|Weapon")
	bool bSkipIfFireMontageIsPlaying = true;

private:
	UPROPERTY()
	TObjectPtr<ASACharacter> OwnerCharacter;

	void PlayFireMontage() const;
};

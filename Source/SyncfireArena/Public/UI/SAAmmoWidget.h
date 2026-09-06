// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "SAAmmoWidget.generated.h"

class UTextBlock;
class UProgressBar;
class ASACharacter;
class ASAWeaponActor;

UCLASS()
class SYNCFIREARENA_API USAAmmoWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintCallable, Category = "Syncfire|UI")
	void RefreshFromCharacter(ASACharacter* InCharacter);

protected:
	virtual void NativeConstruct() override;

	// 当前武器的主弹匣弹量。
	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UTextBlock> MagazineText;

	// 当前武器的备弹。
	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UTextBlock> ReserveText;

	// 换弹中的简单状态提示，蓝图里可以选择是否显示。
	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> ReloadStateText;

	// 先留一个很轻量的弹量条，方便你看武器状态变化。
	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UProgressBar> AmmoProgressBar;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> WeaponNameText;

private:
	UPROPERTY()
	TObjectPtr<ASACharacter> CachedCharacter;

	void UpdateAmmoDisplay(const ASAWeaponActor* Weapon);
};

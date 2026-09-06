// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "InputActionValue.h"
#include "GameFramework/PlayerController.h"
#include "SAPlayerController.generated.h"

class UInputAction;
class UInputMappingContext;
class ASACharacter;
class ASAPlayerState;
class USAMainGameUI;
class USAWaveRewardSelectionUI;
class UUserWidget;

UCLASS()
class SYNCFIREARENA_API ASAPlayerController : public APlayerController
{
	GENERATED_BODY()

protected:
	ASAPlayerController();

	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
	virtual void SetupInputComponent() override;
	virtual void Tick(float DeltaSeconds) override;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Character|Input")
	TObjectPtr<UInputMappingContext> DefaultMappingContext;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Character|Input")
	TObjectPtr<UInputAction> MoveAction;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Character|Input")
	TObjectPtr<UInputAction> FireAction;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Character|Input")
	TObjectPtr<UInputAction> PickupAction;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Character|Input")
	TObjectPtr<UInputAction> ReloadAction;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Character|Input")
	TObjectPtr<UInputAction> EquipPistolAction;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Character|Input")
	TObjectPtr<UInputAction> EquipRifleAction;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Character|UI")
	TSubclassOf<USAMainGameUI> MainGameUIClass;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Character|UI")
	TSubclassOf<USAWaveRewardSelectionUI> WaveRewardSelectionUIClass;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Character|UI")
	TSubclassOf<UUserWidget> CrosshairCursorWidgetClass;

private:
	void HandleMove(const FInputActionValue& Value);
	void HandleFire();
	void HandleStopFire();
	void HandlePickup();
	void HandleReload();
	void HandleEquipPistol();
	void HandleEquipRifle();
	void RefreshMainGameUI();
	void ApplyGameplayCursor();
	void SyncWaveRewardUI();
	void ApplyWaveRewardInputState(bool bWaitingForReward);
	void BindWaveRewardState(ASAPlayerState* InPlayerState);
	void UnbindWaveRewardState();
	void HandleWaveRewardStateChanged();
	ASACharacter* GetControlledSACharacter() const;

	UPROPERTY()
	TObjectPtr<USAMainGameUI> MainGameUI;

	UPROPERTY()
	TObjectPtr<USAWaveRewardSelectionUI> WaveRewardSelectionUI;

	UPROPERTY()
	TObjectPtr<UUserWidget> CrosshairCursorWidget;

	UPROPERTY()
	TObjectPtr<ASAPlayerState> CachedWaveRewardPlayerState;

	bool bIsInWaveRewardInputMode = false;
	
	UFUNCTION(Server, Reliable)
	void ServerSelectWaveReward(int32 RewardIndex);
public:
	UFUNCTION(BlueprintCallable, Category = "Syncfire|Reward")
	void SelectWaveReward(int32 RewardIndex);
};

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "Game/SAWaveRewardTypes.h"
#include "SAWaveRewardSelectionUI.generated.h"

class ASAPlayerState;
class UHorizontalBox;
class UTextBlock;
class USAWaveRewardCardWidget;

UCLASS()
class SYNCFIREARENA_API USAWaveRewardSelectionUI : public UUserWidget
{
	GENERATED_BODY()

public:
	USAWaveRewardSelectionUI(const FObjectInitializer& ObjectInitializer);

	UFUNCTION(BlueprintCallable, Category = "Syncfire|Reward")
	void RefreshFromPlayerState(ASAPlayerState* InPlayerState);

protected:
	virtual void NativeConstruct() override;
	virtual void NativeDestruct() override;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Syncfire|Reward")
	TSubclassOf<USAWaveRewardCardWidget> RewardCardWidgetClass;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> TitleText;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> HintText;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UHorizontalBox> CardContainer;

private:
	UPROPERTY()
	TObjectPtr<ASAPlayerState> CachedPlayerState;

	UPROPERTY()
	TArray<TObjectPtr<USAWaveRewardCardWidget>> CachedCards;

	TArray<FSAWaveRewardOption> CachedRewardOptions;

	void BuildFallbackWidgetTree();
	void ClearCards();
	void RebuildCards();
	void RefreshHeaderText();
	void RefreshHintText(bool bWaiting);
	void BindPlayerState(ASAPlayerState* InPlayerState);
	void UnbindPlayerState();

	void HandleRewardStateChanged();
	static FText BuildHeaderText();
	static FText BuildWaitingHintText();
};

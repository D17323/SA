#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "Game/SAWaveRewardTypes.h"
#include "SAWaveRewardCardWidget.generated.h"

class UBorder;
class UButton;
class UTextBlock;

UCLASS()
class SYNCFIREARENA_API USAWaveRewardCardWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintCallable, Category = "Syncfire|Reward")
	void SetupCard(int32 InRewardIndex, const FSAWaveRewardOption& InRewardOption);

	UFUNCTION(BlueprintCallable, Category = "Syncfire|Reward")
	void SetCardEnabled(bool bInEnabled);

	UFUNCTION(BlueprintCallable, Category = "Syncfire|Reward")
	void SetCardSelected(bool bInSelected);

protected:
	virtual void NativeConstruct() override;
	virtual void NativeDestruct() override;
	virtual FReply NativeOnPreviewMouseButtonDown(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent) override;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UButton> SelectButton;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UBorder> CardBorder;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> TitleText;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> DetailText;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> FooterText;

private:
	UPROPERTY()
	int32 CachedRewardIndex = INDEX_NONE;

	UPROPERTY()
	FSAWaveRewardOption CachedRewardOption;

	bool bIsSelected = false;
	bool bIsEnabled = true;

	UFUNCTION()
	void HandleClicked();

	void BuildFallbackWidgetTree();
	void RefreshVisualState();

	FText BuildTitleText() const;
	FText BuildDetailText() const;
	FText BuildFooterText() const;
	static FText DescribeStatType(ESAStatType StatType);
};

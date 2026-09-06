#include "UI/SAWaveRewardCardWidget.h"

#include "Blueprint/WidgetTree.h"
#include "Components/Border.h"
#include "Components/Button.h"
#include "Components/SizeBox.h"
#include "Components/TextBlock.h"
#include "Components/VerticalBox.h"
#include "Game/SAPlayerController.h"
#include "Input/Reply.h"

void USAWaveRewardCardWidget::NativeConstruct()
{
	Super::NativeConstruct();

	BuildFallbackWidgetTree();

	if (SelectButton)
	{
		SelectButton->OnClicked.RemoveAll(this);
		SelectButton->OnClicked.AddDynamic(this, &USAWaveRewardCardWidget::HandleClicked);
	}

	RefreshVisualState();
}

void USAWaveRewardCardWidget::NativeDestruct()
{
	if (SelectButton)
	{
		SelectButton->OnClicked.RemoveAll(this);
	}

	Super::NativeDestruct();
}

FReply USAWaveRewardCardWidget::NativeOnPreviewMouseButtonDown(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent)
{
	if (InMouseEvent.GetEffectingButton() == EKeys::LeftMouseButton)
	{
		HandleClicked();
		return FReply::Handled();
	}

	return Super::NativeOnPreviewMouseButtonDown(InGeometry, InMouseEvent);
}

void USAWaveRewardCardWidget::SetupCard(int32 InRewardIndex, const FSAWaveRewardOption& InRewardOption)
{
	CachedRewardIndex = InRewardIndex;
	CachedRewardOption = InRewardOption;
	RefreshVisualState();
}

void USAWaveRewardCardWidget::SetCardEnabled(bool bInEnabled)
{
	bIsEnabled = bInEnabled;
	RefreshVisualState();
}

void USAWaveRewardCardWidget::SetCardSelected(bool bInSelected)
{
	bIsSelected = bInSelected;
	RefreshVisualState();
}

void USAWaveRewardCardWidget::HandleClicked()
{
	if (!bIsEnabled || bIsSelected || CachedRewardIndex == INDEX_NONE)
	{
		return;
	}

	bIsSelected = true;
	RefreshVisualState();

	if (ASAPlayerController* PlayerController = GetOwningPlayer<ASAPlayerController>())
	{
		PlayerController->SelectWaveReward(CachedRewardIndex);
	}
}

void USAWaveRewardCardWidget::BuildFallbackWidgetTree()
{
	if (!WidgetTree || WidgetTree->RootWidget)
	{
		return;
	}

	USizeBox* RootSizeBox = WidgetTree->ConstructWidget<USizeBox>(USizeBox::StaticClass(), TEXT("RootSizeBox"));
	RootSizeBox->SetWidthOverride(300.0f);
	RootSizeBox->SetHeightOverride(180.0f);
	WidgetTree->RootWidget = RootSizeBox;

	SelectButton = WidgetTree->ConstructWidget<UButton>(UButton::StaticClass(), TEXT("SelectButton"));
	RootSizeBox->SetContent(SelectButton);

	CardBorder = WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass(), TEXT("CardBorder"));
	CardBorder->SetPadding(FMargin(16.0f));
	CardBorder->SetBrushColor(FLinearColor(0.08f, 0.09f, 0.11f, 0.95f));
	SelectButton->SetContent(CardBorder);

	UVerticalBox* Stack = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass(), TEXT("CardStack"));
	CardBorder->SetContent(Stack);

	TitleText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("TitleText"));
	TitleText->SetJustification(ETextJustify::Center);
	Stack->AddChildToVerticalBox(TitleText);

	DetailText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("DetailText"));
	DetailText->SetAutoWrapText(true);
	DetailText->SetJustification(ETextJustify::Center);
	Stack->AddChildToVerticalBox(DetailText);

	FooterText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("FooterText"));
	FooterText->SetJustification(ETextJustify::Center);
	Stack->AddChildToVerticalBox(FooterText);
}

void USAWaveRewardCardWidget::RefreshVisualState()
{
	if (TitleText)
	{
		TitleText->SetText(BuildTitleText());
	}

	if (DetailText)
	{
		DetailText->SetText(BuildDetailText());
	}

	if (FooterText)
	{
		FooterText->SetText(BuildFooterText());
	}

	if (SelectButton)
	{
		SelectButton->SetIsEnabled(bIsEnabled && !bIsSelected);
	}

	if (CardBorder)
	{
		const FLinearColor EnabledColor = bIsSelected
			? FLinearColor(0.90f, 0.70f, 0.20f, 0.98f)
			: (bIsEnabled ? FLinearColor(0.10f, 0.12f, 0.15f, 0.95f) : FLinearColor(0.06f, 0.06f, 0.06f, 0.70f));
		CardBorder->SetBrushColor(EnabledColor);
	}
}

FText USAWaveRewardCardWidget::BuildTitleText() const
{
	return DescribeStatType(CachedRewardOption.StatType);
}

FText USAWaveRewardCardWidget::BuildDetailText() const
{
	switch (CachedRewardOption.StatType)
	{
	case ESAStatType::MoveSpeedMultiplier:
		return FText::FromString(FString::Printf(TEXT("Move Speed +%.0f%%"), CachedRewardOption.DeltaValue * 100.0f));
	case ESAStatType::DamageMultiplier:
		return FText::FromString(FString::Printf(TEXT("Damage +%.0f%%"), CachedRewardOption.DeltaValue * 100.0f));
	case ESAStatType::FireRateMultiplier:
		return FText::FromString(FString::Printf(TEXT("Fire Rate +%.0f%%"), CachedRewardOption.DeltaValue * 100.0f));
	case ESAStatType::ReloadSpeedMultiplier:
		return FText::FromString(FString::Printf(TEXT("Reload +%.0f%%"), CachedRewardOption.DeltaValue * 100.0f));
	case ESAStatType::MaxHealthBonus:
		return FText::FromString(FString::Printf(TEXT("Max Health +%.0f"), CachedRewardOption.DeltaValue));
	case ESAStatType::ComboWindowBonus:
		return FText::FromString(FString::Printf(TEXT("Combo Window +%.1fs"), CachedRewardOption.DeltaValue));
	default:
		return FText::FromString(TEXT("Unknown Effect"));
	}
}

FText USAWaveRewardCardWidget::BuildFooterText() const
{
	if (bIsSelected)
	{
		return FText::FromString(TEXT("Selected"));
	}

	if (!bIsEnabled)
	{
		return FText::FromString(TEXT("Locked"));
	}

	return FText::FromString(TEXT("Click to pick"));
}

FText USAWaveRewardCardWidget::DescribeStatType(ESAStatType StatType)
{
	switch (StatType)
	{
	case ESAStatType::MoveSpeedMultiplier:
		return FText::FromString(TEXT("Move Speed"));
	case ESAStatType::DamageMultiplier:
		return FText::FromString(TEXT("Damage"));
	case ESAStatType::FireRateMultiplier:
		return FText::FromString(TEXT("Fire Rate"));
	case ESAStatType::ReloadSpeedMultiplier:
		return FText::FromString(TEXT("Reload"));
	case ESAStatType::MaxHealthBonus:
		return FText::FromString(TEXT("Max Health"));
	case ESAStatType::ComboWindowBonus:
		return FText::FromString(TEXT("Combo Window"));
	default:
		return FText::FromString(TEXT("Unknown"));
	}
}

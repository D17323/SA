#include "UI/SAWaveRewardSelectionUI.h"

#include "Blueprint/WidgetTree.h"
#include "Components/Border.h"
#include "Components/HorizontalBox.h"
#include "Components/HorizontalBoxSlot.h"
#include "Components/TextBlock.h"
#include "Components/VerticalBox.h"
#include "Game/SAPlayerState.h"
#include "UI/SAWaveRewardCardWidget.h"

USAWaveRewardSelectionUI::USAWaveRewardSelectionUI(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	RewardCardWidgetClass = USAWaveRewardCardWidget::StaticClass();
}

void USAWaveRewardSelectionUI::NativeConstruct()
{
	Super::NativeConstruct();

	BuildFallbackWidgetTree();
	RefreshHeaderText();
	RefreshHintText(false);
	SetVisibility(ESlateVisibility::Collapsed);
}

void USAWaveRewardSelectionUI::NativeDestruct()
{
	UnbindPlayerState();
	Super::NativeDestruct();
}

void USAWaveRewardSelectionUI::RefreshFromPlayerState(ASAPlayerState* InPlayerState)
{
	BindPlayerState(InPlayerState);

	const bool bShouldShow = InPlayerState && InPlayerState->IsWaitingForWaveReward();
	if (!bShouldShow)
	{
		SetVisibility(ESlateVisibility::Collapsed);
		RefreshHintText(false);
		ClearCards();
		CachedRewardOptions.Reset();
		return;
	}

	if (GetVisibility() != ESlateVisibility::Visible)
	{
		SetVisibility(ESlateVisibility::Visible);
	}

	RefreshHeaderText();
	RefreshHintText(true);

	if (CachedRewardOptions != InPlayerState->GetWaveRewardOptions())
	{
		CachedRewardOptions = InPlayerState->GetWaveRewardOptions();
		RebuildCards();
	}
}

void USAWaveRewardSelectionUI::BuildFallbackWidgetTree()
{
	if (!WidgetTree || WidgetTree->RootWidget)
	{
		return;
	}

	UBorder* RootBorder = WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass(), TEXT("RootBorder"));
	RootBorder->SetBrushColor(FLinearColor(0.0f, 0.0f, 0.0f, 0.65f));
	WidgetTree->RootWidget = RootBorder;

	UVerticalBox* Column = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass(), TEXT("Column"));
	RootBorder->SetContent(Column);

	TitleText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("TitleText"));
	TitleText->SetJustification(ETextJustify::Center);
	Column->AddChildToVerticalBox(TitleText);

	HintText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("HintText"));
	HintText->SetJustification(ETextJustify::Center);
	Column->AddChildToVerticalBox(HintText);

	CardContainer = WidgetTree->ConstructWidget<UHorizontalBox>(UHorizontalBox::StaticClass(), TEXT("CardContainer"));
	Column->AddChildToVerticalBox(CardContainer);
}

void USAWaveRewardSelectionUI::ClearCards()
{
	if (CardContainer)
	{
		CardContainer->ClearChildren();
	}

	CachedCards.Reset();
}

void USAWaveRewardSelectionUI::RebuildCards()
{
	ClearCards();

	if (!CardContainer || !RewardCardWidgetClass || !CachedPlayerState)
	{
		return;
	}

	const TArray<FSAWaveRewardOption>& Options = CachedPlayerState->GetWaveRewardOptions();
	for (int32 Index = 0; Index < Options.Num(); ++Index)
	{
		USAWaveRewardCardWidget* CardWidget = CreateWidget<USAWaveRewardCardWidget>(GetOwningPlayer(), RewardCardWidgetClass);
		if (!CardWidget)
		{
			continue;
		}

		CardWidget->SetupCard(Index, Options[Index]);
		CardWidget->SetCardEnabled(!CachedPlayerState->HasSelectedWaveReward());
		CardWidget->SetCardSelected(CachedPlayerState->HasSelectedWaveReward());

		if (UHorizontalBoxSlot* CardSlot = CardContainer->AddChildToHorizontalBox(CardWidget))
		{
			CardSlot->SetPadding(FMargin(10.0f, 0.0f));
		}

		CachedCards.Add(CardWidget);
	}
}

void USAWaveRewardSelectionUI::RefreshHeaderText()
{
	if (TitleText)
	{
		TitleText->SetText(BuildHeaderText());
	}
}

void USAWaveRewardSelectionUI::RefreshHintText(bool bWaiting)
{
	if (HintText)
	{
		HintText->SetText(bWaiting ? BuildWaitingHintText() : FText::GetEmpty());
	}
}

void USAWaveRewardSelectionUI::BindPlayerState(ASAPlayerState* InPlayerState)
{
	if (CachedPlayerState == InPlayerState)
	{
		return;
	}

	UnbindPlayerState();
	CachedPlayerState = InPlayerState;

	if (CachedPlayerState)
	{
		CachedPlayerState->OnWaveRewardStateChanged.AddUObject(this, &USAWaveRewardSelectionUI::HandleRewardStateChanged);
	}
}

void USAWaveRewardSelectionUI::UnbindPlayerState()
{
	if (CachedPlayerState)
	{
		CachedPlayerState->OnWaveRewardStateChanged.RemoveAll(this);
		CachedPlayerState = nullptr;
	}
}

void USAWaveRewardSelectionUI::HandleRewardStateChanged()
{
	RefreshFromPlayerState(CachedPlayerState);
}

FText USAWaveRewardSelectionUI::BuildHeaderText()
{
	return FText::FromString(TEXT("Wave Reward"));
}

FText USAWaveRewardSelectionUI::BuildWaitingHintText()
{
	return FText::FromString(TEXT("Pick one card to apply it immediately."));
}

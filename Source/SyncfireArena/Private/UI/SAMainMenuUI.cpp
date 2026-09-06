#include "UI/SAMainMenuUI.h"

#include "Components/Button.h"
#include "GameFramework/PlayerController.h"
#include "Kismet/GameplayStatics.h"
#include "Kismet/KismetSystemLibrary.h"
#include "UI/SAMultiplayerMenuUI.h"

USAMainMenuUI::USAMainMenuUI(const FObjectInitializer& ObjectInitializer) : Super(ObjectInitializer)
{
	MultiplayerMenuUIClass = USAMultiplayerMenuUI::StaticClass();
}

void USAMainMenuUI::NativeConstruct()
{
	Super::NativeConstruct();

	if (SinglePlayerButton)
	{
		SinglePlayerButton->OnClicked.AddDynamic(this, &ThisClass::HandleSinglePlayerClicked);
	}

	if (MultiplayerButton)
	{
		MultiplayerButton->OnClicked.AddDynamic(this, &ThisClass::HandleMultiplayerClicked);
	}

	if (QuitButton)
	{
		QuitButton->OnClicked.AddDynamic(this, &ThisClass::HandleQuitClicked);
	}
}

void USAMainMenuUI::NativeDestruct()
{
	if (SinglePlayerButton)
	{
		SinglePlayerButton->OnClicked.RemoveDynamic(this, &ThisClass::HandleSinglePlayerClicked);
	}

	if (MultiplayerButton)
	{
		MultiplayerButton->OnClicked.RemoveDynamic(this, &ThisClass::HandleMultiplayerClicked);
	}

	if (QuitButton)
	{
		QuitButton->OnClicked.RemoveDynamic(this, &ThisClass::HandleQuitClicked);
	}

	if (MultiplayerMenuUI)
	{
		MultiplayerMenuUI->RemoveFromParent();
		MultiplayerMenuUI = nullptr;
	}

	Super::NativeDestruct();
}

void USAMainMenuUI::HandleSinglePlayerClicked()
{
	// 单人游戏不创建网络会话，直接打开普通游戏地图。
	UGameplayStatics::OpenLevel(this, FName(*PlayMapPath));
}

void USAMainMenuUI::HandleMultiplayerClicked()
{
	if (!MultiplayerMenuUIClass)
	{
		return;
	}

	if (!MultiplayerMenuUI)
	{
		MultiplayerMenuUI = CreateWidget<USAMultiplayerMenuUI>(GetOwningPlayer(), MultiplayerMenuUIClass);
		if (MultiplayerMenuUI)
		{
			MultiplayerMenuUI->OnMultiplayerMenuBackRequested.AddUObject(this, &ThisClass::HandleMultiplayerMenuBackRequested);
			MultiplayerMenuUI->AddToViewport(10);
			MultiplayerMenuUI->MenuSetup(PlayMapPath, NumPublicConnections, MatchType);
		}
	}

	if (MainMenuPanel)
	{
		MainMenuPanel->SetVisibility(ESlateVisibility::Collapsed);
	}
}

void USAMainMenuUI::HandleQuitClicked()
{
	APlayerController* PlayerController = GetOwningPlayer();

	UKismetSystemLibrary::QuitGame(
		this,
		PlayerController,
		EQuitPreference::Quit,
		false
	);
}

void USAMainMenuUI::HandleMultiplayerMenuBackRequested()
{
	if (MultiplayerMenuUI)
	{
		MultiplayerMenuUI->RemoveFromParent();
		MultiplayerMenuUI = nullptr;
	}

	if (MainMenuPanel)
	{
		MainMenuPanel->SetVisibility(ESlateVisibility::Visible);
	}
}
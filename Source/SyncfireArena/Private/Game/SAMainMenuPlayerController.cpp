#include "Game/SAMainMenuPlayerController.h"

#include "Blueprint/UserWidget.h"
#include "UI/SAMainMenuUI.h"

ASAMainMenuPlayerController::ASAMainMenuPlayerController()
{
	MainMenuUIClass = USAMainMenuUI::StaticClass();
}

void ASAMainMenuPlayerController::BeginPlay()
{
	Super::BeginPlay();

	if (!IsLocalController())
	{
		return;
	}

	CreateMainMenu();
	ApplyMenuInputMode();
}

void ASAMainMenuPlayerController::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	if (MainMenuUI)
	{
		MainMenuUI->RemoveFromParent();
		MainMenuUI = nullptr;
	}

	Super::EndPlay(EndPlayReason);
}

void ASAMainMenuPlayerController::CreateMainMenu()
{
	if (MainMenuUI || !MainMenuUIClass)
	{
		return;
	}

	MainMenuUI = CreateWidget<USAMainMenuUI>(this, MainMenuUIClass);
	if (MainMenuUI)
	{
		MainMenuUI->AddToViewport();
	}
}

void ASAMainMenuPlayerController::ApplyMenuInputMode()
{
	bShowMouseCursor = true;
	bEnableClickEvents = true;
	bEnableMouseOverEvents = true;
	DefaultMouseCursor = EMouseCursor::Default;

	SetIgnoreMoveInput(true);
	SetIgnoreLookInput(true);

	FInputModeUIOnly InputMode;
	InputMode.SetLockMouseToViewportBehavior(EMouseLockMode::DoNotLock);

	if (MainMenuUI)
	{
		InputMode.SetWidgetToFocus(MainMenuUI->TakeWidget());
	}

	SetInputMode(InputMode);
}

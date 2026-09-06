#pragma once

#include "CoreMinimal.h"
#include "Runtime/UMG/Public/Blueprint/UserWidget.h"
#include "SAMainMenuUI.generated.h"

class UButton;
class UWidget;
class USAMultiplayerMenuUI;

UCLASS()
class SYNCFIREARENA_API USAMainMenuUI : public UUserWidget
{
	GENERATED_BODY()

public:
	USAMainMenuUI(const FObjectInitializer& ObjectInitializer);

protected:
	virtual void NativeConstruct() override;
	virtual void NativeDestruct() override;

private:
	UPROPERTY(EditDefaultsOnly, Category = "Syncfire|Menu")
	TSubclassOf<USAMultiplayerMenuUI> MultiplayerMenuUIClass;

	UPROPERTY(EditDefaultsOnly, Category = "Syncfire|Menu")
	FString PlayMapPath = TEXT("/Game/Maps/Level_test");

	UPROPERTY(EditDefaultsOnly, Category = "Syncfire|Menu")
	int32 NumPublicConnections = 4;

	UPROPERTY(EditDefaultsOnly, Category = "Syncfire|Menu")
	FString MatchType = TEXT("SyncfireLAN");

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UWidget> MainMenuPanel;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UButton> SinglePlayerButton;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UButton> MultiplayerButton;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UButton> QuitButton;

	UPROPERTY()
	TObjectPtr<USAMultiplayerMenuUI> MultiplayerMenuUI;

	UFUNCTION()
	void HandleSinglePlayerClicked();

	UFUNCTION()
	void HandleMultiplayerClicked();

	UFUNCTION()
	void HandleQuitClicked();

	void HandleMultiplayerMenuBackRequested();
};
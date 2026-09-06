// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "Interfaces/OnlineSessionInterface.h"
#include "OnlineSessionSettings.h"
#include "SAMultiplayerMenuUI.generated.h"

class UButton;
class UPanelWidget;
class UTextBlock;
class USAMultiplayerSessionsSubsystem;
class USASessionEntryWidget;

DECLARE_MULTICAST_DELEGATE(FSAOnMultiplayerMenuBackRequested);
UCLASS()
class SYNCFIREARENA_API USAMultiplayerMenuUI : public UUserWidget
{
	GENERATED_BODY()
	
public:
	USAMultiplayerMenuUI(const FObjectInitializer& ObjectInitializer);
	
	void MenuSetup(const FString& InMapPath, int32 InNumPublicConnections,const FString& InMatchType);
	
	FSAOnMultiplayerMenuBackRequested OnMultiplayerMenuBackRequested;
	
protected:
	virtual void NativeConstruct() override;
	virtual void NativeDestruct() override;
	
private:
	
	UPROPERTY(EditDefaultsOnly, Category = "Syncfire|Menu")
	TSubclassOf<USASessionEntryWidget> SessionEntryWidgetClass;
	
	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UButton> CreateSessionButton;
	
	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UButton> FindSessionButton;
	
	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UButton> BackButton;
	
	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UPanelWidget> SessionListPanel;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> StatusText;
	
	UPROPERTY()
	TObjectPtr<USAMultiplayerSessionsSubsystem> MultiplayerSessionsSubsystem;
	
	TArray<FOnlineSessionSearchResult> CachedSessionResults;
	FString PlayMapPath = TEXT("/Game/Maps/Level_test");
	FString MatchType = TEXT("SyncfireLAN");
	int32 NumPublicConnections = 4;
	bool bIsBusy=false;
	
	void BindMultiplayerSubsystemDelegates();
	void UnBindMultiplayerSubsystemDelegates();
	
	UFUNCTION()
	void CreateSessionButtonClicked();
	UFUNCTION()
	void FindSessionButtonClicked();
	UFUNCTION()
	void BackButtonClicked();
	
	UFUNCTION()
	void OnCreateSessionClicked(bool bWasSuccessful);

	void OnFindSessionClicked(const TArray<FOnlineSessionSearchResult>& SessionResults, bool bWasSuccessful);
	void OnJoinSessionClicked(EOnJoinSessionCompleteResult::Type Result);
	
	void HandleSessionEntryClicked(int32 SessionIndex);
	
	void SetBusy(bool InBusy);
	void SetStatusText(const FString& InText);
	void ClearSessionList();
	void RebuildSessionList();
};

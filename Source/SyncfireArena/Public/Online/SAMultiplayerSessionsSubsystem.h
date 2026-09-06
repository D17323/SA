// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Interfaces/OnlineSessionInterface.h"
#include "OnlineSessionSettings.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "SAMultiplayerSessionsSubsystem.generated.h"


DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FSAOnCreateSessionComplete, bool, bWasSuccessful);
DECLARE_MULTICAST_DELEGATE_TwoParams(FSAOnFindSessionComplete, const TArray<FOnlineSessionSearchResult>& SessionResults, bool bWasSuccessful);
DECLARE_MULTICAST_DELEGATE_OneParam(FSAOnJoinSessionComplete, EOnJoinSessionCompleteResult::Type Reuslt);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FSAOnDestroySessionComplete, bool, bWasSuccessful);

UCLASS()
class SYNCFIREARENA_API USAMultiplayerSessionsSubsystem : public UGameInstanceSubsystem
{
	GENERATED_BODY()
	
public:
	USAMultiplayerSessionsSubsystem();
	virtual void Initialize(FSubsystemCollectionBase& Collection) override;
	virtual void Deinitialize() override;
	
	void CreateSession(int32 NumPublicConnections, FString MatchType);
	void FindSession(int32 MaxSearchResults, const FString& MatchType = TEXT(""));
	void JoinSession(const FOnlineSessionSearchResult& Result);
	void DestroySession();
	
	// 用于给UI广播的委托变量
	FSAOnCreateSessionComplete SAOnCreateSessionCompleteDelegate;
	FSAOnFindSessionComplete SAOnFindSessionCompleteDelegate;
	FSAOnJoinSessionComplete SAOnJoinSessionCompleteDelegate;
	FSAOnDestroySessionComplete SAOnDestroySessionCompleteDelegate;
	
private:
	IOnlineSessionPtr SessionInterface;
	TSharedPtr<FOnlineSessionSettings> LastSessionSettings;
	TSharedPtr<FOnlineSessionSearch> LastSessionSearch;
	
	int32 DesiredNumPublicConnections = 0;
	int32 DesiredMaxSearchResults = 0;
	FString DesiredMatchType{};
	FName ActiveSubsystemName = NAME_None;

	bool bCreateSessionAfterDestroy = false;
	bool bFindSessionAfterDestroy = false;
	
	// 用于给IOnlineSessionPtr广播的委托变量
	FOnCreateSessionCompleteDelegate CreateSessionCompleteDelegate;
	FDelegateHandle CreateSessionCompleteDelegateHandle;

	FOnFindSessionsCompleteDelegate FindSessionsCompleteDelegate;
	FDelegateHandle FindSessionsCompleteDelegateHandle;

	FOnJoinSessionCompleteDelegate JoinSessionCompleteDelegate;
	FDelegateHandle JoinSessionCompleteDelegateHandle;

	FOnDestroySessionCompleteDelegate DestroySessionCompleteDelegate;
	FDelegateHandle DestroySessionCompleteDelegateHandle;
	
	
	void OnCreateSessionComplete(FName SessionName, bool bWasSuccessful);
	void OnFindSessionsComplete(bool bWasSuccessful);
	void OnJoinSessionComplete(FName SessionName, EOnJoinSessionCompleteResult::Type Result);
	void OnDestroySessionComplete(FName SessionName, bool bWasSuccessful);
	
public:
	bool TryGetResolvedConnectString(FString& Address) const;
	
	bool IsSteamSubsystem() const;
};

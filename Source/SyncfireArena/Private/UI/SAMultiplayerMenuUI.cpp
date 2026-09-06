// Fill out your copyright notice in the Description page of Project Settings.


#include "UI/SAMultiplayerMenuUI.h"

#include "OnlineSessionSettings.h"
#include "Components/Button.h"
#include "Components/TextBlock.h"
#include "Online/SAMultiplayerSessionsSubsystem.h"
#include "UI/SASessionEntryWidget.h"

USAMultiplayerMenuUI::USAMultiplayerMenuUI(const FObjectInitializer& ObjectInitializer) : Super(ObjectInitializer)
{
	SessionEntryWidgetClass = USASessionEntryWidget::StaticClass();
}

void USAMultiplayerMenuUI::MenuSetup(const FString& InMapPath, int32 InNumPublicConnections, const FString& InMatchType)
{
	PlayMapPath = InMapPath;
	NumPublicConnections = FMath::Max(1, InNumPublicConnections);
	MatchType = InMatchType;

	BindMultiplayerSubsystemDelegates();
	SetBusy(false);
	SetStatusText(TEXT("创建或搜索会话"));
}

void USAMultiplayerMenuUI::NativeConstruct()
{
	Super::NativeConstruct();
	
	if (CreateSessionButton)
	{
		CreateSessionButton->OnClicked.AddDynamic(this, &ThisClass::CreateSessionButtonClicked);
	}
	if (FindSessionButton)
	{
		FindSessionButton->OnClicked.AddDynamic(this, &ThisClass::FindSessionButtonClicked);
	}
	if (BackButton)
	{
		BackButton->OnClicked.AddDynamic(this, &ThisClass::BackButtonClicked);
	}
	
	BindMultiplayerSubsystemDelegates();
}

void USAMultiplayerMenuUI::NativeDestruct()
{
	Super::NativeDestruct();
	
	UnBindMultiplayerSubsystemDelegates();
	
	if (CreateSessionButton)
	{
		CreateSessionButton->OnClicked.RemoveDynamic(this, &ThisClass::CreateSessionButtonClicked);
	}
	if (FindSessionButton)
	{
		FindSessionButton->OnClicked.RemoveDynamic(this, &ThisClass::FindSessionButtonClicked);
	}
	if (BackButton)
	{
		BackButton->OnClicked.RemoveDynamic(this, &ThisClass::BackButtonClicked);
	}
}

void USAMultiplayerMenuUI::BindMultiplayerSubsystemDelegates()
{
	if (MultiplayerSessionsSubsystem)
		return;
	
	UGameInstance* GameInstance = GetGameInstance();
	if (!GameInstance)
	{
		return;
	}
	
	MultiplayerSessionsSubsystem = GameInstance->GetSubsystem<USAMultiplayerSessionsSubsystem>();
	if (!MultiplayerSessionsSubsystem)
		return;
	
	MultiplayerSessionsSubsystem->SAOnCreateSessionCompleteDelegate.AddDynamic(this, &ThisClass::OnCreateSessionClicked);
	MultiplayerSessionsSubsystem->SAOnFindSessionCompleteDelegate.AddUObject(this,&ThisClass::OnFindSessionClicked);
	MultiplayerSessionsSubsystem->SAOnJoinSessionCompleteDelegate.AddUObject(this,&ThisClass::OnJoinSessionClicked);
	
	
}

void USAMultiplayerMenuUI::UnBindMultiplayerSubsystemDelegates()
{
	if (!MultiplayerSessionsSubsystem)
	{
		return;
	}

	MultiplayerSessionsSubsystem->SAOnCreateSessionCompleteDelegate.RemoveDynamic(this,&ThisClass::OnCreateSessionClicked);
	MultiplayerSessionsSubsystem->SAOnFindSessionCompleteDelegate.RemoveAll(this);
	MultiplayerSessionsSubsystem->SAOnJoinSessionCompleteDelegate.RemoveAll(this);

	MultiplayerSessionsSubsystem = nullptr;
}

void USAMultiplayerMenuUI::CreateSessionButtonClicked()
{
	if (!MultiplayerSessionsSubsystem || bIsBusy)
	{
		return;
	}

	SetBusy(true);
	ClearSessionList();
	SetStatusText(TEXT("正在创建会话..."));
	
	MultiplayerSessionsSubsystem->CreateSession(NumPublicConnections, MatchType);
}

void USAMultiplayerMenuUI::FindSessionButtonClicked()
{
	if (!MultiplayerSessionsSubsystem || bIsBusy)
	{
		return;
	}

	SetBusy(true);
	ClearSessionList();
	SetStatusText(TEXT("正在搜索会话..."));

	MultiplayerSessionsSubsystem->FindSession(10000, MatchType);
}

void USAMultiplayerMenuUI::BackButtonClicked()
{
	OnMultiplayerMenuBackRequested.Broadcast();
}

void USAMultiplayerMenuUI::OnCreateSessionClicked(bool bWasSuccessful)
{
	SetBusy(false);
	
	if (!bWasSuccessful)
	{
		SetStatusText(TEXT("创建会话失败"));
		return;
	}

	SetStatusText(TEXT("会话创建成功，正在进入关卡..."));

	UWorld* World = GetWorld();
	if (!World)
	{
		return;
	}

	// Host 创建成功后以 Listen Server 方式进入游戏地图，其他玩家加入的是这个监听服务器。
	// SteamNetDriver 只有收到 URL 上的 bIsLanMatch 选项才会对 LAN 会话回退到普通 IP 传输；
	// 不写的话即使是 LAN 会话也会误用 Steam P2P socket，导致客户端连不上。LAN 模式必须显式追加。
	FString TravelURL = PlayMapPath + TEXT("?listen");
	if (MultiplayerSessionsSubsystem && !MultiplayerSessionsSubsystem->IsSteamSubsystem())
	{
		TravelURL += TEXT("?bIsLanMatch");
	}
	World->ServerTravel(TravelURL);
}

void USAMultiplayerMenuUI::OnFindSessionClicked(const TArray<FOnlineSessionSearchResult>& SessionResults, bool bWasSuccessful)
{
	SetBusy(false);
	CachedSessionResults.Reset();

	if (!bWasSuccessful || SessionResults.Num() <= 0)
	{
		SetStatusText(TEXT("没有找到会话"));
		ClearSessionList();
		return;
	}

	for (const FOnlineSessionSearchResult& Result : SessionResults)
	{
		FString FoundMatchType;
		Result.Session.SessionSettings.Get(FName(TEXT("MatchType")), FoundMatchType);

		if (MatchType.IsEmpty() || FoundMatchType == MatchType)
		{
			CachedSessionResults.Add(Result);
		}
	}

	RebuildSessionList();

	if (CachedSessionResults.Num() <= 0)
	{
		SetStatusText(TEXT("没有找到匹配的 Syncfire 会话"));
		return;
	}

	SetStatusText(FString::Printf(TEXT("找到 %d 个会话"), CachedSessionResults.Num()));
	
	UE_LOG(LogTemp, Warning, TEXT("FindSession Result Num: %d, Success: %d"), SessionResults.Num(), bWasSuccessful);

	for (const FOnlineSessionSearchResult& Result : SessionResults)
	{
		FString FoundMatchType;
		const bool bHasMatchType = Result.Session.SessionSettings.Get(FName(TEXT("MatchType")), FoundMatchType);

		UE_LOG(LogTemp, Warning, TEXT("Found Session: Owner=%s, Ping=%d, HasMatchType=%d, MatchType=%s"),
			*Result.Session.OwningUserName,
			Result.PingInMs,
			bHasMatchType,
			*FoundMatchType);
	}
}

void USAMultiplayerMenuUI::OnJoinSessionClicked(EOnJoinSessionCompleteResult::Type Result)
{
	SetBusy(false);

	if (Result != EOnJoinSessionCompleteResult::Success)
	{
		SetStatusText(TEXT("加入会话失败"));
		return;
	}

	if (!MultiplayerSessionsSubsystem)
	{
		SetStatusText(TEXT("加入失败：会话系统不可用"));
		return;
	}

	FString Address;
	if (!MultiplayerSessionsSubsystem->TryGetResolvedConnectString(Address))
	{
		SetStatusText(TEXT("加入失败：无法解析连接地址"));
		return;
	}

	APlayerController* PlayerController = GetOwningPlayer();
	if (!PlayerController)
	{
		return;
	}

	SetStatusText(TEXT("加入成功，正在进入关卡..."));

	// 客户端通过解析出的地址跳转到 Host 的 Listen Server。
	PlayerController->ClientTravel(Address, TRAVEL_Absolute);
}

void USAMultiplayerMenuUI::HandleSessionEntryClicked(int32 SessionIndex)
{
	if (!MultiplayerSessionsSubsystem || bIsBusy)
	{
		return;
	}

	if (!CachedSessionResults.IsValidIndex(SessionIndex))
	{
		return;
	}

	SetBusy(true);
	SetStatusText(TEXT("正在加入会话..."));

	MultiplayerSessionsSubsystem->JoinSession(CachedSessionResults[SessionIndex]);
}



void USAMultiplayerMenuUI::SetBusy(bool InBusy)
{
	bIsBusy=InBusy;
	
	if (CreateSessionButton)
	{
		CreateSessionButton->SetIsEnabled(!bIsBusy);
	}
	if (FindSessionButton)
	{
		FindSessionButton->SetIsEnabled(!bIsBusy);
	}
}

void USAMultiplayerMenuUI::SetStatusText(const FString& InText)
{
	if (StatusText)
	{
		StatusText->SetText(FText::FromString(InText));
	}
}

void USAMultiplayerMenuUI::ClearSessionList()
{
	if (SessionListPanel)
	{
		SessionListPanel->ClearChildren();
	}
}

void USAMultiplayerMenuUI::RebuildSessionList()
{
	ClearSessionList();

	if (!SessionListPanel || !SessionEntryWidgetClass)
	{
		return;
	}

	for (int32 Index = 0; Index < CachedSessionResults.Num(); ++Index)
	{
		USASessionEntryWidget* EntryWidget = CreateWidget<USASessionEntryWidget>(
			GetOwningPlayer(),
			SessionEntryWidgetClass
		);

		if (!EntryWidget)
		{
			continue;
		}

		EntryWidget->SetupSessionEntry(Index, CachedSessionResults[Index]);
		EntryWidget->SAOnSessionEntryClicked.AddUObject(this, &ThisClass::HandleSessionEntryClicked);

		SessionListPanel->AddChild(EntryWidget);
	}
}

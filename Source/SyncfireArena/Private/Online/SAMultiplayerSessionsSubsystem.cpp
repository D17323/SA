// Fill out your copyright notice in the Description page of Project Settings.

#include "Online/SAMultiplayerSessionsSubsystem.h"

#include "GameFramework/PlayerController.h"
#include "Online/OnlineSessionNames.h"
#include "OnlineSubsystemUtils.h"
#include "OnlineSessionSettings.h"
#include "OnlineSubsystem.h"

USAMultiplayerSessionsSubsystem::USAMultiplayerSessionsSubsystem() :
	CreateSessionCompleteDelegate(FOnCreateSessionCompleteDelegate::CreateUObject(this,&ThisClass::OnCreateSessionComplete)),
	FindSessionsCompleteDelegate(FOnFindSessionsCompleteDelegate::CreateUObject(this, &ThisClass::OnFindSessionsComplete)),
	JoinSessionCompleteDelegate(FOnJoinSessionCompleteDelegate::CreateUObject(this, &ThisClass::OnJoinSessionComplete)),
	DestroySessionCompleteDelegate(FOnDestroySessionCompleteDelegate::CreateUObject(this, &ThisClass::OnDestroySessionComplete))
{
	
}

void USAMultiplayerSessionsSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);
	
	IOnlineSubsystem* OnlineSubsystem = Online::GetSubsystem(GetWorld());
	if (OnlineSubsystem)
	{
		ActiveSubsystemName = OnlineSubsystem->GetSubsystemName();
		SessionInterface = OnlineSubsystem->GetSessionInterface();
	}
}

void USAMultiplayerSessionsSubsystem::Deinitialize()
{
	if (SessionInterface.IsValid())
	{
		if (CreateSessionCompleteDelegateHandle.IsValid())
		{
			SessionInterface->ClearOnCreateSessionCompleteDelegate_Handle(CreateSessionCompleteDelegateHandle);
			CreateSessionCompleteDelegateHandle.Reset();
		}

		if (FindSessionsCompleteDelegateHandle.IsValid())
		{
			SessionInterface->ClearOnFindSessionsCompleteDelegate_Handle(FindSessionsCompleteDelegateHandle);
			FindSessionsCompleteDelegateHandle.Reset();
		}

		if (JoinSessionCompleteDelegateHandle.IsValid())
		{
			SessionInterface->ClearOnJoinSessionCompleteDelegate_Handle(JoinSessionCompleteDelegateHandle);
			JoinSessionCompleteDelegateHandle.Reset();
		}

		if (DestroySessionCompleteDelegateHandle.IsValid())
		{
			SessionInterface->ClearOnDestroySessionCompleteDelegate_Handle(DestroySessionCompleteDelegateHandle);
			DestroySessionCompleteDelegateHandle.Reset();
		}
	}

	// OnlineSubsystemNull 关闭时会检查 SessionInterface 是否只剩子系统自身持有。
	// GameInstanceSubsystem 缓存的共享指针必须在这里释放，否则停止 PIE 时会触发 ensure。
	LastSessionSearch.Reset();
	LastSessionSettings.Reset();
	SessionInterface.Reset();

	Super::Deinitialize();
}

void USAMultiplayerSessionsSubsystem::CreateSession(int32 NumPublicConnections, FString MatchType)
{
	if (!SessionInterface.IsValid())
	{
		SAOnCreateSessionCompleteDelegate.Broadcast(false);
		return;
	}
	
	DesiredMatchType=MatchType;
	DesiredNumPublicConnections=NumPublicConnections;
	
	if (SessionInterface->GetNamedSession(NAME_GameSession))
	{
		bCreateSessionAfterDestroy=true;
		DestroySession();
		
		return;
	}
	// 创建会话绑定
	CreateSessionCompleteDelegateHandle = 
		SessionInterface->AddOnCreateSessionCompleteDelegate_Handle(CreateSessionCompleteDelegate);
	
	const bool bUseSteam = IsSteamSubsystem();
	
	/*
	 * LAN 和 Steam 共用同一个 FOnlineSessionSettings。
	 * 具体区别由这些参数决定。
	 */
	LastSessionSettings = MakeShareable(new FOnlineSessionSettings());
	LastSessionSettings->bIsLANMatch = !bUseSteam;
	LastSessionSettings->NumPublicConnections = DesiredNumPublicConnections;
	LastSessionSettings->bAllowJoinInProgress = true;
	LastSessionSettings->bShouldAdvertise = true;
	if (bUseSteam)
	{
		LastSessionSettings->bAllowJoinViaPresence = true;
    	LastSessionSettings->bUsesPresence = true;
    	LastSessionSettings->bUseLobbiesIfAvailable = true;	
	}
	else
	{
		LastSessionSettings->bAllowJoinViaPresence = false;
		LastSessionSettings->bUsesPresence = false;
		LastSessionSettings->bUseLobbiesIfAvailable = false;	
	}
	
	LastSessionSettings->Set(FName("MatchType"), MatchType, EOnlineDataAdvertisementType::ViaOnlineServiceAndPing);
	LastSessionSettings->Set(FName("RoomName"), FString(TEXT("Syncfire Room")), EOnlineDataAdvertisementType::ViaOnlineServiceAndPing);
	
	/* 
	 * 注意：不要手动设置 BuildUniqueId。FOnlineSessionSettings 构造函数已把它初始化为
	 * GetBuildUniqueId()（即引擎网络版本号）。Null 子系统的 LAN 查找会在客户端拒绝
	 *	BuildUniqueId 不匹配的会话，手动改成固定值会导致客户端永远搜索不到该会话。
	 *	
	 */ 
	
	const ULocalPlayer* LocalPlayer = GetWorld()->GetFirstLocalPlayerFromController();
	if (!SessionInterface->CreateSession(*LocalPlayer->GetPreferredUniqueNetId(), NAME_GameSession, *LastSessionSettings))
	{
		SessionInterface->ClearOnCreateSessionCompleteDelegate_Handle(CreateSessionCompleteDelegateHandle);
		
		SAOnCreateSessionCompleteDelegate.Broadcast(false);
	}
}

void USAMultiplayerSessionsSubsystem::FindSession(int32 MaxSearchResults, const FString& MatchType)
{
	if (!SessionInterface.IsValid())
	{
		SAOnFindSessionCompleteDelegate.Broadcast(TArray<FOnlineSessionSearchResult>(),false);
		return;
	}

	DesiredMaxSearchResults = MaxSearchResults;
	DesiredMatchType = MatchType;

	/*
	 * 若本地残留着一个会话（例如上次 JoinSession 在网络层失败、客户端回到了主菜单），
	 * 必须先退出它再搜索：Steam 的 lobby 搜索会排除当前仍是成员的 lobby（IsMemberOfLobby），
	 * 不退出的话将永远搜不到这个房间。CreateSession 已有同样的先销毁再创建逻辑。
	 */
	if (SessionInterface->GetNamedSession(NAME_GameSession))
	{
		bFindSessionAfterDestroy = true;
		DestroySession();
		return;
	}

	FindSessionsCompleteDelegateHandle =
		SessionInterface->AddOnFindSessionsCompleteDelegate_Handle(FindSessionsCompleteDelegate);
	
	const bool bUseSteam = IsSteamSubsystem();
	
	LastSessionSearch = MakeShareable(new FOnlineSessionSearch());
	LastSessionSearch->MaxSearchResults = MaxSearchResults;
	LastSessionSearch->bIsLanQuery = !bUseSteam;
	if (bUseSteam)
	{
		// UE 5.8 使用 SEARCH_LOBBIES。不再使用 SEARCH_PRESENCE。
		LastSessionSearch->QuerySettings.Set(SEARCH_LOBBIES,true,EOnlineComparisonOp::Equals);

		// SteamDevAppId=480（Spacewar）的 lobby 池与所有其它测试项目共用，Steam 一次最多返回 50 个。
		// 在 Steam 侧直接按 MatchType 字符串过滤（对应 lobby 数据里的 MatchType_s），
		// 避免自己的房间被无关的 lobby 挤掉。菜单里还会再做一次相同的过滤兜底。
		if (!DesiredMatchType.IsEmpty())
		{
			LastSessionSearch->QuerySettings.Set(FName(TEXT("MatchType")), DesiredMatchType, EOnlineComparisonOp::Equals);
		}
	}
	
	const ULocalPlayer* LocalPlayer = GetWorld()->GetFirstLocalPlayerFromController();
	if (!SessionInterface->FindSessions(*LocalPlayer->GetPreferredUniqueNetId(), LastSessionSearch.ToSharedRef()))
	{
		SessionInterface->ClearOnFindSessionsCompleteDelegate_Handle(FindSessionsCompleteDelegateHandle);

		SAOnFindSessionCompleteDelegate.Broadcast(TArray<FOnlineSessionSearchResult>(), false);
	}
}

void USAMultiplayerSessionsSubsystem::JoinSession(const FOnlineSessionSearchResult& Result)
{
	if (!SessionInterface.IsValid())
	{
		SAOnJoinSessionCompleteDelegate.Broadcast(EOnJoinSessionCompleteResult::UnknownError);
		return;
	}

	JoinSessionCompleteDelegateHandle = 
		SessionInterface->AddOnJoinSessionCompleteDelegate_Handle(JoinSessionCompleteDelegate);

	const ULocalPlayer* LocalPlayer = GetWorld()->GetFirstLocalPlayerFromController();
	if (!SessionInterface->JoinSession(*LocalPlayer->GetPreferredUniqueNetId(), NAME_GameSession, Result))
	{
		SessionInterface->ClearOnJoinSessionCompleteDelegate_Handle(JoinSessionCompleteDelegateHandle);

		SAOnJoinSessionCompleteDelegate.Broadcast(EOnJoinSessionCompleteResult::UnknownError);
	}
}

void USAMultiplayerSessionsSubsystem::DestroySession()
{
	if (!SessionInterface.IsValid())
	{
		SAOnDestroySessionCompleteDelegate.Broadcast(false);
		return;
	}

	DestroySessionCompleteDelegateHandle =  
		SessionInterface->AddOnDestroySessionCompleteDelegate_Handle(DestroySessionCompleteDelegate);

	if (!SessionInterface->DestroySession(NAME_GameSession))
	{
		SessionInterface->ClearOnDestroySessionCompleteDelegate_Handle(DestroySessionCompleteDelegateHandle);
		SAOnDestroySessionCompleteDelegate.Broadcast(false);
	}
}

void USAMultiplayerSessionsSubsystem::OnCreateSessionComplete(FName SessionName, bool bWasSuccessful)
{
	if (SessionInterface)
	{
		SessionInterface->ClearOnCreateSessionCompleteDelegate_Handle(CreateSessionCompleteDelegateHandle);
	}

	SAOnCreateSessionCompleteDelegate.Broadcast(bWasSuccessful);
	
}

void USAMultiplayerSessionsSubsystem::OnFindSessionsComplete(bool bWasSuccessful)
{
	if (SessionInterface)
	{
		SessionInterface->ClearOnFindSessionsCompleteDelegate_Handle(FindSessionsCompleteDelegateHandle);
	}
	if (LastSessionSearch->SearchResults.Num() <= 0)
	{
		SAOnFindSessionCompleteDelegate.Broadcast(TArray<FOnlineSessionSearchResult>(), false);
		return;
	}
	
	SAOnFindSessionCompleteDelegate.Broadcast(LastSessionSearch->SearchResults,bWasSuccessful);

}

void USAMultiplayerSessionsSubsystem::OnJoinSessionComplete(FName SessionName,
	EOnJoinSessionCompleteResult::Type Result)
{
	if (SessionInterface)
	{
		SessionInterface->ClearOnJoinSessionCompleteDelegate_Handle(JoinSessionCompleteDelegateHandle);
	}
	SAOnJoinSessionCompleteDelegate.Broadcast(Result);
}

void USAMultiplayerSessionsSubsystem::OnDestroySessionComplete(FName SessionName, bool bWasSuccessful)
{
	if (SessionInterface)
	{
		SessionInterface->ClearOnDestroySessionCompleteDelegate_Handle(DestroySessionCompleteDelegateHandle);
	}

	// 先取走标记再清空，避免在异步回调期间被重复触发。
	const bool bShouldCreate = bCreateSessionAfterDestroy;
	const bool bShouldFind = bFindSessionAfterDestroy;
	bCreateSessionAfterDestroy = false;
	bFindSessionAfterDestroy = false;

	if (bWasSuccessful)
	{
		if (bShouldCreate)
		{
			CreateSession(DesiredNumPublicConnections, DesiredMatchType);
		}
		else if (bShouldFind)
		{
			FindSession(DesiredMaxSearchResults, DesiredMatchType);
		}
	}
	else if (bShouldFind)
	{
		// 退出会话失败（Steam 上极少见），也要把搜索流程走完，避免菜单一直停留在 Busy 状态。
		SAOnFindSessionCompleteDelegate.Broadcast(TArray<FOnlineSessionSearchResult>(), false);
	}

	SAOnDestroySessionCompleteDelegate.Broadcast(bWasSuccessful);
}

bool USAMultiplayerSessionsSubsystem::TryGetResolvedConnectString(FString& Address) const
{
	if (!SessionInterface.IsValid())
	{
		return false;
	}
	
	return SessionInterface->GetResolvedConnectString(NAME_GameSession, Address);
}

bool USAMultiplayerSessionsSubsystem::IsSteamSubsystem() const
{
	return ActiveSubsystemName == FName(TEXT("STEAM"));
}

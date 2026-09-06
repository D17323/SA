#include "UI/SASessionEntryWidget.h"

#include "OnlineSessionSettings.h"
#include "Components/Button.h"
#include "Components/TextBlock.h"

USASessionEntryWidget::USASessionEntryWidget(const FObjectInitializer& ObjectInitializer) : Super(ObjectInitializer)
{
}

void USASessionEntryWidget::NativeConstruct()
{
	Super::NativeConstruct();

	if (JoinButton)
	{
		JoinButton->OnClicked.AddDynamic(this, &ThisClass::OnJoinButtonClicked);
	}
}

void USASessionEntryWidget::NativeDestruct()
{
	if (JoinButton)
	{
		JoinButton->OnClicked.RemoveDynamic(this, &ThisClass::OnJoinButtonClicked);
	}

	Super::NativeDestruct();
}


void USASessionEntryWidget::SetupSessionEntry(int32 InSessionIndex, const FOnlineSessionSearchResult& InResult)
{
	CachedSessionIndex = InSessionIndex;

	FString RoomName;
	InResult.Session.SessionSettings.Get(FName(TEXT("RoomName")), RoomName);
	if (RoomName.IsEmpty())
	{
		RoomName = InResult.Session.OwningUserName.IsEmpty() ? TEXT("LAN Session") : InResult.Session.OwningUserName;
	}

	const int32 MaxPlayers = InResult.Session.SessionSettings.NumPublicConnections;
	const int32 OpenPlayers = InResult.Session.NumOpenPublicConnections;
	const int32 CurrentPlayers = FMath::Clamp(MaxPlayers - OpenPlayers, 0, MaxPlayers);

	if (SessionNameText)
	{
		SessionNameText->SetText(FText::FromString(RoomName));
	}

	if (PlayerCountText)
	{
		PlayerCountText->SetText(FText::FromString(
			FString::Printf(TEXT("%d / %d"), CurrentPlayers, MaxPlayers)
		));
	}

	if (PingText)
	{
		// Steam 的 lobby 搜索不测量延迟，PingInMs 恒为 MAX_QUERY_PING（999/9999）。
		// 未知延迟直接显示 "--"，不要再拼上 "ms" 造成“负值/异常”的错觉。
		const FString PingString = InResult.PingInMs >= MAX_QUERY_PING
			? TEXT("--") : FString::Printf(TEXT("%d ms"), InResult.PingInMs);

		PingText->SetText(FText::FromString(PingString));
	}
}

void USASessionEntryWidget::OnJoinButtonClicked()
{
	if (CachedSessionIndex == INDEX_NONE)
	{
		return;
	}

	SAOnSessionEntryClicked.Broadcast(CachedSessionIndex);
}

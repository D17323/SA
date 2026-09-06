#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "OnlineSessionSettings.h"
#include "SASessionEntryWidget.generated.h"

class UButton;
class UTextBlock;

DECLARE_MULTICAST_DELEGATE_OneParam(FSAOnSessionEntryClicked, int32);

UCLASS()
class SYNCFIREARENA_API USASessionEntryWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	USASessionEntryWidget(const FObjectInitializer& ObjectInitializer);
	void SetupSessionEntry(int32 InSessionIndex, const FOnlineSessionSearchResult& InResult);

	FSAOnSessionEntryClicked SAOnSessionEntryClicked;

protected:
	virtual void NativeConstruct() override;
	virtual void NativeDestruct() override;

private:
	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UButton> JoinButton;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> SessionNameText;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> PlayerCountText;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> PingText;

	UFUNCTION()
	void OnJoinButtonClicked();

	int32 CachedSessionIndex = INDEX_NONE;
};
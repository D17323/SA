#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "SAMainGameUI.generated.h"

class ASACharacter;
class ASAWaveGameState;
class ASAPlayerState;
class UProgressBar;
class UTextBlock;
class USAAmmoWidget;

UCLASS()
class SYNCFIREARENA_API USAMainGameUI : public UUserWidget
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintCallable, Category = "Syncfire|UI")
	void RefreshFromCharacter(ASACharacter* InCharacter);

protected:
	virtual void NativeConstruct() override;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UProgressBar> HealthProgressBar;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> HealthText;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<USAAmmoWidget> AmmoWidget;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> WaveStateText;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> WaveNumberText;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> WaveEnemyText;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> WaveCountdownText;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> ScoreText;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> ComboText;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> ComboMultiplierText;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> BestComboText;

private:
	UPROPERTY()
	TObjectPtr<ASACharacter> CachedCharacter;

	void RefreshWaveDisplay();
	void RefreshScoreDisplay();
	const ASAPlayerState* GetDisplayPlayerState() const;
	FText BuildWaveStateText(const ASAWaveGameState* WaveGameState) const;
};

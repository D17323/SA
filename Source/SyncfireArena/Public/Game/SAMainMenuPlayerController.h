#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerController.h"
#include "SAMainMenuPlayerController.generated.h"

class USAMainMenuUI;

UCLASS()
class SYNCFIREARENA_API ASAMainMenuPlayerController : public APlayerController
{
	GENERATED_BODY()

public:
	ASAMainMenuPlayerController();

protected:
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

private:
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Syncfire|Menu", meta = (AllowPrivateAccess = "true"))
	TSubclassOf<USAMainMenuUI> MainMenuUIClass;

	UPROPERTY()
	TObjectPtr<USAMainMenuUI> MainMenuUI;

	void CreateMainMenu();
	void ApplyMenuInputMode();
};

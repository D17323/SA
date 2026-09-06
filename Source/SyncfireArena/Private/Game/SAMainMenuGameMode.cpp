#include "Game/SAMainMenuGameMode.h"

#include "Game/SAMainMenuPlayerController.h"

ASAMainMenuGameMode::ASAMainMenuGameMode()
{
	PlayerControllerClass = ASAMainMenuPlayerController::StaticClass();
	DefaultPawnClass = nullptr;
	bStartPlayersAsSpectators = true;
}

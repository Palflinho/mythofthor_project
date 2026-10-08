#include "Player/MythGameMode.h"

#include "Player/MythPlayerCharacter.h"
#include "Player/MythPlayerController.h"

AMythGameMode::AMythGameMode()
{
	DefaultPawnClass = AMythPlayerCharacter::StaticClass();
	PlayerControllerClass = AMythPlayerController::StaticClass();
}

#include "Player/MythGameMode.h"

#include "Player/MythPlayerCharacter.h"
#include "Player/MythPlayerController.h"
#include "UI/MythHUD.h"
#include "World/MythDayNightActor.h"
#include "EngineUtils.h"

AMythGameMode::AMythGameMode()
{
	DefaultPawnClass = AMythPlayerCharacter::StaticClass();
	PlayerControllerClass = AMythPlayerController::StaticClass();
	HUDClass = AMythHUD::StaticClass();
}

void AMythGameMode::BeginPlay()
{
	Super::BeginPlay();

	// Usa um AMythDayNightActor colocado no nivel; se nao houver, cria um.
	for (TActorIterator<AMythDayNightActor> It(GetWorld()); It; ++It)
	{
		DayNight = *It;
		return;
	}
	DayNight = GetWorld()->SpawnActor<AMythDayNightActor>();
}

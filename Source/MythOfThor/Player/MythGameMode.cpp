#include "Player/MythGameMode.h"

#include "Player/MythPlayerCharacter.h"
#include "Player/MythPlayerController.h"
#include "UI/MythHUD.h"
#include "World/MythDayNightActor.h"
#include "World/MythMapInfo.h"
#include "World/MythNightAudioComponent.h"
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
		break;
	}
	if (!DayNight)
	{
		DayNight = GetWorld()->SpawnActor<AMythDayNightActor>();
	}

	if (const AMythMapInfo* Info = AMythMapInfo::Find(GetWorld()))
	{
		if (Info->Bgm && DayNight && DayNight->Audio)
		{
			DayNight->Audio->PlayBGM(Info->Bgm, Info->BgmVolume);
		}
	}
}

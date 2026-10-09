#include "Player/MythGameMode.h"

#include "Player/MythPlayerCharacter.h"
#include "Player/MythPlayerController.h"
#include "UI/MythHUD.h"
#include "World/MythDayNightActor.h"
#include "World/MythWeatherActor.h"
#include "World/MythMapInfo.h"
#include "World/MythNightAudioComponent.h"
#include "EngineUtils.h"
#include "NavigationSystem.h"

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

	// Clima HD-2D (comeca limpo; muda pelo console MythWeather ou, com a rede, pelo servidor)
	AMythWeatherActor::FindOrSpawn(GetWorld());

	// O Python do editor nao consegue criar o RecastNavMesh; cria aqui (se faltar) e gera a navegacao
	// dentro dos NavMeshBoundsVolume do nivel para o Point & Click achar caminho.
	if (UNavigationSystemV1* Nav = FNavigationSystem::GetCurrent<UNavigationSystemV1>(GetWorld()))
	{
		Nav->GetDefaultNavDataInstance(FNavigationSystem::Create);
		Nav->Build();
	}

	if (const AMythMapInfo* Info = AMythMapInfo::Find(GetWorld()))
	{
		if (Info->Bgm && DayNight && DayNight->Audio)
		{
			DayNight->Audio->PlayBGM(Info->Bgm, Info->BgmVolume);
		}
	}
}

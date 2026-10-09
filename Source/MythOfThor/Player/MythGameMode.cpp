#include "Player/MythGameMode.h"

#include "Player/MythPlayerCharacter.h"
#include "Player/MythPlayerController.h"
#include "UI/MythHUD.h"
#include "World/MythDayNightActor.h"
#include "World/MythWeatherActor.h"
#include "World/MythNetWorld.h"
#include "Engine/StaticMeshActor.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/StaticMesh.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Components/ExponentialHeightFogComponent.h"
#include "UObject/UObjectIterator.h"
#include "World/MythMapInfo.h"
#include "World/MythNightLight.h"
#include "Components/LightComponent.h"
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
	AMythNetWorld::FindOrSpawn(GetWorld()); // ponte com o rAthena (so age quando conectado)

	// O Python do editor nao consegue criar o RecastNavMesh; cria aqui (se faltar) e gera a navegacao
	// dentro dos NavMeshBoundsVolume do nivel para o Point & Click achar caminho.
	if (UNavigationSystemV1* Nav = FNavigationSystem::GetCurrent<UNavigationSystemV1>(GetWorld()))
	{
		Nav->GetDefaultNavDataInstance(FNavigationSystem::Create);
		Nav->Build();
	}

	if (const AMythMapInfo* Info = AMythMapInfo::Find(GetWorld()); Info && Info->IsIndoor())
	{
		// Interior do RO: o vazio entre as salas e preto (sem neblina) e e sempre noite, com as luzes do mapa acesas
		// A atmosfera continua (a SkyLight precisa dela para a luz ambiente); um piso preto bem abaixo do mapa
		// tapa o ceu que apareceria entre as salas, ja que a camera interna olha para baixo e nao ve o horizonte.
		if (UStaticMesh* Plane = LoadObject<UStaticMesh>(nullptr, TEXT("/Engine/BasicShapes/Plane.Plane")))
		{
			FActorSpawnParameters VoidParams;
			VoidParams.Name = TEXT("MythIndoorVoid");
			if (AStaticMeshActor* Void = GetWorld()->SpawnActor<AStaticMeshActor>(FVector(0.f, 0.f, -2000.f), FRotator::ZeroRotator, VoidParams))
			{
				UStaticMeshComponent* C = Void->GetStaticMeshComponent();
				C->SetMobility(EComponentMobility::Movable);
				C->SetStaticMesh(Plane);
				C->SetWorldScale3D(FVector(2000.f, 2000.f, 1.f)); // 2 km: cobre qualquer mapa
				C->SetCollisionEnabled(ECollisionEnabled::NoCollision);
				C->SetCastShadow(false);
				if (UMaterialInterface* Black = LoadObject<UMaterialInterface>(nullptr, TEXT("/Game/Myth/M_IndoorVoid.M_IndoorVoid")))
				{
					C->SetMaterial(0, Black); // unlit preto (tools/ue_setup_weather.py): nao recebe luz nem sombra
				}
			}
		}
		for (TObjectIterator<UExponentialHeightFogComponent> It; It; ++It)
		{
			if (It->GetWorld() == GetWorld())
			{
				It->SetVisibility(false);
			}
		}
		if (DayNight)
		{
			DayNight->bFreezeTime = true;
			// Noite de interior: tom quente de lamparina, exposicao quase normal e luar fraco; quem ilumina sao as luzes
			// do .rsw (lamparinas, velas, lareiras), mais fortes que os postes da rua
			DayNight->NightExposureBias = -0.5f;
			DayNight->NightColorGain = FVector(1.0f, 0.97f, 0.93f); // quase neutro: o calor vem das lamparinas
			DayNight->NightSaturation = 0.85f;
			DayNight->MoonIntensityLux = 0.f; // sem luar azul: a exposicao automatica o realcava e tingia tudo
			DayNight->NightSkyColor = FLinearColor(0.62f, 0.52f, 0.42f);
			DayNight->NightSkyLightIntensity = 0.06f; // o ceu noturno e azul; quase sem luz ambiente, as lamparinas mandam
			for (TActorIterator<AMythNightLight> L(GetWorld()); L; ++L)
			{
				L->NightIntensity *= IndoorLightBoost;
			}
			// Sempre noite dentro das casas: as luzes do .rsw (lamparinas, velas, lareiras) iluminam pelo Lumen
			DayNight->SetTimeOfDay(22.f);
		}
	}

	if (const AMythMapInfo* Info = AMythMapInfo::Find(GetWorld()))
	{
		if (Info->Bgm && DayNight && DayNight->Audio)
		{
			DayNight->Audio->PlayBGM(Info->Bgm, Info->BgmVolume);
		}
	}
}

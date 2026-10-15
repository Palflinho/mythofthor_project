#include "World/MythNightLight.h"

#include "World/MythDayNightActor.h"
#include "World/MythMapInfo.h"
#include "Sprites/MythSprEffect.h"
#include "Components/PointLightComponent.h"

AMythNightLight::AMythNightLight()
{
	PrimaryActorTick.bCanEverTick = true;
	PrimaryActorTick.TickInterval = 0.04f; // 25 Hz e suave para a oscilacao de chama

	Light = CreateDefaultSubobject<UPointLightComponent>(TEXT("Light"));
	RootComponent = Light;
	Light->SetMobility(EComponentMobility::Movable);
	Light->SetLightColor(FLinearColor(1.0f, 0.58f, 0.16f)); // cor de fogo/tocha viva e quente
	Light->SetAttenuationRadius(450.f);
	Light->SetSourceRadius(16.f);
	Light->SetCastShadows(false); // sem sombras pesadas por tocha para manter alta performance
	Light->SetIntensityUnits(ELightUnits::Candelas);
	NightIntensity = 75.f; // intensidade aconchegante de tocha sem criar mancha estourada
	FlickerAmount = 0.22f;  // oscilacao organica de fogo
	Light->SetIntensity(0.f);
}

void AMythNightLight::BeginPlay()
{
	Super::BeginPlay();
	DayNight = AMythDayNightActor::Find(this);
	FlickerPhase = FMath::FRandRange(0.f, 100.f); // cada tocha oscila fora de fase

	const AMythMapInfo* MapInfo = AMythMapInfo::Find(GetWorld());
	if (MapInfo)
	{
		const FString Low = MapInfo->MapName.ToLower();
		if (MapInfo->IsIndoor() || Low.Contains(TEXT("dun")) || Low.Contains(TEXT("sewb")) || Low.Contains(TEXT("cave")) || Low.Contains(TEXT("anthell")) || Low.Contains(TEXT("maze")))
		{
			bAlwaysOn = true;
		}
	}

	// Adiciona a chama animada viva em loop no topo da tocha (torch_01.spr, 7 quadros do RO original)
	if (bShowFlame)
	{
		FlameEffect = AMythSprEffect::Attach(this, TEXT("torch_01"), false, 0.f, FlameZOffset);
	}
}

void AMythNightLight::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);
	float Night = 1.0f;
	if (DayNight.IsValid())
	{
		Night = DayNight->GetNightFactor();
	}
	else
	{
		DayNight = AMythDayNightActor::Find(this);
		if (DayNight.IsValid())
		{
			Night = DayNight->GetNightFactor();
		}
	}

	const float On = bAlwaysOn ? 1.0f : FMath::SmoothStep(TurnOnAt, FMath::Min(TurnOnAt + 0.35f, 1.f), Night);
	FlickerPhase += DeltaSeconds;
	// Oscilacao multifrequencial organica de labareda de fogo:
	const float Flicker = 1.f + FlickerAmount * (0.55f * FMath::Sin(FlickerPhase * 8.5f) + 0.30f * FMath::Sin(FlickerPhase * 19.3f) + 0.15f * FMath::Sin(FlickerPhase * 37.1f));
	Light->SetIntensity(NightIntensity * On * Flicker);
	Light->SetVisibility(On > 0.001f);

	if (FlameEffect.IsValid())
	{
		FlameEffect->SetActorHiddenInGame(On <= 0.001f);
	}
}


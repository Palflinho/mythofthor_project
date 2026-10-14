#include "World/MythDayNightActor.h"

#include "World/MythNightAudioComponent.h"
#include "World/MythMapInfo.h"

#include "Components/DirectionalLightComponent.h"
#include "Components/SkyAtmosphereComponent.h"
#include "Components/SkyLightComponent.h"
#include "Engine/DirectionalLight.h"
#include "Engine/SkyLight.h"
#include "EngineUtils.h"

namespace
{
	float Smooth(float Edge0, float Edge1, float X)
	{
		const float T = FMath::Clamp((X - Edge0) / (Edge1 - Edge0), 0.f, 1.f);
		return T * T * (3.f - 2.f * T);
	}
}

AMythDayNightActor::AMythDayNightActor()
{
	PrimaryActorTick.bCanEverTick = true;

	Root = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
	SetRootComponent(Root);

	Sun = CreateDefaultSubobject<UDirectionalLightComponent>(TEXT("Sun"));
	Sun->SetupAttachment(Root);
	Sun->SetMobility(EComponentMobility::Movable);
	Sun->SetAtmosphereSunLight(true);
	Sun->SetAtmosphereSunLightIndex(0);

	Moon = CreateDefaultSubobject<UDirectionalLightComponent>(TEXT("Moon"));
	Moon->SetupAttachment(Root);
	Moon->SetMobility(EComponentMobility::Movable);
	Moon->SetAtmosphereSunLight(true);
	Moon->SetAtmosphereSunLightIndex(1);
	Moon->SetLightColor(FLinearColor(0.55f, 0.65f, 1.f));

	SkyLight = CreateDefaultSubobject<USkyLightComponent>(TEXT("SkyLight"));
	SkyLight->SetupAttachment(Root);
	SkyLight->SetMobility(EComponentMobility::Movable);
	SkyLight->SetRealTimeCaptureEnabled(true); // acompanha o ceu conforme a hora muda

	Atmosphere = CreateDefaultSubobject<USkyAtmosphereComponent>(TEXT("Atmosphere"));
	Atmosphere->SetupAttachment(Root);

	Audio = CreateDefaultSubobject<UMythNightAudioComponent>(TEXT("Audio"));
}

void AMythDayNightActor::BeginPlay()
{
	Super::BeginPlay();
	TimeOfDay = FMath::Fmod(StartHour, 24.f);
	if (bReplaceLevelLights)
	{
		HideLevelLights();
	}
	UpdateLighting();
}

void AMythDayNightActor::HideLevelLights()
{
	UWorld* World = GetWorld();
	for (TActorIterator<ADirectionalLight> It(World); It; ++It)
	{
		It->GetLightComponent()->SetVisibility(false);
	}
	for (TActorIterator<ASkyLight> It(World); It; ++It)
	{
		It->GetLightComponent()->SetVisibility(false);
	}
	for (TActorIterator<ASkyAtmosphere> It(World); It; ++It)
	{
		It->GetComponent()->SetVisibility(false);
	}
}

void AMythDayNightActor::SetTimeOfDay(float Hour)
{
	TimeOfDay = FMath::Fmod(FMath::Fmod(Hour, 24.f) + 24.f, 24.f);
	UpdateLighting();
}

void AMythDayNightActor::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);
	if (!bFreezeTime && DayLengthSeconds > 1.f)
	{
		TimeOfDay = FMath::Fmod(TimeOfDay + DeltaSeconds * 24.f / DayLengthSeconds, 24.f);
	}
	UpdateLighting();
}

void AMythDayNightActor::UpdateLighting()
{
	const AMythMapInfo* MapInfo = AMythMapInfo::Find(GetWorld());

	// Calcula a hora local considerando o fuso horário (GridX) e a Noite Polar
	float EffectiveHour = TimeOfDay;
	bool bIsDungeon = false;
	int32 LatitudeY = 0;

	if (MapInfo)
	{
		EffectiveHour = MapInfo->GetLocalHour(TimeOfDay);
		bIsDungeon = MapInfo->EnvironmentData.bIsDungeon;
		LatitudeY = MapInfo->EnvironmentData.GridY;
	}

	if (bIsDungeon)
	{
		// Dungeons / Masmorras (Esgotos, Tumbas, Cavernas): Sol e Lua desligados, iluminação ambiente suave
		Sun->SetIntensity(0.f);
		Moon->SetIntensity(0.f);
		SkyLight->SetIntensity(NightSkyLightIntensity * 0.4f);
		SkyLight->SetLightColor(FLinearColor(0.2f, 0.25f, 0.35f));

		if (!FMath::IsNearlyEqual(1.0f, NightFactor, 0.002f))
		{
			NightFactor = 1.0f;
			Audio->SetNightFactor(NightFactor);
			OnNightFactorChanged.Broadcast(NightFactor);
		}
		return;
	}

	// Ângulo solar: 0 deg às 6h (horizonte), 90 deg ao meio-dia, 180 deg às 18h.
	const float ThetaDeg = (EffectiveHour - 6.f) * 15.f;
	const float S = FMath::Sin(FMath::DegreesToRadians(ThetaDeg)); // elevação do sol (-1..1)

	const float Daylight = Smooth(-0.10f, 0.25f, S);
	const float NewNight = 1.f - Daylight;

	// A luz direcional aponta no sentido oposto ao astro: pitch = -theta (sol) e -(theta + 180) (lua).
	Sun->SetWorldRotation(FRotator(-ThetaDeg, SunYaw, 0.f));
	Moon->SetWorldRotation(FRotator(-(ThetaDeg + 180.f), SunYaw, 0.f));

	// Latitude (GridY):
	// Y < 0 (Sul / Morroc / Comodo): Clima árido/tropical, luz mais quente e dourada.
	// Y > 0 (Norte / Aldebaran / Lutie): Clima frio/ártico, luz mais azulada e fria.
	FLinearColor Warm(1.f, 0.55f, 0.25f);
	FLinearColor White(1.f, 0.97f, 0.90f);

	if (LatitudeY < 0)
	{
		// Mais avermelhado/dourado
		Warm = FLinearColor(1.f, 0.48f, 0.15f);
		White = FLinearColor(1.f, 0.92f, 0.80f);
	}
	else if (LatitudeY > 0)
	{
		// Mais azulado/frio
		Warm = FLinearColor(0.95f, 0.65f, 0.45f);
		White = FLinearColor(0.90f, 0.95f, 1.0f);
	}

	Sun->SetLightColor(FMath::Lerp(Warm, White, Smooth(0.05f, 0.45f, S)));
	Sun->SetIntensity(SunIntensityLux * Daylight);

	Moon->SetIntensity(MoonIntensityLux * Smooth(0.f, 0.3f, -S));

	SkyLight->SetIntensity(FMath::Lerp(NightSkyLightIntensity, DaySkyLightIntensity, Daylight));
	SkyLight->SetLightColor(FMath::Lerp(FLinearColor(0.45f, 0.55f, 1.f), FLinearColor::White, Daylight));

	if (!FMath::IsNearlyEqual(NewNight, NightFactor, 0.002f))
	{
		NightFactor = NewNight;
		Audio->SetNightFactor(NightFactor);
		OnNightFactorChanged.Broadcast(NightFactor);
	}
}

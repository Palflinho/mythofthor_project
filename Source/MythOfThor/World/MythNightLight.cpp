#include "World/MythNightLight.h"

#include "World/MythDayNightActor.h"
#include "Components/PointLightComponent.h"

AMythNightLight::AMythNightLight()
{
	PrimaryActorTick.bCanEverTick = true;
	PrimaryActorTick.TickInterval = 0.05f; // a oscilacao nao precisa de 60 Hz

	Light = CreateDefaultSubobject<UPointLightComponent>(TEXT("Light"));
	RootComponent = Light;
	Light->SetMobility(EComponentMobility::Movable);
	Light->SetLightColor(FLinearColor(1.f, 0.72f, 0.42f)); // luz de lampiao, quente
	Light->SetAttenuationRadius(650.f);
	Light->SetSourceRadius(8.f);
	Light->SetCastShadows(false); // dezenas de luzes: sombras so do sol/lua
	Light->SetIntensityUnits(ELightUnits::Candelas);
	Light->SetIntensity(0.f);
}

void AMythNightLight::BeginPlay()
{
	Super::BeginPlay();
	DayNight = AMythDayNightActor::Find(this);
	FlickerPhase = FMath::FRandRange(0.f, 100.f); // cada luz oscila fora de fase
}

void AMythNightLight::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);
	if (!DayNight.IsValid())
	{
		DayNight = AMythDayNightActor::Find(this);
		if (!DayNight.IsValid())
		{
			return;
		}
	}
	const float Night = DayNight->GetNightFactor();
	const float On = FMath::SmoothStep(TurnOnAt, FMath::Min(TurnOnAt + 0.35f, 1.f), Night);
	FlickerPhase += DeltaSeconds;
	const float Flicker = 1.f + FlickerAmount * (0.6f * FMath::Sin(FlickerPhase * 9.f) + 0.4f * FMath::Sin(FlickerPhase * 23.f));
	Light->SetIntensity(NightIntensity * On * Flicker);
	Light->SetVisibility(On > 0.001f);
}

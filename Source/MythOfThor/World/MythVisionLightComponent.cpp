#include "World/MythVisionLightComponent.h"

#include "World/MythDayNightActor.h"
#include "World/MythMapInfo.h"
#include "Network/RagnarokNetworkSubsystem.h"

#include "Engine/GameInstance.h"
#include "Engine/World.h"

UMythVisionLightComponent::UMythVisionLightComponent()
{
	PrimaryComponentTick.bCanEverTick = true;
	PrimaryComponentTick.TickInterval = 0.1f;
	SetMobility(EComponentMobility::Movable);
	SetIntensityUnits(ELightUnits::Candelas);
	SetIntensity(0.f);
	SetLightColor(FLinearColor(1.f, 0.9f, 0.76f)); // branco quente, como uma tocha distante
	SetSourceRadius(20.f);
	SetSoftSourceRadius(60.f);
	SetCastShadows(false); // sem sombra propria: nao briga com a sombra do sprite nem com a do sol/lua
	SetAttenuationRadius(BaseRadius);
	SetVisibility(false);
}

void UMythVisionLightComponent::BeginPlay()
{
	Super::BeginPlay();
	DayNight = AMythDayNightActor::Find(this);
	const AMythMapInfo* Info = AMythMapInfo::Find(GetWorld());
	bIndoor = Info && Info->IsIndoor();
}

float UMythVisionLightComponent::ClassBonus(int32 JobClass)
{
	switch (JobClass)
	{
	case 3: case 11: case 19: case 20: return 250.f;           // arqueiro, cacador, bardo, odalisca
	case 2: case 9: case 16: case 4: case 8: case 15: return 150.f; // mago, bruxo, sabio, novico, sacerdote, monge
	case 6: case 12: case 17: return 100.f;                      // gatuno, mercenario, arruaceiro
	default: return 0.f;
	}
}

float UMythVisionLightComponent::GetVisionRadius() const
{
	int32 Dex = 1;
	int32 Job = 0;
	if (const UGameInstance* GI = GetWorld() ? GetWorld()->GetGameInstance() : nullptr)
	{
		if (const URagnarokNetworkSubsystem* Net = GI->GetSubsystem<URagnarokNetworkSubsystem>())
		{
			if (Net->GetNetworkState() == ERagnarokNetworkState::ConnectedMap)
			{
				Dex = Net->GetPlayerStats().Dex;
				Job = Net->GetPlayerStats().JobClass;
			}
		}
	}
	return BaseRadius + RadiusPerDex * FMath::Clamp(Dex, 1, 130) + ClassBonus(Job);
}

void UMythVisionLightComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);
	if (!DayNight.IsValid())
	{
		DayNight = AMythDayNightActor::Find(this);
	}
	const float Night = (!bIndoor && DayNight.IsValid()) ? DayNight->GetNightFactor() : 0.f;
	const float On = FMath::SmoothStep(0.3f, 0.85f, Night);
	SetVisibility(On > 0.001f);
	SetIntensity(NightIntensity * On);
	SetAttenuationRadius(GetVisionRadius());
}

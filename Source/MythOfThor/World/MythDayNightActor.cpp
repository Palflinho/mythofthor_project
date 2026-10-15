#include "World/MythDayNightActor.h"

#include "World/MythNightAudioComponent.h"
#include "World/MythMapInfo.h"
#include "World/MythMapEnvironment.h"
#include "UI/MythOptions.h"

#include "Components/DirectionalLightComponent.h"
#include "Components/SkyAtmosphereComponent.h"
#include "Components/SkyLightComponent.h"
#include "Components/PostProcessComponent.h"
#include "Materials/MaterialInterface.h"
#include "Engine/DirectionalLight.h"
#include "Engine/SkyLight.h"
#include "EngineUtils.h"
#include "Components/StaticMeshComponent.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/PlayerController.h"
#include "Camera/PlayerCameraManager.h"
#include "UObject/UObjectIterator.h"

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
	Sun->SetLightSourceAngle(1.5f); // penumbra: borda das sombras mais macia
	Sun->SetAtmosphereSunLight(true);
	Sun->SetAtmosphereSunLightIndex(0);
	Sun->ForwardShadingPriority = 2;

	Moon = CreateDefaultSubobject<UDirectionalLightComponent>(TEXT("Moon"));
	Moon->SetupAttachment(Root);
	Moon->SetMobility(EComponentMobility::Movable);
	Moon->SetAtmosphereSunLight(true);
	Moon->SetAtmosphereSunLightIndex(1);
	Moon->SetLightColor(FLinearColor(0.55f, 0.65f, 1.f));
	Moon->ForwardShadingPriority = 0;

	SkyLight = CreateDefaultSubobject<USkyLightComponent>(TEXT("SkyLight"));
	SkyLight->SetupAttachment(Root);
	SkyLight->SetMobility(EComponentMobility::Movable);
	SkyLight->SetRealTimeCaptureEnabled(true); // acompanha o ceu conforme a hora muda

	Atmosphere = CreateDefaultSubobject<USkyAtmosphereComponent>(TEXT("Atmosphere"));
	Atmosphere->SetupAttachment(Root);
	Atmosphere->SetMultiScatteringFactor(1.5f);
	Atmosphere->SetGroundAlbedo(FColor(175, 210, 245)); // Horizonte e borda do mapa com tom suave de ceu claro, sem vacuo escuro

	Audio = CreateDefaultSubobject<UMythNightAudioComponent>(TEXT("Audio"));

	// Exposicao/cor controladas pelo horario: sem isso a adaptacao automatica da camera "clareia" a noite
	// ate parecer dia. Volume global com prioridade acima do PostProcess do nivel.
	Grading = CreateDefaultSubobject<UPostProcessComponent>(TEXT("Grading"));
	Grading->SetupAttachment(Root);
	Grading->bUnbound = true;
	Grading->Priority = 10.f;
	FPostProcessSettings& PP = Grading->Settings;
	PP.bOverride_AutoExposureBias = true;
	PP.bOverride_ColorGain = true;
	PP.bOverride_ColorSaturation = true;
	// HD-2D lens (GDD: tilt-shift depth of field focused on the character, strong bloom, light shafts); values are set every frame in UpdateHD2DLens
	PP.bOverride_DepthOfFieldFstop = true;
	PP.bOverride_DepthOfFieldFocalDistance = true;
	PP.bOverride_DepthOfFieldSensorWidth = true;
	PP.bOverride_BloomIntensity = true;
	PP.bOverride_BloomThreshold = true;
	PP.bOverride_ColorContrast = true;
	Sun->bEnableLightShaftBloom = true;
	Sun->BloomScale = 0.25f;
	Sun->BloomThreshold = 0.f;
	Sun->BloomMaxBrightness = 20.f;
	Sun->BloomTint = FColor(255, 232, 190);
}

void AMythDayNightActor::BeginPlay()
{
	Super::BeginPlay();
	TimeOfDay = FMath::Fmod(StartHour, 24.f);
	// HD-2D tilt-shift blur (tools/ue_make_tiltshift_material.py): blendable on the global grading volume, weight follows the option
	if (Grading)
	{
		if (UMaterialInterface* TS = LoadObject<UMaterialInterface>(nullptr, TEXT("/Game/Myth/M_TiltShift.M_TiltShift")))
		{
			Grading->Settings.AddBlendable(TS, 1.f);
		}
		if (UMaterialInterface* Occ = LoadObject<UMaterialInterface>(nullptr, TEXT("/Game/Myth/M_OcclusionOutline.M_OcclusionOutline")))
		{
			Grading->Settings.AddBlendable(Occ, 1.f);
		}
	}

	// Sombras de Nuvens HD-2D (GDD s.7.1): material de Light Function para o sol
	if (Sun && bEnableCloudShadows)
	{
		const AMythMapInfo* MapInfo = AMythMapInfo::Find(GetWorld());
		const bool bIsIndoor = MapInfo && MapInfo->IsIndoor();
		if (!bIsIndoor)
		{
			if (UMaterialInterface* CloudMat = LoadObject<UMaterialInterface>(nullptr, TEXT("/Game/Myth/M_CloudShadows.M_CloudShadows")))
			{
				Sun->SetLightFunctionMaterial(CloudMat);
				Sun->SetLightFunctionScale(CloudShadowScale);
			}
		}
	}

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
	UpdateHD2DLens();
}

void AMythDayNightActor::UpdateHD2DLens()
{
	if (!Grading)
	{
		return;
	}
	FPostProcessSettings& PP = Grading->Settings;
	const bool bOn = FMythOptions::Get(TEXT("HD2DLook"));
	for (FWeightedBlendable& B : Grading->Settings.WeightedBlendables.Array)
	{
		if (B.Object && B.Object->GetName().Contains(TEXT("TiltShift")))
		{
			B.Weight = bOn ? 1.f : 0.f; // apenas a lente tilt-shift e ligada/desligada por esta opcao
		}
	}
	if (!bOn)
	{
		PP.DepthOfFieldFstop = 32.f; // no blur
		PP.BloomIntensity = 0.675f;
		PP.BloomThreshold = -1.f;
		PP.ColorContrast = FVector4(1.f, 1.f, 1.f, 1.f);
		Sun->bEnableLightShaftBloom = false;
		return;
	}
	Sun->bEnableLightShaftBloom = true;
	// Focus on the player: the camera-to-character distance is the focal plane, so the foreground and the horizon soften like a miniature
	if (const APlayerController* PC = GetWorld() ? GetWorld()->GetFirstPlayerController() : nullptr; PC && PC->PlayerCameraManager && PC->GetViewTarget())
	{
		const float Dist = FVector::Dist(PC->PlayerCameraManager->GetCameraLocation(), PC->GetViewTarget()->GetActorLocation());
		PP.DepthOfFieldFocalDistance = FMath::Max(Dist, 100.f);
	}
	PP.DepthOfFieldSensorWidth = 24.576f;
	PP.DepthOfFieldFstop = HD2DFstop;
	PP.BloomIntensity = HD2DBloom;
	PP.BloomThreshold = 0.6f;
	PP.ColorContrast = FVector4(1.05f, 1.05f, 1.05f, 1.f);
}

void AMythDayNightActor::UpdateLighting()
{
	// Regional environment (HD-2D GDD branch): local hour by timezone, perpetual night and latitude tint. Not for interiors (fixed lamp lighting).
	float EffectiveHour = TimeOfDay;
	int32 LatitudeY = 0;
	if (!MapInfoCache.IsValid() && GetWorld() && GetWorld()->GetTimeSeconds() >= NextMapInfoSearch)
	{
		MapInfoCache = AMythMapInfo::Find(GetWorld());
		NextMapInfoSearch = GetWorld()->GetTimeSeconds() + 2.0; // do not scan the actors every frame on maps without a MapInfo
	}
	bool bIsDungeon = false;
	if (const AMythMapInfo* MapInfo = MapInfoCache.Get())
	{
		if (const FMythMapEnv* Env = MapInfo->GetEnvironment())
		{
			bIsDungeon = Env->bDungeon;
			if (!MapInfo->IsIndoor() && FMythOptions::Get(TEXT("RegionalTime")))
			{
				EffectiveHour = Env->bPerpetualNight ? 0.f : MapInfo->GetLocalHour(TimeOfDay);
				LatitudeY = Env->GridY;
			}
		}
	}

	if (bIsDungeon)
	{
		// Sinalizador de Masmorra (HD-2D):
		// Apaga o sol e a lua; mantem a iluminacao ambiente suave de masmorra sem escuridao absoluta,
		// e as tochas/cristais (NightGlow) acendem com forca maxima (Lumen).
		Sun->SetVisibility(false);
		Sun->SetIntensity(0.f);
		Moon->SetVisibility(false);
		Moon->SetIntensity(0.f);
		Sun->ForwardShadingPriority = 0;
		Moon->ForwardShadingPriority = 0;
		if (!bDungeonLightingApplied)
		{
			bDungeonLightingApplied = true;
			if (SkyLight)
			{
				SkyLight->SetRealTimeCaptureEnabled(false);
			}
			if (Atmosphere)
			{
				Atmosphere->SetVisibility(false);
			}
		}
		// Luz natural/ambiente de masmorra calibrada 20% mais clara para otima legibilidade e atmosfera
		SkyLight->SetIntensity(0.12f);
		SkyLight->SetLightColor(FLinearColor(0.32f, 0.38f, 0.50f));
		if (Grading)
		{
			FPostProcessSettings& PP = Grading->Settings;
			PP.AutoExposureBias = -0.15f;
			PP.ColorGain = FVector4(0.92f, 0.95f, 1.05f, 1.f);
			PP.ColorSaturation = FVector4(1.f, 1.f, 1.f, 0.92f);
		}
		if (!FMath::IsNearlyEqual(1.0f, AppliedNightGlow, 0.01f))
		{
			AppliedNightGlow = 1.0f;
			for (TObjectIterator<UStaticMeshComponent> It; It; ++It)
			{
				if (It->IsValidLowLevel() && !It->IsUnreachable() && !It->HasAnyFlags(RF_BeginDestroyed | RF_FinishDestroyed) && It->GetWorld() == GetWorld() && It->GetOwner() && !It->GetOwner()->IsA<APawn>())
				{
					It->SetScalarParameterForCustomPrimitiveData(TEXT("NightGlow"), 1.0f);
				}
			}
		}
		NightFactor = 1.0f;
		return;
	}
	else
	{
		if (bDungeonLightingApplied)
		{
			bDungeonLightingApplied = false;
			if (Atmosphere)
			{
				Atmosphere->SetVisibility(true);
			}
			if (SkyLight)
			{
				SkyLight->SetRealTimeCaptureEnabled(true);
			}
		}
	}

	// Angulo solar: 0 deg as 6h (horizonte), 90 deg ao meio-dia, 180 deg as 18h.
	const float ThetaDeg = (EffectiveHour - 6.f) * 15.f;
	const float S = FMath::Sin(FMath::DegreesToRadians(ThetaDeg)); // elevacao do sol (-1..1)

	const float Daylight = Smooth(-0.10f, 0.25f, S);
	const float NewNight = 1.f - Daylight;

	// A luz direcional aponta no sentido oposto ao astro: pitch = -theta (sol) e -(theta + 180) (lua).
	Sun->SetWorldRotation(FRotator(-ThetaDeg, SunYaw, 0.f));
	Moon->SetWorldRotation(FRotator(-(ThetaDeg + 180.f), SunYaw, 0.f));

	// Sol dourado perto do horizonte, branco alto no ceu.
	FLinearColor Warm(1.f, 0.55f, 0.25f);
	FLinearColor White(1.f, 0.97f, 0.90f);
	if (LatitudeY < 0) // south (Morroc, Comodo): warmer, golden light
	{
		Warm = FLinearColor(1.f, 0.48f, 0.15f);
		White = FLinearColor(1.f, 0.92f, 0.80f);
	}
	else if (LatitudeY > 0) // north (Aldebaran, Lutie): colder, bluer light
	{
		Warm = FLinearColor(0.95f, 0.65f, 0.45f);
		White = FLinearColor(0.90f, 0.95f, 1.0f);
	}
	Sun->SetLightColor(FMath::Lerp(Warm, White, Smooth(0.05f, 0.45f, S)));
	// Chuva/neve/neblina (AMythWeatherActor) escurecem o sol e o ceu
	Sun->SetIntensity(SunIntensityLux * Daylight * (1.f - 0.75f * WeatherDim));

	Moon->SetIntensity(MoonIntensityLux * Smooth(0.f, 0.3f, -S));

	// So uma direcional "dominante" por vez: a outra fica desligada quando nao contribui
	// (evita o aviso de luzes direcionais competindo pelo sombreamento e pela neblina).
	const bool bSunUp = Daylight > 0.001f;
	Sun->SetVisibility(bSunUp);
	Moon->SetVisibility(!bSunUp);
	Sun->SetCastShadows(bSunUp);
	Moon->SetCastShadows(!bSunUp);
	Sun->ForwardShadingPriority = bSunUp ? 2 : 0;
	Moon->ForwardShadingPriority = bSunUp ? 0 : 2;

	// Noite de verdade: exposicao abaixo da do dia, cores mais frias e menos saturadas (os postes se destacam).
	if (Grading)
	{
		FPostProcessSettings& PP = Grading->Settings;
		PP.AutoExposureBias = FMath::Lerp(0.f, NightExposureBias, NewNight) - 0.6f * WeatherDim;
		PP.ColorGain = FVector4(FMath::Lerp(FVector(1.f), NightColorGain, NewNight), 1.f);
		PP.ColorSaturation = FVector4(1.f, 1.f, 1.f, FMath::Lerp(1.f, NightSaturation, NewNight) * (1.f - 0.25f * WeatherDim));
	}

	SkyLight->SetIntensity(FMath::Lerp(NightSkyLightIntensity, DaySkyLightIntensity, Daylight) * (1.f - 0.35f * WeatherDim));
	SkyLight->SetLightColor(FMath::Lerp(NightSkyColor, FLinearColor::White, Daylight));

	if (!FMath::IsNearlyEqual(NewNight, NightFactor, 0.002f))
	{
		NightFactor = NewNight;
		Audio->SetNightFactor(NightFactor);
		OnNightFactorChanged.Broadcast(NightFactor);
	}

	// Janelas das casas acendem a noite: Custom Primitive Data 0 = NightGlow nos materiais do mapa (o Lumen espalha a luz na rua)
	const float Glow = Smooth(0.35f, 0.8f, NewNight);
	if (!FMath::IsNearlyEqual(Glow, AppliedNightGlow, 0.01f) || (Glow != AppliedNightGlow && (Glow == 0.f || Glow == 1.f)))
	{
		AppliedNightGlow = Glow;
		for (TObjectIterator<UStaticMeshComponent> It; It; ++It)
		{
			if (It->IsValidLowLevel() && !It->IsUnreachable() && !It->HasAnyFlags(RF_BeginDestroyed | RF_FinishDestroyed) && It->GetWorld() == GetWorld() && It->GetOwner() && !It->GetOwner()->IsA<APawn>())
			{
				It->SetScalarParameterForCustomPrimitiveData(TEXT("NightGlow"), Glow);
			}
		}
	}
}

FVector AMythDayNightActor::GetLightDirection() const
{
	const UDirectionalLightComponent* Light = NightFactor > 0.5f ? Moon.Get() : Sun.Get();
	return Light ? Light->GetForwardVector() : FVector(0.f, 0.f, -1.f);
}

AMythDayNightActor* AMythDayNightActor::Find(const UObject* WorldContext)
{
	const UWorld* World = WorldContext ? WorldContext->GetWorld() : nullptr;
	if (!World)
	{
		return nullptr;
	}
	for (TActorIterator<AMythDayNightActor> It(World); It; ++It)
	{
		return *It;
	}
	return nullptr;
}

#include "World/MythWeatherActor.h"

#include "World/MythDayNightActor.h"
#include "Camera/PlayerCameraManager.h"
#include "Components/ExponentialHeightFogComponent.h"
#include "Components/InstancedStaticMeshComponent.h"
#include "Engine/ExponentialHeightFog.h"
#include "Engine/StaticMesh.h"
#include "EngineUtils.h"
#include "GameFramework/PlayerController.h"
#include "Materials/MaterialInterface.h"

AMythWeatherActor::AMythWeatherActor()
{
	PrimaryActorTick.bCanEverTick = true;
	PrimaryActorTick.TickGroup = TG_PostUpdateWork; // depois da camera do quadro

	Root = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
	SetRootComponent(Root);

	Particles = CreateDefaultSubobject<UInstancedStaticMeshComponent>(TEXT("Particles"));
	Particles->SetupAttachment(Root);
	Particles->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	Particles->SetCastShadow(false);
	Particles->SetMobility(EComponentMobility::Movable);
	Particles->bAffectDistanceFieldLighting = false;
	Particles->SetCanEverAffectNavigation(false);
	if (UStaticMesh* Plane = LoadObject<UStaticMesh>(nullptr, TEXT("/Engine/BasicShapes/Plane.Plane")))
	{
		Particles->SetStaticMesh(Plane); // plano de 100x100 cm no XY: vira risco (chuva) ou floco (neve) pela escala
	}
}

AMythWeatherActor* AMythWeatherActor::FindOrSpawn(UWorld* World)
{
	if (!World)
	{
		return nullptr;
	}
	for (TActorIterator<AMythWeatherActor> It(World); It; ++It)
	{
		return *It;
	}
	return World->SpawnActor<AMythWeatherActor>();
}

void AMythWeatherActor::BeginPlay()
{
	Super::BeginPlay();
	RainMaterial = LoadObject<UMaterialInterface>(nullptr, TEXT("/Game/Weather/M_RainStreak.M_RainStreak"));
	SnowMaterial = LoadObject<UMaterialInterface>(nullptr, TEXT("/Game/Weather/M_SnowFlake.M_SnowFlake"));
	if (!RainMaterial || !SnowMaterial)
	{
		UE_LOG(LogTemp, Warning, TEXT("MythWeather: materiais do clima ausentes; rode tools/ue_setup_weather.py"));
	}
	for (TActorIterator<AExponentialHeightFog> It(GetWorld()); It; ++It)
	{
		Fog = *It;
		BaseFogDensity = It->GetComponent()->FogDensity;
		break;
	}
	DayNight = AMythDayNightActor::Find(this);
}

void AMythWeatherActor::SetWeather(EMythWeather NewWeather)
{
	if (NewWeather == Weather)
	{
		return;
	}
	Weather = NewWeather;
	UE_LOG(LogTemp, Log, TEXT("MythWeather: clima -> %s"), *UEnum::GetValueAsString(NewWeather));
}

FVector AMythWeatherActor::RandomOffset(bool bAnyHeight) const
{
	const float Angle = FMath::FRandRange(0.f, 2.f * PI);
	const float R = AreaRadius * FMath::Sqrt(FMath::FRand());
	const float Z = bAnyHeight ? FMath::FRandRange(-200.f, AreaHeight) : AreaHeight + FMath::FRandRange(0.f, 300.f);
	return FVector(FMath::Cos(Angle) * R, FMath::Sin(Angle) * R, Z);
}

void AMythWeatherActor::RebuildParticles(EMythWeather Kind)
{
	Particles->ClearInstances();
	State.Reset();
	Transforms.Reset();
	const int32 Count = Kind == EMythWeather::Rain ? RainDrops : (Kind == EMythWeather::Snow ? SnowFlakes : 0);
	UMaterialInterface* Mat = Kind == EMythWeather::Rain ? RainMaterial.Get() : SnowMaterial.Get();
	if (Count <= 0 || !Mat)
	{
		return;
	}
	Particles->SetMaterial(0, Mat);
	State.SetNum(Count);
	Transforms.SetNum(Count);
	for (FParticle& P : State)
	{
		P.Offset = RandomOffset(true);
		P.Speed = Kind == EMythWeather::Rain ? FMath::FRandRange(1300.f, 1700.f) : FMath::FRandRange(70.f, 130.f);
		P.Phase = FMath::FRandRange(0.f, 2.f * PI);
	}
	Particles->AddInstances(Transforms, false, /*bWorldSpace=*/true);
}

void AMythWeatherActor::UpdateParticles(float DeltaSeconds, const FVector& Center, const FVector& CamLoc, const FRotator& CamRot)
{
	if (State.IsEmpty())
	{
		return;
	}
	const bool bRain = Visible == EMythWeather::Rain;
	// Densidade pela transicao: so as primeiras N particulas aparecem (as demais ficam com escala zero)
	const int32 Active = FMath::RoundToInt(State.Num() * Intensity);
	const FRotator RainRot(90.f, CamRot.Yaw, 0.f);                   // plano em pe, de frente para a camera
	const FRotator SnowRot(CamRot.Pitch + 90.f, CamRot.Yaw, 0.f);    // floco sempre de frente para a camera
	const FVector RainScale(0.9f, 0.025f, 1.f);                      // risco: 90 cm de comprimento, 2,5 cm de largura
	const FVector SnowScale(0.16f, 0.16f, 1.f);                      // floco de 16 cm (a camera fica a ~15 m)
	const float Time = GetWorld()->GetTimeSeconds();

	for (int32 i = 0; i < State.Num(); ++i)
	{
		FParticle& P = State[i];
		if (bRain)
		{
			P.Offset += FVector(-60.f, 30.f, -P.Speed) * DeltaSeconds; // leve vento
		}
		else
		{
			P.Offset.Z -= P.Speed * DeltaSeconds;
			P.Offset.X += FMath::Sin(Time * 0.9f + P.Phase) * 35.f * DeltaSeconds; // floco balanca
			P.Offset.Y += FMath::Cos(Time * 0.7f + P.Phase) * 25.f * DeltaSeconds;
		}
		if (P.Offset.Z < -250.f || P.Offset.SizeSquared2D() > FMath::Square(AreaRadius * 1.1f))
		{
			P.Offset = RandomOffset(false);
		}
		// Particulas coladas na camera viram borroes gigantes: somem dentro de 4 m da lente
		const bool bVisible = i < Active && FVector::DistSquared(Center + P.Offset, CamLoc) > FMath::Square(400.f);
		const FVector Scale = bVisible ? (bRain ? RainScale : SnowScale) : FVector::ZeroVector;
		Transforms[i] = FTransform(bRain ? RainRot : SnowRot, Center + P.Offset, Scale);
	}
	Particles->BatchUpdateInstancesTransforms(0, Transforms, /*bWorldSpace=*/true, /*bMarkRenderStateDirty=*/true, /*bTeleport=*/true);
}

void AMythWeatherActor::ApplyAtmosphere()
{
	// Neblina e escurecimento acompanham a transicao
	const float Rain = Visible == EMythWeather::Rain ? Intensity : 0.f;
	const float Snow = Visible == EMythWeather::Snow ? Intensity : 0.f;
	const float FogW = Visible == EMythWeather::Fog ? Intensity : 0.f;
	if (Fog.IsValid())
	{
		const float Target = BaseFogDensity + (FogDensityFog - BaseFogDensity) * FogW + (FogDensityRain - BaseFogDensity) * FMath::Max(Rain, Snow * 0.6f);
		Fog->GetComponent()->SetFogDensity(FMath::Max(Target, BaseFogDensity * 0.5f));
	}
	if (!DayNight.IsValid())
	{
		DayNight = AMythDayNightActor::Find(this);
	}
	if (DayNight.IsValid())
	{
		DayNight->WeatherDim = FMath::Max3(Rain, Snow * 0.45f, FogW * 0.5f);
	}
}

void AMythWeatherActor::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);

	// Troca de clima: o visivel some ate 0 e so entao o novo entra
	const float Step = DeltaSeconds / FMath::Max(TransitionSeconds, 0.1f);
	if (Visible != Weather)
	{
		Intensity = FMath::Max(0.f, Intensity - Step);
		if (Intensity <= 0.f)
		{
			Visible = Weather;
			RebuildParticles(Visible);
		}
	}
	else if (Visible != EMythWeather::Clear)
	{
		Intensity = FMath::Min(1.f, Intensity + Step);
	}

	const APlayerController* PC = GetWorld()->GetFirstPlayerController();
	const APawn* Pawn = PC ? PC->GetPawn() : nullptr;
	if (Pawn && PC->PlayerCameraManager)
	{
		UpdateParticles(DeltaSeconds, Pawn->GetActorLocation(), PC->PlayerCameraManager->GetCameraLocation(), PC->PlayerCameraManager->GetCameraRotation());
	}
	ApplyAtmosphere();
}

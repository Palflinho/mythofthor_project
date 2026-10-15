#include "World/MythWeatherComponent.h"
#include "Kismet/GameplayStatics.h"
#include "Kismet/KismetMaterialLibrary.h"
#include "Camera/PlayerCameraManager.h"
#include "GameFramework/PlayerController.h"
#include "NiagaraFunctionLibrary.h"

UMythWeatherComponent::UMythWeatherComponent()
{
	PrimaryComponentTick.bCanEverTick = true;
}

void UMythWeatherComponent::BeginPlay()
{
	Super::BeginPlay();

	// Inicializa o clima a partir dos metadados do mapa atual
	if (const UWorld* World = GetWorld())
	{
		if (const AMythMapInfo* MapInfo = AMythMapInfo::Find(World))
		{
			SetWeather(MapInfo->EnvironmentData.DefaultWeather);
		}
	}
}

void UMythWeatherComponent::SetWeather(EMythWeatherType NewWeather)
{
	if (CurrentWeather == NewWeather && WeatherNiagaraComp)
	{
		return;
	}

	CurrentWeather = NewWeather;
	TargetWetness = (CurrentWeather == EMythWeatherType::Rain) ? 1.0f : 0.0f;

	UpdateNiagaraEmitter();
	OnWeatherChanged.Broadcast(CurrentWeather);

	UE_LOG(LogTemp, Log, TEXT("[Weather] Clima atualizado para enum %d (TargetWetness: %.2f)"),
		static_cast<int32>(CurrentWeather), TargetWetness);
}

void UMythWeatherComponent::TransitionRandomWeather()
{
	if (const UWorld* World = GetWorld())
	{
		if (const AMythMapInfo* MapInfo = AMythMapInfo::Find(World))
		{
			const TArray<EMythWeatherType>& Weathers = MapInfo->EnvironmentData.AllowedWeathers;
			if (Weathers.Num() > 0)
			{
				const int32 RandomIndex = FMath::RandRange(0, Weathers.Num() - 1);
				SetWeather(Weathers[RandomIndex]);
			}
		}
	}
}

void UMythWeatherComponent::UpdateNiagaraEmitter()
{
	UWorld* World = GetWorld();
	if (!World)
	{
		return;
	}

	UNiagaraSystem* SelectedFX = nullptr;
	switch (CurrentWeather)
	{
	case EMythWeatherType::Rain:              SelectedFX = FX_Rain; break;
	case EMythWeatherType::Snow:              SelectedFX = FX_Snow; break;
	case EMythWeatherType::Sandstorm:         SelectedFX = FX_Sandstorm; break;
	case EMythWeatherType::Fog:               SelectedFX = FX_Fog; break;
	case EMythWeatherType::IndustrialSmog:    SelectedFX = FX_IndustrialSmog; break;
	case EMythWeatherType::VolcanicAsh:       SelectedFX = FX_VolcanicAsh; break;
	case EMythWeatherType::CherryBlossom:     SelectedFX = FX_CherryBlossoms; break;
	case EMythWeatherType::DimensionalAurora: SelectedFX = FX_DimensionalAurora; break;
	default:
		SelectedFX = nullptr;
		break;
	}

	if (SelectedFX)
	{
		if (!WeatherNiagaraComp)
		{
			WeatherNiagaraComp = UNiagaraFunctionLibrary::SpawnSystemAtLocation(World, SelectedFX, FVector::ZeroVector);
		}
		else
		{
			WeatherNiagaraComp->SetAsset(SelectedFX);
			WeatherNiagaraComp->Activate(true);
		}
	}
	else if (WeatherNiagaraComp)
	{
		WeatherNiagaraComp->Deactivate();
	}
}

void UMythWeatherComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	// Ancora a caixa de emissão Niagara de 1500x1500 UU na posição da câmera do jogador (GDD Técnico HD-2D)
	if (WeatherNiagaraComp && WeatherNiagaraComp->IsActive())
	{
		if (const UWorld* World = GetWorld())
		{
			if (const APlayerController* PC = World->GetFirstPlayerController())
			{
				if (PC->PlayerCameraManager)
				{
					const FVector CamLoc = PC->PlayerCameraManager->GetCameraLocation();
					WeatherNiagaraComp->SetWorldLocation(CamLoc);
				}
			}
		}
	}

	UpdateWetness(DeltaTime);
}

void UMythWeatherComponent::UpdateWetness(float DeltaTime)
{
	if (!FMath::IsNearlyEqual(CurrentWetness, TargetWetness, 0.001f))
	{
		// Interpolação suave de molhado/seco
		CurrentWetness = FMath::FInterpTo(CurrentWetness, TargetWetness, DeltaTime, 0.25f);

		// Atualiza o parâmetro escalar no Material Parameter Collection (MPC_Environment)
		if (EnvironmentMPC)
		{
			if (UWorld* World = GetWorld())
			{
				UKismetMaterialLibrary::SetScalarParameterValue(World, EnvironmentMPC, TEXT("Wetness"), CurrentWetness);
			}
		}
	}
}

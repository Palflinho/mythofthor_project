#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "MythWeatherActor.generated.h"

class UInstancedStaticMeshComponent;
class UMaterialInterface;
class AExponentialHeightFog;
class AMythDayNightActor;

/** Climas do Myth of Thor (extra HD-2D sobre o cliente original). */
UENUM(BlueprintType)
enum class EMythWeather : uint8
{
	Clear UMETA(DisplayName = "Limpo"),
	Rain  UMETA(DisplayName = "Chuva"),
	Snow  UMETA(DisplayName = "Neve"),
	Fog   UMETA(DisplayName = "Neblina")
};

/**
 * Clima HD-2D: chuva e neve como instancias de um plano (riscos/flocos) que acompanham o jogador, neblina pelo
 * ExponentialHeightFog do nivel, e escurecimento do sol/ceu durante a chuva (AMythDayNightActor::WeatherDim).
 * Transicoes suaves (TransitionSeconds). Materiais: tools/ue_setup_weather.py (/Game/Weather/M_RainStreak, M_SnowFlake).
 * Quando a rede estiver ligada, o clima vem do servidor (mapflags do rAthena); offline: console MythWeather.
 */
UCLASS()
class MYTHOFTHOR_API AMythWeatherActor : public AActor
{
	GENERATED_BODY()

public:
	AMythWeatherActor();

	virtual void Tick(float DeltaSeconds) override;

	UFUNCTION(BlueprintCallable, Category = "Weather")
	void SetWeather(EMythWeather NewWeather);

	UFUNCTION(BlueprintPure, Category = "Weather")
	EMythWeather GetWeather() const { return Weather; }

	/** Segundos para entrar/sair de um clima. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Weather")
	float TransitionSeconds = 6.f;

	/** Raio (cm) da area com chuva/neve em volta do jogador e altura de onde caem. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Weather")
	float AreaRadius = 1800.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Weather")
	float AreaHeight = 1000.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Weather")
	int32 RainDrops = 1600;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Weather")
	int32 SnowFlakes = 1100;

	/** Densidade da neblina por clima (o valor do nivel e usado no tempo limpo). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Weather")
	float FogDensityFog = 0.08f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Weather")
	float FogDensityRain = 0.03f;

	/** Atalho para encontrar (ou criar) o ator de clima do mundo. */
	static AMythWeatherActor* FindOrSpawn(UWorld* World);

protected:
	virtual void BeginPlay() override;

private:
	UPROPERTY(VisibleAnywhere) TObjectPtr<USceneComponent> Root;
	UPROPERTY(VisibleAnywhere) TObjectPtr<UInstancedStaticMeshComponent> Particles;
	UPROPERTY() TObjectPtr<UMaterialInterface> RainMaterial;
	UPROPERTY() TObjectPtr<UMaterialInterface> SnowMaterial;
	UPROPERTY() TObjectPtr<class UMaterialParameterCollection> EnvironmentMPC;
	TWeakObjectPtr<AExponentialHeightFog> Fog;
	TWeakObjectPtr<AMythDayNightActor> DayNight;
	float BaseFogDensity = 0.02f;

	EMythWeather Weather = EMythWeather::Clear;
	/** 0..1: quanto do clima atual ja "entrou" (transicao). */
	float Intensity = 0.f;
	/** Clima que ainda esta saindo (durante a troca, o anterior some antes do novo entrar). */
	EMythWeather Visible = EMythWeather::Clear;

	float Wetness = 0.f;
	float WeatherTimer = 0.f;
	float NextWeatherChangeTime = 120.f;
	TArray<EMythWeather> AllowedWeatherList;
	bool bBiomeWeatherInitialized = false;

	void InitBiomeWeather();
	void UpdateWetness(float DeltaSeconds);

	struct FParticle
	{
		FVector Offset;   // relativo ao centro (jogador)
		float Speed = 0.f;
		float Phase = 0.f;
	};
	TArray<FParticle> State;
	TArray<FTransform> Transforms;

	void RebuildParticles(EMythWeather Kind);
	void UpdateParticles(float DeltaSeconds, const FVector& Center, const FVector& CamLoc, const FRotator& CamRot);
	void ApplyAtmosphere();
	FVector RandomOffset(bool bAnyHeight) const;
};

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "MythDayNightActor.generated.h"

class UDirectionalLightComponent;
class USkyLightComponent;
class USkyAtmosphereComponent;
class UMythNightAudioComponent;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnMythNightFactorChanged, float, NightFactor);

/**
 * Ciclo Dia/Noite: sol, lua, luz do ceu e atmosfera, mais o audio noturno (Audio).
 * O ator cria os proprios componentes e, no BeginPlay, esconde (so em runtime) as luzes/atmosfera
 * que o nivel ja tenha, para nao haver duas fontes. O AMythGameMode o cria automaticamente.
 *
 * Hora do dia em [0,24): 6h nascer, 12h meio-dia, 18h por do sol, 0h meia-noite.
 */
UCLASS()
class MYTHOFTHOR_API AMythDayNightActor : public AActor
{
	GENERATED_BODY()

public:
	AMythDayNightActor();

	virtual void Tick(float DeltaSeconds) override;

	/** Duracao de um dia de jogo em segundos reais. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "DayNight")
	float DayLengthSeconds = 1200.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "DayNight", meta = (ClampMin = "0", ClampMax = "24"))
	float StartHour = 9.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "DayNight")
	bool bFreezeTime = false;

	/** Rotacao (yaw) do arco do sol/lua no mundo. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "DayNight")
	float SunYaw = 0.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "DayNight|Light")
	float SunIntensityLux = 10.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "DayNight|Light")
	float MoonIntensityLux = 0.35f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "DayNight|Light")
	float DaySkyLightIntensity = 1.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "DayNight|Light")
	float NightSkyLightIntensity = 0.12f;

	/** Esconde (so em runtime) as luzes/atmosfera ja existentes no nivel. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "DayNight")
	bool bReplaceLevelLights = true;

	UFUNCTION(BlueprintCallable, Category = "DayNight")
	void SetTimeOfDay(float Hour);

	UFUNCTION(BlueprintPure, Category = "DayNight")
	float GetTimeOfDay() const { return TimeOfDay; }

	/** 0 = pleno dia, 1 = plena noite (suave no amanhecer e entardecer). */
	UFUNCTION(BlueprintPure, Category = "DayNight")
	float GetNightFactor() const { return NightFactor; }

	UFUNCTION(BlueprintPure, Category = "DayNight")
	bool IsNight() const { return NightFactor > 0.5f; }

	UPROPERTY(BlueprintAssignable, Category = "DayNight")
	FOnMythNightFactorChanged OnNightFactorChanged;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "DayNight")
	TObjectPtr<UMythNightAudioComponent> Audio;

protected:
	virtual void BeginPlay() override;

private:
	UPROPERTY(VisibleAnywhere) TObjectPtr<USceneComponent> Root;
	UPROPERTY(VisibleAnywhere) TObjectPtr<UDirectionalLightComponent> Sun;
	UPROPERTY(VisibleAnywhere) TObjectPtr<UDirectionalLightComponent> Moon;
	UPROPERTY(VisibleAnywhere) TObjectPtr<USkyLightComponent> SkyLight;
	UPROPERTY(VisibleAnywhere) TObjectPtr<USkyAtmosphereComponent> Atmosphere;

	float TimeOfDay = 9.f;
	float NightFactor = 0.f;

	void HideLevelLights();
	void UpdateLighting();
};

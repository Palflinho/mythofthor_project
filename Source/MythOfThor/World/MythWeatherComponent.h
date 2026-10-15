#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "World/MythMapInfo.h"
#include "NiagaraSystem.h"
#include "NiagaraComponent.h"
#include "Materials/MaterialParameterCollection.h"
#include "MythWeatherComponent.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnWeatherChanged, EMythWeatherType, NewWeather);

/**
 * Componente de Clima Local Niagara, Piso Molhado e Atmosfera HD-2D (GDD Técnico: Módulo 2)
 * Ancora um emissor Niagara de 1500x1500 UU na câmera do jogador, adapta os efeitos
 * por bioma/field de cada região do Renewal e gerencia o parâmetro escalar Wetness.
 */
UCLASS(ClassGroup = (Myth), meta = (BlueprintSpawnableComponent))
class MYTHOFTHOR_API UMythWeatherComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UMythWeatherComponent();

	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

	/** Define o clima ativo e atualiza os emissores Niagara e o estado de piso molhado */
	UFUNCTION(BlueprintCallable, Category = "Myth|Weather")
	void SetWeather(EMythWeatherType NewWeather);

	/** Sorteia um novo clima válido com base na lista de AllowedWeathers do mapa atual */
	UFUNCTION(BlueprintCallable, Category = "Myth|Weather")
	void TransitionRandomWeather();

	UFUNCTION(BlueprintPure, Category = "Myth|Weather")
	EMythWeatherType GetCurrentWeather() const { return CurrentWeather; }

	UFUNCTION(BlueprintPure, Category = "Myth|Weather")
	float GetCurrentWetness() const { return CurrentWetness; }

	UPROPERTY(BlueprintAssignable, Category = "Myth|Weather")
	FOnWeatherChanged OnWeatherChanged;

	/** Coleção de parâmetros de material (MPC_Environment) contendo o scalar 'Wetness' */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Myth|Weather|Materials")
	TObjectPtr<UMaterialParameterCollection> EnvironmentMPC;

	/** Templates Niagara para os biomas canônicos do GDD Técnico */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Myth|Weather|Niagara")
	TObjectPtr<UNiagaraSystem> FX_Rain;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Myth|Weather|Niagara")
	TObjectPtr<UNiagaraSystem> FX_Snow;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Myth|Weather|Niagara")
	TObjectPtr<UNiagaraSystem> FX_Sandstorm;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Myth|Weather|Niagara")
	TObjectPtr<UNiagaraSystem> FX_Fog;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Myth|Weather|Niagara")
	TObjectPtr<UNiagaraSystem> FX_IndustrialSmog;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Myth|Weather|Niagara")
	TObjectPtr<UNiagaraSystem> FX_VolcanicAsh;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Myth|Weather|Niagara")
	TObjectPtr<UNiagaraSystem> FX_CherryBlossoms;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Myth|Weather|Niagara")
	TObjectPtr<UNiagaraSystem> FX_DimensionalAurora;

protected:
	virtual void BeginPlay() override;

private:
	UPROPERTY(VisibleInstanceOnly, Category = "Myth|Weather")
	EMythWeatherType CurrentWeather = EMythWeatherType::ClearSunny;

	UPROPERTY(VisibleInstanceOnly, Category = "Myth|Weather")
	float CurrentWetness = 0.f;

	float TargetWetness = 0.f;

	UPROPERTY()
	TObjectPtr<UNiagaraComponent> WeatherNiagaraComp;

	void UpdateNiagaraEmitter();
	void UpdateWetness(float DeltaTime);
};

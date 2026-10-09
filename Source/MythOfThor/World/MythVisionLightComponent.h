#pragma once

#include "CoreMinimal.h"
#include "Components/PointLightComponent.h"
#include "MythVisionLightComponent.generated.h"

class AMythDayNightActor;

/**
 * "Raio de visao" noturno do remaster: uma luz suave em volta do proprio personagem nos mapas externos, que acende
 * com a noite (nos campos de caca nao ha postes). So visual: nao revela nada que o servidor nao mande.
 * O raio cresce com a DEX e com a classe, lidas do servidor (URagnarokNetworkSubsystem); offline usa os valores base.
 * Nos mapas internos fica apagada: eles ja sao iluminados pelas luzes do proprio mapa.
 */
UCLASS(ClassGroup = (Myth), meta = (BlueprintSpawnableComponent))
class MYTHOFTHOR_API UMythVisionLightComponent : public UPointLightComponent
{
	GENERATED_BODY()

public:
	UMythVisionLightComponent();

	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

	/** Raio base em cm (aprendiz com DEX 1). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Vision")
	float BaseRadius = 450.f;

	/** cm a mais por ponto de DEX. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Vision")
	float RadiusPerDex = 6.f;

	/** Intensidade em plena noite (candelas); suave, para iluminar so o entorno. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Vision")
	float NightIntensity = 14.f;

	/** Raio atual (cm), depois de classe e DEX. */
	UFUNCTION(BlueprintPure, Category = "Vision")
	float GetVisionRadius() const;

	/** Bonus de raio da classe (cm): arqueiros enxergam mais longe, depois magos/novicos e gatunos. */
	static float ClassBonus(int32 JobClass);

protected:
	virtual void BeginPlay() override;

private:
	TWeakObjectPtr<AMythDayNightActor> DayNight;
	bool bIndoor = false;
};

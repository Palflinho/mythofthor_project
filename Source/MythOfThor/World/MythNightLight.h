#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "MythNightLight.generated.h"

class UPointLightComponent;
class AMythDayNightActor;

/**
 * Luz que acende com a noite (postes de Prontera, fachadas das casas). A intensidade segue o
 * GetNightFactor() do AMythDayNightActor, com uma leve oscilacao de chama. Colocada pelo
 * tools/ue_import_map.py a partir do <mapa>_full_lights.json gerado pelo compile_map_to_obj.py.
 */
UCLASS()
class MYTHOFTHOR_API AMythNightLight : public AActor
{
	GENERATED_BODY()

public:
	AMythNightLight();

	virtual void Tick(float DeltaSeconds) override;

	/** Intensidade em plena noite (candelas). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "NightLight")
	float NightIntensity = 110.f;

	/** Fator de noite a partir do qual a luz comeca a acender (0 = dia, 1 = noite). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "NightLight")
	float TurnOnAt = 0.25f;

	/** Variacao da chama (0 = luz fixa). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "NightLight")
	float FlickerAmount = 0.22f;

	/** Sempre acesa (masmorras subterraneas, interiores escuros). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "NightLight")
	bool bAlwaysOn = false;

	/** Desenha chama animada em loop sobre a tocha (torch_01.spr). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "NightLight")
	bool bShowFlame = true;

	/** Deslocamento vertical da chama animada em relacao a tocha (cm). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "NightLight")
	float FlameZOffset = -85.f;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "NightLight")
	TObjectPtr<UPointLightComponent> Light;

protected:
	virtual void BeginPlay() override;

private:
	TWeakObjectPtr<AMythDayNightActor> DayNight;
	TWeakObjectPtr<class AMythSprEffect> FlameEffect;
	float FlickerPhase = 0.f;

};

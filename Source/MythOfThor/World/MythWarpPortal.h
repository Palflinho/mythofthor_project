#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "MythWarpPortal.generated.h"

class UProceduralMeshComponent;
class UPointLightComponent;
class UMaterialInstanceDynamic;

/**
 * Portal do cliente original (NPC de classe 45, WARPNPC): aneis azuis (texture/effect/ring_blue.tga) que sobem
 * girando e somem no alto. No RO isso e desenhado pelo executavel, nao por um .str. HD-2D: luz azul do Lumen
 * pulsando no chao em volta.
 */
UCLASS()
class MYTHOFTHOR_API AMythWarpPortal : public AActor
{
	GENERATED_BODY()

public:
	AMythWarpPortal();

	virtual void BeginPlay() override;
	virtual void Tick(float DeltaSeconds) override;

	UPROPERTY(EditAnywhere, Category = "Portal")
	int32 RingCount = 4;

	/** Raio dos aneis (cm): pouco mais de uma celula, como no RO. */
	UPROPERTY(EditAnywhere, Category = "Portal")
	float Radius = 45.f;

	/** Altura que cada anel sobe antes de sumir (cm). */
	UPROPERTY(EditAnywhere, Category = "Portal")
	float RiseHeight = 110.f;

	/** Altura da faixa de cada anel (cm). */
	UPROPERTY(EditAnywhere, Category = "Portal")
	float BandHeight = 22.f;

	/** Segundos de um ciclo completo de subida. */
	UPROPERTY(EditAnywhere, Category = "Portal")
	float CycleSeconds = 1.8f;

	UPROPERTY(VisibleAnywhere, Category = "Portal")
	TObjectPtr<UProceduralMeshComponent> Mesh;

	UPROPERTY(VisibleAnywhere, Category = "Portal")
	TObjectPtr<UPointLightComponent> Light;

private:
	UPROPERTY() TObjectPtr<UMaterialInstanceDynamic> RingMaterial;
	float Time = 0.f;
	bool bCreated = false;
	void BuildRings();
};

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "MythSpiritBalls.generated.h"

class UProceduralMeshComponent;
class UMaterialInstanceDynamic;

/**
 * Esferas espirituais do Monge (EF_CHOOKGI, SpiritSphere do roBrowser): ate 5 brilhos azuis (thunder_center) girando em
 * volta do personagem, devagar (1 volta a cada 23 s), com halo maior quando passa de 5. Quantas sao e do servidor
 * (ZC_SPIRITS 0x01d0 e 0x01e1). Fica presa ao personagem.
 */
UCLASS()
class MYTHOFTHOR_API AMythSpiritBalls : public AActor
{
	GENERATED_BODY()

public:
	AMythSpiritBalls();

	/** Define quantas esferas o personagem tem (0 remove). Idempotente. */
	static void Ensure(AActor* Owner, int32 Count);

	virtual void Tick(float DeltaSeconds) override;

private:
	UPROPERTY() TObjectPtr<UProceduralMeshComponent> Mesh;
	UPROPERTY() TObjectPtr<UMaterialInstanceDynamic> Material;

	int32 Count = 0;
	float Age = 0.f;
	float Fade = 0.f;
	float BaseHeadHeight = 200.f;
	float CurrentHeight = 200.f;
};

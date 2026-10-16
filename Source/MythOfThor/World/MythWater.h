#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "MythWater.generated.h"

class UProceduralMeshComponent;
class UMaterialInstanceDynamic;
class UTexture2D;

/**
 * Agua dos mapas (canais, lagos, esgoto): o plano de agua do RSW desenhado so onde o chao esta abaixo dele, como o cliente
 * original (criterio do roBrowser). Os dados vem de Config/water/<mapa>.json (tools/gen_water.py); o material e o
 * /Game/Myth/M_Water (translucido) e a textura troca entre os 32 quadros water<tipo>NN.jpg do cliente.
 */
UCLASS()
class MYTHOFTHOR_API AMythWater : public AActor
{
	GENERATED_BODY()

public:
	AMythWater();

	/** Cria a agua do mapa atual (se Config/water/<mapa>.json existir); chamado quando o mundo comeca. */
	static AMythWater* SpawnForMap(UWorld* World);

	virtual void Tick(float DeltaSeconds) override;

	/** Liga a agua moderna (Single Layer Water) ou a classica do RO; aplica em todas as aguas do mundo (opcao ModernWater). */
	static void ApplyStyleToAll(UWorld* World);
	void ApplyStyle();

	/** Retorna se o mapa atual possui plano de agua ativo e sua altura Z em UU. */
	static bool GetWaterPlane(const UObject* WorldContext, float& OutWaterZ);
	float GetWaterZ() const { return WaterZ; }
	bool HasWater() const { return bHasWater; }

private:
	UPROPERTY() TObjectPtr<UProceduralMeshComponent> Mesh;
	UPROPERTY() TObjectPtr<UMaterialInstanceDynamic> Material;
	UPROPERTY() TArray<TObjectPtr<UTexture2D>> Frames;
	int32 LastFrame = -1;
	float BaseOpacity = 0.78f;
	float AnimSpeed = 3.f;
	float Time = 0.f;
	float WaterZ = 0.f;
	bool bHasWater = false;
	bool bSewerWater = false;

	bool Build(const FString& MapName, const FVector2D& Origin);
};

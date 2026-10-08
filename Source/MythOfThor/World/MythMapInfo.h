#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Info.h"
#include "MythMapInfo.generated.h"

class USoundBase;

/**
 * Metadados do mapa montado (colocado no nivel por tools/ue_import_map.py):
 * nome no rAthena, escala das celulas, origem do grid e BGM do mapa.
 *
 * Grid do servidor: celula (0,0) fica em CellOriginUU; X cresce para +X do mundo e Y cresce para -Y
 * (o importador espelha o eixo Y do .obj para manter o mapa sem espelhamento).
 */
UCLASS()
class MYTHOFTHOR_API AMythMapInfo : public AInfo
{
	GENERATED_BODY()

public:
	/** Nome do mapa no rAthena (ex.: "prontera", "new_1-1"). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Map")
	FString MapName = TEXT("prontera");

	/** Tamanho de uma celula do servidor em unidades da Unreal. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Map")
	float CellSizeUU = 50.f;

	/** Posicao no mundo (X,Y) da celula (0,0). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Map")
	FVector2D CellOriginUU = FVector2D::ZeroVector;

	/** Musica de fundo do mapa; passa pelo submix com filtro dia/noite. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Map|Audio")
	TObjectPtr<USoundBase> Bgm;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Map|Audio")
	float BgmVolume = 1.f;

	UFUNCTION(BlueprintPure, Category = "Map")
	FIntPoint WorldToCell(const FVector& WorldLocation) const;

	UFUNCTION(BlueprintPure, Category = "Map")
	FVector CellToWorld(FIntPoint Cell, float Z = 0.f) const;

	/** Primeiro AMythMapInfo do mundo, ou nulo. */
	static AMythMapInfo* Find(const UWorld* World);
};

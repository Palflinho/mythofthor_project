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

	/** Tamanho do mapa em celulas (largura x altura do .gat); usado pelo minimapa. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Map")
	FIntPoint MapSizeCells = FIntPoint::ZeroValue;

	/**
	 * Mapa interno (casas, lojas): varias salas lado a lado no mesmo mapa. Camera mais perto e fixa (nao mostra as
	 * outras salas), fundo preto em vez de ceu e iluminacao de dia fixa, como no cliente original.
	 * Automatico pelo nome do RO (prt_in, geffen_in, payon_in01...); IndoorOverride forca ligado/desligado.
	 */
	UFUNCTION(BlueprintPure, Category = "Map")
	bool IsIndoor() const;

	/** 0 = automatico pelo nome; 1 = interno; -1 = externo. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Map")
	int32 IndoorOverride = 0;

	/** Distancia da camera nos mapas internos (cm; o padrao externo e 1200). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Map")
	float IndoorArmLength = 720.f;

	/** Musica de fundo do mapa; o filtro passa-baixa noturno e aplicado pelo UMythNightAudioComponent. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Map|Audio")
	TObjectPtr<USoundBase> Bgm;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Map|Audio")
	float BgmVolume = 1.f;

	UFUNCTION(BlueprintPure, Category = "Map")
	FIntPoint WorldToCell(const FVector& WorldLocation) const;

	UFUNCTION(BlueprintPure, Category = "Map")
	FVector CellToWorld(FIntPoint Cell, float Z = 0.f) const;

	/** Regional environment of this map (timezone, latitude, perpetual night...), or nullptr when the map is not in Config/map_environment_table.json. */
	const struct FMythMapEnv* GetEnvironment() const;

	/** Local hour of day: the world clock shifted one hour per 4 map columns east/west of Prontera (HD-2D regional time). */
	float GetLocalHour(float WorldHour) const;

	/** Primeiro AMythMapInfo do mundo, ou nulo. */
	static AMythMapInfo* Find(const UWorld* World);
};

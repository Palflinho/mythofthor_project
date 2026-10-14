#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Info.h"
#include "MythMapInfo.generated.h"

class USoundBase;

/**
 * Dados de ambiente, fuso horário e bioma geográfico do mapa (GDD Técnico HD-2D).
 */
USTRUCT(BlueprintType)
struct MYTHOFTHOR_API FMapEnvironmentData : public FTableRowBase
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Environment")
	FName MapID;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Environment")
	FText DisplayName;

	/** Longitude geográfica relativa a Prontera (0). A cada 4 mapas, o fuso avança ou recua 1 hora. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Environment")
	int32 GridX = 0;

	/** Latitude geográfica relativa a Prontera (0). Y < 0 clima quente/árido, Y > 0 clima frio/ártico. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Environment")
	int32 GridY = 0;

	/** Se verdadeiro, desliga o sol direto e mantém apenas iluminação ambiente interna. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Environment")
	bool bIsDungeon = false;

	/** Se verdadeiro (ex.: Lutie, Nifflheim), a iluminação fica travada em noite polar/eterna. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Environment")
	bool bIsPerpetualNight = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Environment")
	FString BiomeTag = TEXT("Temperate");
};

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

	/** Dados de bioma, fuso e clima calculados para este mapa. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Map|Environment")
	FMapEnvironmentData EnvironmentData;

	/** Altura da superficie da agua no mapa (GDD Tecnico: Sistema de Agua e Reflexos HD-2D). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Map|Water")
	bool bHasWater = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Map|Water")
	float WaterLevelUU = 0.f;

	/** Calcula a hora local considerando o fuso horario do GridX e condicoes especiais (Noite Polar). */
	UFUNCTION(BlueprintPure, Category = "Map|Environment")
	float GetLocalHour(float ServerHour) const;

	UFUNCTION(BlueprintPure, Category = "Map")
	FIntPoint WorldToCell(const FVector& WorldLocation) const;

	UFUNCTION(BlueprintPure, Category = "Map")
	FVector CellToWorld(FIntPoint Cell, float Z = 0.f) const;

	/** Primeiro AMythMapInfo do mundo, ou nulo. */
	static AMythMapInfo* Find(const UWorld* World);
};

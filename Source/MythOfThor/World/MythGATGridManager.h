#pragma once

#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"
#include "MythGATGridManager.generated.h"

/**
 * Tipos de célula do arquivo .gat clássico do Ragnarok Online (GDD Técnico: Módulo 5)
 */
UENUM(BlueprintType)
enum class EGATCellType : uint8
{
	Walkable       UMETA(DisplayName = "Walkable (Transitável)"),
	NonWalkable    UMETA(DisplayName = "Non-Walkable (Bloqueada)"),
	SnipeableWall  UMETA(DisplayName = "Snipeable Wall (Parede com Linha de Visão)"),
	Water          UMETA(DisplayName = "Water (Água)")
};

/**
 * Gerenciador de Grid Celular GAT em C++ (GDD Técnico: Módulo 5 - UMythGATGridManager)
 * Garante a fidelidade matemática do combate celular (1x1, 3x3) e Line of Sight,
 * separando a posição lógica da posição visual com interpolação suave (Hermite/Spline).
 */
UCLASS()
class MYTHOFTHOR_API UMythGATGridManager : public UWorldSubsystem
{
	GENERATED_BODY()

public:
	virtual void Initialize(FSubsystemCollectionBase& Collection) override;

	/** Configura as dimensões e parâmetros do grid celular */
	UFUNCTION(BlueprintCallable, Category = "Myth|GAT")
	void SetupGrid(int32 InWidth, int32 InHeight, float InCellSizeUU = 50.f, FVector2D InOriginUU = FVector2D::ZeroVector);

	/** Define o tipo de uma célula específica */
	UFUNCTION(BlueprintCallable, Category = "Myth|GAT")
	void SetCell(int32 X, int32 Y, EGATCellType CellType);

	/** Retorna o tipo de uma célula */
	UFUNCTION(BlueprintPure, Category = "Myth|GAT")
	EGATCellType GetCell(int32 X, int32 Y) const;

	/** Verifica se a célula é transitável por personagens */
	UFUNCTION(BlueprintPure, Category = "Myth|GAT")
	bool IsCellWalkable(int32 X, int32 Y) const;

	/**
	 * Validação de Linha de Visão (Line of Sight - LoS):
	 * Verifica se projéteis e feitiços podem passar entre duas células (algoritmo de Bresenham).
	 * Células SnipeableWall NÃO bloqueiam visão/tiro, apenas NonWalkable bloqueiam.
	 */
	UFUNCTION(BlueprintPure, Category = "Myth|GAT")
	bool HasLineOfSight(FIntPoint StartCell, FIntPoint EndCell) const;

	/** Converte coordenadas de mundo da UE5 para coordenadas de célula GAT */
	UFUNCTION(BlueprintPure, Category = "Myth|GAT")
	FIntPoint WorldToCell(const FVector& WorldLocation) const;

	/** Converte coordenadas de célula GAT para o centro da célula no mundo 3D */
	UFUNCTION(BlueprintPure, Category = "Myth|GAT")
	FVector CellToWorld(FIntPoint Cell, float Z = 0.f) const;

	/**
	 * Interpolação visual suave (Hermite / Spline) entre nós do grid para eliminar rigidez de passos
	 * sem quebrar a posição lógica celular no servidor.
	 */
	UFUNCTION(BlueprintPure, Category = "Myth|GAT")
	FVector InterpolateHermite(const FVector& P0, const FVector& P1, const FVector& V0, const FVector& V1, float Alpha) const;

	UFUNCTION(BlueprintPure, Category = "Myth|GAT")
	int32 GetGridWidth() const { return Width; }

	UFUNCTION(BlueprintPure, Category = "Myth|GAT")
	int32 GetGridHeight() const { return Height; }

	UFUNCTION(BlueprintPure, Category = "Myth|GAT")
	float GetCellSizeUU() const { return CellSizeUU; }

private:
	UPROPERTY()
	int32 Width = 0;

	UPROPERTY()
	int32 Height = 0;

	UPROPERTY()
	float CellSizeUU = 50.f;

	UPROPERTY()
	FVector2D OriginUU = FVector2D::ZeroVector;

	/** Armazenamento compacto dos tipos de célula em memória */
	TArray<uint8> CellTypes;

	FORCEINLINE int32 GetIndex(int32 X, int32 Y) const
	{
		return (X >= 0 && X < Width && Y >= 0 && Y < Height) ? (Y * Width + X) : -1;
	}
};

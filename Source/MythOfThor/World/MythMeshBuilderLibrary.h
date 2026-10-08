#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "MythMeshBuilderLibrary.generated.h"

class UStaticMesh;

/**
 * Utilitario de EDITOR para tools/ue_import_map.py: monta um StaticMesh a partir de arrays planos.
 * (O StaticMeshDescription exposto ao Python nao registra o canal de UV e derruba o editor.)
 * Em builds sem editor a funcao apenas retorna false.
 */
UCLASS()
class MYTHOFTHOR_API UMythMeshBuilderLibrary : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

public:
	/**
	 * @param PositionsXYZ     x,y,z de cada vertice (unidades da Unreal, ja convertidos).
	 * @param CornerVertex     indice de vertice de cada canto de triangulo (3 por triangulo).
	 * @param CornerUV         u,v de cada canto (2 floats por canto; origem do UV no topo).
	 * @param TriangleSection  indice (em SlotNames) do material de cada triangulo.
	 * @param SlotNames        nome do slot de material de cada secao.
	 */
	UFUNCTION(BlueprintCallable, Category = "Myth|Editor")
	static bool BuildStaticMeshFromTriangles(UStaticMesh* Mesh, const TArray<float>& PositionsXYZ, const TArray<int32>& CornerVertex,
		const TArray<float>& CornerUV, const TArray<int32>& TriangleSection, const TArray<FName>& SlotNames);
};

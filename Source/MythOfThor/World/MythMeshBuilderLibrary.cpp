#include "World/MythMeshBuilderLibrary.h"

#include "Engine/StaticMesh.h"

#if WITH_EDITOR
#include "MeshDescription.h"
#include "StaticMeshAttributes.h"
#include "StaticMeshOperations.h"
#endif

bool UMythMeshBuilderLibrary::BuildStaticMeshFromTriangles(UStaticMesh* Mesh, const TArray<float>& PositionsXYZ, const TArray<int32>& CornerVertex,
	const TArray<float>& CornerUV, const TArray<int32>& TriangleSection, const TArray<FName>& SlotNames)
{
#if WITH_EDITOR
	const int32 NumVerts = PositionsXYZ.Num() / 3;
	const int32 NumTris = CornerVertex.Num() / 3;
	if (!Mesh || NumVerts == 0 || NumTris == 0 || CornerUV.Num() != CornerVertex.Num() * 2 || TriangleSection.Num() != NumTris || SlotNames.IsEmpty())
	{
		UE_LOG(LogTemp, Error, TEXT("BuildStaticMeshFromTriangles: arrays inconsistentes (verts=%d tris=%d uv=%d sections=%d slots=%d)"),
			NumVerts, NumTris, CornerUV.Num(), TriangleSection.Num(), SlotNames.Num());
		return false;
	}

	FMeshDescription MD;
	FStaticMeshAttributes Attributes(MD);
	Attributes.Register();

	TVertexAttributesRef<FVector3f> Positions = Attributes.GetVertexPositions();
	TVertexInstanceAttributesRef<FVector2f> UVs = Attributes.GetVertexInstanceUVs();
	TPolygonGroupAttributesRef<FName> Slots = Attributes.GetPolygonGroupMaterialSlotNames();
	UVs.SetNumChannels(1);

	MD.ReserveNewVertices(NumVerts);
	MD.ReserveNewVertexInstances(NumTris * 3);
	MD.ReserveNewTriangles(NumTris);
	MD.ReserveNewPolygonGroups(SlotNames.Num());

	TArray<FVertexID> VertexIds;
	VertexIds.Reserve(NumVerts);
	for (int32 i = 0; i < NumVerts; ++i)
	{
		const FVertexID Id = MD.CreateVertex();
		Positions[Id] = FVector3f(PositionsXYZ[i * 3], PositionsXYZ[i * 3 + 1], PositionsXYZ[i * 3 + 2]);
		VertexIds.Add(Id);
	}

	TArray<FPolygonGroupID> Groups;
	for (const FName& Slot : SlotNames)
	{
		const FPolygonGroupID G = MD.CreatePolygonGroup();
		Slots[G] = Slot;
		Groups.Add(G);
	}

	for (int32 t = 0; t < NumTris; ++t)
	{
		TArray<FVertexInstanceID, TInlineAllocator<3>> Corners;
		for (int32 k = 0; k < 3; ++k)
		{
			const int32 C = t * 3 + k;
			const int32 V = CornerVertex[C];
			if (!VertexIds.IsValidIndex(V))
			{
				UE_LOG(LogTemp, Error, TEXT("BuildStaticMeshFromTriangles: indice de vertice invalido %d"), V);
				return false;
			}
			const FVertexInstanceID VI = MD.CreateVertexInstance(VertexIds[V]);
			UVs.Set(VI, 0, FVector2f(CornerUV[C * 2], CornerUV[C * 2 + 1]));
			Corners.Add(VI);
		}
		const int32 Section = Groups.IsValidIndex(TriangleSection[t]) ? TriangleSection[t] : 0;
		MD.CreateTriangle(Groups[Section], Corners);
	}

	// Normais/tangentes: sem grupos de suavizacao no .obj, usa as normais de face com suavizacao por angulo.
	FStaticMeshOperations::ComputeTriangleTangentsAndNormals(MD);
	FStaticMeshOperations::ComputeTangentsAndNormals(MD, EComputeNTBsFlags::Normals | EComputeNTBsFlags::Tangents);

	UStaticMesh::FBuildMeshDescriptionsParams Params;
	Params.bBuildSimpleCollision = false;
	Params.bFastBuild = true;
	return Mesh->BuildFromMeshDescriptions({ &MD }, Params);
#else
	return false;
#endif
}

#include "World/MythGATGridManager.h"
#include "World/MythMapInfo.h"

void UMythGATGridManager::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);

	// Tenta auto-configurar as dimensões do grid a partir do AMythMapInfo presente no mapa
	if (const UWorld* World = GetWorld())
	{
		if (const AMythMapInfo* MapInfo = AMythMapInfo::Find(World))
		{
			// Configuração inicial com dimensões padrão se o mapa tiver MapInfo
			SetupGrid(200, 200, MapInfo->CellSizeUU, MapInfo->CellOriginUU);
		}
	}
}

void UMythGATGridManager::SetupGrid(int32 InWidth, int32 InHeight, float InCellSizeUU, FVector2D InOriginUU)
{
	Width = FMath::Max(1, InWidth);
	Height = FMath::Max(1, InHeight);
	CellSizeUU = FMath::Max(1.f, InCellSizeUU);
	OriginUU = InOriginUU;

	CellTypes.Init(static_cast<uint8>(EGATCellType::Walkable), Width * Height);
	UE_LOG(LogTemp, Log, TEXT("[GATGrid] Inicializado %dx%d células (Tamanho: %.1f UU, Origem: %s)"),
		Width, Height, CellSizeUU, *OriginUU.ToString());
}

void UMythGATGridManager::SetCell(int32 X, int32 Y, EGATCellType CellType)
{
	const int32 Index = GetIndex(X, Y);
	if (Index != -1 && CellTypes.IsValidIndex(Index))
	{
		CellTypes[Index] = static_cast<uint8>(CellType);
	}
}

EGATCellType UMythGATGridManager::GetCell(int32 X, int32 Y) const
{
	const int32 Index = GetIndex(X, Y);
	if (Index != -1 && CellTypes.IsValidIndex(Index))
	{
		return static_cast<EGATCellType>(CellTypes[Index]);
	}
	return EGATCellType::NonWalkable;
}

bool UMythGATGridManager::IsCellWalkable(int32 X, int32 Y) const
{
	return GetCell(X, Y) == EGATCellType::Walkable;
}

bool UMythGATGridManager::HasLineOfSight(FIntPoint StartCell, FIntPoint EndCell) const
{
	// Algoritmo de Bresenham em 2D para traçado de raio sobre a malha de células GAT
	int32 X0 = StartCell.X;
	int32 Y0 = StartCell.Y;
	const int32 X1 = EndCell.X;
	const int32 Y1 = EndCell.Y;

	const int32 Dx = FMath::Abs(X1 - X0);
	const int32 Dy = -FMath::Abs(Y1 - Y0);
	const int32 Sx = (X0 < X1) ? 1 : -1;
	const int32 Sy = (Y0 < Y1) ? 1 : -1;
	int32 Err = Dx + Dy;

	while (true)
	{
		// Células intermediárias: se encontrar uma NonWalkable, a linha de visão é obstruída.
		// Nota canônica do RO: células SnipeableWall bloqueiam movimento físico MAS PERMITEM Line of Sight!
		if (!(X0 == StartCell.X && Y0 == StartCell.Y) && !(X0 == X1 && Y0 == Y1))
		{
			const EGATCellType Type = GetCell(X0, Y0);
			if (Type == EGATCellType::NonWalkable)
			{
				return false; // Obstruído por parede maciça
			}
		}

		if (X0 == X1 && Y0 == Y1)
		{
			break;
		}

		const int32 E2 = 2 * Err;
		if (E2 >= Dy)
		{
			Err += Dy;
			X0 += Sx;
		}
		if (E2 <= Dx)
		{
			Err += Dx;
			Y0 += Sy;
		}
	}

	return true;
}

FIntPoint UMythGATGridManager::WorldToCell(const FVector& WorldLocation) const
{
	const float RelX = WorldLocation.X - OriginUU.X;
	const float RelY = OriginUU.Y - WorldLocation.Y; // Y cresce para o Sul no RO (-Y da Unreal)
	return FIntPoint(FMath::FloorToInt(RelX / CellSizeUU), FMath::FloorToInt(RelY / CellSizeUU));
}

FVector UMythGATGridManager::CellToWorld(FIntPoint Cell, float Z) const
{
	const float WX = OriginUU.X + (Cell.X + 0.5f) * CellSizeUU;
	const float WY = OriginUU.Y - (Cell.Y + 0.5f) * CellSizeUU;
	return FVector(WX, WY, Z);
}

FVector UMythGATGridManager::InterpolateHermite(const FVector& P0, const FVector& P1, const FVector& V0, const FVector& V1, float Alpha) const
{
	const float T = FMath::Clamp(Alpha, 0.f, 1.f);
	const float T2 = T * T;
	const float T3 = T2 * T;

	const float H00 = 2.f * T3 - 3.f * T2 + 1.f;
	const float H10 = T3 - 2.f * T2 + T;
	const float H01 = -2.f * T3 + 3.f * T2;
	const float H11 = T3 - T2;

	return H00 * P0 + H10 * V0 + H01 * P1 + H11 * V1;
}

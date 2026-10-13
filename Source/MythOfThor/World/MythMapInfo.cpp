#include "World/MythMapInfo.h"

#include "EngineUtils.h"

FIntPoint AMythMapInfo::WorldToCell(const FVector& WorldLocation) const
{
	const float Size = FMath::Max(CellSizeUU, 1.f);
	// O pacote do rAthena usa 10 bits por eixo (0..1023).
	return FIntPoint(
		FMath::Clamp(FMath::FloorToInt((WorldLocation.X - CellOriginUU.X) / Size), 0, 1023),
		FMath::Clamp(FMath::FloorToInt((CellOriginUU.Y - WorldLocation.Y) / Size), 0, 1023));
}

FVector AMythMapInfo::CellToWorld(FIntPoint Cell, float Z) const
{
	const float Size = FMath::Max(CellSizeUU, 1.f);
	return FVector(CellOriginUU.X + (Cell.X + 0.5f) * Size, CellOriginUU.Y - (Cell.Y + 0.5f) * Size, Z);
}

AMythMapInfo* AMythMapInfo::Find(const UWorld* World)
{
	if (World)
	{
		for (TActorIterator<AMythMapInfo> It(World); It; ++It)
		{
			return *It;
		}
	}
	return nullptr;
}

bool AMythMapInfo::IsIndoor() const
{
	if (IndoorOverride != 0)
	{
		return IndoorOverride > 0;
	}
	// Interiores do RO sem o sufixo _in
	const FString Name = MapName.ToLower();
	static const TSet<FString> KnownInteriors = { TEXT("prt_church"), TEXT("prt_castle"), TEXT("new_1-2"), TEXT("new_1-4") }; // Campo de Treinamento: salao e capela
	if (KnownInteriors.Contains(Name))
	{
		return true;
	}
	// Convencao do RO: <cidade>_in ou <cidade>_in01, _in02...
	const int32 Idx = Name.Find(TEXT("_in"), ESearchCase::CaseSensitive, ESearchDir::FromEnd);
	if (Idx == INDEX_NONE)
	{
		return false;
	}
	const FString Rest = Name.Mid(Idx + 3);
	return Rest.IsEmpty() || Rest.IsNumeric();
}

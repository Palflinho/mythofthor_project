#include "UI/MythRoClientTables.h"

#include "UI/MythRoUITextures.h"

#include "Dom/JsonObject.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "Serialization/JsonReader.h"
#include "Serialization/JsonSerializer.h"

namespace
{
	struct FRoTableEntry
	{
		FString Name;
		FString Resource; // item: resource_name; skill: aegis_name
	};

	TMap<int32, FRoTableEntry> LoadRoTable(const TCHAR* FileName, const TCHAR* ResourceField)
	{
		TMap<int32, FRoTableEntry> Out;
		FString Text;
		const FString Path = FPaths::Combine(FPaths::ProjectConfigDir(), FileName);
		if (!FFileHelper::LoadFileToString(Text, *Path))
		{
			UE_LOG(LogTemp, Warning, TEXT("MythRoClientTables: %s nao encontrado"), *Path);
			return Out;
		}
		TSharedPtr<FJsonObject> Root;
		if (!FJsonSerializer::Deserialize(TJsonReaderFactory<>::Create(Text), Root) || !Root.IsValid())
		{
			UE_LOG(LogTemp, Warning, TEXT("MythRoClientTables: %s invalido"), *Path);
			return Out;
		}
		for (const TPair<FString, TSharedPtr<FJsonValue>>& Pair : Root->Values)
		{
			const TSharedPtr<FJsonObject>* Obj = nullptr;
			if (!Pair.Value.IsValid() || !Pair.Value->TryGetObject(Obj))
			{
				continue;
			}
			int32 Id = 0;
			if (!(*Obj)->TryGetNumberField(TEXT("id"), Id) || Id <= 0)
			{
				continue; // skills.json tambem indexa por aegis_name: as duplicatas sem id numerico sao ignoradas
			}
			FRoTableEntry& E = Out.FindOrAdd(Id);
			(*Obj)->TryGetStringField(TEXT("name"), E.Name);
			(*Obj)->TryGetStringField(ResourceField, E.Resource);
		}
		UE_LOG(LogTemp, Log, TEXT("MythRoClientTables: %s com %d entradas"), FileName, Out.Num());
		return Out;
	}

	const TMap<int32, FRoTableEntry>& RoItems()
	{
		static const TMap<int32, FRoTableEntry> Table = LoadRoTable(TEXT("items.json"), TEXT("resource_name"));
		return Table;
	}

	const TMap<int32, FRoTableEntry>& RoSkills()
	{
		static const TMap<int32, FRoTableEntry> Table = LoadRoTable(TEXT("skills.json"), TEXT("aegis_name"));
		return Table;
	}
}

FString FMythRoClientTables::ItemName(int32 ItemID)
{
	const FRoTableEntry* E = RoItems().Find(ItemID);
	return E ? E->Name : FString();
}

UTexture2D* FMythRoClientTables::ItemIcon(int32 ItemID)
{
	const FRoTableEntry* E = RoItems().Find(ItemID);
	return E && !E->Resource.IsEmpty() ? FMythRoUITextures::Get(FString::Printf(TEXT("item/%s.bmp"), *E->Resource)) : nullptr;
}

FString FMythRoClientTables::SkillName(int32 SkillID)
{
	const FRoTableEntry* E = RoSkills().Find(SkillID);
	return E ? E->Name : FString();
}

UTexture2D* FMythRoClientTables::SkillIcon(int32 SkillID)
{
	const FRoTableEntry* E = RoSkills().Find(SkillID);
	return E && !E->Resource.IsEmpty() ? FMythRoUITextures::Get(FString::Printf(TEXT("item/%s.bmp"), *E->Resource.ToLower())) : nullptr;
}

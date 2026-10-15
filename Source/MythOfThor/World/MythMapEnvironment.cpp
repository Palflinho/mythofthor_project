#include "World/MythMapEnvironment.h"

#include "Dom/JsonObject.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "Serialization/JsonReader.h"
#include "Serialization/JsonSerializer.h"

const FMythMapEnv* FMythMapEnvironment::Find(const FString& MapName)
{
	static TMap<FString, FMythMapEnv> Table;
	static bool bLoaded = false;
	if (!bLoaded)
	{
		bLoaded = true;
		FString Text;
		TSharedPtr<FJsonObject> Root;
		if (FFileHelper::LoadFileToString(Text, *FPaths::Combine(FPaths::ProjectConfigDir(), TEXT("map_environment_table.json")))
			&& FJsonSerializer::Deserialize(TJsonReaderFactory<>::Create(Text), Root) && Root.IsValid())
		{
			for (const TPair<FString, TSharedPtr<FJsonValue>>& Pair : Root->Values)
			{
				const TSharedPtr<FJsonObject>* Obj = nullptr;
				if (!Pair.Value->TryGetObject(Obj) || !Obj)
				{
					continue;
				}
				FMythMapEnv E;
				E.GridX = static_cast<int32>((*Obj)->GetNumberField(TEXT("grid_x")));
				E.GridY = static_cast<int32>((*Obj)->GetNumberField(TEXT("grid_y")));
				E.bDungeon = (*Obj)->GetBoolField(TEXT("dungeon"));
				E.bPerpetualNight = (*Obj)->GetBoolField(TEXT("perpetual_night"));
				E.Biome = (*Obj)->GetStringField(TEXT("biome"));
				E.DefaultWeather = (*Obj)->GetStringField(TEXT("default_weather"));
				for (const TSharedPtr<FJsonValue>& W : (*Obj)->GetArrayField(TEXT("allowed_weathers")))
				{
					E.AllowedWeathers.Add(W->AsString());
				}
				Table.Add(Pair.Key.ToLower(), MoveTemp(E));
			}
		}
		UE_LOG(LogTemp, Log, TEXT("MythMapEnvironment: %d maps from Config/map_environment_table.json"), Table.Num());
	}
	return Table.Find(MapName.ToLower());
}

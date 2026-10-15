#pragma once

#include "CoreMinimal.h"

/**
 * Regional environment of a map (Config/map_environment_table.json, from the HD-2D GDD branch): timezone (GridX: one hour per 4 maps east/west of
 * Prontera), latitude (GridY: warmer/golden to the south, colder/bluer to the north), dungeon, perpetual night, biome and the weather a map may have.
 * Maps that are not in the table get nullptr (they use the world clock unchanged).
 */
struct FMythMapEnv
{
	int32 GridX = 0;
	int32 GridY = 0;
	bool bDungeon = false;
	bool bPerpetualNight = false;
	FString Biome;
	TArray<FString> AllowedWeathers;
	FString DefaultWeather;
};

struct MYTHOFTHOR_API FMythMapEnvironment
{
	/** Environment of a map by its rAthena name (e.g. "morocc"), or nullptr. */
	static const FMythMapEnv* Find(const FString& MapName);
};

#pragma once

#include "CoreMinimal.h"

class UTexture2D;

/**
 * Texturas da interface do cliente original, lidas em runtime de
 * <AssetsRoot>/data/texture/유저인터페이스/<caminho>.bmp (sem importar assets).
 * O magenta (255,0,255) vira transparente, como no cliente; filtro Nearest (pixel art). Cache por caminho.
 */
struct MYTHOFTHOR_API FMythRoUITextures
{
	/** Ex.: Get(TEXT("basic_interface/basewin_bg.bmp")) ou Get(TEXT("map/prontera.bmp")). Nulo se nao existir. */
	static UTexture2D* Get(const FString& RelativePath);
};

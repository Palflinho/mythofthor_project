#pragma once

#include "CoreMinimal.h"
#include "Fonts/SlateFontInfo.h"

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

	/** Cor media da textura (ponderada pelo brilho dos pixels), usada para a cor da luz dos efeitos. */
	static FLinearColor AverageColor(const FString& RelativePath);

	/**
	 * Fonte da interface do cliente (System/font/SCDream4.otf regular, SCDream6.otf negrito; copiadas para
	 * Content/Myth/Fonts). Carregada do arquivo em runtime, sem asset; sem o arquivo, cai na fonte padrao.
	 */
	static FSlateFontInfo RoFont(int32 Size, bool bBold = false);

	/**
	 * Um quadro de um .spr do cliente como textura (ex.: digitos de dano de 이팩트/숫자.spr).
	 * SprPath e relativo a data/sprite, sem extensao. Indice conta as imagens indexadas e depois as RGBA.
	 */
	static UTexture2D* GetSpriteFrame(const FString& SprPath, int32 Index);
};

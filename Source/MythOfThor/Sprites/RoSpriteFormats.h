#pragma once

#include "CoreMinimal.h"

/**
 * Leitores dos formatos de sprite do cliente original do RO, em runtime (sem assets da Unreal):
 *   .spr: imagens (indexadas com paleta de 256 cores e/ou RGBA);
 *   .act: acoes -> quadros -> camadas (posicao, imagem, espelhamento, cor, escala, rotacao) e ancoras.
 * Mesma interpretacao do roBrowser/tools/extract_act_manifest.py. Coordenadas em pixels, Y para baixo,
 * origem no ponto de apoio (pes) do sprite.
 */

/** Uma imagem do .spr ja convertida para RGBA (indice 0 da paleta = transparente). */
struct FRoSprImage
{
	int32 Width = 0;
	int32 Height = 0;
	TArray<FColor> Pixels; // linha a linha, de cima para baixo
};

struct MYTHOFTHOR_API FRoSpr
{
	/** Imagens indexadas primeiro, depois as RGBA (o .act referencia cada grupo pelo SprType). */
	TArray<FRoSprImage> Indexed;
	TArray<FRoSprImage> Rgba;
	/** Paleta original do arquivo (256 cores). */
	TArray<FColor> Palette;

	/** Le o .spr; se Palette != nullptr (ex.: paleta de cor de cabelo .pal), ela substitui a do arquivo. */
	bool Load(const TArray<uint8>& Bytes, const TArray<FColor>* PaletteOverride = nullptr);

	/** Imagem pelo par (indice, tipo) usado no .act: SprType 0 = indexada, 1 = RGBA. */
	const FRoSprImage* Find(int32 Index, int32 SprType) const;

	/** Le um .pal (1024 bytes, RGBA) como paleta. */
	static bool LoadPalette(const TArray<uint8>& Bytes, TArray<FColor>& OutPalette);
};

struct FRoActLayer
{
	FIntPoint Offset = FIntPoint::ZeroValue; // centro da imagem relativo a origem do quadro
	int32 SprIndex = -1;                     // -1 = camada vazia
	int32 SprType = 0;                       // 0 indexada, 1 RGBA
	bool bMirror = false;
	FColor Color = FColor::White;
	FVector2f Scale = FVector2f(1.f, 1.f);
	int32 RotationDeg = 0;
};

struct FRoActFrame
{
	TArray<FRoActLayer> Layers;
	int32 EventId = -1;                 // indice em FRoAct::Events (som), -1 = nenhum
	TArray<FIntPoint> Anchors;          // pontos de encaixe (cabeca no corpo, etc.)
};

struct FRoActAction
{
	TArray<FRoActFrame> Frames;
	float IntervalMs = 100.f;           // tempo por quadro
};

struct MYTHOFTHOR_API FRoAct
{
	int32 Version = 0;                  // ex.: 0x205 = 2.5
	TArray<FRoActAction> Actions;       // indice = grupo * 8 + direcao
	TArray<FString> Events;             // nomes de .wav

	bool Load(const TArray<uint8>& Bytes);
};

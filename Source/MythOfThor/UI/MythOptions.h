#pragma once

#include "CoreMinimal.h"

/**
 * Opcoes ON/OFF da janela "Game settings" (abas Effect, Control e Other do cliente LATAM, docs/ref/hud_latam/INDICE.md).
 * Guardadas em GameUserSettings.ini, secao [Myth.Options]. Cada sistema le a chave que lhe diz respeito
 * (FMythOptions::Get(TEXT("ScreenShake"))); chaves ainda sem sistema ficam guardadas para quando ele existir.
 */
class MYTHOFTHOR_API FMythOptions
{
public:
	/** Valor atual (o padrao da tabela quando nunca foi mudado). */
	static bool Get(const FName& Key);
	static void Set(const FName& Key, bool bValue);

	/** Padrao de cada chave (as que o cliente original liga). */
	static bool DefaultOf(const FName& Key);
};

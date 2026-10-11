#pragma once

#include "CoreMinimal.h"

/** Titulo de uma quest pelo id, de Config/quests.json (tools/gen_quest_titles.py, vindo do quest_db.yml do servidor). */
namespace MythQuests
{
	FString TitleFor(int32 QuestId);

	/** Mostrar o rastreador na tela (a caixa "Mostrar missoes na tela" da janela de missoes). */
	bool& TrackerVisible();
}

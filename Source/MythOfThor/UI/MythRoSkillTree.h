#pragma once

#include "CoreMinimal.h"

/**
 * Arvore de habilidades do cliente LATAM (Config/skill_tree.json, gerado por tools/gen_skill_tree.py a partir de
 * skilltreeview.lub e skillinfolist.lub): posicao de cada skill na grade de 7 colunas por classe, nome em pt-BR,
 * nivel maximo e pre-requisitos. So leitura: quem decide o que pode subir e o servidor.
 */
struct FMythTreeSkill
{
	FString Name;
	int32 MaxLevel = 1;
	/** Pre-requisitos (skill, nivel). */
	TArray<FIntPoint> Need;
};

struct MYTHOFTHOR_API FMythRoSkillTree
{
	static constexpr int32 Columns = 7;

	/** Grade da classe: indice (linha * 7 + coluna) -> skill. Nulo se a classe nao tem arvore. */
	static const TMap<int32, int32>* Grid(int32 JobClass);

	static const FMythTreeSkill* Skill(int32 SkillID);

	/** Etapas da classe na ordem das abas: Aprendiz -> 1a classe -> 2a classe (-> transclasse). */
	static TArray<int32> JobChain(int32 JobClass);

	/** Nome curto da etapa para a aba. */
	static FString JobTabName(int32 JobClass);
};

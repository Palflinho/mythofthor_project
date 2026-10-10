#pragma once

#include "CoreMinimal.h"

class AActor;

/** Em que momento da skill o efeito toca (campos da tabela Config/skill_effects.json). */
enum class EMythSkillEffectStage : uint8
{
	Effect,  // ao usar (alvo ou o proprio usuario)
	Hit,     // em cada alvo atingido
	Ground,  // na celula do chao
	Cast,    // no proprio conjurador ao usar (effectIdOnCaster do cliente: Impacto Explosivo, Golpe Fulminante)
};

/**
 * Efeitos visuais das skills a partir de Config/skill_effects.json (gerado por tools/gen_skill_effects.py):
 * .str do cliente original + o .wav dele, e sempre uma luz do Lumen na cor do elemento (HD-2D).
 * Skills sem .str (efeitos que no RO ficam no executavel) recebem so o clarao do elemento.
 */
struct MYTHOFTHOR_API FMythSkillEffects
{
	/** Toca o efeito da skill nesse estagio. Target pode ser nulo (usa Location). Retorna falso se a skill nao tem nada. */
	static bool Play(UObject* WorldContext, int32 SkillID, EMythSkillEffectStage Stage, AActor* Target, const FVector& Location, bool bFlashIfMissing = true, AActor* Source = nullptr);

	/** Id do efeito da skill nesse estagio (0 = nenhum). */
	static int32 EffectIdFor(int32 SkillID, EMythSkillEffectStage Stage);

	/** Toca um efeito da EffectTable pelo id (os ZC_NOTIFY_EFFECT do servidor e os de cima). */
	static bool PlayEffectId(UObject* WorldContext, int32 EffectID, AActor* Target, const FVector& Location, AActor* Source = nullptr);

	/** Cor da luz do elemento da skill (branco morno para skills sem elemento). */
	static FLinearColor ElementColor(int32 SkillID);
};

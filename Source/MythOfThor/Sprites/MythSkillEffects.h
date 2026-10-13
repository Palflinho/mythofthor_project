#pragma once

#include "CoreMinimal.h"

class AActor;

/** Em que momento da skill o efeito toca (campos da tabela Config/skill_effects.json). */
enum class EMythSkillEffectStage : uint8
{
	Effect,          // effectId: ao usar (alvo ou o proprio usuario)
	Hit,             // hitEffectId: em cada golpe com dano no alvo
	Ground,          // groundEffectId: na celula do chao
	Cast,            // effectIdOnCaster: no proprio conjurador ao usar (Impacto Explosivo, Perfurar)
	Success,         // successEffectId: no alvo quando a skill funciona (sem dano)
	SuccessCaster,   // successEffectIdOnCaster
	Release,         // releaseEffectId: ao soltar a skill, saindo do conjurador ate o alvo (trilha da Rajada Congelante)
	BeforeHit,       // beforeHitEffectId: antes de cada golpe (projeteis: flecha, Lanca de Fogo); o dano chega 200 ms depois
	BeforeHitCaster, // beforeHitEffectIdOnCaster
	HitCaster,       // hitEffectIdOnCaster
	BeginCast,       // beginCastEffectId: no conjurador quando a conjuracao comeca (ou ao usar, se nao tem conjuracao)
	Count
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

	/** Id do efeito da skill nesse estagio (0 = nenhum); o primeiro, se a skill tiver varios. */
	static int32 EffectIdFor(int32 SkillID, EMythSkillEffectStage Stage);

	/** Lado (celulas) da area de uma skill de chao no nivel dado (Config/cast_size.json e skill_area.json); 1 se nao ha. */
	static int32 AreaSize(int32 SkillID, int32 Level);

	/**
	 * Aura de conjuracao embaixo do conjurador (EF_BEGINSPELL do roBrowser: 12 amarela, 54 agua, 57 terra, 55 fogo, 56 vento,
	 * 59 veneno, 58 sagrado, 454 sombra), com a duracao da conjuracao. Devolve os atores para encerrar quando a skill sair.
	 */
	static void PlayCastAura(UObject* WorldContext, AActor* Caster, int32 SkillID, int32 CastMs, TArray<TWeakObjectPtr<AActor>>& OutActors);

	/** Alvo travado (EF_LOCKON) ou circulo mágico da area no chao (EF_GROUNDSAMPLE) durante a conjuracao. */
	static void PlayCastMarker(UObject* WorldContext, int32 SkillID, int32 Level, int32 CastMs, AActor* Target, const FVector& GroundLocation, TArray<TWeakObjectPtr<AActor>>& OutActors);

	/** A skill pede para nao desenhar a aura de conjuracao (hideCastAura do roBrowser). */
	static bool HidesCastAura(int32 SkillID);

	/** Toca um efeito da EffectTable pelo id (os ZC_NOTIFY_EFFECT do servidor e os de cima). */
	/** A tabela do cliente original desenha algo (STR, SPR, 3D, cilindro...) para a skill em algum momento, ou so toca som? */
	static bool SkillHasVisual(int32 SkillID);

	static bool PlayEffectId(UObject* WorldContext, int32 EffectID, AActor* Target, const FVector& Location, AActor* Source = nullptr);

	/** Cor da luz do elemento da skill (branco morno para skills sem elemento). */
	static FLinearColor ElementColor(int32 SkillID);

	/** Duracao (s) de um efeito da tabela: o maior .str ou efeito procedural dele. Serve para repetir os efeitos de unidades de skill no chao. */
	static float EffectLength(int32 EffectID);
	/** Duracao (s) de data/texture/effect/<StrFile>.str; 0 se nao existir. */
	static float StrLength(const FString& StrFile);
};

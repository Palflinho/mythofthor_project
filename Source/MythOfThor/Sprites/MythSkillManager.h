#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "Sprites/MythSkillEffects.h"
#include "MythSkillManager.generated.h"

/**
 * Entrada unica dos efeitos visuais de skill, dirigida por dados:
 *  1. recebe o ID da skill do pacote do rAthena e o momento (conjurar, acertar, chao, buff...);
 *  2. toca a animacao do cliente original se a tabela (Config/skill_effects_full.json) tem uma para a skill;
 *  3. se a skill nao tem NENHUM visual na tabela, usa a classe de efeito dela (Config/skill_vfx_classes.json: element +
 *     effect_class, gerado do skill_db do rAthena por tools/gen_skill_vfx_classes.py) para montar um efeito generico com a
 *     cor do elemento. Se a classe (ou a skill) tem "niagara" no JSON, usa esse Niagara System.
 * Skill nova no rAthena = regenerar o JSON, sem mexer no codigo.
 */
UCLASS()
class MYTHOFTHOR_API UMythSkillManager : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

public:
	/** Substitui as chamadas diretas a FMythSkillEffects::Play; mesmo contrato e retorno. */
	static bool Play(UObject* WorldContext, int32 SkillID, EMythSkillEffectStage Stage, AActor* Target, const FVector& Location, bool bFlashIfMissing = true, AActor* Source = nullptr);

	/** Classe de efeito da skill ("buff_self", "ground_area"...), vazia se a skill nao esta no JSON. */
	static FString EffectClassOf(int32 SkillID);

private:
	static bool PlayGeneric(UObject* WorldContext, int32 SkillID, EMythSkillEffectStage Stage, AActor* Target, const FVector& Location, AActor* Source);
};

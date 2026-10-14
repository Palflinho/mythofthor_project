#pragma once

#include "CoreMinimal.h"
#include "Engine/DataTable.h"
#include "NiagaraSystem.h"
#include "Camera/CameraShakeBase.h"
#include "MythSkillVisualData.generated.h"

/**
 * GDD Técnico: Sistema Universal de Magias e Habilidades HD-2D
 * Classificação por Arquétipos Geométricos
 */
UENUM(BlueprintType)
enum class ESkillArchetype : uint8
{
	Missile    UMETA(DisplayName = "Single Target Missile"),
	MeleeHit   UMETA(DisplayName = "Melee / Instant Hit"),
	GroundAoE  UMETA(DisplayName = "Ground AoE"),
	Wall       UMETA(DisplayName = "Cell Wall"),
	Buff       UMETA(DisplayName = "Buff / Aura"),
	Debuff     UMETA(DisplayName = "Debuff / Status")
};

/**
 * Matriz Elemental Universal do Ragnarok HD-2D
 */
UENUM(BlueprintType)
enum class ESkillElement : uint8
{
	Neutral,
	Fire,
	Water,
	Wind,
	Earth,
	Holy,
	Shadow,
	Poison,
	Ghost
};

/**
 * Estrutura de Dados da Habilidade (Data-Driven VFX)
 * Alinhada ao GDD Técnico: Sistema Universal de Magias e Habilidades HD-2D
 */
USTRUCT(BlueprintType)
struct MYTHOFTHOR_API FSkillVisualData : public FTableRowBase
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Identification")
	FName SkillID; // Ex: "WZ_METEOR", "MO_ASURA", "NPC_EARTHQUAKE", "MG_FIREBOLT"

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Classification")
	ESkillArchetype Archetype = ESkillArchetype::Missile;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Classification")
	ESkillElement Element = ESkillElement::Neutral;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "FX | Niagara")
	TObjectPtr<UNiagaraSystem> CastEffect;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "FX | Niagara")
	TObjectPtr<UNiagaraSystem> ActionEffect;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "FX | Niagara")
	TObjectPtr<UNiagaraSystem> ImpactEffect;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Feedback")
	TSubclassOf<UCameraShakeBase> CameraShakeClass;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Feedback", meta = (ClampMin = "0.0", ClampMax = "0.5"))
	float HitstopDuration = 0.03f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Environment")
	bool bLeavesDecal = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Environment", meta = (EditCondition = "bLeavesDecal"))
	TObjectPtr<UMaterialInterface> DecalMaterial;

	/** Retorna a cor primária canônica do elemento segundo o GDD */
	static FLinearColor GetElementPrimaryColor(ESkillElement InElement)
	{
		switch (InElement)
		{
		case ESkillElement::Fire:    return FLinearColor::FromSRGBColor(FColor::FromHex(TEXT("FF4500"))); // Laranja Incandescente
		case ESkillElement::Water:   return FLinearColor::FromSRGBColor(FColor::FromHex(TEXT("00CED1"))); // Azul Turquesa
		case ESkillElement::Wind:    return FLinearColor::FromSRGBColor(FColor::FromHex(TEXT("FFE4B5"))); // Amarelo Elétrico
		case ESkillElement::Earth:   return FLinearColor::FromSRGBColor(FColor::FromHex(TEXT("556B2F"))); // Verde Musgo
		case ESkillElement::Holy:    return FLinearColor::FromSRGBColor(FColor::FromHex(TEXT("FFF8DC"))); // Dourado Puro
		case ESkillElement::Shadow:  return FLinearColor::FromSRGBColor(FColor::FromHex(TEXT("4B0082"))); // Roxo Escuro
		case ESkillElement::Poison:  return FLinearColor::FromSRGBColor(FColor::FromHex(TEXT("32CD32"))); // Verde Tóxico
		case ESkillElement::Ghost:   return FLinearColor::FromSRGBColor(FColor::FromHex(TEXT("8A2BE2"))); // Violeta Translúcido
		case ESkillElement::Neutral:
		default:                     return FLinearColor::FromSRGBColor(FColor::FromHex(TEXT("E6E6FA"))); // Branco Prateado
		}
	}
};

#pragma once

#include "CoreMinimal.h"
#include "Engine/DataTable.h"
#include "NiagaraSystem.h"
#include "Camera/CameraShakeBase.h"
#include "SkillVisualData.generated.h"

UENUM(BlueprintType)
enum class ESkillArchetype : uint8
{
	Missile UMETA(DisplayName = "Single Target Missile"),
	MeleeHit UMETA(DisplayName = "Melee / Instant Hit"),
	GroundAoE UMETA(DisplayName = "Ground AoE"),
	Wall UMETA(DisplayName = "Cell Wall"),
	Buff UMETA(DisplayName = "Buff / Aura"),
	Debuff UMETA(DisplayName = "Debuff / Status")
};

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

USTRUCT(BlueprintType)
struct FSkillVisualData : public FTableRowBase
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Identification")
	FName SkillID; // Ex: "WZ_METEOR", "MO_ASURA", "NPC_EARTHQUAKE"

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
};

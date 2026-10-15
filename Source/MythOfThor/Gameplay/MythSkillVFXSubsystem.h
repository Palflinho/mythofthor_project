#pragma once

#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"
#include "Gameplay/MythSkillVisualData.h"
#include "NiagaraComponent.h"
#include "Components/PointLightComponent.h"
#include "Components/DecalComponent.h"
#include "MythSkillVFXSubsystem.generated.h"

/** Instância em pool de componente Niagara */
USTRUCT()
struct FNiagaraPoolEntry
{
	GENERATED_BODY()

	UPROPERTY()
	TObjectPtr<UNiagaraComponent> Component = nullptr;

	UPROPERTY()
	bool bInUse = false;
};

/**
 * Executor Universal de Magias e Object Pool em C++ (GDD Técnico HD-2D: Módulo 1 - MythSkillVFXSubsystem)
 * Gerencia o ciclo completo (Cast, Travel, Hitstop, Hit-Flash, Decals) e pooling de emissores Niagara
 * para sustentar combates massivos (WoE/GvG) acima de 60 FPS sem paradas de Garbage Collection.
 */
UCLASS()
class MYTHOFTHOR_API UMythSkillVFXSubsystem : public UWorldSubsystem
{
	GENERATED_BODY()

public:
	virtual void Initialize(FSubsystemCollectionBase& Collection) override;
	virtual void Deinitialize() override;

	/**
	 * Fase 1: Conjuração (Cast)
	 * Spawna decalque rúnico concêntrico alinhado à célula GAT + luz pontual ascendente + partículas de absorção.
	 */
	UFUNCTION(BlueprintCallable, Category = "Myth|Skills")
	void PlayCastVFX(const FVector& CellWorldLocation, ESkillElement Element, float CastDuration = 1.5f);

	/**
	 * Fase 2: Projétil / Ação (Travel)
	 * Spawna projétil com luz dinâmica ou efeito instantâneo melee/AoE.
	 */
	UFUNCTION(BlueprintCallable, Category = "Myth|Skills")
	void PlaySkillAction(const FSkillVisualData& SkillData, const FVector& OriginLocation, AActor* TargetActor);

	/**
	 * Fase 3: Impacto e Dissipação (Hit & Aftermath)
	 * Aciona Hitstop (micro-congelamento de quadros), Hit-Flash emissivo e decalque transitório de impacto no cenário.
	 */
	UFUNCTION(BlueprintCallable, Category = "Myth|Skills")
	void TriggerSkillImpact(AActor* TargetActor, const FSkillVisualData& SkillData, const FVector& ImpactLocation);

	/** Adquire ou recicla um UNiagaraComponent do pool pré-alocado (evita GC stalls) */
	UFUNCTION(BlueprintCallable, Category = "Myth|Skills")
	UNiagaraComponent* AcquireNiagaraFromPool(UNiagaraSystem* SystemTemplate, const FVector& Location, const FRotator& Rotation = FRotator::ZeroRotator);

	/** Devolve um componente Niagara ao pool */
	UFUNCTION(BlueprintCallable, Category = "Myth|Skills")
	void ReleaseNiagaraToPool(UNiagaraComponent* Component);

	/** Quantidade máxima de luzes dinâmicas de feitiços simultâneas para garantir 60+ FPS */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Myth|Performance")
	int32 MaxSimultaneousDynamicLights = 24;

private:
	/** Pools de componentes Niagara indexados por template */
	UPROPERTY()
	TMap<TObjectPtr<UNiagaraSystem>, FNiagaraPoolEntry> NiagaraPool;

	int32 ActiveDynamicLightCount = 0;
};

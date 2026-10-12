#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "MythTrailEffect.generated.h"

class UProceduralMeshComponent;
class UMaterialInstanceDynamic;
class FJsonObject;

/**
 * Parte "TRAIL" da EffectTable (roBrowser Renderer/Effects/Trail.js): um cursor anda no chao do conjurador ate o alvo
 * (speed celulas/s, para no alvo + overshoot) e a cada "interval" ms cria as partes de "spawn" ali, com um desvio
 * aleatorio "spread". A Rajada Congelante (efeito 27) usa pontas de gelo QuadHorn.
 */
UCLASS()
class MYTHOFTHOR_API AMythTrailEffect : public AActor
{
	GENERATED_BODY()

public:
	AMythTrailEffect();

	/** Comeca a trilha de From ate To. */
	static bool Spawn(UObject* WorldContext, const TSharedPtr<FJsonObject>& Part, const FVector& From, const FVector& To);

	virtual void Tick(float DeltaSeconds) override;

private:
	TSharedPtr<FJsonObject> Def;
	FVector Start = FVector::ZeroVector;
	FVector2D Dir = FVector2D(0.f, -1.f);
	float TargetDistance = 0.f; // uu
	float Speed = 24.f;         // celulas/s
	float Interval = 0.017f;    // s
	float StartOffset = 0.f;    // celulas
	float Overshoot = 0.f;      // celulas
	bool bStopAtTarget = false;
	float Time = 0.f;
	float NextSpawn = 0.f;
	float Life = 2.5f;

	void SpawnAt(float DistanceCells);
};

/**
 * Parte "QuadHorn" (roBrowser Renderer/Effects/QuadHorn.js): piramide de 4 faces com textura (ponta de gelo da Rajada
 * Congelante, da Nova Glacial...). Tamanhos em celulas: bottomSize (meia largura da base), height, offsetZ; animacao 1 cresce,
 * 2 sobe, 3 sobe passando do chao, 4 sai de baixo do chao (riseDistance) desacelerando; fadeOut no fim.
 */
UCLASS()
class MYTHOFTHOR_API AMythQuadHorn : public AActor
{
	GENERATED_BODY()

public:
	AMythQuadHorn();

	static bool Spawn(UObject* WorldContext, const TSharedPtr<FJsonObject>& Part, const FVector& Ground);

	virtual void Tick(float DeltaSeconds) override;

private:
	UPROPERTY() TObjectPtr<UProceduralMeshComponent> Mesh;
	UPROPERTY() TObjectPtr<UMaterialInstanceDynamic> Material;

	float Height = 1.f;       // uu
	float Bottom = 10.f;      // uu
	float OffsetZ = 0.f;      // uu
	float Rise = 0.f;         // uu
	FRotator Tilt = FRotator::ZeroRotator;
	int32 Animation = 0;
	float AnimSeconds = 0.1f;
	float Duration = 0.67f;
	float FadeOut = 0.f;
	float Alpha = 1.f;
	float Time = 0.f;

	void Build(float CurHeight, float CurOffsetZ, float CurAlpha);
};

/**
 * Circulo de conjuracao no chao (roBrowser MagicTarget.js e LockOnTarget.js):
 *  - Magic: skill de chao (EF_GROUNDSAMPLE): textura effect/magic_target.tga girando a 40 graus/s, do tamanho da area (celulas),
 *    colada ao terreno, ate o fim da conjuracao.
 *  - LockOn: alvo travado (EF_LOCKON): effect/lockon128.tga em cima do alvo, fecha de 4 celulas ate 1 enquanto gira e pisca.
 */
UCLASS()
class MYTHOFTHOR_API AMythCastRing : public AActor
{
	GENERATED_BODY()

public:
	AMythCastRing();

	/** Cria o circulo; SizeCells em celulas (quadrado); Follow (opcional) acompanha o ator (alvo travado). */
	static AMythCastRing* Spawn(UObject* WorldContext, bool bLockOn, const FVector& Center, float SizeCells, float DurationSec, AActor* Follow);

	virtual void Tick(float DeltaSeconds) override;

private:
	UPROPERTY() TObjectPtr<UProceduralMeshComponent> Mesh;
	UPROPERTY() TObjectPtr<UMaterialInstanceDynamic> Material;
	TWeakObjectPtr<AActor> FollowActor;
	FVector Center = FVector::ZeroVector;
	float SizeCells = 1.f;
	float Duration = 1.f;
	float Time = 0.f;
	bool bLockOn = false;

	void Rebuild(float HalfSizeUU, float AngleRad, float Alpha);
};

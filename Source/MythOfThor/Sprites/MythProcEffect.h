#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "MythProcEffect.generated.h"

class UProceduralMeshComponent;
class UMaterialInstanceDynamic;
class FJsonObject;

/**
 * Efeito desenhado pelo executavel do RO, sem .str (docs/ref/11): uma parte da EffectTable do tipo CYLINDER (cone ou
 * tubo com textura girando/expandindo), 3D (particulas e projeteis voltados para a camera) ou 2D (raios em leque do
 * acerto). Os campos e unidades sao os do cliente: tamanhos de cilindro e alturas em celulas (50 uu), tamanho de
 * particula 3D em centesimos de celula, 2D em pixels; tempos em ms.
 */
UCLASS()
class MYTHOFTHOR_API AMythProcEffect : public AActor
{
	GENERATED_BODY()

public:
	AMythProcEffect();

	/**
	 * Cria a parte do efeito. Target: quem recebe (os pes viram a origem; segue se "attached"); Source: quem lancou
	 * (projeteis "toSrc" saem dele). Retorna falso se o tipo nao e procedural ou falta a textura.
	 */
	static bool Spawn(UObject* WorldContext, const TSharedPtr<FJsonObject>& Part, AActor* Target, const FVector& Feet, AActor* Source = nullptr, AActor** OutActor = nullptr);

	virtual void Tick(float DeltaSeconds) override;

private:
	UPROPERTY() TObjectPtr<UProceduralMeshComponent> Mesh;
	UPROPERTY() TObjectPtr<UMaterialInstanceDynamic> Material;

	TWeakObjectPtr<AActor> Follow;
	FVector Origin = FVector::ZeroVector;
	FVector SourcePos = FVector::ZeroVector;
	bool bHasSource = false;
	/** Projetil de sprite (.spr): uma imagem, com a ponta girada para o alvo. */
	bool bSpriteImage = false;
	FVector2D ImagePixels = FVector2D::ZeroVector;

	FString Type;
	float Time = 0.f;
	float Duration = 1.f;
	float Delay = 0.f;
	float AlphaMax = 1.f;
	bool bFade = false;
	bool bFadeIn = false;
	bool bFadeOut = false;
	bool bRotate = false;
	bool bRepeat = false;
	int32 Animation = 0;
	FLinearColor Tint = FLinearColor::White;
	float Spin = 0.f;

	// CYLINDER (uu)
	float BottomRadius = 50.f;
	float TopRadius = 50.f;
	float Height = 50.f;
	float PosZ = 0.f;

	// 3D / 2D: copias, cada uma com atraso e deslocamento proprios
	struct FCopy
	{
		float Delay = 0.f;
		FVector Offset = FVector::ZeroVector;
		float Angle = 0.f;
	};
	TArray<FCopy> Copies;
	FVector2D SizeStart = FVector2D(50.f);
	FVector2D SizeEnd = FVector2D(50.f);
	float ZStart = 0.f;
	float ZEnd = 0.f;
	bool bToSrc = false;
	bool bCircle = false;
	float CircleInner = 0.f;
	float CircleOuter = 0.f;

	bool Setup(const TSharedPtr<FJsonObject>& Part);
	float AlphaAt(float T) const;
	void BuildCylinder(float T);
	void BuildQuads();
};

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "MythAura99.generated.h"

class UProceduralMeshComponent;
class UMaterialInstanceDynamic;

/**
 * Aura do nivel 99 (EF_LEVEL99, _2 e _3; docs/ref/13): tres faixas espirais em cone (ring_blue), dois aneis de chao
 * pulsantes (pikapika2) e 16 particulas de brilho subindo (whitelight), tudo aditivo e azul. Fica presa ao personagem.
 * Modo global como o /aura do cliente: 0 desligada, 1 so as particulas, 2 completa.
 */
UCLASS()
class MYTHOFTHOR_API AMythAura99 : public AActor
{
	GENERATED_BODY()

public:
	AMythAura99();

	/** Liga ou desliga a aura de um personagem (nivel de base >= 99). Idempotente. */
	static void Ensure(AActor* Owner, bool bWanted);

	/** Modo global (0, 1, 2), igual ao /aura e /aura2 do cliente. */
	static int32 GetMode();
	static void SetMode(int32 NewMode);

	virtual void Tick(float DeltaSeconds) override;

private:
	UPROPERTY() TObjectPtr<UProceduralMeshComponent> RibbonMesh;
	UPROPERTY() TObjectPtr<UProceduralMeshComponent> GroundMesh;
	UPROPERTY() TObjectPtr<UProceduralMeshComponent> BubbleMesh;
	UPROPERTY() TObjectPtr<UMaterialInstanceDynamic> RibbonMat;
	UPROPERTY() TObjectPtr<UMaterialInstanceDynamic> GroundMat;
	UPROPERTY() TObjectPtr<UMaterialInstanceDynamic> BubbleMat;

	float Age = 0.f;

	struct FBubble
	{
		float Height = 0.f;   // uu acima dos pes
		float PhaseA = 0.f;
		float PhaseB = 0.f;
		float TargetA = 0.f;
		float TargetB = 0.f;
		float SignX = 1.f;
		float SignZ = 1.f;
		float Column = 0.f;   // angulo da coluna (graus)
	};
	TArray<FBubble> Bubbles;

	void BuildRibbons(const FVector& Right, const FVector& Up);
	void BuildGround();
	void BuildBubbles(const FVector& Right, const FVector& Up);
	static UMaterialInstanceDynamic* MakeMaterial(UObject* Outer, const FString& TextureFile, float Brightness);
};

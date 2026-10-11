#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "MythSprEffect.generated.h"

class UProceduralMeshComponent;
class UMaterialInstanceDynamic;

/**
 * Efeito de skill feito de sprite (parte "SPR" da tabela de efeitos, data/sprite/<File>.spr + .act): a acao 0 do .act
 * toca uma vez (ou em loop ate a duracao), virada para a camera, no chao em Location ou presa a quem recebeu.
 */
UCLASS()
class MYTHOFTHOR_API AMythSprEffect : public AActor
{
	GENERATED_BODY()

public:
	AMythSprEffect();

	/**
	 * Toca o sprite (caminho sem extensao, relativo a data/sprite/ ou a data/sprite/<pasta de efeitos>/).
	 * DurationSec <= 0: um ciclo da animacao; bRepeat repete ate DurationSec (padrao 2 s). Nulo se o arquivo nao existir.
	 */
	static AMythSprEffect* Play(UObject* WorldContext, const FString& SprFile, const FVector& Location, AActor* Follow, bool bRepeat, float DurationSec);

	/** Adereco de estado que fica em loop preso a quem esta com o estado (sono, atordoado, maldicao); destrua o ator para tirar. */
	static AMythSprEffect* Attach(AActor* Owner, const FString& SprFile, bool bAboveHead, float SideOffset = 0.f, float UpOffset = 0.f);

	virtual void Tick(float DeltaSeconds) override;

private:
	UPROPERTY() TObjectPtr<UProceduralMeshComponent> Mesh;
	UPROPERTY() TArray<TObjectPtr<UMaterialInstanceDynamic>> Materials;
	UPROPERTY() TArray<TObjectPtr<UMaterialInstanceDynamic>> AdditiveMaterials;
	TWeakObjectPtr<AActor> FollowActor;
	FString Key;
	float Age = 0.f;
	float Length = 1.f;
	bool bLoop = false;
	float HeightOffset = 0.f;
	float SideOffset = 0.f; // para a direita da camera (uu)
};

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "MythEmotion.generated.h"

class UProceduralMeshComponent;
class UMaterialInstanceDynamic;
class UTexture2D;

/**
 * Emocao sobre a cabeca (ZC_EMOTION 0x00c0, docs/ref/18 2.3): a acao <tipo> do data/sprite/이팩트/emotion.act (29 quadros de
 * 50 ms; o ultimo fica ate completar 2,5 s). Presa a quem emocionou; um por pessoa de cada vez.
 */
UCLASS()
class MYTHOFTHOR_API AMythEmotion : public AActor
{
	GENERATED_BODY()

public:
	AMythEmotion();

	/** Mostra a emocao de tipo Type (acao do .act) sobre Owner. Troca a que ja estiver ali. */
	static void Show(AActor* Owner, int32 Type);

	/** Tipo de emocao (acao do emotion.act) do atalho /eN (N de 1 a 64), tabela EMOTION_ORDERLIST do emotionlist.lub; -1 se invalido. */
	static int32 TypeFromOrder(int32 Order);

	/** Icone do balao da emocao Type (quadro do meio da acao composto numa textura 48x48), para a janela Alt+L. */
	static UTexture2D* IconFor(int32 Type);

	virtual void Tick(float DeltaSeconds) override;

private:
	UPROPERTY() TObjectPtr<UProceduralMeshComponent> Mesh;
	UPROPERTY() TArray<TObjectPtr<UMaterialInstanceDynamic>> Materials;

	int32 Action = 0;
	float Age = 0.f;
	float HeadHeight = 120.f;
};

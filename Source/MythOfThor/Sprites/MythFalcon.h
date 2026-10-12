#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "MythFalcon.generated.h"

class UMythRoSpriteComponent;

/**
 * Falcao do Cacador (OPTION_FALCON 0x10), como o roBrowserLegacy faz com o cliente atual: um companheiro separado que voa
 * acima do chao e segue o dono quando ele se afasta 2 celulas (para 1 celula antes, um pouco mais rapido que o dono),
 * planando (acao 0 do .act). No Ataque Aereo / Ataque do Falcao ele voa ate o alvo batendo as asas (acao 1), passa um
 * pouco do alvo e volta. Sprite: data/sprite/이팩트/매 (Cacador) ou 매2 (Atirador de Elite).
 */
UCLASS()
class MYTHOFTHOR_API AMythFalcon : public AActor
{
	GENERATED_BODY()

public:
	AMythFalcon();

	/** Cria (ou devolve) o falcao do dono; JobClass escolhe o sprite. */
	static AMythFalcon* Spawn(AActor* Master, int32 JobClass);

	/** Voa ate o alvo (passando dele) e volta para o dono. */
	void AttackTo(const FVector& Target);

	virtual void Tick(float DeltaSeconds) override;

	UPROPERTY(VisibleAnywhere, Category = "Myth|Falcon")
	TObjectPtr<UMythRoSpriteComponent> Sprite;

private:
	UPROPERTY() TObjectPtr<USceneComponent> Root;
	TWeakObjectPtr<AActor> Master;
	bool bFollowing = false;
	bool bAttacking = false;
	bool bReturning = false;
	FVector AttackPoint = FVector::ZeroVector;
};

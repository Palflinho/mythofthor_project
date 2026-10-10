#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "MythMonster.generated.h"

class UMythRoSpriteComponent;
class UMythCellMoverComponent;
class UBoxComponent;
class USoundBase;

/**
 * Monstro base (Poring, Lunatic...). O sprite e lido do cliente original:
 * defina Sprite->MonsterSpriteName (ex.: "poring" = data/sprite/몬스터/poring.spr/.act).
 */
UCLASS()
class MYTHOFTHOR_API AMythMonster : public ACharacter
{
	GENERATED_BODY()

public:
	AMythMonster();

	virtual void Tick(float DeltaSeconds) override;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Sprite")
	TObjectPtr<UMythRoSpriteComponent> Sprite;

	/** Anda pelas celulas do .gat seguindo o servidor (unidades online). */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Myth")
	TObjectPtr<UMythCellMoverComponent> Mover;

	/** Volume que recebe o clique do mouse (canal Visibility); o sprite 2D nao tem colisao. */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Targeting")
	TObjectPtr<UBoxComponent> ClickBox;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Targeting")
	FString MonsterName = TEXT("Monstro");

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Combat")
	int32 BaseLevel = 1;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Combat")
	int32 MaxHP = 50;

	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category = "Combat")
	int32 HP = 50;

	/** Efeitos sonoros (opcionais). Tocam em 3D na posicao do monstro. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Audio")
	TObjectPtr<USoundBase> SfxAttack;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Audio")
	TObjectPtr<USoundBase> SfxHit;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Audio")
	TObjectPtr<USoundBase> SfxDie;

	/** Tocado ao comecar a andar. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Audio")
	TObjectPtr<USoundBase> SfxMove;

	/** Para a IA de combate: toca a animacao e o som de ataque. */
	UFUNCTION(BlueprintCallable, Category = "Combat")
	void PlayAttackFeedback();

	/** Segundos entre a morte e o Destroy do ator (tempo da animacao de morte). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Combat")
	float DeathDestroyDelay = 2.f;

	/** Ex.: "Poring [Lv 1]". */
	UFUNCTION(BlueprintPure, Category = "Combat")
	FString GetDisplayName() const;

	UFUNCTION(BlueprintPure, Category = "Combat")
	float GetHealthFraction() const;

	/**
	 * Aplica dano: perde HP e toca a animacao Hit; se o HP chegar a 0, chama Kill().
	 * Ponto de entrada unico, usado pelo combate local e (depois) pelos pacotes de dano do servidor.
	 * @return true se o golpe matou o monstro.
	 */
	UFUNCTION(BlueprintCallable, Category = "Combat")
	bool ReceiveMythDamage(int32 Amount);

	UFUNCTION(BlueprintPure, Category = "Targeting")
	bool IsDead() const { return bIsDead; }

	/** Morte: desliga colisoes, limpa o alvo dos jogadores, toca a animacao e destroi o ator apos DeathDestroyDelay. */
	UFUNCTION(BlueprintCallable, Category = "Targeting")
	void Kill();

	/** ID da unidade no rAthena (AID do pacote de entrada). 0 = monstro local de teste, sem servidor. */
	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category = "Network")
	int64 NetID = 0;

	/** Velocidade do servidor (ms por celula). */
	int32 NetSpeed = 150;

	/** Tipo da unidade no pacote (0 jogador, 5 monstro, 6 NPC, 7 pet...). */
	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category = "Network")
	uint8 NetObjectType = 5;

	bool IsNetUnit() const { return NetID != 0; }
	bool IsNpc() const { return IsNetUnit() && (NetObjectType == 6 || NetObjectType == 0xC); }
	bool IsAttackable() const { return !IsNetUnit() || NetObjectType == 5; }

	/** Golpe confirmado pelo servidor: so a animacao e o som (HP e morte tambem vem do servidor). */
	void NetHit();

protected:
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

private:
	bool bIsDead = false;
	bool bWasMoving = false;

	void PlaySfx(USoundBase* Sound) const;

	/** Ajusta a ClickBox aos limites do quadro desenhado pelo sprite. */
	void FitClickBoxToSprite();
};

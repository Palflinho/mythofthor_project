#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "MythMonster.generated.h"

class UMythSpriteAnimComponent;
class UBoxComponent;

/**
 * Monstro base (Poring, Lunatic...). Crie um Blueprint filho e aponte
 * Sprite->SpriteSet para o DA_ gerado por tools/ue_build_flipbooks.py.
 */
UCLASS()
class MYTHOFTHOR_API AMythMonster : public ACharacter
{
	GENERATED_BODY()

public:
	AMythMonster();

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Sprite")
	TObjectPtr<UMythSpriteAnimComponent> Sprite;

	/** Volume que recebe o clique do mouse (canal Visibility); o sprite 2D nao tem colisao. */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Targeting")
	TObjectPtr<UBoxComponent> ClickBox;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Targeting")
	FString MonsterName = TEXT("Monstro");

	UFUNCTION(BlueprintPure, Category = "Targeting")
	bool IsDead() const { return bIsDead; }

	/** Marca como morto: toca a animacao de morte e deixa de ser alvo. */
	UFUNCTION(BlueprintCallable, Category = "Targeting")
	void Kill();

private:
	bool bIsDead = false;
};

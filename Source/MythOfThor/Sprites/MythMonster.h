#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "MythMonster.generated.h"

class UMythSpriteAnimComponent;

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
};

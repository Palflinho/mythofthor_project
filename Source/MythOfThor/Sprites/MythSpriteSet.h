#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "MythSpriteSet.generated.h"

class UPaperFlipbook;

/**
 * Conjunto de animacoes de um personagem/monstro: 5 estados x 8 direcoes.
 * Cada array tem 8 flipbooks indexados por ERagnarokDirection (0 = Sul ... 7 = Sudeste).
 * Gerado por tools/ue_build_flipbooks.py a partir do .act + spritesheet.
 */
UCLASS(BlueprintType)
class MYTHOFTHOR_API UMythSpriteSet : public UDataAsset
{
	GENERATED_BODY()

public:
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Sprite")
	TArray<TObjectPtr<UPaperFlipbook>> Idle;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Sprite")
	TArray<TObjectPtr<UPaperFlipbook>> Walk;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Sprite")
	TArray<TObjectPtr<UPaperFlipbook>> Attack;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Sprite")
	TArray<TObjectPtr<UPaperFlipbook>> Hit;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Sprite")
	TArray<TObjectPtr<UPaperFlipbook>> Die;
};

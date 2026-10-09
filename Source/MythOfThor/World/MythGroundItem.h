#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "MythGroundItem.generated.h"

class UBoxComponent;
class UMythRoSpriteComponent;

/**
 * Item caido no chao, como no cliente original: sprite de data/sprite/아이템/<recurso>.spr com a sombra redonda,
 * um pulinho ao cair e nome ao passar o mouse. Criado pelo AMythNetWorld a partir de ZC_ITEM_FALL_ENTRY /
 * ZC_ITEM_ENTRY e removido no ZC_ITEM_DISAPPEAR; clicar anda ate ele e pede CZ_ITEM_PICKUP ao servidor.
 */
UCLASS()
class MYTHOFTHOR_API AMythGroundItem : public AActor
{
	GENERATED_BODY()

public:
	AMythGroundItem();

	virtual void Tick(float DeltaSeconds) override;

	/** Configura o item (antes do BeginPlay): ID no chao (servidor), ID do item e quantidade. */
	void Setup(int64 InNetID, int32 InItemID, int32 InAmount, bool bJustDropped);

	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category = "Item")
	int64 NetID = 0;

	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category = "Item")
	int32 ItemID = 0;

	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category = "Item")
	int32 Amount = 1;

	/** "Jellopy: 3", como na dica do cliente original. */
	UFUNCTION(BlueprintPure, Category = "Item")
	FString GetLabel() const;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Item")
	TObjectPtr<USceneComponent> Root;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Item")
	TObjectPtr<UMythRoSpriteComponent> Sprite;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Item")
	TObjectPtr<UBoxComponent> ClickBox;

	/** Raridade visual (0 comum .. 4 lendario), pela taxa de drop do servidor (Config/item_rarity.json). */
	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category = "Item")
	int32 Rarity = 0;

	/** Brilho no chao (raro em diante) e feixe vertical (epico e lendario), estilo Diablo 4. */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Item")
	TObjectPtr<class UPointLightComponent> Glow;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Item")
	TObjectPtr<UStaticMeshComponent> Beam;

protected:
	virtual void BeginPlay() override;

private:
	float DropTime = -1.f; // > 0 enquanto o pulinho de queda estiver tocando
	FVector RestLocation = FVector::ZeroVector;
};

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/HUD.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "MythHUD.generated.h"

/** Tipos de texto flutuante de combate (diretrizes, secao 5). */
UENUM(BlueprintType)
enum class ECombatTextType : uint8
{
	DamageDealt UMETA(DisplayName = "Dano Causado"),   // branco/amarelado
	Critical    UMETA(DisplayName = "Critico"),        // amarelo ampliado
	DamageTaken UMETA(DisplayName = "Dano Sofrido"),   // vermelho, sobe
	Miss        UMETA(DisplayName = "MISS"),           // azul
	Heal        UMETA(DisplayName = "Cura")            // verde
};

/**
 * HUD que desenha os numeros de dano classicos sobre o alvo (sem assets/UMG).
 * Dano causado "salta" e some; dano sofrido sobe em vermelho; MISS aparece em azul.
 */
UCLASS()
class MYTHOFTHOR_API AMythHUD : public AHUD
{
	GENERATED_BODY()

public:
	virtual void DrawHUD() override;

	/** Texto livre (use para MISS ou valores ja formatados). */
	UFUNCTION(BlueprintCallable, Category = "Myth|CombatText")
	void AddCombatText(const FVector& WorldLocation, const FString& Text, ECombatTextType Type);

	/** Numero de dano/cura; Type Miss ignora Amount e mostra "MISS". */
	UFUNCTION(BlueprintCallable, Category = "Myth|CombatText")
	void AddDamageNumber(const FVector& WorldLocation, int32 Amount, ECombatTextType Type);

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Myth|CombatText")
	float Lifetime = 1.5f;

	/** Pixels que o texto sobe durante a vida. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Myth|CombatText")
	float RisePixels = 55.f;

	/** Altura do salto inicial do dano causado, em pixels. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Myth|CombatText")
	float HopPixels = 45.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Myth|CombatText")
	float BaseScale = 1.6f;

private:
	void DrawTargetRing();

	struct FEntry
	{
		FVector Location;
		FString Text;
		ECombatTextType Type;
		float Age = 0.f;
		float JitterX = 0.f;
	};
	TArray<FEntry> Entries;
};

/** Atalho estatico para Blueprints e C++ (ex.: ao receber pacote de dano do servidor). */
UCLASS()
class MYTHOFTHOR_API UMythCombatText : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintCallable, Category = "Myth|CombatText", meta = (WorldContext = "WorldContextObject"))
	static void ShowDamage(const UObject* WorldContextObject, const FVector& WorldLocation, int32 Amount, ECombatTextType Type);

	UFUNCTION(BlueprintCallable, Category = "Myth|CombatText", meta = (WorldContext = "WorldContextObject"))
	static void ShowMiss(const UObject* WorldContextObject, const FVector& WorldLocation);
};

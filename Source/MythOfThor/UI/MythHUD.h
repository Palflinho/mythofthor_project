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

	/** Barra de conjuracao sobre a unidade (ZC_USESKILL_ACK), como no cliente original. */
	void AddCastBar(AActor* Caster, float Seconds);

	/** Nome do mapa em letras grandes ao entrar (cliente LATAM, print 61034761): aparece, segura e some em ~4 s. */
	void ShowMapBanner(const FString& MapId);

	/** Nome da skill sobre quem a usou, por um instante (o RO mostra o grito da skill). */
	void AddSkillShout(AActor* Caster, const FString& SkillName);
	/** Balao de fala sobre a cabeca (fala publica; dura 4,5 s, docs/ref/18 2.3). */
	void AddSpeechBubble(AActor* Speaker, const FString& Text);

	/**
	 * Icone de estado (buff/debuff) na coluna da direita, como no cliente original, com uma barra de duracao ao lado.
	 * EfstID = "type" dos pacotes de status do rAthena (EFST_*). TotalMs 0 = sem duracao (sem barra).
	 */
	void SetStatusIcon(int32 EfstID, bool bActive, int32 TotalMs, int32 RemainMs);
	void ClearStatusIcons() { StatusIcons.Reset(); }

private:
	void DrawStatusIcons();
	struct FStatusIcon { int32 Efst = 0; float End = 0.f; float Total = 0.f; };
	TArray<FStatusIcon> StatusIcons;

	void DrawTargetRing();
	void DrawMapBanner();
	FString BannerText;
	double BannerStart = -100.0;
	/** Quadrado do tile sob o mouse (cantos verdes) e, mirando skill de area, o anel branco com o nivel da skill. */
	void DrawCursorTile();
	/** Barras finas de HP (verde) e SP (azul) logo abaixo dos pes do proprio jogador (docs/ref/12, no mundo). */
	void DrawSelfBars();
	/** Nome dos itens no chao, na cor da raridade. */
	void DrawGroundItemLabels();
	void DrawCastBarsAndShouts();
	struct FCastBar { TWeakObjectPtr<AActor> Actor; float Start = 0.f; float Duration = 0.f; };
	struct FShout { TWeakObjectPtr<AActor> Actor; FString Text; float Start = 0.f; };
	struct FBubble { TWeakObjectPtr<AActor> Actor; FString Text; float Start = 0.f; };
	TArray<FBubble> Bubbles;
	TArray<FCastBar> CastBars;
	TArray<FShout> Shouts;

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

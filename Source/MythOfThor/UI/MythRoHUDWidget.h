#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "Types/SlateEnums.h"
#include "MythRoHUDWidget.generated.h"

class UButton;
class UCanvasPanel;
class UCanvasPanelSlot;
class UEditableTextBox;
class UImage;
class UTextBlock;
class UTexture2D;
class UVerticalBox;
class UMythStatsComponent;
class UAutoBattleComponent;

/** Barra de medidor classica (gze*_left/mid/right sobre gze_bg): o meio estica conforme a fracao. */
USTRUCT()
struct FMythRoGauge
{
	GENERATED_BODY()

	UPROPERTY() TObjectPtr<UImage> Left;
	UPROPERTY() TObjectPtr<UImage> Mid;
	UPROPERTY() TObjectPtr<UImage> Right;
	float X = 0.f, Y = 0.f, Width = 0.f, Height = 0.f, Scale = 1.f;

	void SetFraction(float Fraction);
};

/**
 * HUD com o layout moderno combinado (painel do personagem, minimapa, atalhos, chat, Auto-Battle) e o
 * visual do cliente original: todas as artes vem do data.grf (texture/유저인터페이스), via FMythRoUITextures.
 *   topo-esquerda : Informacoes basicas (basewin_bg) com HP/SP, EXP de base/classe, peso, zeny e os 6 botoes;
 *   topo-direita  : minimapa (map/<mapa>.bmp) com o ponto do jogador e as coordenadas da celula;
 *   baixo-centro  : barra de atalhos F1-F9 (shortitem_bg);
 *   baixo-esquerda: chat com abas e campo de digitacao (Enter abre/envia);
 *   baixo-direita : Auto-Battle (estado, vigor de 3h, peso).
 */
UCLASS()
class MYTHOFTHOR_API UMythRoHUDWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	/** Escala da interface (as artes originais sao pequenas para telas atuais). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Myth|HUD")
	float UIScale = 1.35f;

	UFUNCTION(BlueprintCallable, Category = "Myth|HUD")
	void BindToPawn(APawn* Pawn);

	/** Adiciona uma linha ao chat (sistema, mensagens, erros). */
	UFUNCTION(BlueprintCallable, Category = "Myth|HUD")
	void AddChatLine(const FString& Text, FLinearColor Color);

	/** Enter: foca o campo do chat (ou envia, se ja estiver focado). */
	UFUNCTION(BlueprintCallable, Category = "Myth|HUD")
	void FocusChat();

	UFUNCTION(BlueprintPure, Category = "Myth|HUD")
	bool IsChatFocused() const;

protected:
	virtual void NativeOnInitialized() override;
	virtual void NativeTick(const FGeometry& MyGeometry, float InDeltaTime) override;

private:
	UPROPERTY() TObjectPtr<UCanvasPanel> Root;

	// Informacoes basicas
	UPROPERTY() TObjectPtr<UTextBlock> NameText;
	UPROPERTY() TObjectPtr<UTextBlock> JobText;
	UPROPERTY() TObjectPtr<UTextBlock> HPText;
	UPROPERTY() TObjectPtr<UTextBlock> SPText;
	UPROPERTY() TObjectPtr<UTextBlock> BaseLvText;
	UPROPERTY() TObjectPtr<UTextBlock> JobLvText;
	UPROPERTY() TObjectPtr<UTextBlock> WeightZenyText;
	UPROPERTY() FMythRoGauge HPGauge;
	UPROPERTY() FMythRoGauge SPGauge;
	UPROPERTY() FMythRoGauge BaseExpGauge;
	UPROPERTY() FMythRoGauge JobExpGauge;

	// Minimapa
	UPROPERTY() TObjectPtr<UCanvasPanel> MinimapWindow;
	UPROPERTY() TObjectPtr<UImage> MinimapImage;
	UPROPERTY() TObjectPtr<UImage> PlayerDot;
	UPROPERTY() TObjectPtr<UTextBlock> MapTitleText;
	UPROPERTY() TObjectPtr<UTextBlock> CoordsText;
	FString LoadedMinimap;

	// Chat
	UPROPERTY() TObjectPtr<UVerticalBox> ChatLines;
	UPROPERTY() TObjectPtr<UEditableTextBox> ChatInput;

	// Auto-Battle
	UPROPERTY() TObjectPtr<UButton> AutoButton;
	UPROPERTY() TObjectPtr<UTextBlock> AutoButtonText;
	UPROPERTY() TObjectPtr<UTextBlock> StaminaText;
	UPROPERTY() FMythRoGauge StaminaGauge;
	int32 LastShownSecond = -1;

	UPROPERTY() TObjectPtr<UMythStatsComponent> Stats;
	UPROPERTY() TObjectPtr<UAutoBattleComponent> AutoBattle;
	TWeakObjectPtr<APawn> BoundPawn;
	float WeightPercent = 0.f;

	void BuildLayout();
	void BuildBasicInfo();
	void BuildMinimap();
	void BuildHotbar();
	void BuildChat();
	void BuildAutoBattle();
	void UpdateMinimap();

	// Construcao (coordenadas em pixels das artes originais; multiplicadas por UIScale)
	UCanvasPanel* MakeWindow(const FVector2D& Anchor, const FVector2D& Alignment, const FVector2D& Offset, const FVector2D& Size);
	UImage* AddImage(UCanvasPanel* Parent, UTexture2D* Tex, float X, float Y, float W = -1.f, float H = -1.f);
	UImage* AddColor(UCanvasPanel* Parent, const FLinearColor& Color, float X, float Y, float W, float H);
	UTextBlock* AddText(UCanvasPanel* Parent, float X, float Y, int32 Size, const FLinearColor& Color, bool bShadow = false);
	void AddTitleBar(UCanvasPanel* Parent, float Width, const FString& Title, UTextBlock** OutText = nullptr);
	FMythRoGauge MakeGauge(UCanvasPanel* Parent, const TCHAR* Color, float X, float Y, float W, bool bBackground = true);
	UButton* AddRoButton(UCanvasPanel* Parent, const TCHAR* BaseName, float X, float Y);

	UFUNCTION() void RefreshStats();
	UFUNCTION() void RefreshAutoState(bool bIsActive);
	UFUNCTION() void RefreshStamina(float RemainingSeconds);
	UFUNCTION() void RefreshWeight(float InWeightPercent, bool bOverweight);
	UFUNCTION() void ShowError(const FString& Reason);
	UFUNCTION() void OnAutoButtonClicked();
	UFUNCTION() void OnChatCommitted(const FText& Text, ETextCommit::Type CommitMethod);
	UFUNCTION() void OnStatusClicked();
	UFUNCTION() void OnItemsClicked();
	UFUNCTION() void OnEquipClicked();
	UFUNCTION() void OnSkillClicked();
	UFUNCTION() void OnMapClicked();
	UFUNCTION() void OnOptionClicked();
	void RefreshAutoAll();
};

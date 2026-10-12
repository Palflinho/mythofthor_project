#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "Types/SlateEnums.h"
#include "Network/RagnarokNetworkSubsystem.h"

/** Opcoes graficas do menu Esc (valores da tela; aplicadas pelo UGameUserSettings). */
struct FMythGfxOptions
{
	TArray<FIntPoint> Resolutions;
	int32 ResolutionIndex = 0;
	int32 WindowMode = 0;      // 0 janela, 1 tela cheia sem borda, 2 tela cheia
	int32 Quality = 3;         // 0 baixa .. 4 cinematica
	int32 Shadows = 3;
	int32 Lumen = 1;
	int32 AntiAliasing = 1;    // 0 nenhum, 1 FXAA (padrao do projeto: TSR/TAA deixam rastro nos sprites), 2 TAA, 3 TSR
	int32 ResolutionScale = 10; // 5..10 = 50%..100%
	int32 FpsIndex = 1;        // 30, 60, 120, 144, sem limite
	bool bVSync = true;
	int32 Volume = 10;         // 0..10
};

#include "MythRoHUDWidget.generated.h"

struct FRagnarokShopItem;

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
class URagnarokNetworkSubsystem;
class UWidget;

/** Janelas classicas abertas pelos atalhos Alt (como no cliente original). */
UENUM(BlueprintType)
enum class EMythRoWindow : uint8
{
	Status,   // Alt+A: atributos
	Items,    // Alt+E: inventario
	Equip,    // Alt+Q: equipamento
	Skills,   // Alt+S: habilidades
	Basic,    // Alt+V: informacoes basicas
	Minimap,  // Ctrl+Tab: minimapa
	Party,    // Alt+Z: grupo
	Emotions, // Alt+L: lista de emocoes
	Macros,   // Alt+M: lista de atalhos de emocao
	Friends,  // Alt+H: amigos
	ChatRoom, // Alt+C: sala de chat
	Quests,   // Alt+U: missoes
	Shortcuts, // F12: tabela de teclas
	Bank,     // Ctrl+B: banco
	Achievements, // Alt+Y: conquistas
	EquipSwitch, // Alt+W: troca rapida
	Navi,     // Alt+N: navegacao
	Mail,     // Alt+R: correio
	WorldMap, // Ctrl+`: mapa-mundi
	Guild,    // Alt+G: cla
	Pet       // Alt+J: mascote
};

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
	/** Indice do primeiro atalho da linha ativa da barra (F1 = base + 0). */
	int32 HotbarActiveBase() const { return HotbarActiveRow * 9; }
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
	/** Trata o texto do chat: comandos de barra, cochicho, grupo, emocao; false = fala publica normal. */
	bool HandleChatCommand(const FString& Msg);
	/** Envia um texto como se tivesse sido digitado e confirmado no chat (comandos, cochicho, fala). */
	void SubmitChat(const FString& Msg);
	/** Historico de entrada: ultimas 20 mensagens, navegadas por Cima e Baixo (docs/ref/18 2.1). */
	TArray<FString> ChatHistory;
	int32 ChatHistoryPos = INDEX_NONE;
	FString LastWhisperFrom;
	void SetLastWhisperFrom(const FString& Name) { LastWhisperFrom = Name; }

	UFUNCTION(BlueprintPure, Category = "Myth|HUD")
	bool IsChatFocused() const;

	/** Abre/fecha uma janela classica (atalhos Alt do cliente original). */
	UFUNCTION(BlueprintCallable, Category = "Myth|HUD")
	void ToggleWindow(EMythRoWindow Window);

	// ---- Dialogo de NPC (ZC_SAY_DIALOG / ZC_WAIT_DIALOG / ZC_CLOSE_DIALOG / ZC_MENU_LIST do rAthena)

	/** Acrescenta uma fala do NPC (mes). Cores do RO (^RRGGBB) sao respeitadas. */
	void NpcSay(int64 NpcID, const FString& Text);
	/** next; do script: mostra o botao "Proximo". */
	void NpcWait(int64 NpcID);
	/** close; do script: mostra o botao "Fechar". */
	void NpcCloseButton(int64 NpcID);
	/** input do script: caixa de numero (ZC_OPEN_EDITDLG) ou de texto (ZC_OPEN_EDITDLGSTR) abaixo da fala. */
	void NpcInput(int64 NpcID, bool bNumber);
	/** Teste: preenche a caixa de entrada e confirma. */
	void NpcInputSubmitText(const FString& Text);
	/** Teste: aba da janela de Equipamento (0 Geral, 1 Especial, 2 Titulo, 3 Fonte de dano). */
	void SetEquipTab(int32 Tab) { EquipTab = FMath::Clamp(Tab, 0, 3); RefreshEquipWindow(); }
	/** Teste: troca a aba da janela de Habilidades (0 = 1a, 1 = 2a, 2 = 3a, 3 = Etc). */
	void SetSkillTab(int32 Tab) { SkillTab = FMath::Clamp(Tab, 0, 3); RefreshSkillsWindow(); }
	/** select(...) do script: lista de opcoes (vazias ficam ocultas, mas contam no indice enviado). */
	void NpcMenu(int64 NpcID, const TArray<FString>& Options);
	/** Fecha a janela de dialogo sem enviar nada (ex.: o servidor encerrou). */
	void HideNpcDialog();
	/** Enter: avanca/fecha o dialogo aberto. Devolve false se nao havia dialogo esperando. */
	bool NpcDialogConfirm();
	/** Dialogo de NPC aberto: como no cliente original, cliques no mundo nao andam nem atacam (docs/ref/04 6). */
	bool IsNpcDialogOpen() const;
	/** Menu do NPC: tecla 1..9 escolhe a opcao (Row 0..8); setas movem a selecao. False sem menu aberto. */
	bool NpcMenuChoose(int32 Row);
	bool NpcMenuMove(int32 Delta);

	/** Esc: fecha a janela classica aberta mais recente. Devolve false se nao havia nenhuma. */
	UFUNCTION(BlueprintCallable, Category = "Myth|HUD")
	bool CloseTopWindow();

	/** Menu do jogo (Esc sem janela aberta). Retorna verdadeiro se abriu. */
	bool ToggleGameMenu();

protected:
	virtual void NativeOnInitialized() override;
	virtual void NativeTick(const FGeometry& MyGeometry, float InDeltaTime) override;
	void UpdateButtons();
	/** Poe todas as caixas de texto no estilo do cliente (mesma fonte e tamanho do resto da interface). */
	void StyleEditBoxes();
	// Arrastar janelas pela barra de titulo, como no cliente original
	virtual FReply NativeOnMouseButtonDown(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent) override;
	virtual FReply NativeOnPreviewKeyDown(const FGeometry& InGeometry, const FKeyEvent& InKeyEvent) override;
	virtual FReply NativeOnMouseWheel(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent) override;
	virtual FReply NativeOnMouseMove(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent) override;
	virtual FReply NativeOnMouseButtonUp(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent) override;
	virtual FReply NativeOnMouseButtonDoubleClick(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent) override;

protected:
	/** Janelas arrastaveis e a altura (px originais) da faixa que serve de "alca" (barra de titulo). */
	struct FDragWindow
	{
		TWeakObjectPtr<UCanvasPanel> Window;
		float HandleHeight = 17.f;
	};
	TArray<FDragWindow> DragWindows;
	/** Janelas travadas no lugar (botao de cadeado, ex.: Grupo): nao arrastam. */
	TSet<const UWidget*> LockedWindows;
	TWeakObjectPtr<UCanvasPanel> Dragging;
	FVector2D DragOffset = FVector2D::ZeroVector;
	int32 TopZOrder = 10;
	/** Torna a janela arrastavel: cria a alca invisivel (recebe o clique) sobre a faixa do topo. */
	void MakeDraggable(UCanvasPanel* Window, float Width, float HandleHeight);
	UPROPERTY() TObjectPtr<UCanvasPanel> Root;

	// Informacoes basicas
	UPROPERTY() TObjectPtr<UTextBlock> NameText;
	UPROPERTY() TObjectPtr<UTextBlock> JobText;
	UPROPERTY() TObjectPtr<UTextBlock> HPText;
	UPROPERTY() TObjectPtr<UTextBlock> HPPctText;
	UPROPERTY() TObjectPtr<UTextBlock> SPPctText;
	UPROPERTY() TObjectPtr<UTextBlock> SPText;
	UPROPERTY() TObjectPtr<UTextBlock> BaseLvText;
	UPROPERTY() TObjectPtr<UTextBlock> BaseExpPctText;
	UPROPERTY() TObjectPtr<UTextBlock> JobExpPctText;
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
	UPROPERTY() TObjectPtr<UCanvasPanel> MinimapView;
	UPROPERTY() TObjectPtr<UImage> MinimapZoomIn;
	UPROPERTY() TObjectPtr<UImage> MinimapZoomOut;
	UPROPERTY() TObjectPtr<UImage> MinimapFold;
	float MinimapZoom = 1.f;
	float MinimapShownSize = 0.f;
	FString LoadedMinimapBrush;
	bool bMinimapFolded = false;
	bool HandleMinimapClick(const FVector2D& ScreenPos);
	UPROPERTY() TObjectPtr<UTextBlock> MapTitleText;
	UPROPERTY() TObjectPtr<UTextBlock> CoordsText;
	FString LoadedMinimap;

	// Chat
	UPROPERTY() TObjectPtr<UVerticalBox> ChatLines;
	/** Historico do chat: as 7 linhas da tela sao uma janela sobre ele (roda do mouse, setas e barra de rolagem). */
	struct FChatEntry { FString Text; FLinearColor Color; bool bBattle = false; };
	TArray<FChatEntry> ChatLog;
	int32 ChatScroll = 0;
	int32 ChatTab = 0;
	UPROPERTY() TObjectPtr<UCanvasPanel> ChatWindow;
	UPROPERTY() TArray<TObjectPtr<UImage>> ChatTabBoxes;
	UPROPERTY() TArray<TObjectPtr<UTextBlock>> ChatTabTexts;
	UPROPERTY() TObjectPtr<UImage> ChatUpArrow;
	UPROPERTY() TObjectPtr<UImage> ChatDownArrow;
	UPROPERTY() TObjectPtr<UImage> ChatThumb;
	void RebuildChatView();
	bool HandleChatClick(const FVector2D& ScreenPos);
	bool HandleChatWheel(const FVector2D& ScreenPos, float Delta);
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
	/** Botao do cliente (btn_<Nome>.bmp, _a ao passar o mouse, _b ao apertar): o rotulo ja vem desenhado na arte. */
	UImage* AddButton(UCanvasPanel* Parent, const TCHAR* Name, float X, float Y);
	UImage* AddButtonEx(UCanvasPanel* Parent, const FString& Normal, const FString& Hover, const FString& Pressed, float X, float Y);
	/** Botao do cliente com o rotulo desenhado na arte; devolve o texto invisivel que cobre o botao (para IsUnder). */
	UTextBlock* AddButtonHit(UCanvasPanel* Parent, const TCHAR* Name, float X, float Y);
	/** Botao de texto (btn_out/over/press em 3 partes) com o rotulo no meio; devolve o proprio texto, que cobre o botao. */
	UTextBlock* AddTextButton(UCanvasPanel* Parent, const FString& Label, float X, float Y, float W);
	UImage* AddColor(UCanvasPanel* Parent, const FLinearColor& Color, float X, float Y, float W, float H);
	/** Retangulo de cantos arredondados (RoundedBox do Slate) com borda fina: corpo de janela translucido como nos prints do LATAM. */
	UImage* AddRounded(UCanvasPanel* Parent, const FLinearColor& Fill, const FLinearColor& Border, float X, float Y, float W, float H, float RadiusTop, float RadiusBottom);
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

	// ---------------------------------------------------------------- janelas classicas (MythRoHUDWindows.cpp)
	// Tudo o que aparece nelas vem do servidor (URagnarokNetworkSubsystem); o cliente so desenha e envia pedidos.
	UPROPERTY() TObjectPtr<UCanvasPanel> BasicWindow;
	// Informacoes basicas do LATAM: fundo, barra de recolher e a grade de icones do menu (menu_icon/bt_*.bmp)
	UPROPERTY() TObjectPtr<UImage> BasicBody;
	UPROPERTY() TObjectPtr<UImage> MenuToggleBar;
	UPROPERTY() TObjectPtr<UImage> MenuBack;
	UPROPERTY() TArray<TObjectPtr<UImage>> MenuIcons;
	TArray<FString> MenuIconNames;
	bool bBasicMenuOpen = true;
	int32 BasicDetailStart = 0;
	TMap<TWeakObjectPtr<UWidget>, ESlateVisibility> BasicSavedVisibility;
	UPROPERTY() TObjectPtr<UTextBlock> BasicMini1;
	UPROPERTY() TObjectPtr<UTextBlock> BasicMini2;
	UPROPERTY() TObjectPtr<UImage> BasicMiniButton;
	static constexpr float BasicFullH = 368.f;
	/** Altura do quadro branco das informacoes (ate a barra de recolher); a grade do menu fica abaixo, sobre o fundo translucido. */
	static constexpr float BasicInfoH = 134.f;
	void SetBasicMenuOpen(bool bOpen);
	bool HandleBasicMenuClick(const FVector2D& ScreenPos);
	UPROPERTY() TObjectPtr<UCanvasPanel> StatusWindow;
	UPROPERTY() TObjectPtr<UCanvasPanel> ItemsWindow;
	UPROPERTY() TObjectPtr<UCanvasPanel> EquipWindow;
	UPROPERTY() TObjectPtr<UCanvasPanel> SkillsWindow;
	TArray<TWeakObjectPtr<UCanvasPanel>> OpenOrder;

	// Botoes de fechar (sys_close) de cada janela
	TMap<TWeakObjectPtr<UImage>, TWeakObjectPtr<UCanvasPanel>> CloseButtons;

	// Atributos: valores de Str..Luk, setas de aumentar e os derivados (Atk, Matk, Hit, Crit, Def, Mdef, Flee, Aspd, pontos)
	UPROPERTY() TArray<TObjectPtr<UTextBlock>> StatTexts;
	UPROPERTY() TArray<TObjectPtr<UTextBlock>> StatCostTexts;
	UPROPERTY() TArray<TObjectPtr<UImage>> StatUpArrows;
	UPROPERTY() TArray<TObjectPtr<UTextBlock>> DerivedTexts;

	// Itens: abas item/equip/etc, celulas 32x32 com icone e quantidade, linha de informacao
	UPROPERTY() TObjectPtr<UImage> ItemTabImage;
	UPROPERTY() TObjectPtr<UCanvasPanel> ItemInfoWindow;
	UPROPERTY() TObjectPtr<UImage> ItemInfoImage;
	UPROPERTY() TObjectPtr<UTextBlock> ItemInfoName;
	UPROPERTY() TObjectPtr<class UScrollBox> ItemInfoBody;
	UPROPERTY() TArray<TObjectPtr<UImage>> ItemCells;
	UPROPERTY() TArray<TObjectPtr<UImage>> ItemIcons;
	UPROPERTY() TArray<TObjectPtr<UTextBlock>> ItemAmounts;
	UPROPERTY() TObjectPtr<UTextBlock> ItemInfoText;
	UPROPERTY() TObjectPtr<UTextBlock> ItemCountText;
	TArray<int32> ItemCellIndex; // indice do inventario (servidor) em cada celula, -1 se vazia
	int32 ItemTab = 0;

	// Equipamento: icone + nome em cada um dos 10 encaixes
	UPROPERTY() TArray<TObjectPtr<UImage>> EquipIcons;
	UPROPERTY() TArray<TObjectPtr<UTextBlock>> EquipNames;
	UPROPERTY() TObjectPtr<UImage> EquipBack;
	UPROPERTY() TArray<TObjectPtr<UImage>> EquipTabBoxes;
	UPROPERTY() TArray<TObjectPtr<UTextBlock>> EquipTabTexts;
	int32 EquipTab = 0;
	TArray<int32> EquipCellIndex;
	// Aba Geral: encaixe da municao acima do personagem e o botao de soltar falcao / descer do Peco abaixo dele
	UPROPERTY() TObjectPtr<UImage> EquipAmmoFrame;
	UPROPERTY() TObjectPtr<UImage> EquipAmmoIcon;
	UPROPERTY() TObjectPtr<UTextBlock> EquipAmmoName;
	int32 EquipAmmoIndex = -1;
	/** Botao redondo "off" sob o personagem (btn_off): solta o falcao / desce do Peco Peco. */
	UPROPERTY() TObjectPtr<UImage> EquipMountBtn;
	/** Previa do personagem no centro (UMythRoSpriteComponent::RenderPortrait), redesenhada quando o visual muda. */
	UPROPERTY() TObjectPtr<UImage> EquipPreview;
	UPROPERTY() TObjectPtr<UTexture2D> EquipPreviewTex;
	int32 EquipPreviewVersion = -1;
	/** Rodape: caixa "Mostrar equip." (CZ_CONFIG) e os botoes atributos / troca rapida / tirar tudo. */
	UPROPERTY() TObjectPtr<UImage> EquipShowCheck;
	UPROPERTY() TObjectPtr<UImage> EquipBtnStatus;
	UPROPERTY() TObjectPtr<UImage> EquipBtnSwitch;
	UPROPERTY() TObjectPtr<UImage> EquipBtnOffAll;
	/** Abas Titulo e Fonte de dano: texto no lugar dos encaixes. */
	UPROPERTY() TObjectPtr<UTextBlock> EquipTabNote;
	/** Aba Titulo: "(sem titulo)" e os titulos das conquistas com premio recebido (Config/titles.json); clique usa. */
	UPROPERTY() TArray<TObjectPtr<UImage>> EquipTitleHits;
	UPROPERTY() TArray<TObjectPtr<UTextBlock>> EquipTitleTexts;
	TArray<int32> EquipTitleIds;
	void UpdateEquipPreview();

	// Habilidades: linhas com icone, nome e nivel
	UPROPERTY() TArray<TObjectPtr<UImage>> SkillIcons;
	UPROPERTY() TArray<TObjectPtr<UTextBlock>> SkillNames;
	UPROPERTY() TArray<TObjectPtr<UTextBlock>> SkillLevels;
	UPROPERTY() TObjectPtr<UTextBlock> SkillPointsText;
	UPROPERTY() TObjectPtr<UImage> SkillApplyImg;
	UPROPERTY() TObjectPtr<UImage> SkillResetImg;
	/** Pontos de habilidade escolhidos mas ainda nao confirmados (OK envia, Cancelar descarta), como o Apply/Reset do cliente moderno. */
	TMap<int32, int32> PendingSkillUps;
	int32 PendingSkillTotal() const { int32 N = 0; for (const TPair<int32, int32>& P : PendingSkillUps) { N += P.Value; } return N; }
	// Arvore de habilidades (skilltreeview.lub do cliente): celulas da grade 7 colunas, abas por etapa da classe e
	// as linhas que ligam cada skill ao pre-requisito
	UPROPERTY() TArray<TObjectPtr<UImage>> SkillCellFrames;
	UPROPERTY() TArray<TObjectPtr<UTextBlock>> SkillTitles;
	UPROPERTY() TArray<TObjectPtr<UImage>> SkillTabBacks;
	UPROPERTY() TArray<TObjectPtr<UTextBlock>> SkillTabTexts;
	UPROPERTY() TArray<TObjectPtr<UImage>> SkillLinePool;
	UPROPERTY() TArray<TObjectPtr<UWidget>> SkillBottomBar;
	void ResizeSkillTree(int32 Rows);
	/** Abas do LATAM (print 13): 1a = Aprendiz + 1a classe, 2a = 2a classe (+ transclasse), 3a e Etc vazias. Cada aba junta as
	 * grades dessas classes do skilltreeview (as casas nao se sobrepoem: a 1a classe deixa livres as do Aprendiz). */
	TArray<TArray<int32>> SkillTabJobs;
	int32 SkillTab = -1; // -1 = a aba da classe atual
	/** Personagem no canto de baixo da janela (como no print), feito por UMythRoSpriteComponent::RenderPortrait. */
	UPROPERTY() TObjectPtr<UImage> SkillPortrait;
	UPROPERTY() TObjectPtr<UTexture2D> SkillPortraitTex;
	int32 SkillPortraitVersion = -1;

	float WindowRefreshTimer = 0.f;

	// Barra de atalhos F1..F9 e arrastar skills/itens para ela (MythRoHotbar.cpp)
	UPROPERTY() TObjectPtr<UCanvasPanel> HotbarWindow;
	UPROPERTY() TArray<TObjectPtr<UImage>> HotbarSlots;
	UPROPERTY() TArray<TObjectPtr<UImage>> HotbarIcons;
	UPROPERTY() TArray<TObjectPtr<UTextBlock>> HotbarCounts;
	UPROPERTY() TArray<TObjectPtr<UImage>> HotbarRowBoxes;
	UPROPERTY() TArray<TObjectPtr<UTextBlock>> HotbarRowTexts;
	UPROPERTY() TObjectPtr<UImage> HotbarUp;
	UPROPERTY() TObjectPtr<UImage> HotbarDown;
	/** Linhas da barra: 4 de 9 atalhos (F1..F9); 2 aparecem por vez, e a ativa e a que as teclas F usam. */
	int32 HotbarFirstRow = 0;
	int32 HotbarActiveRow = 0;
	bool HandleHotbarClick(const FVector2D& ScreenPos);
	UPROPERTY() TObjectPtr<UImage> DragIcon;
	TArray<int32> SkillRowID;
	TArray<int32> SkillRowLevel;
	/** Nivel escolhido para usar cada skill (setas ao lado do nivel na arvore); vai para o atalho e para o duplo clique. */
	TMap<int32, int32> SkillSelLevel;
	/** Botao "Lv Up" de cada linha (lv_up_on.bmp do cliente), visivel com pontos e skill que ainda sobe. */
	UPROPERTY() TArray<TObjectPtr<UImage>> SkillUpButtons;
	/** Primeira skill mostrada (a roda do mouse rola a lista, que tem mais skills que linhas). */
	int32 SkillScroll = 0;
	bool bHotDragging = false;
	bool bDragIsSkill = false;
	int32 DragID = 0;
	int32 DragCount = 0;
	int32 DragFromHotbar = INDEX_NONE;
	int32 DragItemIndex = INDEX_NONE; // indice do item da mochila arrastado (solta no chao se largar fora das janelas)
	bool IsOverAnyWindow(const FVector2D& ScreenPos) const;
	void RefreshHotbar();
	bool BeginHotDrag(const FVector2D& ScreenPos);
	void UpdateHotDrag(const FVector2D& ScreenPos);
	bool EndHotDrag(const FVector2D& ScreenPos);
	int32 HotbarSlotAt(const FVector2D& ScreenPos) const;

	// Dialogo de NPC (MythRoNpcDialog.cpp)
	UPROPERTY() TObjectPtr<UCanvasPanel> NpcWindow;
	UPROPERTY() TObjectPtr<class UWrapBox> NpcText;
	UPROPERTY() TObjectPtr<UImage> NpcNextButton;
	UPROPERTY() TObjectPtr<UImage> NpcCloseBtn;
	UPROPERTY() TObjectPtr<UCanvasPanel> NpcMenuWindow;
	UPROPERTY() TArray<TObjectPtr<UImage>> NpcMenuRows;
	UPROPERTY() TArray<TObjectPtr<UTextBlock>> NpcMenuTexts;
	UPROPERTY() TObjectPtr<UImage> NpcMenuOk;
	UPROPERTY() TObjectPtr<UImage> NpcMenuCancel;
	UPROPERTY() TObjectPtr<UCanvasPanel> NpcInputWindow;
	UPROPERTY() TObjectPtr<UEditableTextBox> NpcInputBox;
	UPROPERTY() TObjectPtr<UTextBlock> NpcInputHint;
	UPROPERTY() TObjectPtr<UImage> NpcInputOk;
	UPROPERTY() TObjectPtr<UImage> NpcInputCancel;
	bool bNpcInputNumber = true;
	void SubmitNpcInput();
	UFUNCTION() void OnNpcInputCommitted(const FText& Text, ETextCommit::Type CommitMethod);
	UPROPERTY() TObjectPtr<UImage> NpcMenuEdge;
	UPROPERTY() TObjectPtr<UImage> NpcMenuBody;
	TArray<int32> NpcMenuIndex; // indice (1..N) enviado ao servidor para cada linha visivel
	int32 NpcMenuSelected = 0;
	int64 NpcTalking = 0;
	bool bNpcClearOnNextSay = false;
	void BuildNpcDialog();
	bool HandleNpcClick(const FVector2D& ScreenPos, bool bDouble);
	void RefreshNpcMenuHighlight();

	URagnarokNetworkSubsystem* GetNet() const;
	UCanvasPanel* MakeRoPanel(const FString& Title, const FVector2D& Size, const FVector2D& Offset);
	void BuildStatusWindow();
	void BuildItemsWindow();
	void BuildEquipWindow();
	void UpdateEquipMount();
public:
	void SetItemTab(int32 Tab) { ItemTab = Tab; if (ItemsWindow) { RefreshItemsWindow(); } }
private:
	void BuildSkillsWindow();
	void BuildItemInfoWindow();
	/** Janela de descricao do item (botao direito no inventario/equipamento), como no cliente original. */
	void ShowItemInfo(const struct FRagnarokInventoryItem& Item);
	void ShowSkillInfo(int32 SkillID);
	void ShowInfoLines(const FString& Name, UTexture2D* Illust, const TArray<FString>& Lines);
	bool HandleItemInfoClick(const FVector2D& ScreenPos);

	// Menu Esc e configuracoes graficas (MythRoGameMenu.cpp)
	static constexpr int32 GfxRowCount = 9;
	void BuildGameMenu();
	void LoadGraphicsOptions();
	void ApplyGraphicsOptions();
	void ApplyMythGraphics();
	void RefreshGraphicsWindow();
	void StepGraphicsOption(int32 Row, int32 Delta);
	bool HandleGameMenuClick(const FVector2D& ScreenPos);
	// Entrada do jogo: login, escolha e criacao de personagem (MythRoEntry.cpp)
public:
	void ShowEntryLogin(const FString& Message = FString());
	/** Menu Esc "Seleção de personagem": sai do mapa e entra de novo com o login da sessão (a senha fica só na memoria). */
	void ReturnToCharacterSelect();
	void ShowEntryCharacters(const TArray<FRagnarokCharacterSlotInfo>& Chars);
	void HideEntry();
	/** Teste: preenche usuario e senha e entra. */
	void EntryFillAndLogin(const FString& User, const FString& Pass);
	/** Teste: age sobre a tela de personagens (0 jogar, 1 criar com Nome, 2 so mostra). */
	void EntryTestAction(int32 Action, const FString& Name);
	bool IsEntryActive() const { return EntryState != 0; }
	void EntryLoginFailed(const FString& Message);
	void EntryCharCreateResult(bool bOk, const FString& Message);
protected:
	void BuildEntryScreens();
	void SetEntryPanel(int32 State);
	void SubmitEntryLogin();
	void RefreshEntryCharacters();
	void ShowEntryCreate(int32 Slot);
	const FRagnarokCharacterSlotInfo* EntrySlotChar(int32 Slot) const;
	bool HandleEntryClick(const FVector2D& ScreenPos);
	UFUNCTION() void OnEntryCommitted(const FText& Text, ETextCommit::Type CommitMethod);
	static constexpr int32 EntryCardCount = 9;
	int32 EntryState = 0; // 0 fora, 1 login, 2 conectando, 3 personagens, 4 criar
	TArray<FRagnarokCharacterSlotInfo> EntryChars;
	int32 EntrySelected = -1;
	int32 EntryCreateSlot = 0;
	int32 EntryHair = 1;
	int32 EntryColor = 0;
	UPROPERTY() TObjectPtr<UCanvasPanel> EntryBackdrop;
	UPROPERTY() TObjectPtr<UCanvasPanel> EntryLoginPanel;
	UPROPERTY() TObjectPtr<UCanvasPanel> EntryCharPanel;
	UPROPERTY() TObjectPtr<UCanvasPanel> EntryCreatePanel;
	UPROPERTY() TObjectPtr<UEditableTextBox> EntryUserBox;
	UPROPERTY() TObjectPtr<UEditableTextBox> EntryPassBox;
	UPROPERTY() TObjectPtr<UEditableTextBox> EntryNameBox;
	UPROPERTY() TObjectPtr<UTextBlock> EntryStatus;
	UPROPERTY() TObjectPtr<UTextBlock> EntryLoginLink;
	UPROPERTY() TObjectPtr<UTextBlock> EntryQuitLink;
	UPROPERTY() TObjectPtr<UTextBlock> EntryPlayLink;
	UPROPERTY() TObjectPtr<UTextBlock> EntryCreateLink;
	UPROPERTY() TObjectPtr<UTextBlock> EntryBackLink;
	UPROPERTY() TObjectPtr<UTextBlock> EntryCharStatus;
	UPROPERTY() TObjectPtr<UTextBlock> EntryHairLeft;
	UPROPERTY() TObjectPtr<UTextBlock> EntryHairText;
	UPROPERTY() TObjectPtr<UTextBlock> EntryHairRight;
	UPROPERTY() TObjectPtr<UTextBlock> EntryColorLeft;
	UPROPERTY() TObjectPtr<UTextBlock> EntryColorText;
	UPROPERTY() TObjectPtr<UTextBlock> EntryColorRight;
	UPROPERTY() TObjectPtr<UTextBlock> EntryCreateStatus;
	UPROPERTY() TObjectPtr<UTextBlock> EntryDoCreateLink;
	UPROPERTY() TObjectPtr<UTextBlock> EntryCancelCreateLink;
	UPROPERTY() TArray<TObjectPtr<UImage>> EntryCardBack;
	UPROPERTY() TArray<TObjectPtr<UTextBlock>> EntryCardName;
	UPROPERTY() TArray<TObjectPtr<UTextBlock>> EntryCardInfo;
	UPROPERTY() TArray<TObjectPtr<UTextBlock>> EntryCardInfo2;
	UPROPERTY() TArray<TObjectPtr<UTextBlock>> EntryCardPlus;
	UPROPERTY() TObjectPtr<UTextBlock> EntryInfoTitle;
	UPROPERTY() TArray<TObjectPtr<UTextBlock>> EntryInfoValues;

	// Correio (MythRoMail.cpp)
public:
	void OpenMailWindow();
	void RefreshMail();
protected:
	void BuildMailWindows();
	bool HandleMailClick(const FVector2D& ScreenPos);
	/** Botao de minimizar da barra de titulo: a janela encolhe para a barra e a segunda vez volta ao tamanho. */
	struct FRoMinimize { TWeakObjectPtr<UCanvasPanel> Win; TWeakObjectPtr<UImage> Button; FVector2D FullSize = FVector2D::ZeroVector; bool bMin = false; };
	TArray<FRoMinimize> Minimizables;
	bool HandleMinimizeClick(const FVector2D& ScreenPos);
	UPROPERTY() TObjectPtr<UCanvasPanel> MailWindow;
	UPROPERTY() TObjectPtr<UCanvasPanel> MailWriteWindow;
	UPROPERTY() TObjectPtr<UTextBlock> MailHeader;
	UPROPERTY() TArray<TObjectPtr<UTextBlock>> MailRows;
	UPROPERTY() TObjectPtr<UTextBlock> MailFrom;
	UPROPERTY() TArray<TObjectPtr<UTextBlock>> MailBody;
	UPROPERTY() TObjectPtr<UTextBlock> MailTakeZeny;
	UPROPERTY() TObjectPtr<UTextBlock> MailTakeItems;
	UPROPERTY() TObjectPtr<UTextBlock> MailDelete;
	UPROPERTY() TObjectPtr<UImage> MailWrite;
	UPROPERTY() TObjectPtr<UImage> MailRefresh;
	UPROPERTY() TObjectPtr<UImage> MailPrev;
	UPROPERTY() TObjectPtr<UImage> MailNext;
	UPROPERTY() TArray<TObjectPtr<UImage>> MailRowHits;
	UPROPERTY() TArray<TObjectPtr<UImage>> MailRowIcons;
	UPROPERTY() TArray<TObjectPtr<UImage>> MailRowGifts;
	UPROPERTY() TArray<TObjectPtr<UTextBlock>> MailSenders;
	UPROPERTY() TObjectPtr<UTextBlock> MailClose;
	UPROPERTY() TObjectPtr<UTextBlock> MailReadClose;
	UPROPERTY() TObjectPtr<UCanvasPanel> MailReadWindow;
	int32 MailPage = 0;
	UPROPERTY() TObjectPtr<UEditableTextBox> MailToBox;
	UPROPERTY() TObjectPtr<UEditableTextBox> MailTitleBox;
	UPROPERTY() TObjectPtr<UEditableTextBox> MailTextBox;
	UPROPERTY() TObjectPtr<UTextBlock> MailSend;
	UPROPERTY() TObjectPtr<UTextBlock> MailCancel;

	// Navegacao (MythRoNavi.cpp)
public:
	void OpenNaviWindow(const FString& Query = FString());
	/** Teste e atalho: inicia a navegacao automatica ate o mapa escolhido na janela. */
	void NaviGoSelected() { StartNaviAuto(); }
	/** Teste: o mesmo que o botao Procurar (mostra a rota na lista da direita). */
	void NaviFindSelected() { if (!NaviSelected.IsEmpty()) { ShowNaviRoute(NaviSelected); bNaviShowRoute = true; RefreshNaviLists(); } }
	void SetCurrentMapId(const FString& MapId) { CurrentMapId = MapId; }
protected:
	void BuildNaviWindow();
	void SearchNavi(const FString& Query);
	/** Escolhe o mapa (painel direito: nome, imagem e monstros) e calcula a rota pelos portais a partir do mapa atual. */
	void ShowNaviRoute(const FString& Destination);
	/** Redesenha as duas listas (resultados a esquerda; monstros ou rota a direita) com a rolagem atual. */
	void RefreshNaviLists();
	bool HandleNaviClick(const FVector2D& ScreenPos);
	bool HandleNaviWheel(const FVector2D& ScreenPos, float Delta);
	UFUNCTION() void OnNaviCommitted(const FText& Text, ETextCommit::Type CommitMethod);
	UPROPERTY() TObjectPtr<UCanvasPanel> NaviWindow;
	UPROPERTY() TObjectPtr<UEditableTextBox> NaviBox;
	// Linha de busca: voltar, filtro (Todos / Mapas / Monstros) com a lista suspensa, lupa
	UPROPERTY() TObjectPtr<UImage> NaviBackBtn;
	UPROPERTY() TObjectPtr<UImage> NaviFilterBtn;
	UPROPERTY() TObjectPtr<UTextBlock> NaviFilterText;
	UPROPERTY() TObjectPtr<UImage> NaviFilterMenuBack;
	UPROPERTY() TArray<TObjectPtr<UTextBlock>> NaviFilterItems;
	UPROPERTY() TObjectPtr<UImage> NaviSearchBtn;
	// Esquerda: "Resultado[n]" e a lista
	UPROPERTY() TObjectPtr<UTextBlock> NaviCount;
	UPROPERTY() TArray<TObjectPtr<UTextBlock>> NaviRows;
	UPROPERTY() TArray<TObjectPtr<UImage>> NaviRowIcons;
	// Direita: nome do mapa, imagem, lista (o mapa e seus monstros, ou a rota depois de "Procurar") e os botoes
	UPROPERTY() TObjectPtr<UTextBlock> NaviTitle;
	UPROPERTY() TObjectPtr<UImage> NaviPreview;
	UPROPERTY() TArray<TObjectPtr<UTextBlock>> NaviListRows;
	UPROPERTY() TArray<TObjectPtr<UImage>> NaviListIcons;
	UPROPERTY() TObjectPtr<UImage> NaviListSel;
	UPROPERTY() TObjectPtr<UImage> NaviScrollUp;
	UPROPERTY() TObjectPtr<UImage> NaviScrollDown;
	UPROPERTY() TObjectPtr<UImage> NaviShareBtn;
	UPROPERTY() TObjectPtr<UImage> NaviFindBtn;
	UPROPERTY() TObjectPtr<UImage> NaviMoveBtn;
	int32 NaviFilter = 0; // 0 todos, 1 mapas, 2 monstros
	struct FNaviResult { uint8 Type = 0; FString MapId; FString Label; }; // Type 0 mapa, 1 monstro
	TArray<FNaviResult> NaviResults;
	int32 NaviResultScroll = 0;
	int32 NaviListScroll = 0;
	bool bNaviShowRoute = false;
	TArray<FString> NaviHistory;
	FString NaviSelected;
	struct FNaviStep { FString From; int32 X = 0, Y = 0; FString To; };
	TArray<FNaviStep> NaviSteps; // rota da selecao a partir do mapa atual
	UPROPERTY() TObjectPtr<UTextBlock> NaviAutoBtn; // texto do botao Mover ("Mover" / "Parar")
	bool bNaviAuto = false;
	FString NaviAutoDest;
	float NaviAutoTimer = 0.f;
	FIntPoint NaviLastTarget = FIntPoint(-100, -100);
	float NaviLastSend = 0.f;
	bool bNaviBackoff = false;
	int32 NaviProbe = 0;
	void StartNaviAuto();
	void TickNaviAuto(float DeltaTime);
	FString CurrentMapId;

	// Troca rapida (MythRoEquipSwitch.cpp)
public:
	void RefreshEquipSwitch();
protected:
	void BuildEquipSwitchWindow();
	bool HandleEquipSwitchClick(const FVector2D& ScreenPos);
	UPROPERTY() TObjectPtr<UCanvasPanel> EquipSwitchWindow;
	UPROPERTY() TArray<TObjectPtr<UTextBlock>> EsNames;
	UPROPERTY() TArray<TObjectPtr<UTextBlock>> EsRemove;
	UPROPERTY() TObjectPtr<UTextBlock> EsSwitchAll;

	// Conquistas (MythRoAchievements.cpp)
public:
	void OpenAchievementWindow();
	void RefreshAchievements();
protected:
	void BuildAchievementWindow();
	bool HandleAchievementClick(const FVector2D& ScreenPos);
	UPROPERTY() TObjectPtr<UCanvasPanel> AchievementWindow;
	UPROPERTY() TObjectPtr<UTextBlock> AchHeader;
	UPROPERTY() TObjectPtr<UTextBlock> AchPoints;
	UPROPERTY() TObjectPtr<UTextBlock> AchRank;
	UPROPERTY() TArray<TObjectPtr<UImage>> AchTabBoxes;
	UPROPERTY() TArray<TObjectPtr<UTextBlock>> AchTabTexts;
	UPROPERTY() TArray<TObjectPtr<UImage>> AchCards;
	UPROPERTY() TArray<TObjectPtr<UTextBlock>> AchNames;
	UPROPERTY() TArray<TObjectPtr<UTextBlock>> AchProgress;
	UPROPERTY() TArray<TObjectPtr<UTextBlock>> AchScores;
	UPROPERTY() TArray<TObjectPtr<UTextBlock>> AchClaim;
	UPROPERTY() TObjectPtr<UTextBlock> AchDetailTitle;
	UPROPERTY() TObjectPtr<UImage> AchStamp;
	UPROPERTY() TArray<TObjectPtr<UTextBlock>> AchDetailLines;
	UPROPERTY() TObjectPtr<UImage> AchReceive;
	UPROPERTY() TObjectPtr<UTextBlock> AchReceiveLabel;
	UPROPERTY() TArray<TObjectPtr<UTextBlock>> AchOverview;
	UPROPERTY() TArray<TObjectPtr<UImage>> AchOverviewBars;
	int32 AchTab = 0;
	int32 AchScroll = 0;
	int32 AchSelectedId = 0;
	bool HandleAchievementWheel(const FVector2D& ScreenPos, float Delta);

	// Banco (MythRoBank.cpp)
public:
	void OpenBankWindow();
	void RefreshBank();
protected:
	void BuildBankWindow();
	bool HandleBankClick(const FVector2D& ScreenPos);
	UPROPERTY() TObjectPtr<UCanvasPanel> BankWindow;
	UPROPERTY() TObjectPtr<UTextBlock> BankBalanceText;
	UPROPERTY() TObjectPtr<UTextBlock> BankZenyText;
	UPROPERTY() TObjectPtr<UEditableTextBox> BankAmountBox;
	UPROPERTY() TObjectPtr<UTextBlock> BankDepositLink;
	UPROPERTY() TObjectPtr<UTextBlock> BankWithdrawLink;
	UPROPERTY() TArray<TObjectPtr<UTextBlock>> BankQuick;

	// Atalhos de teclado (MythRoShortcuts.cpp)
	void BuildShortcutWindow();
	UPROPERTY() TObjectPtr<UCanvasPanel> ShortcutWindow;

	// Missoes (MythRoQuests.cpp)
public:
	void RefreshQuests();
protected:
	void BuildQuestWindow();
	bool HandleQuestClick(const FVector2D& ScreenPos);
	struct FRoButton
	{
		TWeakObjectPtr<UImage> Image;
		TWeakObjectPtr<UTexture2D> Normal;
		TWeakObjectPtr<UTexture2D> Hover;
		TWeakObjectPtr<UTexture2D> Pressed;
		int32 Shown = 0;
		// botao de texto de 3 partes: as imagens do meio e das pontas trocam juntas
		TWeakObjectPtr<UImage> Left;
		TWeakObjectPtr<UImage> Right;
		TWeakObjectPtr<UTextBlock> Label;
		FString Part;
	};
	TArray<FRoButton> RoButtons;
	/** Mostra ou esconde um botao de texto (AddTextButton) inteiro: rotulo, meio e pontas. */
	void SetTextButtonShown(UTextBlock* Label, bool bShow);
	UPROPERTY() TObjectPtr<UCanvasPanel> QuestWindow;
	UPROPERTY() TObjectPtr<UTextBlock> QuestHeader;
	UPROPERTY() TObjectPtr<UImage> QuestBack;
	UPROPERTY() TArray<TObjectPtr<UImage>> QuestCards;
	UPROPERTY() TArray<TObjectPtr<UImage>> QuestIcons;
	UPROPERTY() TArray<TObjectPtr<UTextBlock>> QuestTitles;
	UPROPERTY() TArray<TObjectPtr<UTextBlock>> QuestObjectives;
	UPROPERTY() TArray<TObjectPtr<UImage>> QuestChecks;
	UPROPERTY() TArray<TObjectPtr<UImage>> QuestEyes;
	UPROPERTY() TObjectPtr<UTextBlock> QuestEmpty;
	UPROPERTY() TObjectPtr<UImage> QuestViewBox;
	UPROPERTY() TObjectPtr<UTextBlock> QuestClose;
	UPROPERTY() TObjectPtr<UCanvasPanel> QuestDetailWindow;
	UPROPERTY() TArray<TObjectPtr<UTextBlock>> QuestDetail;
	int32 QuestTab = 0;
	int32 QuestScroll = 0;
	int32 QuestSelectedId = 0;
	bool HandleQuestWheel(const FVector2D& ScreenPos, float Delta);

	// Sala de chat (MythRoChatRoom.cpp)
public:
	void OpenChatRoomWindow();
	void RefreshChatRoom();
	void AskChatRoomPassword(int32 ChatId);
protected:
	void BuildChatRoomWindows();
	bool HandleChatRoomClick(const FVector2D& ScreenPos);
	static constexpr int32 RoomRowCount = 14;
	UPROPERTY() TObjectPtr<UCanvasPanel> ChatRoomCreateWindow;
	UPROPERTY() TObjectPtr<UCanvasPanel> ChatRoomWindow;
	UPROPERTY() TObjectPtr<UCanvasPanel> ChatRoomEnterWindow;
	UPROPERTY() TObjectPtr<UEditableTextBox> RoomTitleBox;
	UPROPERTY() TObjectPtr<UEditableTextBox> RoomPassBox;
	UPROPERTY() TObjectPtr<UEditableTextBox> RoomEnterPassBox;
	UPROPERTY() TObjectPtr<UTextBlock> RoomLimitLeft;
	UPROPERTY() TObjectPtr<UTextBlock> RoomLimitText;
	UPROPERTY() TObjectPtr<UTextBlock> RoomLimitRight;
	UPROPERTY() TObjectPtr<UTextBlock> RoomTypeLink;
	UPROPERTY() TObjectPtr<UTextBlock> RoomOkLink;
	UPROPERTY() TObjectPtr<UTextBlock> RoomCancelLink;
	UPROPERTY() TObjectPtr<UTextBlock> RoomTitleText;
	UPROPERTY() TArray<TObjectPtr<UTextBlock>> RoomMemberRows;
	UPROPERTY() TArray<TObjectPtr<UTextBlock>> RoomKickLinks;
	UPROPERTY() TObjectPtr<UTextBlock> RoomLeaveLink;
	UPROPERTY() TObjectPtr<UTextBlock> RoomEnterOkLink;
	UPROPERTY() TObjectPtr<UTextBlock> RoomEnterCancelLink;
	int32 RoomNewLimit = 10;
	bool RoomNewPublic = true;
	int32 PendingRoomId = 0;

	// Configuracoes: faixa de abas comum as tres janelas de opcoes (MythRoSettingsTabs.cpp)
	struct FSettingsTab { UCanvasPanel* Window = nullptr; UImage* Box = nullptr; UTextBlock* Label = nullptr; int32 Index = 0; };
	TArray<FSettingsTab> SettingsTabs;
	bool bMailBadge = false;
	void AddSettingsTabs(UCanvasPanel* Win);
	int32 ActiveSettingsTab() const;
	void UpdateSettingsTabs();
	bool HandleSettingsTab(const FVector2D& ScreenPos);
	// Amigos e Grupo dividem o cliente moderno numa janela so: o par de botoes de radio troca de uma para a outra
	void AddFriendPartySwitch(UCanvasPanel* Win, float H, bool bParty);
	bool HandleFriendPartySwitch(const FVector2D& ScreenPos);
	UPROPERTY() TObjectPtr<UImage> PartySwitchFriend;
	UPROPERTY() TObjectPtr<UImage> FriendsSwitchParty;
	// Amigos (MythRoFriends.cpp)
public:
	void RefreshFriends();
	void OnFriendRequestReceived(int32 AccountId, int32 CharId, const FString& Name);
	void AnswerFriendRequest(bool bAccept);
protected:
	void BuildFriendsWindow();
	bool HandleFriendsClick(const FVector2D& ScreenPos);
	static constexpr int32 FriendRowCount = 14;
	UPROPERTY() TObjectPtr<UCanvasPanel> FriendsWindow;
	UPROPERTY() TObjectPtr<UCanvasPanel> FriendAskWindow;
	UPROPERTY() TObjectPtr<UTextBlock> FriendsTitle;
	UPROPERTY() TArray<TObjectPtr<UTextBlock>> FriendNames;
	UPROPERTY() TArray<TObjectPtr<UTextBlock>> FriendStates;
	UPROPERTY() TArray<TObjectPtr<UTextBlock>> FriendDelete;
	UPROPERTY() TObjectPtr<UTextBlock> FriendAskText;
	UPROPERTY() TObjectPtr<UTextBlock> FriendAcceptLink;
	UPROPERTY() TObjectPtr<UTextBlock> FriendDeclineLink;
	int32 PendingFriendAid = 0;
	int32 PendingFriendCid = 0;

	// Troca entre jogadores e menu do botao direito sobre um jogador (MythRoTrade.cpp)
public:
	void OnTradeRequestReceived(const FString& FromName);
	void RefreshTrade();
	bool TryTradeAddFromInventory(int32 InventoryIndex, int32 Amount);
	void ShowPlayerMenu(int64 AccountId, const FString& Name, const FVector2D& ScreenPos);
protected:
	void BuildTradeWindows();
	bool HandleTradeClick(const FVector2D& ScreenPos);
	static constexpr int32 TradeRowCount = 10;
	UPROPERTY() TObjectPtr<UCanvasPanel> TradeWindow;
	UPROPERTY() TObjectPtr<UCanvasPanel> TradeAskWindow;
	UPROPERTY() TObjectPtr<UCanvasPanel> PlayerMenuWindow;
	UPROPERTY() TObjectPtr<UTextBlock> TradeMineTitle;
	UPROPERTY() TObjectPtr<UTextBlock> TradeTheirTitle;
	UPROPERTY() TArray<TObjectPtr<UTextBlock>> TradeMineRows;
	UPROPERTY() TArray<TObjectPtr<UTextBlock>> TradeTheirRows;
	UPROPERTY() TObjectPtr<UTextBlock> TradeHint;
	UPROPERTY() TObjectPtr<UTextBlock> TradeLockLink;
	UPROPERTY() TObjectPtr<UTextBlock> TradeCommitLink;
	UPROPERTY() TObjectPtr<UTextBlock> TradeCancelLink;
	UPROPERTY() TObjectPtr<UTextBlock> TradeAskText;
	UPROPERTY() TObjectPtr<UTextBlock> TradeAcceptLink;
	UPROPERTY() TObjectPtr<UTextBlock> TradeDeclineLink;
	UPROPERTY() TArray<TObjectPtr<UTextBlock>> PlayerMenuItems;
	int64 MenuTargetAid = 0;
	FString MenuTargetName;

	// Lista de emocoes (Alt+L) e de atalhos (Alt+M) (MythRoEmotionList.cpp)
protected:
	void BuildEmotionWindows();
	void RefreshEmotionList();
	bool HandleEmotionClick(const FVector2D& ScreenPos);
	static constexpr int32 EmotionCellCount = 30;
	UPROPERTY() TObjectPtr<UCanvasPanel> EmotionWindow;
	UPROPERTY() TObjectPtr<UCanvasPanel> MacroWindow;
	UPROPERTY() TArray<TObjectPtr<UImage>> EmotionIcons;
	UPROPERTY() TObjectPtr<UTextBlock> EmotionPrev;
	UPROPERTY() TObjectPtr<UTextBlock> EmotionNext;
	UPROPERTY() TObjectPtr<UTextBlock> EmotionPageText;
	int32 EmotionPage = 0;

	// Janela de opcoes ON/OFF (MythRoOptions.cpp): abas Efeito, Controle e Outros
public:
	void ShowOptionsWindow(int32 Tab);
	void ApplyOptionEffects();
protected:
	void BuildOptionsWindow();
	void RefreshOptions();
	bool HandleOptionsClick(const FVector2D& ScreenPos);
	bool HandleOptionsWheel(const FVector2D& ScreenPos, float Delta);
	static constexpr int32 OptRowCount = 12;
	UPROPERTY() TObjectPtr<UCanvasPanel> OptionsWindow;
	UPROPERTY() TArray<TObjectPtr<UTextBlock>> OptTabTexts;
	UPROPERTY() TObjectPtr<UTextBlock> OptTabGraphics;
	UPROPERTY() TArray<TObjectPtr<UTextBlock>> OptRowName;
	UPROPERTY() TArray<TObjectPtr<UTextBlock>> OptRowToggle;
	UPROPERTY() TArray<TObjectPtr<UTextBlock>> OptRowDesc;
	UPROPERTY() TObjectPtr<UTextBlock> OptClose;
	int32 OptTab = 0;
	int32 OptScroll = 0;

	// Grupo (MythRoParty.cpp)
public:
	/** O cursor esta sobre janela, atalhos ou dialogo: o clique e da interface, nunca do mundo. */
	bool IsPointerOverUi() const;
	void RefreshParty();
	void ShowPartyInvite(int32 PartyId, const FString& PartyName);
	void AnswerPartyInvite(bool bAccept);
protected:
	void BuildPartyWindow();
	bool HandlePartyClick(const FVector2D& ScreenPos);
	static constexpr int32 PartyRowCount = 12;
	UPROPERTY() TObjectPtr<UCanvasPanel> PartyWindow;
	UPROPERTY() TObjectPtr<UCanvasPanel> PartyInviteWindow;
	UPROPERTY() TObjectPtr<UTextBlock> PartyNameText;
	// Barra de ferramentas do Grupo (print 19, mesbtn_*): procura de grupo, busca, cadeado, criar; no grupo, convidar e sair
	UPROPERTY() TObjectPtr<UImage> PartyToolBooking;
	UPROPERTY() TObjectPtr<UImage> PartyToolSearch;
	UPROPERTY() TObjectPtr<UImage> PartyToolLock;
	UPROPERTY() TObjectPtr<UImage> PartyToolCreate;
	UPROPERTY() TObjectPtr<UImage> PartyToolInvite;
	UPROPERTY() TObjectPtr<UImage> PartyToolLeave;
	UPROPERTY() TObjectPtr<UTextBlock> PartyInviteText;
	UPROPERTY() TObjectPtr<UTextBlock> PartyAcceptLink;
	UPROPERTY() TObjectPtr<UTextBlock> PartyDeclineLink;
	UPROPERTY() TArray<TObjectPtr<UTextBlock>> PartyRowNames;
	UPROPERTY() TArray<TObjectPtr<UImage>> PartyCards;
	UPROPERTY() TArray<TObjectPtr<UImage>> PartyJobIcons;
	UPROPERTY() TArray<TObjectPtr<UImage>> PartyCrowns;
	UPROPERTY() TArray<TObjectPtr<UTextBlock>> PartyRowInfo;
	UPROPERTY() TArray<TObjectPtr<UImage>> PartyBarBg;
	UPROPERTY() TArray<TObjectPtr<UImage>> PartyBarFill;
	UPROPERTY() TArray<TObjectPtr<UTextBlock>> PartyExpelLinks;
	int32 PendingPartyId = 0;

	// Armazem da Kafra (MythRoStorage.cpp, docs/ref/15; pacotes conferidos no clif.cpp)
public:
	void OpenStorage();
	void RefreshStorage();
	void CloseStorage(bool bNotifyServer);
	bool IsStorageOpen() const;
	void StorageMove(bool bToStorage, int32 Row, int32 Amount);
protected:
	void BuildStorageWindow();
	bool HandleStorageClick(const FVector2D& ScreenPos);
	bool HandleStorageWheel(const FVector2D& ScreenPos, float Delta);
	static constexpr int32 StorageRowCount = 12;
	UPROPERTY() TObjectPtr<UCanvasPanel> StorageWindow;
	UPROPERTY() TArray<TObjectPtr<UImage>> StoreHit;
	UPROPERTY() TArray<TObjectPtr<UImage>> StoreIcons;
	UPROPERTY() TArray<TObjectPtr<UTextBlock>> StoreNames;
	UPROPERTY() TArray<TObjectPtr<UImage>> BagHit;
	UPROPERTY() TArray<TObjectPtr<UImage>> BagIcons;
	UPROPERTY() TArray<TObjectPtr<UTextBlock>> BagNames;
	UPROPERTY() TObjectPtr<UTextBlock> StorageCountText;
	UPROPERTY() TObjectPtr<UTextBlock> StorageCloseLink;
	int32 StoreScroll = 0;
	int32 BagScroll = 0;

	// Lista de escolha depois de skill/item (MythRoSelectList.cpp): Portal, Lupa, carta, flechas, forja, Autoconjurar, refino
public:
	void OpenSelectList(uint8 ListType, const TArray<int32>& Values, const TArray<FString>& Names);
	/** Teste: escolhe a linha Row (ou cancela com Row < 0). */
	void SelectListChoose(int32 Row);
protected:
	void BuildSelectListWindow();
	void RefreshSelectList();
	bool HandleSelectListClick(const FVector2D& ScreenPos, bool bDouble);
	bool HandleSelectListWheel(const FVector2D& ScreenPos, float Delta);
	UPROPERTY() TObjectPtr<UCanvasPanel> SelectWindow;
	UPROPERTY() TObjectPtr<UTextBlock> SelectTitle;
	UPROPERTY() TArray<TObjectPtr<UImage>> SelectRowBacks;
	UPROPERTY() TArray<TObjectPtr<UImage>> SelectIcons;
	UPROPERTY() TArray<TObjectPtr<UTextBlock>> SelectTexts;
	UPROPERTY() TObjectPtr<UImage> SelectOk;
	UPROPERTY() TObjectPtr<UImage> SelectCancel;
	uint8 SelectType = 0;
	TArray<int32> SelectValues;
	TArray<FString> SelectNames;
	int32 SelectScroll = 0;
	int32 SelectRow = 0;

	// Mapa-mundi (MythRoWorldMap.cpp): worldmap.jpg com os retangulos do mappostable.txt, mapa atual marcado, nome sob o mouse
public:
	void ToggleWorldMap();
	/** Nome do mapa em portugues (tabela da Navegacao do LATAM), ou o proprio id. */
	static FString NaviDisplayName(const FString& MapId);
protected:
	void BuildWorldMapWindow();
	void TickWorldMap();
	bool HandleWorldMapClick(const FVector2D& ScreenPos);
	UPROPERTY() TObjectPtr<UCanvasPanel> WorldMapWindow;
	UPROPERTY() TObjectPtr<UImage> WorldMapImage;
	UPROPERTY() TObjectPtr<UImage> WorldMapHere;
	UPROPERTY() TObjectPtr<UImage> WorldMapHover;
	UPROPERTY() TObjectPtr<UTextBlock> WorldMapLabel;
	UPROPERTY() TObjectPtr<UTextBlock> WorldMapTitle;
	FString WorldMapHoverId;

	// Loja de jogador (MythRoVending.cpp): montar a minha loja, a minha loja aberta e comprar na loja de outro
public:
	void OnVendingEvent(uint8 Event);
	/** Testes: monta e abre a loja com os itens do carrinho (preco igual para todos); compra a linha Row (Count unidades). */
	void VendTestOpen(const FString& Title, int32 Price);
	void VendTestBuy(int32 Row, int32 Count);
protected:
	void BuildVendingWindows();
	void RefreshVending();
	bool HandleVendingClick(const FVector2D& ScreenPos);
	static constexpr int32 VendRows = 10;
	UPROPERTY() TObjectPtr<UCanvasPanel> VendSetupWindow;
	UPROPERTY() TObjectPtr<UEditableTextBox> VendTitleBox;
	UPROPERTY() TObjectPtr<UEditableTextBox> VendPriceBox;
	UPROPERTY() TObjectPtr<UEditableTextBox> VendAmountBox;
	UPROPERTY() TArray<TObjectPtr<UImage>> VendCartHit;
	UPROPERTY() TArray<TObjectPtr<UTextBlock>> VendCartNames;
	UPROPERTY() TArray<TObjectPtr<UImage>> VendShopHit;
	UPROPERTY() TArray<TObjectPtr<UTextBlock>> VendShopNames;
	UPROPERTY() TObjectPtr<UTextBlock> VendSetupInfo;
	UPROPERTY() TObjectPtr<UTextBlock> VendOpenBtn;
	UPROPERTY() TObjectPtr<UTextBlock> VendCancelBtn;
	TArray<FIntVector> VendPlanned; // {indice no carrinho, quantidade, preco}
	UPROPERTY() TObjectPtr<UCanvasPanel> VendShopWindow;    // a minha loja aberta
	UPROPERTY() TArray<TObjectPtr<UTextBlock>> VendMyNames;
	UPROPERTY() TObjectPtr<UTextBlock> VendCloseShopBtn;
	UPROPERTY() TObjectPtr<UCanvasPanel> VendBuyWindow;     // a loja de outro
	UPROPERTY() TArray<TObjectPtr<UImage>> VendBuyHit;
	UPROPERTY() TArray<TObjectPtr<UTextBlock>> VendBuyNames;
	UPROPERTY() TObjectPtr<UTextBlock> VendBuyTotal;
	UPROPERTY() TObjectPtr<UTextBlock> VendBuyBtn;
	UPROPERTY() TObjectPtr<UTextBlock> VendBuyCancelBtn;
	TMap<int32, int32> VendBuyChosen; // indice na loja -> quantidade escolhida

	// Mascote (MythRoPet.cpp): janela Alt+J com retrato, nome, nivel, fome, lealdade, acessorio e os comandos
public:
	void OnPetEvent(uint8 Event);
protected:
	void TogglePetWindow();
	void BuildPetWindow();
	void RefreshPet();
	bool HandlePetClick(const FVector2D& ScreenPos);
	UPROPERTY() TObjectPtr<UCanvasPanel> PetWindow;
	UPROPERTY() TObjectPtr<UImage> PetPortrait;
	UPROPERTY() TObjectPtr<UEditableTextBox> PetNameBox;
	UPROPERTY() TObjectPtr<UTextBlock> PetRenameBtn;
	UPROPERTY() TArray<TObjectPtr<UTextBlock>> PetLines;   // nivel, fome, lealdade, acessorio
	UPROPERTY() TObjectPtr<UTextBlock> PetAutoFeed;
	UPROPERTY() TArray<TObjectPtr<UTextBlock>> PetCmdBtns;  // alimentar, brincar, voltar para o ovo, desequipar
	FString PetPortraitKey;
	FString PetShownName; // nome que a caixa mostra (o servidor confirma a troca mandando o ZC_PROPERTY_PET de novo)

	// Cla (MythRoGuild.cpp): janela Alt+G com as abas do cliente (Cla, Membros, Cargos, Habil., Expulsos, Avisos) e o convite
public:
	void OnGuildEvent(uint8 Event);
	void AnswerGuildInvite(bool bAccept);
	void SetGuildTab(int32 Tab);
	/** Teste: escolhe a linha Row da aba Membros. */
	void GuildTestSelect(int32 Row) { GuildSelected = Row; RefreshGuild(); }
protected:
	void ToggleGuildWindow();
	void BuildGuildWindow();
	void RefreshGuild();
	bool HandleGuildClick(const FVector2D& ScreenPos);
	bool HandleGuildWheel(const FVector2D& ScreenPos, float Delta);
	static constexpr int32 GuildRows = 11;
	static constexpr int32 GuildTabCount = 6;
	UPROPERTY() TObjectPtr<UCanvasPanel> GuildWindow;
	UPROPERTY() TArray<TObjectPtr<UImage>> GuildTabBg;
	UPROPERTY() TArray<TObjectPtr<UTextBlock>> GuildTabText;
	UPROPERTY() TArray<TObjectPtr<UTextBlock>> GuildInfoLines;   // aba Cla: coluna da esquerda e da direita
	UPROPERTY() TArray<TObjectPtr<UImage>> GuildAllyBoxes;    // caixas de aliancas e inimigos
	UPROPERTY() TObjectPtr<UTextBlock> GuildNoneText;              // "Voce nao tem cla."
	UPROPERTY() TObjectPtr<UTextBlock> GuildCreateBtn;
	UPROPERTY() TArray<TObjectPtr<UTextBlock>> GuildHeader;        // cabecalho da aba Membros
	UPROPERTY() TArray<TObjectPtr<UImage>> GuildRowBg;
	UPROPERTY() TArray<TObjectPtr<UImage>> GuildRowIcon;
	UPROPERTY() TArray<TObjectPtr<UTextBlock>> GuildRowName;
	UPROPERTY() TArray<TObjectPtr<UTextBlock>> GuildRowPos;
	UPROPERTY() TArray<TObjectPtr<UTextBlock>> GuildRowJob;
	UPROPERTY() TArray<TObjectPtr<UTextBlock>> GuildRowLevel;
	UPROPERTY() TArray<TObjectPtr<UTextBlock>> GuildRowExp;
	UPROPERTY() TObjectPtr<UTextBlock> GuildNoticeLabel1;
	UPROPERTY() TObjectPtr<UTextBlock> GuildNoticeLabel2;
	UPROPERTY() TObjectPtr<UEditableTextBox> GuildSubjectBox;
	UPROPERTY() TObjectPtr<UEditableTextBox> GuildNoticeBox;
	UPROPERTY() TObjectPtr<UTextBlock> GuildOkBtn;
	UPROPERTY() TObjectPtr<UTextBlock> GuildLeaveBtn;
	UPROPERTY() TObjectPtr<UTextBlock> GuildExpelBtn;
	UPROPERTY() TObjectPtr<UTextBlock> GuildDisbandBtn;
	UPROPERTY() TObjectPtr<UTextBlock> GuildFooterText;
	UPROPERTY() TObjectPtr<UCanvasPanel> GuildInviteWindow;
	UPROPERTY() TObjectPtr<UTextBlock> GuildInviteText;
	UPROPERTY() TObjectPtr<UTextBlock> GuildAcceptLink;
	UPROPERTY() TObjectPtr<UTextBlock> GuildDeclineLink;
	int32 GuildTab = 0;
	int32 GuildScroll = 0;
	int32 GuildSelected = -1;
	bool bGuildNoticeFilled = false; // as caixas do aviso so recebem o texto do servidor uma vez por abertura
	int32 GuildConfirm = 0;          // 1 sair, 2 expulsar: o segundo clique confirma
	double GuildConfirmUntil = 0.0;

	// Carrinho do Mercador (MythRoCart.cpp): mesma forma do armazem, com peso e quantidade do servidor (ZC_NOTIFY_CARTITEM_COUNTINFO)
public:
	void ToggleCartWindow();
	void RefreshCart();
	/** Teste: move o item da linha Row (bToCart: da mochila para o carrinho). */
	void CartMove(bool bToCart, int32 Row, int32 Amount);
protected:
	void BuildCartWindow();
	bool HandleCartClick(const FVector2D& ScreenPos);
	bool HandleCartWheel(const FVector2D& ScreenPos, float Delta);
	UPROPERTY() TObjectPtr<UCanvasPanel> CartWindow;
	UPROPERTY() TArray<TObjectPtr<UImage>> CartHit;
	UPROPERTY() TArray<TObjectPtr<UImage>> CartIcons;
	UPROPERTY() TArray<TObjectPtr<UTextBlock>> CartNames;
	UPROPERTY() TArray<TObjectPtr<UImage>> CartBagHit;
	UPROPERTY() TArray<TObjectPtr<UImage>> CartBagIcons;
	UPROPERTY() TArray<TObjectPtr<UTextBlock>> CartBagNames;
	UPROPERTY() TObjectPtr<UTextBlock> CartInfoText;
	UPROPERTY() TObjectPtr<UTextBlock> CartCloseLink;
	/** Botao do carrinho na coluna do meio do Equipamento (btn_items), so com carrinho. */
	UPROPERTY() TObjectPtr<UImage> EquipCartBtn;
	int32 CartScroll = 0;
	int32 CartBagScroll = 0;

	// Loja de NPC (MythRoShop.cpp, docs/ref/14): escolher comprar/vender, vitrine e cesta
public:
	void ShowShopDealType(int64 NpcID);
	void ShowShopItems(bool bSell, const TArray<FRagnarokShopItem>& Items);
	void ShopResult(bool bSell, int32 Result);
	void CloseShop(bool bNotifyServer);
	bool IsShopOpen() const;
	/** Acoes da janela (usadas pelos cliques e pelos comandos de teste). */
	void ShopChooseDeal(int32 Type);
	void ShopAddToBasket(int32 Row, int32 Qty);
	void ShopConfirm();
protected:
	void BuildShopWindows();
	bool HandleShopClick(const FVector2D& ScreenPos);
	bool HandleShopWheel(const FVector2D& ScreenPos, float Delta);
	void RefreshShop();
	static constexpr int32 ShopRowCount = 12;
	struct FShopRow { int32 ItemID = 0; int32 Index = 0; int32 Price = 0; int32 Discount = 0; int32 MaxAmount = 30000; };
	TArray<FShopRow> ShopRows;
	TMap<int32, int32> ShopBasket; // linha da vitrine -> quantidade
	bool bShopSell = false;
	int64 ShopNpcID = 0;
	int32 ShopScroll = 0;
	UPROPERTY() TObjectPtr<UCanvasPanel> DealWindow;
	UPROPERTY() TObjectPtr<UTextBlock> DealBuyLink;
	UPROPERTY() TObjectPtr<UTextBlock> DealSellLink;
	UPROPERTY() TObjectPtr<UTextBlock> DealCancelLink;
	UPROPERTY() TObjectPtr<UCanvasPanel> ShopWindow;
	UPROPERTY() TArray<TObjectPtr<UImage>> ShopRowHit;
	UPROPERTY() TArray<TObjectPtr<UImage>> ShopRowIcons;
	UPROPERTY() TArray<TObjectPtr<UTextBlock>> ShopRowNames;
	UPROPERTY() TArray<TObjectPtr<UTextBlock>> ShopRowPrices;
	UPROPERTY() TArray<TObjectPtr<UTextBlock>> BasketRowTexts;
	UPROPERTY() TObjectPtr<UTextBlock> ShopHeaderText;
	UPROPERTY() TObjectPtr<UTextBlock> ShopTotalText;
	UPROPERTY() TObjectPtr<UTextBlock> ShopHintText;
	UPROPERTY() TObjectPtr<UTextBlock> ShopConfirmLink;
	UPROPERTY() TObjectPtr<UTextBlock> ShopCancelLink;

	/** Janela de morte (docs/ref: Escape em modo morte): voltar ao ponto de salvamento ou aguardar socorro. */
public:
	void ShowDeathWindow();
	void HideDeathWindow();
protected:
	UPROPERTY() TObjectPtr<UCanvasPanel> DeathWindow;
	UPROPERTY() TObjectPtr<UTextBlock> DeathSaveLink;
	UPROPERTY() TObjectPtr<UTextBlock> DeathWaitLink;
	FMythGfxOptions Gfx;
	bool bGfxDirty = false;
	UPROPERTY() TObjectPtr<UCanvasPanel> GameMenuWindow;
	UPROPERTY() TArray<TObjectPtr<UTextBlock>> GameMenuItems;
	UPROPERTY() TObjectPtr<UCanvasPanel> GraphicsWindow;
	UPROPERTY() TObjectPtr<UTextBlock> GfxTabOptions;
	UPROPERTY() TObjectPtr<UTextBlock> SndTabOptions;
	UPROPERTY() TArray<TObjectPtr<UImage>> GfxLeft;
	UPROPERTY() TArray<TObjectPtr<UImage>> GfxRight;
	UPROPERTY() TArray<TObjectPtr<UTextBlock>> GfxValues;
	UPROPERTY() TObjectPtr<UImage> GfxApply;
	UPROPERTY() TObjectPtr<UTextBlock> GfxClose;
	UPROPERTY() TObjectPtr<UTextBlock> GfxHint;
	// Som e interface
	static constexpr int32 SndRowCount = 4;
	void RefreshSoundWindow();
	void StepSoundOption(int32 Row, int32 Delta);
	int32 SndMusic = 10, SndEffects = 10;
	float SndUIScale = 1.35f;
	UPROPERTY() TObjectPtr<UCanvasPanel> SoundWindow;
	UPROPERTY() TArray<TObjectPtr<UImage>> SndLeft;
	UPROPERTY() TArray<TObjectPtr<UImage>> SndRight;
	UPROPERTY() TArray<TObjectPtr<UTextBlock>> SndValues;
	UPROPERTY() TObjectPtr<UTextBlock> SndClose;
	UPROPERTY() TObjectPtr<UTextBlock> SndHint;
	UPROPERTY() TObjectPtr<UTextBlock> GfxTabOther;
	UPROPERTY() TObjectPtr<UTextBlock> SndTabOther;
	void RefreshWindows();
	void RefreshStatusWindow();
	void RefreshItemsWindow();
	void RefreshEquipWindow();
	void RefreshSkillsWindow();
	bool HandleWindowClick(const FVector2D& ScreenPos);
	static bool IsUnder(const UWidget* Widget, const FVector2D& ScreenPos);
};

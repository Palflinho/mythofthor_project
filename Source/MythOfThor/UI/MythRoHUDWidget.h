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
	Friends   // Alt+H: amigos
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
	static constexpr float BasicFullH = 368.f;
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
	TArray<int32> EquipCellIndex;

	// Habilidades: linhas com icone, nome e nivel
	UPROPERTY() TArray<TObjectPtr<UImage>> SkillIcons;
	UPROPERTY() TArray<TObjectPtr<UTextBlock>> SkillNames;
	UPROPERTY() TArray<TObjectPtr<UTextBlock>> SkillLevels;
	UPROPERTY() TObjectPtr<UTextBlock> SkillPointsText;
	// Arvore de habilidades (skilltreeview.lub do cliente): celulas da grade 7 colunas, abas por etapa da classe e
	// as linhas que ligam cada skill ao pre-requisito
	UPROPERTY() TArray<TObjectPtr<UImage>> SkillCellFrames;
	UPROPERTY() TArray<TObjectPtr<UImage>> SkillTabBacks;
	UPROPERTY() TArray<TObjectPtr<UTextBlock>> SkillTabTexts;
	UPROPERTY() TArray<TObjectPtr<UImage>> SkillLinePool;
	UPROPERTY() TArray<TObjectPtr<UWidget>> SkillBottomBar;
	void ResizeSkillTree(int32 Rows);
	TArray<int32> SkillTabJobs;
	int32 SkillTab = -1; // -1 = a aba da classe atual

	float WindowRefreshTimer = 0.f;

	// Barra de atalhos F1..F9 e arrastar skills/itens para ela (MythRoHotbar.cpp)
	UPROPERTY() TObjectPtr<UCanvasPanel> HotbarWindow;
	UPROPERTY() TArray<TObjectPtr<UImage>> HotbarSlots;
	UPROPERTY() TArray<TObjectPtr<UImage>> HotbarIcons;
	UPROPERTY() TArray<TObjectPtr<UTextBlock>> HotbarCounts;
	UPROPERTY() TObjectPtr<UImage> DragIcon;
	TArray<int32> SkillRowID;
	TArray<int32> SkillRowLevel;
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
	void BuildSkillsWindow();
	void BuildItemInfoWindow();
	/** Janela de descricao do item (botao direito no inventario/equipamento), como no cliente original. */
	void ShowItemInfo(const struct FRagnarokInventoryItem& Item);
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
	UPROPERTY() TObjectPtr<UTextBlock> PartyLeaveLink;
	UPROPERTY() TObjectPtr<UTextBlock> PartyInviteText;
	UPROPERTY() TObjectPtr<UTextBlock> PartyAcceptLink;
	UPROPERTY() TObjectPtr<UTextBlock> PartyDeclineLink;
	UPROPERTY() TArray<TObjectPtr<UTextBlock>> PartyRowNames;
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

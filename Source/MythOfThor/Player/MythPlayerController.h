#pragma once

#include "CoreMinimal.h"
#include "UI/MythOptions.h"
#include "GameFramework/PlayerController.h"
#include "InputActionValue.h"
#include "UI/MythRoHUDWidget.h"
#include "MythPlayerController.generated.h"

class UInputMappingContext;
class UInputAction;
class AMythPlayerCharacter;
class UMythRoHUDWidget;
class AMythMonster;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnMythAttackPerformed, AMythMonster*, Target);

/**
 * Controles hibridos: Point & Click (mouse) e WASD (teclado) ao mesmo tempo, sem tela de opcoes.
 * Mapeamentos do Enhanced Input sao montados em C++ (sem assets .uasset).
 *
 * - Botao esquerdo: anda ate o ponto clicado (segurar continua seguindo o cursor).
 * - WASD: anda relativo ao yaw da camera; cancela o movimento por clique.
 * - Camera (como o cliente original): botao direito + arrastar gira, Shift + botao direito + arrastar inclina,
 *   scroll da zoom, duplo clique direito reseta; Q/E tambem giram. C trava/destrava no angulo padrao.
 *
 * Todo destino e enviado ao rAthena como CZ_REQUEST_MOVE (celulas) via URagnarokNetworkSubsystem.
 */
UCLASS()
class MYTHOFTHOR_API AMythPlayerController : public APlayerController
{
	GENERATED_BODY()

public:
	/** Comandos de barra do cliente que mexem no mundo (/sit /where /doridori /bangbang /bingbing /noctrl /aura /sound /bgm); true se tratou. */
	bool RunSlashCommand(const FString& Command, const FString& Args);
	bool bContinuousAttack = true;   // /noctrl: ataque continuo (acao 7) ou golpe unico (0)
	bool bAutoDoridori = false;      // /doridori: a cabeca vira de um lado para o outro sozinha
	float DoridoriTimer = 0.f;

	AMythPlayerController();

	/** Tamanho de uma celula do grid do Ragnarok em unidades da Unreal. Ajustar conforme a escala dos mapas importados. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Myth|Network")
	float CellSizeUU = 50.f;

	/** Quantas celulas a frente o WASD projeta o destino enviado ao servidor. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Myth|Network")
	int32 WasdLookaheadCells = 4;

	/** Intervalo minimo entre pacotes de movimento do WASD. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Myth|Network")
	float WasdSendInterval = 0.2f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Myth|Camera")
	float MouseRotateSensitivity = 0.4f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Myth|Camera")
	float KeyRotateSpeed = 90.f; // graus/s para Q/E

	/** Intervalo maximo entre dois cliques direitos para resetar a camera. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Myth|Camera")
	float DoubleClickSeconds = 0.3f;

	/** Console: MythTestDamage -> mostra os 5 tipos de texto sobre o personagem. */
	UFUNCTION(Exec)
	void MythTestDamage();

	/** Console: MythTestHud -> mexe em HP/SP/EXP/peso para testar o HUD. */
	UFUNCTION(Exec)
	void MythTestHud();

	/** Alvo focado (clique esquerdo em um monstro). Nulo se nao houver. */
	UFUNCTION(BlueprintPure, Category = "Myth|Targeting")
	AMythMonster* GetFocusedTarget() const;

	UFUNCTION(BlueprintCallable, Category = "Myth|Targeting")
	void SetFocusedTarget(AMythMonster* NewTarget);

	UFUNCTION(BlueprintCallable, Category = "Myth|Targeting")
	void ClearFocusedTarget();

	/** Disparado a cada golpe (apos PlayAttack); ponto de ligacao para o pacote de ataque e o dano. */
	UPROPERTY(BlueprintAssignable, Category = "Myth|Targeting")
	FOnMythAttackPerformed OnAttackPerformed;

	/** Console: MythSpawnMonster -> cria um AMythMonster de teste 400 cm a frente do personagem. */
	UFUNCTION(Exec)
	void MythSpawnMonster();

	/** Console: MythSetHour <0-24> -> muda a hora do dia (ex.: MythSetHour 22). */
	UFUNCTION(Exec)
	void MythSetHour(float Hour);

	/** Console: MythDayLength <segundos> -> duracao de um dia de jogo (ex.: MythDayLength 60). */
	UFUNCTION(Exec)
	void MythDayLength(float Seconds);

	/**
	 * Console (testes automaticos): MythShot 20 21 noite -> aos 20 s muda para as 21h e, 2 s depois, salva
	 * Saved/Screenshots/.../noite.png com a HUD. Hora < 0 mantem o horario.
	 */
	UFUNCTION(Exec)
	void MythShot(float DelaySeconds, float Hour, const FString& Name);

	/** Console: MythWeather Rain | Snow | Fog | Clear -> muda o clima (transicao suave). */
	UFUNCTION(Exec)
	void MythWeather(const FString& Type);

	/** Console: MythDelayed 24.5 MythTestDamage -> roda um comando de console depois de N segundos (testes automaticos). */
	UFUNCTION(Exec)
	void MythDelayed(float DelaySeconds, const FString& Command);

	/** Console (depuracao offline): MythGoto 120 200 -> leva o personagem a celula. Com servidor, o movimento vem do rAthena. */
	UFUNCTION(Exec)
	void MythGoto(int32 X, int32 Y);

	/** Console: MythWindow Status | Items | Equip | Skills | Basic | Minimap -> abre/fecha a janela (testes automaticos). */
	UFUNCTION(Exec)
	void MythWindow(const FString& Name);

	/** Console: MythLogin [usuario senha] -> conecta no rAthena (sem argumentos usa [Myth] NetUser/NetPassword). */
	UFUNCTION(Exec)
	void MythLogin(const FString& User = TEXT(""), const FString& Password = TEXT(""));

	/** Leva o personagem a celula (ou a andavel mais proxima). Usado pela entrada no mapa do servidor e pelo MythGoto. */
	bool TeleportToCell(int32 X, int32 Y);

	/** Console: MythSay @spawn poring 3 -> fala no chat do servidor (comandos @ inclusos). Com MythDelayed use + no lugar dos espacos. */
	UFUNCTION(Exec)
	void MythSay(const FString& Text);

	/** Console: MythAttackNearest -> ataca o monstro do servidor mais proximo (testes automaticos). */
	UFUNCTION(Exec)
	void MythAttackNearest();

	/** Teste: MythChat <texto> -> digita o texto no chat (comandos de barra, grupo e cochicho). */
	UFUNCTION(Exec)
	void MythChat(const FString& Text);

	/** Teste do armazem (abra com @storage): MythStore 1 <linha> <qtd> guarda; MythStore 0 <linha> <qtd> pega. */
	UFUNCTION(Exec)
	void MythStore(int32 ToStorage, int32 Row, int32 Qty);

	/** Teste do grupo: MythPartyAnswer 1 aceita o convite recebido, 0 recusa. */
	UFUNCTION(Exec)
	void MythPartyAnswer(int32 Accept);

	/** Teste: abre a janela de opcoes na aba 0 Efeito, 1 Controle, 2 Outros. */
	UFUNCTION(Exec)
	void MythOptions(int32 Tab);
	UFUNCTION(Exec) void MythFriendReply(int32 Accept);
	UFUNCTION(Exec) void MythEquip(int32 ItemID);
	/** Testes da loja de jogador: abre a minha loja (itens do carrinho, mesmo preco), abre a primeira loja vista, compra a linha Row. */
	UFUNCTION(Exec) void MythVendOpen(const FString& Title, int32 Price) { if (HudWidget) { HudWidget->VendTestOpen(Title.Replace(TEXT("+"), TEXT(" ")), Price); } }
	UFUNCTION(Exec) void MythVendBrowse();
	/** Teste: pega o premio da conquista (como o botao [Receber]). */
	UFUNCTION(Exec) void MythAchReward(int32 AchievementId) { if (URagnarokNetworkSubsystem* N = GetGameInstance()->GetSubsystem<URagnarokNetworkSubsystem>()) { N->SendAchievementReward(AchievementId); } }
	UFUNCTION(Exec) void MythEquipTab(int32 Tab) { if (HudWidget) { HudWidget->SetEquipTab(Tab); } }
	/** Teste: usa o titulo (0 tira). */
	UFUNCTION(Exec) void MythTitle(int32 TitleId) { if (URagnarokNetworkSubsystem* N = GetGameInstance()->GetSubsystem<URagnarokNetworkSubsystem>()) { N->SendChangeTitle(TitleId); } }
	UFUNCTION(Exec) void MythVendBuy(int32 Row, int32 Count) { if (HudWidget) { HudWidget->VendTestBuy(Row, Count); } }
	/** Testes do cla: aba (0 Cla, 1 Membros, 5 Avisos), responder convite, convidar pelo nome, escolher linha de Membros. */
	UFUNCTION(Exec) void MythGuildTab(int32 Tab) { if (HudWidget) { HudWidget->SetGuildTab(Tab); } }
	UFUNCTION(Exec) void MythGuildAnswer(int32 Accept) { if (HudWidget) { HudWidget->AnswerGuildInvite(Accept != 0); } }
	UFUNCTION(Exec) void MythGuildInvite(const FString& Name) { if (URagnarokNetworkSubsystem* N = GetGameInstance()->GetSubsystem<URagnarokNetworkSubsystem>()) { N->SendGuildInvite(Name); } }
	UFUNCTION(Exec) void MythGuildSelect(int32 Row) { if (HudWidget) { HudWidget->GuildTestSelect(Row); } }
	UFUNCTION(Exec) void MythGuild() { ToggleHudWindow(EMythRoWindow::Guild); }
	/** Testes do mascote: abre a janela, comando (1 alimentar, 2 brincar, 3 voltar ao ovo, 4 desequipar), renomeia, captura o monstro mais perto. */
	UFUNCTION(Exec) void MythPet() { ToggleHudWindow(EMythRoWindow::Pet); }
	UFUNCTION(Exec) void MythPetCmd(int32 Command) { if (URagnarokNetworkSubsystem* N = GetGameInstance()->GetSubsystem<URagnarokNetworkSubsystem>()) { N->SendPetMenu(Command); } }
	UFUNCTION(Exec) void MythPetRename(const FString& Name) { if (URagnarokNetworkSubsystem* N = GetGameInstance()->GetSubsystem<URagnarokNetworkSubsystem>()) { N->SendPetRename(Name); } }
	UFUNCTION(Exec) void MythCaptureNearest();
	/** Alt+T: o homunculo aguarda (volta para o dono e para de atacar sozinho) ou volta a ajudar. */
	void OnHotkeyHomunStandby();
	/** Testes do homunculo: janela, comando do menu (0 info, 1 alimentar), ataca o monstro mais perto. */
	UFUNCTION(Exec) void MythHomun() { ToggleHudWindow(EMythRoWindow::Homun); }
	UFUNCTION(Exec) void MythHomunCmd(int32 Command) { if (URagnarokNetworkSubsystem* N = GetGameInstance()->GetSubsystem<URagnarokNetworkSubsystem>()) { N->SendHomunMenu(Command); } }
	UFUNCTION(Exec) void MythHomunAttackNearest();
	/** O servidor pediu o alvo da captura (ZC_START_CAPTURE): cursor de mira ate o clique. */
	void BeginCapture();
	UFUNCTION(Exec) void MythGuildLeave() { if (URagnarokNetworkSubsystem* N = GetGameInstance()->GetSubsystem<URagnarokNetworkSubsystem>()) { N->SendLeaveGuild(TEXT("teste")); } }
	UFUNCTION(Exec) void MythGuildExpel(const FString& Name) { if (URagnarokNetworkSubsystem* N = GetGameInstance()->GetSubsystem<URagnarokNetworkSubsystem>()) { for (const FRoGuildMember& M : N->GetGuildMembers()) { if (M.Name == Name) { N->SendGuildExpel(M.AccountId, M.CharId, TEXT("teste")); } } } }
	UFUNCTION(Exec) void MythGuildNotice(const FString& Subject, const FString& Text) { if (URagnarokNetworkSubsystem* N = GetGameInstance()->GetSubsystem<URagnarokNetworkSubsystem>()) { N->SendGuildNotice(Subject, Text); } }
	/** Teste: aba da janela de Habilidades (0 = 1a, 1 = 2a, 2 = 3a, 3 = Etc). */
	UFUNCTION(Exec) void MythSkillTab(int32 Tab) { if (HudWidget) { HudWidget->SetSkillTab(Tab); } }
	UFUNCTION(Exec) void MythSkillHover(int32 SkillId) { if (HudWidget) { HudWidget->SkillTestHover(SkillId); } }
	UFUNCTION(Exec) void MythTestShop(int32 Tab = 0, const FString& Query = TEXT("")) { if (HudWidget) { HudWidget->TestTestShop(Tab, Query.Replace(TEXT("+"), TEXT(" "))); } }
	UFUNCTION(Exec) void MythCooldown(int32 SkillId = 0, float Seconds = 8.f) { if (URagnarokNetworkSubsystem* N = GetGameInstance()->GetSubsystem<URagnarokNetworkSubsystem>()) { N->DebugSetSkillCooldown(SkillId, Seconds); } }
	UFUNCTION(Exec) void MythBooking() { if (HudWidget) { HudWidget->ToggleBookingWindow(); } }
	UFUNCTION(Exec) void MythBookingReg(int32 Level = 50) { if (URagnarokNetworkSubsystem* N = GetGameInstance()->GetSubsystem<URagnarokNetworkSubsystem>()) { N->SendPartyBookingRegister(Level, 0, TArray<int32>()); } }
	UFUNCTION(Exec) void MythBookingDel() { if (URagnarokNetworkSubsystem* N = GetGameInstance()->GetSubsystem<URagnarokNetworkSubsystem>()) { N->SendPartyBookingDelete(); } }
	UFUNCTION(Exec) void MythBookingSearch() { if (URagnarokNetworkSubsystem* N = GetGameInstance()->GetSubsystem<URagnarokNetworkSubsystem>()) { N->SendPartyBookingSearch(0, 0, -1, 0, 9); } }
	UFUNCTION(Exec) void MythOpt(const FString& Key, int32 On) { FMythOptions::Set(FName(*Key), On != 0); if (HudWidget) { HudWidget->ApplyOptionEffectsPublic(); } }
	UFUNCTION(Exec) void MythHotbarRows(int32 Rows) { if (HudWidget) { HudWidget->SetHotbarRows(Rows); } }
	UFUNCTION(Exec) void MythStatPending(int32 Index, int32 N = 1) { if (HudWidget) { HudWidget->TestStatPending(Index, N); } }
	UFUNCTION(Exec) void MythMainMenu(int32 Index = -1) { if (HudWidget) { HudWidget->TestMainMenu(Index); } }
	UFUNCTION(Exec) void MythSkillInfo(int32 SkillId) { if (HudWidget) { HudWidget->TestShowSkillInfo(SkillId); } }
	/** Teste: usa a skill (nivel Level) no monstro mais perto. */
	UFUNCTION(Exec) void MythSkillOnNearest(int32 SkillId, int32 Level);
	/** Teste: o falcao voa ate o monstro mais perto (como o Ataque Aereo). */
	UFUNCTION(Exec) void MythFalconAttack();
	/** Teste: cria o circulo de conjuracao (0 magico, 1 alvo travado) de Size celulas sob o personagem por 6 s. */
	UFUNCTION(Exec) void MythCastRingTest(int32 LockOn, float Size);
	/** Teste: entra no modo de mira da skill (como a tecla de atalho), para ver o cursor de area. */
	UFUNCTION(Exec) void MythBeginSkill(int32 SkillId, int32 Level) { BeginSkill(SkillId, Level); int32 W = 0, H = 0; GetViewportSize(W, H); SetMouseLocation(W * 0.55f, H * 0.62f); }
	/** Teste: usa a skill de chao na celula X,Y. */
	UFUNCTION(Exec) void MythSkillAt(int32 SkillId, int32 Level, int32 X, int32 Y) { if (URagnarokNetworkSubsystem* N = GetGameInstance()->GetSubsystem<URagnarokNetworkSubsystem>()) { N->UseSkill(SkillId, Level, 0, X, Y); } }
	/** Teste: usa a skill em si mesmo (ou no alvo de rede Target, se dado). */
	UFUNCTION(Exec) void MythUseSkill(int32 SkillId, int32 Level, int32 Target = 0) { if (URagnarokNetworkSubsystem* N = GetGameInstance()->GetSubsystem<URagnarokNetworkSubsystem>()) { N->UseSkill(SkillId, Level, Target ? Target : N->GetSelfID()); } }
	/** Teste: escolhe a linha Row da lista de escolha aberta (Portal, Lupa, carta...); Row < 0 cancela. */
	UFUNCTION(Exec) void MythSelect(int32 Row) { if (HudWidget) { HudWidget->SelectListChoose(Row); } }
	/** Teste: usa o primeiro item com esse ID (carta: pede a lista de equipamentos, como o duplo clique). */
	UFUNCTION(Exec) void MythUseItem(int32 ItemID);
	/** Teste: o botao "tirar tudo" da janela de Equipamento (aba Geral). */
	UFUNCTION(Exec) void MythTakeOffAll();
	/** Teste: abre/fecha o carrinho; com Row >= 0 move o item dessa linha (ToCart 1: mochila -> carrinho). */
	UFUNCTION(Exec) void MythCart(int32 Row = -1, int32 ToCart = 1, int32 Amount = 1) { if (HudWidget) { if (Row < 0) { HudWidget->ToggleCartWindow(); } else { HudWidget->CartMove(ToCart != 0, Row, Amount); } } }
	UFUNCTION(Exec) void MythGameMenu() { if (HudWidget) { HudWidget->ToggleGameMenu(); } }
	UFUNCTION(Exec) void MythItemTab(int32 Tab) { if (HudWidget) { HudWidget->SetItemTab(Tab); } }
	UFUNCTION(Exec) void MythNavi(const FString& Query, int32 Go = 0) { if (HudWidget) { HudWidget->OpenNaviWindow(Query); if (Go == 1) { HudWidget->NaviGoSelected(); } else if (Go == 2) { HudWidget->NaviFindSelected(); } } }
	UFUNCTION(Exec) void MythRoom(int32 Create, const FString& Title);
	UFUNCTION(Exec) void MythEntryLogin(const FString& User, const FString& Pass);
	UFUNCTION(Exec) void MythEntryAction(int32 Action, const FString& Name);

	/** Teste da troca: MythTradeReply 1 aceita o pedido, MythTradeAdd <indice> <qtd>, MythTradeLock, MythTradeCommit. */
	UFUNCTION(Exec) void MythTradeReply(int32 Accept);
	UFUNCTION(Exec) void MythTradeAdd(int32 Index, int32 Amount);
	UFUNCTION(Exec) void MythTradeLock();
	UFUNCTION(Exec) void MythTradeCommit();

	/** Teste: MythDropFirst [quantidade] -> joga no chao o primeiro item nao equipado da mochila (CZ_ITEM_THROW). */
	UFUNCTION(Exec)
	void MythDropFirst(int32 Amount = 1);

	/** Teste da loja: MythShop 0 (comprar) ou 1 (vender); MythShopAdd <linha> <quantidade>; MythShopConfirm. */
	UFUNCTION(Exec)
	void MythShop(int32 Type);
	UFUNCTION(Exec)
	void MythShopAdd(int32 Row, int32 Qty);
	UFUNCTION(Exec)
	void MythShopConfirm();

	/** Teste: MythFloatCheck -> no log, a altura dos pes de cada unidade contra o chao do .gat embaixo dela. */
	UFUNCTION(Exec)
	void MythFloatCheck();

	/** /aura do cliente: MythAura 0 desligada, 1 so particulas, 2 completa. */
	UFUNCTION(Exec)
	void MythAura(int32 Mode);

	/** Teste: MythRestart 0 -> CZ_RESTART como o botao "Voltar ao ponto de salvamento" da janela de morte. */
	UFUNCTION(Exec)
	void MythRestart(int32 Type = 0);

	/** Teste: MythFireArrow -> a flecha do arco (efeito 2003) do personagem ate o monstro mais proximo. */
	UFUNCTION(Exec)
	void MythFireArrow(int32 Count = 1);

	/** Console: MythTalkNearest -> conversa com o NPC do servidor mais proximo; MythNpcNext = Enter no dialogo. */
	UFUNCTION(Exec)
	void MythTalkNearest();

	UFUNCTION(Exec)
	void MythNpcNext();

	/** Teste: escolhe a opcao Row (0 = primeira visivel) da caixa de opcoes do NPC. */
	UFUNCTION(Exec) void MythNpcChoose(int32 Row) { if (HudWidget) { HudWidget->NpcMenuChoose(Row); } }

	/** Teste: escreve na caixa de entrada do NPC (input) e confirma. Use + no lugar de espaco com MythDelayed. */
	UFUNCTION(Exec) void MythNpcInput(const FString& Text) { if (HudWidget) { HudWidget->NpcInputSubmitText(Text.Replace(TEXT("+"), TEXT(" "))); } }

	/** Teste: MythCharSelect -> o item "Selecao de personagem" do menu Esc. */
	UFUNCTION(Exec)
	void MythCharSelect() { if (HudWidget) { HudWidget->ReturnToCharacterSelect(); } }

	/** Teste: MythSwitch 0 poe o primeiro equipamento solto na troca rapida; MythSwitch 1 troca tudo. */
	UFUNCTION(Exec)
	void MythSwitch(int32 Op);

	/** Teste: MythMail 0 Nome prepara a carta; MythMail 1 Nome Titulo Texto (+ no lugar de espacos) envia. */
	UFUNCTION(Exec)
	void MythMail(int32 Op, const FString& To, const FString& Title = TEXT(""), const FString& Body = TEXT(""));

	/** Teste: MythBank 1 500 deposita, MythBank 2 200 saca. */
	UFUNCTION(Exec)
	void MythBank(int32 Op, int32 Amount);

	/** Console: MythWalk 133 183 -> anda ate a celula como um clique no chao (CZ_REQUEST_MOVE ao servidor). */
	UFUNCTION(Exec)
	void MythWalk(int32 X, int32 Y);

	/** Console: MythPickNearest -> anda ate o item no chao mais proximo e o pega (testes automaticos). */
	UFUNCTION(Exec)
	void MythPickNearest();

	/** Console (testes): MythHotkey 0 5 1 -> grava a skill 5 nivel 1 no F1; MythPressF 1 -> aperta F1;
	 *  MythTargetNearest -> com a mira de skill ativa, escolhe o monstro mais proximo. */
	UFUNCTION(Exec)
	void MythHotkey(int32 Index, int32 SkillID, int32 Level);

	UFUNCTION(Exec)
	void MythPressF(int32 Number);

	UFUNCTION(Exec)
	void MythTargetNearest();

	/** Teste: MythSit -> como apertar Insert (senta/levanta pelo servidor). */
	UFUNCTION(Exec)
	void MythSit() { OnHotkeySit(); }

	/** Teste: MythTurn 160 185 -> como Shift+clique na celula: vira sem andar (CZ_CHANGE_DIRECTION). */
	UFUNCTION(Exec)
	void MythTurn(int32 X, int32 Y);

	/** Teste: MythHead 1 -> vira a cabeca como a seta (0 frente, 1 direita, 2 esquerda); doridori. */
	UFUNCTION(Exec)
	void MythHead(int32 Turn) { SetHeadTurn(static_cast<uint8>(FMath::Clamp(Turn, 0, 2))); }

	/** Teste: MythPointAt 160 185 -> poe o mouse sobre a celula e registra no log o cursor do RO escolhido. */
	UFUNCTION(Exec)
	void MythPointAt(int32 X, int32 Y);

	/** Direcao do WASD conectado (o servidor anda; aqui so a intencao). */
	FVector WasdDirection = FVector::ZeroVector;

	/** Verdadeiro so durante o clique inicial (forca o pedido de movimento). */
	bool bForceNextMove = false;
	/** O clique atual foi usado para escolher o alvo de uma skill: segurar o botao nao vira ataque nem andada. */
	bool bSkillClickHeld = false;
	/** Fim da conjuracao propria (tempo do mundo); 0 se nao esta conjurando. */
	double CastUntil = 0.0;

	/** Teste: toca data/texture/effect/<File>.str no personagem (sem servidor). */
	UFUNCTION(Exec)
	void MythPlayStr(const FString& File);

	/** Teste: toca o efeito da skill (estagio 0 efeito, 1 impacto) no personagem, como se o servidor mandasse. */
	UFUNCTION(Exec)
	void MythPlaySkill(int32 SkillID, int32 Stage = 0);

	/** Teste: mostra o icone do estado EfstID por Seconds (0 = sem duracao, -1 = remove), sem servidor. */
	UFUNCTION(Exec)
	void MythBuff(int32 EfstID, float Seconds);

	/** Teste: muda o visual do personagem como um ZC_SPRITE_CHANGE (Look do rAthena: 0 classe, 2 arma, 3/4/5 acessorio, 8 escudo). */
	UFUNCTION(Exec)
	void MythLook(int32 Look, int32 Value);

	UMythRoHUDWidget* GetHud() const { return HudWidget; }

	/**
	 * Usa uma skill como no cliente original: em si (inf 0x04) vai direto; no inimigo (0x01), suporte (0x10) ou chao (0x02)
	 * entra no modo de mira e o proximo clique escolhe o alvo. O servidor valida SP, alcance e conjuracao.
	 */
	void BeginSkill(int32 SkillID, int32 Level);

	/** F1..F9: usa o item ou a skill do atalho (guardado no servidor). */
	void UseHotkey(int32 Index);

	bool IsTargetingSkill() const { return PendingSkillID != 0; }
	/** Circulo magico de selecao de area que acompanha o cursor (cria, move e remove sozinho). */
	void UpdateSkillSelectRing();
	TWeakObjectPtr<class AMythCastRing> SelectRing;
	int32 GetPendingSkillID() const { return PendingSkillID; }
	int32 GetPendingSkillLevel() const { return PendingSkillLevel; }
	int32 GetPendingSkillInf() const { return PendingSkillInf; }
	void CancelSkillTargeting();

	/** Console: MythQuitAfter 40 -> fecha o jogo apos 40 s (usado com -ExecCmds nos testes automaticos). */
	UFUNCTION(Exec)
	void MythQuitAfter(float Seconds);

	/** Console: MythHurtMonster <dano> -> da dano ao alvo focado (ou ao monstro mais proximo). */
	UFUNCTION(Exec)
	void MythHurtMonster(int32 Damage);

	/** Console: MythKillTarget -> mata o alvo focado. */
	UFUNCTION(Exec)
	void MythKillTarget();

	/** Converte posicao do mundo em celula do grid do servidor. */
	UFUNCTION(BlueprintPure, Category = "Myth|Network")
	FIntPoint WorldToCell(const FVector& WorldLocation) const;
	/**
	 * Andar por rota (MythPlayerRoute.cpp): destino alem do que o servidor aceita num pedido (max_walk_path 17; 14 com obstaculo
	 * no meio) ou atras de obstaculo. O cliente tira o caminho inteiro no .gat (A*) e manda trechos de 12 celulas, sempre a
	 * frente de onde o personagem esta, entao ele anda sem parar e contorna paredes. Falso se o pedido direto ja basta.
	 */
	bool StartRoute(const FIntPoint& Dest);
	void CancelRoute();
	/** O servidor avisou que voce comecou a conjurar (ZC_USESKILL_ACK, CastMs): ate la, clicar num monstro nao ataca. */
	void NoteSelfCast(float Seconds) { CastUntil = GetWorld()->GetTimeSeconds() + Seconds; }
	bool IsSelfCasting() const { return GetWorld() && GetWorld()->GetTimeSeconds() < CastUntil; }
	bool IsRouting() const { return RouteCells.Num() > 0; }
	FIntPoint GetRouteDest() const { return RouteCells.Num() > 0 ? RouteCells.Last() : FIntPoint(-1, -1); }
	/** Celulas da rota que ainda faltam (para desenhar o caminho no minimapa). */
	void GetRouteRemaining(TArray<FIntPoint>& Out) const;

protected:
	virtual void BeginPlay() override;
	virtual void SetPawn(APawn* InPawn) override;
	virtual void SetupInputComponent() override;
	virtual void PlayerTick(float DeltaTime) override;

private:
	TWeakObjectPtr<AMythMonster> FocusedTarget;
	double LastAttackTime = -1000.0;
	float ChaseRepathTimer = 0.f;

	UPROPERTY() TObjectPtr<UMythRoHUDWidget> HudWidget;

	UPROPERTY() TObjectPtr<UInputMappingContext> MappingContext;
	UPROPERTY() TObjectPtr<UInputAction> MoveAction;
	UPROPERTY() TObjectPtr<UInputAction> ClickAction;
	UPROPERTY() TObjectPtr<UInputAction> RotateMouseAction;
	UPROPERTY() TObjectPtr<UInputAction> PitchMouseAction;
	UPROPERTY() TObjectPtr<UInputAction> RotateKeyAction;
	UPROPERTY() TObjectPtr<UInputAction> ZoomAction;
	UPROPERTY() TObjectPtr<UInputAction> ToggleCameraAction;

	// Destino direto usado quando nao ha NavMesh no mapa.
	bool bHasDirectTarget = false;
	FVector DirectTarget = FVector::ZeroVector;

	FIntPoint LastSentCell = FIntPoint(-1, -1);
	float TimeSinceWasdSend = 0.f;
	bool bWasdActive = false;

	// Ritmo dos pedidos de movimento como o cliente original (docs/ref/01 3.5): no maximo um a cada 75 ms, e
	// espera a confirmacao do servidor (ZC_NOTIFY_PLAYERMOVE) por ate 1 s antes do proximo. O ultimo destino
	// barrado fica guardado e sai assim que puder.
	double LastMoveSendTime = -1000.0;
	bool bAwaitingMoveAck = false;
	bool bHasPendingMove = false;
	FIntPoint PendingMoveCell = FIntPoint(-1, -1);
	void FlushPendingMove();
	UFUNCTION() void HandleMoveConfirmed(int32 TargetX, int32 TargetY);

	// Ataque fora de alcance (ZC_ATTACK_FAILURE_FOR_DISTANCE): anda ate o alvo e repete o pedido de ataque
	int64 ChaseTargetID = 0;
	int32 ChaseRange = 1;
	UFUNCTION() void HandleAttackOutOfRange(int64 TargetID, int32 TargetX, int32 TargetY, int32 Range);
	void UpdateNetChase(float DeltaTime);
	void SendChaseMove(const FIntPoint& TargetCell);

	// Cursores do cliente (data/sprite/cursors.act -> tools/gen_cursors.py): escolhidos a cada quadro pelo que esta
	// sob o mouse, na prioridade do RO (docs/ref/04 3.1)
	void SetupRoCursors();
	/** Setas <- e -> parado ou sentado: vira a cabeca (doridori) e avisa o servidor. */
	void UpdateHeadTurn();
	void SetHeadTurn(uint8 HeadDir);
	/** Vira o personagem para o ponto sem andar e avisa o servidor (Shift+clique). */
	void TurnToward(const FVector& WorldLocation);
	void UpdateRoCursor();
	bool IsCursorOverUi() const;
	/** Celula (ou vizinha) de um NPC de portal (45/139) */
	bool IsWarpCell(const FIntPoint& Cell) const;
	double LastPickupTryTime = -1000.0;
	int32 PickupTries = 0;

	void BuildInputAssets();

	void OnClickStarted(const FInputActionValue& Value);
	void OnClickTriggered(const FInputActionValue& Value);
	void OnMoveStarted(const FInputActionValue& Value);
	void OnMove(const FInputActionValue& Value);
	void OnMoveCompleted(const FInputActionValue& Value);
	void OnRotateMouse(const FInputActionValue& Value);
	void OnPitchMouse(const FInputActionValue& Value);
	bool IsShiftDown() const;
	bool IsAltDown() const;
	void ToggleHudWindow(EMythRoWindow Window);
	void OnHotkeyStatus();
	void OnHotkeyParty();
	void OnHotkeyGuild();
	void OnHotkeyPet();
	void OnHotkeyEmotions();
	void OnHotkeyMacros();
	void OnHotkeyFriends();
	void OnHotkeyChatRoom();
	void OnHotkeyQuests();
	void OnHotkeyShortcuts();
	void OnHotkeyBank();
	void OnHotkeyMail();
	// Homunculo (MythPlayerHomun.cpp): IA basica no lugar dos scripts AI/*.lua do cliente original
	void UpdateHomunAI(float DeltaTime);
	/** Alt + botao direito: o homunculo anda ate a celula ou ataca o monstro clicado. */
	bool HandleHomunCommandClick(const FHitResult& Hit);
	float HomunAITimer = 0.f;
	double HomunManualUntil = 0.0;
	int64 HomunLastTarget = 0;
	bool bHomunStandby = false;
	void OnHotkeyNavi();
	void OnHotkeyWorldMap();
	void OnHotkeyEquipSwitch();
	void OnHotkeyAchievements();
	void OnHotkeyMimir();
	void OnHotkeyItems();
	void OnHotkeyEquip();
	void OnHotkeySkills();
	void OnHotkeyBasic();
	void OnHotkeyMinimap();
	void OnHotkeyEscape();
	void OnHotkeySit();
	void UpdateCameraReset();
	double LastRightClickTime = -1.0;
	void OnRotateKey(const FInputActionValue& Value);
	void OnZoom(const FInputActionValue& Value);
	void OnToggleCamera(const FInputActionValue& Value);

	void MoveToCursor();
	void MoveToLocation(const FVector& Destination, bool bForceSend = true);
	void UpdateRoute();
	TArray<FIntPoint> RouteCells;
	int32 RouteProgress = -1;  // indice da rota onde o personagem esta (-1: antes da primeira celula)
	int32 RouteSentIdx = -1;   // indice do ultimo trecho mandado ao servidor
	double RouteProgressTime = 0.0;
	int32 RouteRetries = 0;
	void UpdateCombat(float DeltaTime);
	void UpdatePickup();
	double RightClickHeldSince = 0.0;
	int32 PendingSkillID = 0;
	int32 PendingSkillLevel = 0;
	int32 PendingSkillInf = 0;
	bool HandleSkillTargetClick(const FHitResult& Hit);
	TWeakObjectPtr<class AMythGroundItem> PendingPickup;
	void SendCellToServer(const FVector& WorldLocation, bool bForce);
	AMythPlayerCharacter* GetMythCharacter() const;
};

#pragma once

#include "CoreMinimal.h"
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

	/** Console: MythTalkNearest -> conversa com o NPC do servidor mais proximo; MythNpcNext = Enter no dialogo. */
	UFUNCTION(Exec)
	void MythTalkNearest();

	UFUNCTION(Exec)
	void MythNpcNext();

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

	/** Direcao do WASD conectado (o servidor anda; aqui so a intencao). */
	FVector WasdDirection = FVector::ZeroVector;

	/** Verdadeiro so durante o clique inicial (forca o pedido de movimento). */
	bool bForceNextMove = false;

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
	void OnHotkeyItems();
	void OnHotkeyEquip();
	void OnHotkeySkills();
	void OnHotkeyBasic();
	void OnHotkeyMinimap();
	void OnHotkeyEscape();
	void UpdateCameraReset();
	double LastRightClickTime = -1.0;
	void OnRotateKey(const FInputActionValue& Value);
	void OnZoom(const FInputActionValue& Value);
	void OnToggleCamera(const FInputActionValue& Value);

	void MoveToCursor();
	void MoveToLocation(const FVector& Destination, bool bForceSend = true);
	void UpdateCombat(float DeltaTime);
	void UpdatePickup();
	int32 PendingSkillID = 0;
	int32 PendingSkillLevel = 0;
	int32 PendingSkillInf = 0;
	bool HandleSkillTargetClick(const FHitResult& Hit);
	TWeakObjectPtr<class AMythGroundItem> PendingPickup;
	void SendCellToServer(const FVector& WorldLocation, bool bForce);
	AMythPlayerCharacter* GetMythCharacter() const;
};

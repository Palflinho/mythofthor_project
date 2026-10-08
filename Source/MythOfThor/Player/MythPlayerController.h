#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerController.h"
#include "InputActionValue.h"
#include "MythPlayerController.generated.h"

class UInputMappingContext;
class UInputAction;
class AMythPlayerCharacter;
class UMythClassicHUDWidget;
class AMythMonster;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnMythAttackPerformed, AMythMonster*, Target);

/**
 * Controles hibridos: Point & Click (mouse) e WASD (teclado) ao mesmo tempo, sem tela de opcoes.
 * Mapeamentos do Enhanced Input sao montados em C++ (sem assets .uasset).
 *
 * - Botao esquerdo: anda ate o ponto clicado (segurar continua seguindo o cursor).
 * - WASD: anda relativo ao yaw da camera; cancela o movimento por clique.
 * - Camera: C alterna Fixa 45 / Livre 360; no modo livre, botao direito+mouse ou Q/E giram, scroll da zoom.
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

	UPROPERTY() TObjectPtr<UMythClassicHUDWidget> HudWidget;

	UPROPERTY() TObjectPtr<UInputMappingContext> MappingContext;
	UPROPERTY() TObjectPtr<UInputAction> MoveAction;
	UPROPERTY() TObjectPtr<UInputAction> ClickAction;
	UPROPERTY() TObjectPtr<UInputAction> RotateMouseAction;
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
	void OnRotateKey(const FInputActionValue& Value);
	void OnZoom(const FInputActionValue& Value);
	void OnToggleCamera(const FInputActionValue& Value);

	void MoveToCursor();
	void MoveToLocation(const FVector& Destination, bool bForceSend = true);
	void UpdateCombat(float DeltaTime);
	void SendCellToServer(const FVector& WorldLocation, bool bForce);
	AMythPlayerCharacter* GetMythCharacter() const;
};

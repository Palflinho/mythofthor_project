#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "AutoBattleComponent.generated.h"

// Delegates para a Interface de Usuário (HUD / UMG)
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnAutoBattleStateChanged, bool, bIsActive);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnAutoBattleTimeUpdated, float, RemainingSeconds);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnAutoBattleError, const FString&, Reason);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnInventoryWeightChanged, float, WeightPercent, bool, bOverweight);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnPlayerDiedInAutoBattle, float, ExpLostPercent, float, DownedSecondsRemaining);

/**
 * Componente de jogabilidade responsável por gerenciar o sistema de Auto-Battle,
 * o limite de 3 horas diárias de vigor e as regras de penalidade de morte (5% de EXP).
 */
UCLASS(ClassGroup=(Custom), meta=(BlueprintSpawnableComponent))
class MYTHOFTHOR_API UAutoBattleComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UAutoBattleComponent();

protected:
	virtual void BeginPlay() override;

public:
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

	// Constantes de Regras de Jogo
	static constexpr float MAX_DAILY_STAMINA_SECONDS = 3.0f * 3600.0f; // 3 Horas = 10.800s
	static constexpr float OVERWEIGHT_THRESHOLD_PERCENT = 90.0f;       // Trava em 90%
	static constexpr float DEATH_PENALTY_AUTOBATTLE_PERCENT = 5.0f;     // Perda de 5% de EXP
	static constexpr float DEATH_PENALTY_MANUAL_PERCENT = 1.0f;         // Perda de 1% de EXP
	static constexpr float DOWNED_WAIT_TIME_SECONDS = 120.0f;           // 2 minutos caído

	/**
	 * Tenta ativar ou desativar o modo Auto-Battle.
	 * @param bEnable True para ligar, False para desligar.
	 * @return True se a transição foi bem sucedida.
	 */
	UFUNCTION(BlueprintCallable, Category = "Ragnarok|AutoBattle")
	bool ToggleAutoBattle(bool bEnable);

	/**
	 * Notifica o componente sobre a morte do personagem.
	 * @return A porcentagem de EXP perdida (1% ou 5%).
	 */
	UFUNCTION(BlueprintCallable, Category = "Ragnarok|AutoBattle")
	float NotifyPlayerDeath();

	/**
	 * Atualiza a porcentagem de sobrepeso atual do inventário.
	 */
	UFUNCTION(BlueprintCallable, Category = "Ragnarok|AutoBattle")
	void UpdateInventoryWeight(float CurrentWeightPercent);

	// Getters
	UFUNCTION(BlueprintPure, Category = "Ragnarok|AutoBattle")
	bool IsAutoBattleActive() const { return bAutoBattleActive; }

	UFUNCTION(BlueprintPure, Category = "Ragnarok|AutoBattle")
	float GetRemainingStaminaSeconds() const { return RemainingStaminaSeconds; }

	UFUNCTION(BlueprintPure, Category = "Ragnarok|AutoBattle")
	bool IsPlayerDowned() const { return bIsDowned; }

	UFUNCTION(BlueprintPure, Category = "Ragnarok|AutoBattle")
	float GetDownedTimerRemaining() const { return DownedTimer; }

	UFUNCTION(BlueprintPure, Category = "Ragnarok|AutoBattle")
	float GetInventoryWeightPercent() const { return InventoryWeightPercent; }

	UFUNCTION(BlueprintPure, Category = "Ragnarok|AutoBattle")
	bool IsOverweight() const { return InventoryWeightPercent >= OVERWEIGHT_THRESHOLD_PERCENT; }

	// Eventos Dispatchers para Blueprint/HUD
	UPROPERTY(BlueprintAssignable, Category = "Ragnarok|AutoBattle|Events")
	FOnInventoryWeightChanged OnInventoryWeightChanged;

	UPROPERTY(BlueprintAssignable, Category = "Ragnarok|AutoBattle|Events")
	FOnAutoBattleStateChanged OnAutoBattleStateChanged;

	UPROPERTY(BlueprintAssignable, Category = "Ragnarok|AutoBattle|Events")
	FOnAutoBattleTimeUpdated OnAutoBattleTimeUpdated;

	UPROPERTY(BlueprintAssignable, Category = "Ragnarok|AutoBattle|Events")
	FOnAutoBattleError OnAutoBattleError;

	UPROPERTY(BlueprintAssignable, Category = "Ragnarok|AutoBattle|Events")
	FOnPlayerDiedInAutoBattle OnPlayerDiedInAutoBattle;

private:
	UPROPERTY(VisibleAnywhere, Category = "Ragnarok|AutoBattle")
	bool bAutoBattleActive;

	UPROPERTY(VisibleAnywhere, Category = "Ragnarok|AutoBattle")
	float RemainingStaminaSeconds;

	UPROPERTY(VisibleAnywhere, Category = "Ragnarok|AutoBattle")
	float InventoryWeightPercent;

	UPROPERTY(VisibleAnywhere, Category = "Ragnarok|AutoBattle")
	bool bIsDowned;

	UPROPERTY(VisibleAnywhere, Category = "Ragnarok|AutoBattle")
	float DownedTimer;

	void RespawnToSavePoint();
};

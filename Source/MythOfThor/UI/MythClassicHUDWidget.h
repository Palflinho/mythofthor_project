#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "MythClassicHUDWidget.generated.h"

class UProgressBar;
class UTextBlock;
class UButton;
class UMythStatsComponent;
class UAutoBattleComponent;

/**
 * HUD classico do Ragnarok em UMG, montado inteiro em C++ (sem Widget Blueprint):
 *  - topo-esquerda: nome, nivel, barras de HP (verde) e SP (azul) com valores;
 *  - rodape: barra de EXP com 10 marcadores de 10%;
 *  - baixo-direita: Auto-Battle (liga/desliga), relogio do vigor de 3h e bloqueio por sobrepeso (>= 90%).
 * Ligue a um Pawn com BindToPawn(); le UMythStatsComponent e UAutoBattleComponent dele.
 */
UCLASS()
class MYTHOFTHOR_API UMythClassicHUDWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintCallable, Category = "Myth|HUD")
	void BindToPawn(APawn* Pawn);

protected:
	virtual void NativeOnInitialized() override;
	virtual void NativeTick(const FGeometry& MyGeometry, float InDeltaTime) override;

private:
	UPROPERTY() TObjectPtr<UTextBlock> NameText;
	UPROPERTY() TObjectPtr<UProgressBar> HPBar;
	UPROPERTY() TObjectPtr<UTextBlock> HPText;
	UPROPERTY() TObjectPtr<UProgressBar> SPBar;
	UPROPERTY() TObjectPtr<UTextBlock> SPText;
	UPROPERTY() TObjectPtr<UProgressBar> ExpBar;
	UPROPERTY() TObjectPtr<UTextBlock> ExpText;
	UPROPERTY() TObjectPtr<UButton> AutoButton;
	UPROPERTY() TObjectPtr<UTextBlock> AutoButtonText;
	UPROPERTY() TObjectPtr<UProgressBar> StaminaBar;
	UPROPERTY() TObjectPtr<UTextBlock> StaminaText;
	UPROPERTY() TObjectPtr<UTextBlock> WeightText;
	UPROPERTY() TObjectPtr<UTextBlock> MessageText;

	UPROPERTY() TObjectPtr<UMythStatsComponent> Stats;
	UPROPERTY() TObjectPtr<UAutoBattleComponent> AutoBattle;

	float MessageTimeLeft = 0.f;
	int32 LastShownSecond = -1;

	void BuildLayout();

	UFUNCTION() void RefreshStats();
	UFUNCTION() void RefreshAutoState(bool bIsActive);
	UFUNCTION() void RefreshStamina(float RemainingSeconds);
	UFUNCTION() void RefreshWeight(float WeightPercent, bool bOverweight);
	UFUNCTION() void ShowMessage(const FString& Reason);
	UFUNCTION() void OnAutoButtonClicked();

	void RefreshAutoAll();
};

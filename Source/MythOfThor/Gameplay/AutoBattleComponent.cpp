#include "AutoBattleComponent.h"

UAutoBattleComponent::UAutoBattleComponent()
	: bIsActive(false)
	, RemainingStaminaSeconds(MAX_DAILY_STAMINA_SECONDS)
	, InventoryWeightPercent(0.0f)
	, bIsDowned(false)
	, DownedTimer(0.0f)
{
	PrimaryComponentTick.bCanEverTick = true;
}

void UAutoBattleComponent::BeginPlay()
{
	Super::BeginPlay();
}

void UAutoBattleComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	// Consome vigor se o Auto-Battle estiver ativo
	if (bIsActive)
	{
		RemainingStaminaSeconds = FMath::Max(0.0f, RemainingStaminaSeconds - DeltaTime);
		OnAutoBattleTimeUpdated.Broadcast(RemainingStaminaSeconds);

		// Vigor esgotado: desativa automaticamente
		if (RemainingStaminaSeconds <= 0.0f)
		{
			ToggleAutoBattle(false);
			OnAutoBattleError.Broadcast(TEXT("Seu limite diário de 3 horas de Auto-Battle foi concluído."));
		}
	}

	// Contagem regressiva do estado caído no chão
	if (bIsDowned)
	{
		DownedTimer -= DeltaTime;
		if (DownedTimer <= 0.0f)
		{
			RespawnToSavePoint();
		}
	}
}

bool UAutoBattleComponent::ToggleAutoBattle(bool bEnable)
{
	if (bEnable)
	{
		// Validações de segurança antes de ativar
		if (bIsDowned)
		{
			OnAutoBattleError.Broadcast(TEXT("Você não pode ativar o Auto-Battle enquanto estiver derrotado."));
			return false;
		}

		if (RemainingStaminaSeconds <= 0.0f)
		{
			OnAutoBattleError.Broadcast(TEXT("Limite diário de 3 horas de Auto-Battle esgotado."));
			return false;
		}

		if (InventoryWeightPercent >= OVERWEIGHT_THRESHOLD_PERCENT)
		{
			OnAutoBattleError.Broadcast(TEXT("Inventário com sobrepeso (>= 90%). Esvazie seu inventário para continuar."));
			return false;
		}

		bIsActive = true;
		OnAutoBattleStateChanged.Broadcast(true);
		return true;
	}
	else
	{
		bIsActive = false;
		OnAutoBattleStateChanged.Broadcast(false);
		return true;
	}
}

float UAutoBattleComponent::NotifyPlayerDeath()
{
	bIsDowned = true;
	DownedTimer = DOWNED_WAIT_TIME_SECONDS;

	float ExpLostPercent = DEATH_PENALTY_MANUAL_PERCENT;

	if (bIsActive)
	{
		// Morte durante o Auto-Battle: desativa imediatamente e aplica 5% de penalidade
		bIsActive = false;
		ExpLostPercent = DEATH_PENALTY_AUTOBATTLE_PERCENT;
		OnAutoBattleStateChanged.Broadcast(false);
	}

	OnPlayerDiedInAutoBattle.Broadcast(ExpLostPercent, DownedTimer);
	return ExpLostPercent;
}

void UAutoBattleComponent::UpdateInventoryWeight(float CurrentWeightPercent)
{
	InventoryWeightPercent = CurrentWeightPercent;
	OnInventoryWeightChanged.Broadcast(InventoryWeightPercent, IsOverweight());

	// Trava de segurança: se o peso atingir 90% durante o combate, desativa na hora
	if (bIsActive && InventoryWeightPercent >= OVERWEIGHT_THRESHOLD_PERCENT)
	{
		ToggleAutoBattle(false);
		OnAutoBattleError.Broadcast(TEXT("Auto-Battle pausado: Inventário atingiu 90% de peso."));
	}
}

void UAutoBattleComponent::RespawnToSavePoint()
{
	bIsDowned = false;
	DownedTimer = 0.0f;
	// Auto-Battle permanece RIGOROSAMENTE desligado após respawn na cidade
	bIsActive = false;
	OnAutoBattleStateChanged.Broadcast(false);
}

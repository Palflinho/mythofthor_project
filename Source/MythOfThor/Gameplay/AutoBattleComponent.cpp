#include "AutoBattleComponent.h"

UAutoBattleComponent::UAutoBattleComponent()
	: bAutoBattleActive(false)
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
	if (bAutoBattleActive)
	{
		RemainingStaminaSeconds = FMath::Max(0.0f, RemainingStaminaSeconds - DeltaTime);
		OnAutoBattleTimeUpdated.Broadcast(RemainingStaminaSeconds);

		// Vigor esgotado: desativa automaticamente
		if (RemainingStaminaSeconds <= 0.0f)
		{
			ToggleAutoBattle(false);
			OnAutoBattleError.Broadcast(TEXT("Your daily 3-hour Auto-Battle limit has been used up."));
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
			OnAutoBattleError.Broadcast(TEXT("You cannot enable Auto-Battle while defeated."));
			return false;
		}

		if (RemainingStaminaSeconds <= 0.0f)
		{
			OnAutoBattleError.Broadcast(TEXT("Daily 3-hour Auto-Battle limit reached."));
			return false;
		}

		if (InventoryWeightPercent >= OVERWEIGHT_THRESHOLD_PERCENT)
		{
			OnAutoBattleError.Broadcast(TEXT("Inventory overweight (>= 90%). Empty your inventory to continue."));
			return false;
		}

		bAutoBattleActive = true;
		OnAutoBattleStateChanged.Broadcast(true);
		return true;
	}
	else
	{
		bAutoBattleActive = false;
		OnAutoBattleStateChanged.Broadcast(false);
		return true;
	}
}

float UAutoBattleComponent::NotifyPlayerDeath()
{
	bIsDowned = true;
	DownedTimer = DOWNED_WAIT_TIME_SECONDS;

	float ExpLostPercent = DEATH_PENALTY_MANUAL_PERCENT;

	if (bAutoBattleActive)
	{
		// Morte durante o Auto-Battle: desativa imediatamente e aplica 5% de penalidade
		bAutoBattleActive = false;
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
	if (bAutoBattleActive && InventoryWeightPercent >= OVERWEIGHT_THRESHOLD_PERCENT)
	{
		ToggleAutoBattle(false);
		OnAutoBattleError.Broadcast(TEXT("Auto-Battle paused: inventory reached 90% weight."));
	}
}

void UAutoBattleComponent::RespawnToSavePoint()
{
	bIsDowned = false;
	DownedTimer = 0.0f;
	// Auto-Battle permanece RIGOROSAMENTE desligado após respawn na cidade
	bAutoBattleActive = false;
	OnAutoBattleStateChanged.Broadcast(false);
}

// Homunculo: o cliente original anda e ataca com ele pelos scripts AI/AI.lua (IA padrao): segue o dono quando fica longe,
// ataca o que o dono ataca e obedece Alt + botao direito (andar ate a celula ou atacar o monstro) e Alt+T (aguardar:
// volta para o dono e para de atacar sozinho). Aqui a mesma IA basica, mandando os pedidos de sempre ao servidor
// (CZ_REQUEST_MOVENPC, CZ_REQUEST_ACTNPC, CZ_REQUEST_MOVETOOWNER); quem decide se pode e o rAthena.

#include "Player/MythPlayerController.h"

#include "Network/RagnarokNetworkSubsystem.h"
#include "Sprites/MythMonster.h"
#include "UI/MythRoHUDWidget.h"

#include "EngineUtils.h"
#include "Engine/GameInstance.h"

namespace
{
	constexpr float HomunFollowCells = 3.f;  // o AI.lua padrao volta para o dono a partir de 3 celulas
	constexpr float HomunCellSize = 50.f;     // celula do mapa em unidades do mundo

	AMythMonster* FindNetUnit(UWorld* World, int64 Id)
	{
		for (TActorIterator<AMythMonster> It(World); It; ++It)
		{
			if (It->IsNetUnit() && It->NetID == Id)
			{
				return *It;
			}
		}
		return nullptr;
	}
}

void AMythPlayerController::OnHotkeyHomunStandby()
{
	URagnarokNetworkSubsystem* Net = GetGameInstance() ? GetGameInstance()->GetSubsystem<URagnarokNetworkSubsystem>() : nullptr;
	if (!Net || !Net->HasHomun())
	{
		return;
	}
	bHomunStandby = !bHomunStandby;
	HomunLastTarget = 0;
	Net->SendHomunToMaster();
	if (HudWidget)
	{
		HudWidget->AddChatLine(bHomunStandby ? TEXT("Homunculus: aguardando (Alt+T de novo volta a ajudar).") : TEXT("Homunculus: ajudando no combate."),
			FLinearColor(0.6f, 0.8f, 1.f));
	}
}

bool AMythPlayerController::HandleHomunCommandClick(const FHitResult& Hit)
{
	URagnarokNetworkSubsystem* Net = GetGameInstance() ? GetGameInstance()->GetSubsystem<URagnarokNetworkSubsystem>() : nullptr;
	if (!Net || !Net->HasHomun())
	{
		return false;
	}
	HomunManualUntil = GetWorld()->GetTimeSeconds() + 3.0; // ordem do jogador: a IA nao puxa de volta logo em seguida
	if (const AMythMonster* Mob = Cast<AMythMonster>(Hit.GetActor()); Mob && Mob->IsNetUnit() && Mob->NetObjectType == 5 && !Mob->IsDead())
	{
		HomunLastTarget = Mob->NetID;
		Net->SendHomunAttack(static_cast<int32>(Mob->NetID));
		return true;
	}
	const FIntPoint Cell = WorldToCell(Hit.ImpactPoint);
	HomunLastTarget = 0;
	Net->SendHomunMove(Cell.X, Cell.Y);
	return true;
}

void AMythPlayerController::UpdateHomunAI(float DeltaTime)
{
	HomunAITimer -= DeltaTime;
	if (HomunAITimer > 0.f)
	{
		return;
	}
	HomunAITimer = 0.5f;
	URagnarokNetworkSubsystem* Net = GetGameInstance() ? GetGameInstance()->GetSubsystem<URagnarokNetworkSubsystem>() : nullptr;
	APawn* Self = GetPawn();
	if (!Net || !Self || !Net->HasHomun() || Net->GetHomunInfo().bResting)
	{
		return;
	}
	AMythMonster* Homun = FindNetUnit(GetWorld(), Net->GetHomunId());
	if (!Homun || Homun->IsDead() || GetWorld()->GetTimeSeconds() < HomunManualUntil)
	{
		return;
	}
	// Ajuda no combate: o alvo do dono vira o alvo do homunculo (uma vez por alvo)
	const AMythMonster* Target = FocusedTarget.Get();
	if (!bHomunStandby && Target && Target->IsNetUnit() && Target->NetObjectType == 5 && !Target->IsDead())
	{
		if (Target->NetID != HomunLastTarget)
		{
			HomunLastTarget = Target->NetID;
			Net->SendHomunAttack(static_cast<int32>(Target->NetID));
		}
		return;
	}
	if (HomunLastTarget != 0)
	{
		const AMythMonster* Last = FindNetUnit(GetWorld(), HomunLastTarget);
		if (Last && !Last->IsDead() && !bHomunStandby)
		{
			return; // ainda lutando com o ultimo alvo
		}
		HomunLastTarget = 0;
	}
	if (FVector::Dist2D(Homun->GetActorLocation(), Self->GetActorLocation()) > HomunFollowCells * HomunCellSize)
	{
		Net->SendHomunToMaster();
	}
}

void AMythPlayerController::MythHomunAttackNearest()
{
	URagnarokNetworkSubsystem* Net = GetGameInstance() ? GetGameInstance()->GetSubsystem<URagnarokNetworkSubsystem>() : nullptr;
	if (!Net || !Net->HasHomun() || !GetPawn())
	{
		return;
	}
	const AMythMonster* Best = nullptr;
	float BestDist = TNumericLimits<float>::Max();
	for (TActorIterator<AMythMonster> It(GetWorld()); It; ++It)
	{
		const float D = FVector::DistSquared(It->GetActorLocation(), GetPawn()->GetActorLocation());
		if (It->IsNetUnit() && It->NetObjectType == 5 && !It->IsDead() && D < BestDist)
		{
			BestDist = D;
			Best = *It;
		}
	}
	if (Best)
	{
		HomunManualUntil = GetWorld()->GetTimeSeconds() + 3.0;
		HomunLastTarget = Best->NetID;
		Net->SendHomunAttack(static_cast<int32>(Best->NetID));
	}
}

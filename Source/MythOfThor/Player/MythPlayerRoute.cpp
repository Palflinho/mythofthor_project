// Andar por rota: o rAthena recusa um pedido de movimento cujo caminho passe de max_walk_path (17 celulas) ou de 14 com
// obstaculo no meio (OFFICIAL_WALKPATH, unit.cpp), entao um clique longe ou atras de uma parede nao andava. Aqui o cliente
// tira o caminho inteiro no .gat (UMythCellMoverComponent::FindRoute, o mesmo A* do servidor) e manda CZ_REQUEST_MOVE para
// celulas da rota 12 a frente de onde o personagem esta; cada trecho e curto o bastante para o servidor aceitar e o proximo
// sai antes do anterior acabar, entao ele anda sem parar e contorna obstaculos. A Navegacao usa o mesmo mecanismo.

#include "Player/MythPlayerController.h"

#include "World/MythCellMoverComponent.h"
#include "World/MythMapInfo.h"

#include "Engine/World.h"

namespace
{
	constexpr int32 DirectLimit = 14; // ate aqui o pedido direto basta (o servidor acha o caminho sozinho)
	constexpr int32 ChunkAhead = 12;  // trecho mandado a frente da posicao atual (margem para o servidor estar 1-2 celulas atras)
	constexpr int32 ResendStep = 4;   // manda o proximo trecho quando o alvo puder avancar 4 celulas
	constexpr double StuckSeconds = 2.5;
	constexpr int32 MaxRetries = 4;

	int32 Chebyshev(const FIntPoint& A, const FIntPoint& B)
	{
		return FMath::Max(FMath::Abs(A.X - B.X), FMath::Abs(A.Y - B.Y));
	}
}

bool AMythPlayerController::StartRoute(const FIntPoint& Dest)
{
	const APawn* P = GetPawn();
	const UMythCellMoverComponent* Mover = P ? P->FindComponentByClass<UMythCellMoverComponent>() : nullptr;
	if (!Mover)
	{
		return false;
	}
	if (IsRouting() && Chebyshev(GetRouteDest(), Dest) <= 2)
	{
		return true; // botao segurado no mesmo lugar: a rota em andamento continua
	}
	const FIntPoint Me = Mover->GetCurrentCell();
	if (Chebyshev(Me, Dest) <= 3)
	{
		CancelRoute();
		return false;
	}
	TArray<FIntPoint> Route = UMythCellMoverComponent::FindRoute(GetWorld(), Me, Dest);
	if (Route.Num() <= DirectLimit)
	{
		CancelRoute(); // perto (ou sem caminho conhecido): o pedido direto de sempre
		return false;
	}
	RouteCells = MoveTemp(Route);
	RouteProgress = -1;
	RouteSentIdx = -1;
	RouteRetries = 0;
	RouteProgressTime = GetWorld()->GetTimeSeconds();
	UE_LOG(LogTemp, Log, TEXT("MythRoute: (%d,%d) -> (%d,%d), %d celulas"), Me.X, Me.Y, Dest.X, Dest.Y, RouteCells.Num());
	UpdateRoute();
	return true;
}

void AMythPlayerController::CancelRoute()
{
	RouteCells.Reset();
	RouteProgress = -1;
	RouteSentIdx = -1;
}

void AMythPlayerController::GetRouteRemaining(TArray<FIntPoint>& Out) const
{
	Out.Reset();
	for (int32 i = FMath::Max(RouteProgress + 1, 0); i < RouteCells.Num(); ++i)
	{
		Out.Add(RouteCells[i]);
	}
}

void AMythPlayerController::UpdateRoute()
{
	if (!IsRouting())
	{
		return;
	}
	const APawn* P = GetPawn();
	const UMythCellMoverComponent* Mover = P ? P->FindComponentByClass<UMythCellMoverComponent>() : nullptr;
	const AMythMapInfo* Info = AMythMapInfo::Find(GetWorld());
	if (!Mover || !Info)
	{
		CancelRoute();
		return;
	}
	const FIntPoint Me = Mover->GetCurrentCell();
	if (Me == RouteCells.Last())
	{
		CancelRoute(); // chegou
		return;
	}
	// Onde o personagem esta na rota (procura um pouco adiante do ultimo ponto conhecido)
	const double Now = GetWorld()->GetTimeSeconds();
	// O servidor pode escolher outro caminho de mesmo tamanho (paralelo ao nosso): vale a celula da rota mais perto do
	// personagem (ate 3 celulas; no empate, a mais adiante)
	int32 Found = -1;
	int32 FoundDist = 4;
	for (int32 i = FMath::Max(RouteProgress, 0); i < RouteCells.Num() && i <= RouteProgress + 24; ++i)
	{
		const int32 D = Chebyshev(RouteCells[i], Me);
		if (D <= FoundDist)
		{
			Found = i;
			FoundDist = D;
		}
	}
	if (Found > RouteProgress)
	{
		RouteProgress = Found;
		RouteProgressTime = Now;
		RouteRetries = 0;
	}
	const int32 Last = RouteCells.Num() - 1;
	const int32 Want = FMath::Min(RouteProgress + ChunkAhead, Last);
	bool bSend = RouteSentIdx < 0 || Want - RouteSentIdx >= ResendStep || (Want == Last && RouteSentIdx != Last);
	if (!bSend && Now - RouteProgressTime > StuckSeconds)
	{
		// Parado (pedido recusado ou o personagem saiu da rota): refaz a rota de onde esta
		if (++RouteRetries > MaxRetries)
		{
			UE_LOG(LogTemp, Log, TEXT("MythRoute: desisti em (%d,%d)"), Me.X, Me.Y);
			CancelRoute();
			return;
		}
		const FIntPoint Dest = RouteCells.Last();
		TArray<FIntPoint> Route = UMythCellMoverComponent::FindRoute(GetWorld(), Me, Dest);
		if (Route.Num() == 0)
		{
			CancelRoute();
			return;
		}
		RouteCells = MoveTemp(Route);
		RouteProgress = -1;
		RouteSentIdx = -1;
		RouteProgressTime = Now;
		bSend = true;
	}
	if (!bSend)
	{
		return;
	}
	const int32 Idx = FMath::Min(RouteProgress + ChunkAhead, RouteCells.Num() - 1);
	RouteSentIdx = Idx;
	SendCellToServer(Info->CellToWorld(RouteCells[Idx], P->GetActorLocation().Z), true);
}

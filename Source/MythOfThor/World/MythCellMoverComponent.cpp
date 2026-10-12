#include "World/MythCellMoverComponent.h"

#include "World/MythMapInfo.h"

#include "Components/CapsuleComponent.h"
#include "Algo/Reverse.h"
#include "Engine/World.h"
#include "GameFramework/Character.h"
#include "GameFramework/CharacterMovementComponent.h"

namespace
{
	constexpr int32 MoveCost = 10;         // MOVE_COST do rAthena
	constexpr int32 MoveDiagonalCost = 14; // MOVE_DIAGONAL_COST
	constexpr int32 MaxSearch = 40;        // raio maximo da busca (max_walk_path do servidor e 17)

	/** Celulas ja testadas no mapa atual: Z do chao, ou NaN quando nao ha chao (celula bloqueada). */
	struct FGroundCache
	{
		TWeakObjectPtr<const UWorld> World;
		TMap<FIntPoint, float> Z;
	};

	FGroundCache& Cache()
	{
		static FGroundCache C;
		return C;
	}

	float CellGround(const UWorld* World, const FIntPoint& Cell)
	{
		FGroundCache& C = Cache();
		if (C.World.Get() != World)
		{
			C.World = World;
			C.Z.Reset();
		}
		if (const float* Found = C.Z.Find(Cell))
		{
			return *Found;
		}
		float Z = NAN;
		if (const AMythMapInfo* Info = AMythMapInfo::Find(const_cast<UWorld*>(World)))
		{
			float Hit = 0.f;
			if (UMythCellMoverComponent::GroundZ(World, Info->CellToWorld(Cell, 0.f), Hit))
			{
				Z = Hit;
			}
		}
		C.Z.Add(Cell, Z);
		return Z;
	}
}

UMythCellMoverComponent::UMythCellMoverComponent()
{
	PrimaryComponentTick.bCanEverTick = true;
	PrimaryComponentTick.TickGroup = TG_PrePhysics;
}

namespace
{
	/** Traco de chao que ignora tudo que nao e cenario fixo (caixas de clique das unidades, itens...): os filtros de mobilidade
	 *  da Unreal nao as excluem, e o "chao" virava a cabeca de um NPC. */
	bool TraceStaticGround(const UWorld* World, const FVector& From, const FVector& To, FCollisionQueryParams Params, FHitResult& OutHit)
	{
		for (int32 Guard = 0; Guard < 12; ++Guard)
		{
			if (!World->LineTraceSingleByChannel(OutHit, From, To, ECC_Visibility, Params))
			{
				return false;
			}
			const UPrimitiveComponent* Comp = OutHit.GetComponent();
			if (!Comp || Comp->Mobility == EComponentMobility::Static)
			{
				return true;
			}
			if (AActor* Owner = OutHit.GetActor())
			{
				Params.AddIgnoredActor(Owner);
			}
			else
			{
				Params.AddIgnoredComponent(Comp);
			}
		}
		return false;
	}
}

bool UMythCellMoverComponent::GroundZ(const UWorld* World, const FVector& At, float& OutZ, const float* NearZ)
{
	// O chao do .gat so existe nas celulas livres e so responde ao canal Visibility (a malha visual nao)
	// So pecas estaticas: as caixas de clique das unidades tambem respondem a Visibility e nao sao chao
	FHitResult Hit;
	FCollisionQueryParams Params(TEXT("MythCellGround"), false);
	Params.MobilityType = EQueryMobilityType::Static;
	if (World && NearZ)
	{
		// Perto dos pes primeiro: copas de arvore, postes e telhados acima nao viram chao
		const FVector From(At.X, At.Y, *NearZ + StepUp);
		if (TraceStaticGround(World, From, From - FVector(0.f, 0.f, StepUp + 2000.f), Params, Hit))
		{
			OutZ = Hit.ImpactPoint.Z;
			return true;
		}
	}
	const FVector Top(At.X, At.Y, 20000.f);
	if (World && TraceStaticGround(World, Top, Top - FVector(0.f, 0.f, 40000.f), Params, Hit))
	{
		OutZ = Hit.ImpactPoint.Z;
		return true;
	}
	return false;
}

bool UMythCellMoverComponent::IsWalkable(const UWorld* World, const FIntPoint& Cell)
{
	return !FMath::IsNaN(CellGround(World, Cell));
}

TArray<FIntPoint> UMythCellMoverComponent::StraightPath(const UWorld* World, const FIntPoint& From, const FIntPoint& To)
{
	// Como o PathFinder do cliente: anda na diagonal enquanto os dois eixos faltam, depois reto; qualquer celula
	// bloqueada (ou quina cortada) desiste e deixa para o A*
	TArray<FIntPoint> Result;
	FIntPoint Cur = From;
	while (Cur != To)
	{
		const FIntPoint D(FMath::Sign(To.X - Cur.X), FMath::Sign(To.Y - Cur.Y));
		const FIntPoint N = Cur + D;
		if (!IsWalkable(World, N) || (D.X != 0 && D.Y != 0 && (!IsWalkable(World, Cur + FIntPoint(D.X, 0)) || !IsWalkable(World, Cur + FIntPoint(0, D.Y)))))
		{
			Result.Reset();
			return Result;
		}
		Result.Add(N);
		Cur = N;
	}
	return Result;
}

TArray<FIntPoint> UMythCellMoverComponent::FindPath(const UWorld* World, const FIntPoint& From, const FIntPoint& To)
{
	TArray<FIntPoint> Result;
	if (From == To || !IsWalkable(World, To) || FMath::Max(FMath::Abs(To.X - From.X), FMath::Abs(To.Y - From.Y)) > MaxSearch)
	{
		return Result;
	}
	Result = StraightPath(World, From, To);
	if (Result.Num() > 0)
	{
		return Result;
	}
	struct FNode { FIntPoint Cell; int32 G; int32 F; };
	auto Heuristic = [&To](const FIntPoint& C)
	{
		const int32 DX = FMath::Abs(To.X - C.X), DY = FMath::Abs(To.Y - C.Y);
		return MoveCost * (DX + DY) + (MoveDiagonalCost - 2 * MoveCost) * FMath::Min(DX, DY);
	};
	TArray<FNode> Open;
	TMap<FIntPoint, int32> Best;
	TMap<FIntPoint, FIntPoint> Parent;
	Open.Add({ From, 0, Heuristic(From) });
	Best.Add(From, 0);
	static const FIntPoint Dirs[8] = { {1,0}, {-1,0}, {0,1}, {0,-1}, {1,1}, {1,-1}, {-1,1}, {-1,-1} };
	while (Open.Num() > 0)
	{
		int32 BestIdx = 0;
		for (int32 i = 1; i < Open.Num(); ++i)
		{
			if (Open[i].F < Open[BestIdx].F) { BestIdx = i; }
		}
		const FNode Cur = Open[BestIdx];
		Open.RemoveAtSwap(BestIdx);
		if (Cur.Cell == To)
		{
			for (FIntPoint C = To; C != From; C = Parent[C])
			{
				Result.Insert(C, 0);
			}
			return Result;
		}
		if (Cur.G > Best[Cur.Cell])
		{
			continue;
		}
		for (const FIntPoint& D : Dirs)
		{
			const FIntPoint N = Cur.Cell + D;
			if (FMath::Abs(N.X - From.X) > MaxSearch || FMath::Abs(N.Y - From.Y) > MaxSearch || !IsWalkable(World, N))
			{
				continue;
			}
			const bool bDiagonal = D.X != 0 && D.Y != 0;
			// Diagonal so com os dois lados retos livres (nao corta quina), como o rAthena
			if (bDiagonal && (!IsWalkable(World, Cur.Cell + FIntPoint(D.X, 0)) || !IsWalkable(World, Cur.Cell + FIntPoint(0, D.Y))))
			{
				continue;
			}
			const int32 G = Cur.G + (bDiagonal ? MoveDiagonalCost : MoveCost);
			const int32* Known = Best.Find(N);
			if (Known && *Known <= G)
			{
				continue;
			}
			Best.Add(N, G);
			Parent.Add(N, Cur.Cell);
			Open.Add({ N, G, G + Heuristic(N) });
		}
	}
	return Result;
}

void UMythCellMoverComponent::MoveToCell(const FIntPoint& Dest, int32 InMsPerCell)
{
	const AActor* Owner = GetOwner();
	const AMythMapInfo* Info = AMythMapInfo::Find(GetWorld());
	if (!Owner || !Info)
	{
		return;
	}
	MsPerCell = FMath::Max(InMsPerCell, 20);
	if (Path.Num() > 0)
	{
		// Trocou de rota andando: como o servidor (change_walk_target do rAthena), termina o passo atual e segue
		// dali. Recalcular da celula "atual" fazia meia-volta quando o passo ainda nao tinha passado do meio
		const FIntPoint Next = Path[0];
		TArray<FIntPoint> Rest = FindPath(GetWorld(), Next, Dest);
		if (Rest.Num() == 0 && Next != Dest)
		{
			Rest.Add(Dest);
		}
		Path.Reset();
		Path.Add(Next);
		Path.Append(Rest);
		return; // o segmento em andamento (SegmentStart/Time) continua igual
	}
	const FIntPoint From = Info->WorldToCell(Owner->GetActorLocation());
	Path = FindPath(GetWorld(), From, Dest);
	if (Path.Num() == 0 && From != Dest)
	{
		Path.Add(Dest); // sem caminho conhecido (chao nao carregado ainda): vai em linha reta, o servidor corrige
	}
	BeginSegment();
}

void UMythCellMoverComponent::ServerWalk(const FIntPoint& From, const FIntPoint& Dest, int32 InMsPerCell)
{
	const FIntPoint Cur = GetCurrentCell();
	if (const APawn* Pawn = Cast<APawn>(GetOwner()); Pawn && Pawn->IsPlayerControlled())
	{
		UE_LOG(LogTemp, Log, TEXT("MythMove: servidor (%d,%d)->(%d,%d), cliente em (%d,%d)"), From.X, From.Y, Dest.X, Dest.Y, Cur.X, Cur.Y);
	}
	if (FMath::Max(FMath::Abs(Cur.X - From.X), FMath::Abs(Cur.Y - From.Y)) > MaxDesyncCells)
	{
		SnapToCell(From);
	}
	MoveToCell(Dest, InMsPerCell);
}

void UMythCellMoverComponent::ServerStop(const FIntPoint& Cell)
{
	const FIntPoint Cur = GetCurrentCell();
	if (const APawn* Pawn = Cast<APawn>(GetOwner()); Pawn && Pawn->IsPlayerControlled())
	{
		UE_LOG(LogTemp, Log, TEXT("MythMove: parada do servidor em (%d,%d), cliente em (%d,%d)"), Cell.X, Cell.Y, Cur.X, Cur.Y);
	}
	if (IsMoving())
	{
		// Servidor igual ou um pouco atras do que o jogador ve: so termina o passo atual (sem passo para tras)
		if (Passed.Contains(Cell) || Cell == Cur || Cell == Path[0])
		{
			Path.SetNum(1);
			return;
		}
		// Mais a frente na rota: para la
		const int32 Index = Path.Find(Cell);
		if (Index != INDEX_NONE)
		{
			Path.SetNum(Index + 1);
			return;
		}
	}
	else if (Cell == Cur)
	{
		return;
	}
	if (FMath::Max(FMath::Abs(Cur.X - Cell.X), FMath::Abs(Cur.Y - Cell.Y)) > MaxDesyncCells)
	{
		SnapToCell(Cell);
		return;
	}
	MoveToCell(Cell, MsPerCell);
}

FIntPoint UMythCellMoverComponent::GetCurrentCell() const
{
	const AMythMapInfo* Info = AMythMapInfo::Find(GetWorld());
	return Info && GetOwner() ? Info->WorldToCell(GetOwner()->GetActorLocation()) : FIntPoint::ZeroValue;
}

FIntPoint UMythCellMoverComponent::GetNextCell() const
{
	return Path.Num() > 0 ? Path[0] : GetCurrentCell();
}

void UMythCellMoverComponent::SnapToCell(const FIntPoint& Cell)
{
	Stop();
	AActor* Owner = GetOwner();
	const AMythMapInfo* Info = AMythMapInfo::Find(GetWorld());
	if (!Owner || !Info)
	{
		return;
	}
	FVector Pos = Info->CellToWorld(Cell, Owner->GetActorLocation().Z);
	float Offset = FeetOffset;
	if (const ACharacter* Char = Cast<ACharacter>(Owner); Char && Offset <= 0.f && Char->GetCapsuleComponent())
	{
		Offset = Char->GetCapsuleComponent()->GetScaledCapsuleHalfHeight();
	}
	float Ground = 0.f;
	if (GroundZ(GetWorld(), Pos, Ground))
	{
		Pos.Z = Ground + Offset;
	}
	Owner->SetActorLocation(Pos, false, nullptr, ETeleportType::TeleportPhysics);
	Passed.Reset();
}

void UMythCellMoverComponent::PlaceOnGround()
{
	AActor* Owner = GetOwner();
	if (!Owner)
	{
		return;
	}
	float Offset = FeetOffset;
	if (const ACharacter* Char = Cast<ACharacter>(Owner); Char && Offset <= 0.f && Char->GetCapsuleComponent())
	{
		Offset = Char->GetCapsuleComponent()->GetScaledCapsuleHalfHeight();
	}
	FVector Pos = Owner->GetActorLocation();
	float Ground = 0.f;
	if (GroundZ(GetWorld(), Pos, Ground))
	{
		Pos.Z = Ground + Offset;
		Owner->SetActorLocation(Pos, false, nullptr, ETeleportType::TeleportPhysics);
	}
	SetOwnerVelocity(FVector::ZeroVector); // tambem desliga a fisica (MOVE_None)
}

void UMythCellMoverComponent::Stop()
{
	Path.Reset();
	SetOwnerVelocity(FVector::ZeroVector);
}

void UMythCellMoverComponent::BeginSegment()
{
	const AActor* Owner = GetOwner();
	const AMythMapInfo* Info = AMythMapInfo::Find(GetWorld());
	if (!Owner || !Info || Path.Num() == 0)
	{
		Stop();
		return;
	}
	SegmentStart = Owner->GetActorLocation();
	const FVector Target = Info->CellToWorld(Path[0], SegmentStart.Z);
	// Tempo do passo pelo tamanho real (o primeiro pode comecar no meio da celula): 50 cm = MsPerCell
	const float Cells = FVector::Dist2D(SegmentStart, Target) / 50.f;
	SegmentDuration = FMath::Max(Cells * MsPerCell / 1000.f, 0.01f);
	SegmentTime = 0.f;
}

void UMythCellMoverComponent::SetOwnerVelocity(const FVector& Velocity)
{
	// O sprite escolhe andar/parado e a direcao pela velocidade do dono
	if (ACharacter* Char = Cast<ACharacter>(GetOwner()))
	{
		if (UCharacterMovementComponent* Move = Char->GetCharacterMovement())
		{
			// Sem fisica nunca, nem parado: quem decide a posicao e o servidor. Parado em MOVE_Walking, a capsula
			// era empurrada pelos monstros que passavam por cima (desencaixe) e o jogador "escorregava" celulas
			if (Move->MovementMode != MOVE_None)
			{
				Move->SetMovementMode(MOVE_None);
			}
			Move->Velocity = Velocity;
		}
	}
}

void UMythCellMoverComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);
	AActor* Owner = GetOwner();
	const AMythMapInfo* Info = AMythMapInfo::Find(GetWorld());
	if (!Owner || !Info || Path.Num() == 0)
	{
		return;
	}
	float Remaining = DeltaTime;
	FVector Pos = Owner->GetActorLocation();
	FVector Dir = FVector::ZeroVector;
	while (Remaining > 0.f && Path.Num() > 0)
	{
		const FVector Target = Info->CellToWorld(Path[0], SegmentStart.Z);
		const float Step = FMath::Min(Remaining, SegmentDuration - SegmentTime);
		SegmentTime += Step;
		Remaining -= Step;
		const float Alpha = FMath::Clamp(SegmentTime / SegmentDuration, 0.f, 1.f);
		Pos = FMath::Lerp(SegmentStart, Target, Alpha);
		Dir = (Target - SegmentStart).GetSafeNormal2D();
		if (Alpha >= 1.f)
		{
			Passed.Add(Path[0]);
			if (Passed.Num() > 4)
			{
				Passed.RemoveAt(0);
			}
			Path.RemoveAt(0);
			if (Path.Num() > 0)
			{
				SegmentStart = FVector(Target.X, Target.Y, Pos.Z);
				const FVector Next = Info->CellToWorld(Path[0], Pos.Z);
				const bool bDiagonal = !FMath::IsNearlyEqual(Next.X, Target.X, 1.f) && !FMath::IsNearlyEqual(Next.Y, Target.Y, 1.f);
				SegmentDuration = MsPerCell * (bDiagonal ? MoveDiagonalCost : MoveCost) / MoveCost / 1000.f;
				SegmentTime = 0.f;
			}
		}
	}
	// Altura pelo chao do .gat no ponto atual (sobe calcadas e escadas sem colisao)
	float Offset = FeetOffset;
	if (const ACharacter* Char = Cast<ACharacter>(Owner); Char && Offset <= 0.f && Char->GetCapsuleComponent())
	{
		Offset = Char->GetCapsuleComponent()->GetScaledCapsuleHalfHeight();
	}
	const float FeetZ = Owner->GetActorLocation().Z - Offset;
	float Ground = 0.f;
	if (GroundZ(GetWorld(), Pos, Ground, &FeetZ))
	{
		Pos.Z = Ground + Offset;
	}
	Owner->SetActorLocation(Pos, false, nullptr, ETeleportType::TeleportPhysics);
	const float Speed = 50.f * 1000.f / MsPerCell;
	SetOwnerVelocity(Path.Num() > 0 ? Dir * Speed : FVector::ZeroVector);
}

TArray<FIntPoint> UMythCellMoverComponent::FindRoute(const UWorld* World, const FIntPoint& From, const FIntPoint& To, int32 MaxRadius)
{
	// Mesmo A* do FindPath (custos e quinas do rAthena), com fila de prioridade e teto de nos para nao travar o jogo
	TArray<FIntPoint> Result;
	if (From == To || !IsWalkable(World, To) || FMath::Max(FMath::Abs(To.X - From.X), FMath::Abs(To.Y - From.Y)) > MaxRadius)
	{
		return Result;
	}
	Result = StraightPath(World, From, To);
	if (Result.Num() > 0)
	{
		return Result;
	}
	struct FNode
	{
		FIntPoint Cell;
		int32 G;
		int32 F;
	};
	struct FByF { bool operator()(const FNode& A, const FNode& B) const { return A.F < B.F; } };
	auto Heuristic = [&To](const FIntPoint& C)
	{
		const int32 DX = FMath::Abs(To.X - C.X), DY = FMath::Abs(To.Y - C.Y);
		return MoveCost * (DX + DY) + (MoveDiagonalCost - 2 * MoveCost) * FMath::Min(DX, DY);
	};
	constexpr int32 MaxNodes = 60000;
	TArray<FNode> Open;
	TMap<FIntPoint, int32> Best;
	TMap<FIntPoint, FIntPoint> Parent;
	Open.HeapPush({ From, 0, Heuristic(From) }, FByF());
	Best.Add(From, 0);
	static const FIntPoint Dirs[8] = { {1,0}, {-1,0}, {0,1}, {0,-1}, {1,1}, {1,-1}, {-1,1}, {-1,-1} };
	int32 Visited = 0;
	while (Open.Num() > 0 && Visited < MaxNodes)
	{
		FNode Cur;
		Open.HeapPop(Cur, FByF());
		if (Cur.Cell == To)
		{
			for (FIntPoint C = To; C != From; C = Parent[C])
			{
				Result.Add(C);
			}
			Algo::Reverse(Result);
			return Result;
		}
		if (Cur.G > Best[Cur.Cell])
		{
			continue;
		}
		++Visited;
		for (const FIntPoint& D : Dirs)
		{
			const FIntPoint N = Cur.Cell + D;
			if (FMath::Abs(N.X - From.X) > MaxRadius || FMath::Abs(N.Y - From.Y) > MaxRadius || !IsWalkable(World, N))
			{
				continue;
			}
			const bool bDiagonal = D.X != 0 && D.Y != 0;
			if (bDiagonal && (!IsWalkable(World, Cur.Cell + FIntPoint(D.X, 0)) || !IsWalkable(World, Cur.Cell + FIntPoint(0, D.Y))))
			{
				continue;
			}
			const int32 G = Cur.G + (bDiagonal ? MoveDiagonalCost : MoveCost);
			if (const int32* Known = Best.Find(N); Known && *Known <= G)
			{
				continue;
			}
			Best.Add(N, G);
			Parent.Add(N, Cur.Cell);
			Open.HeapPush({ N, G, G + Heuristic(N) }, FByF());
		}
	}
	return Result;
}

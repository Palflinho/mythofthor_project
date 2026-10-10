#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "MythCellMoverComponent.generated.h"

/**
 * Movimento por celulas, como o cliente original: o servidor diz o destino (ZC_NOTIFY_MOVE / PLAYERMOVE), o
 * cliente calcula o mesmo caminho no .gat (A* de 8 direcoes sem cortar quinas, custo 10 reto / 14 diagonal, como
 * o path.cpp do rAthena) e anda celula a celula na velocidade do servidor (ms por celula; diagonal x1,4).
 * A altura vem do chao do .gat (trace de Visibility), sem fisica: degraus e calcadas nao bloqueiam, porque quem
 * decide o que e andavel e o servidor.
 */
UCLASS(ClassGroup = (Myth), meta = (BlueprintSpawnableComponent))
class MYTHOFTHOR_API UMythCellMoverComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UMythCellMoverComponent();

	/** Anda ate a celula (caminho calculado aqui). MsPerCell = velocidade do servidor (150 = padrao). */
	void MoveToCell(const FIntPoint& Dest, int32 MsPerCell);

	/** Para no lugar (ZC_STOPMOVE, ataque, teleporte). */
	void Stop();

	bool IsMoving() const { return Path.Num() > 0; }

	/** Distancia (cm) dos pes ao centro da capsula do dono: o ator fica a essa altura do chao. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Myth|Move")
	float FeetOffset = 0.f;

	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

	/** Celula andavel? (existe chao do .gat nela) */
	static bool IsWalkable(const UWorld* World, const FIntPoint& Cell);

	/** Altura do chao do .gat no ponto (Z), ou falso se nao ha chao. */
	static bool GroundZ(const UWorld* World, const FVector& At, float& OutZ);

	/** Caminho de From ate To (sem incluir From); vazio se nao houver. */
	static TArray<FIntPoint> FindPath(const UWorld* World, const FIntPoint& From, const FIntPoint& To);

private:
	TArray<FIntPoint> Path;
	FVector SegmentStart = FVector::ZeroVector;
	float SegmentTime = 0.f;
	float SegmentDuration = 0.f;
	int32 MsPerCell = 150;

	void BeginSegment();
	void SetOwnerVelocity(const FVector& Velocity);
};

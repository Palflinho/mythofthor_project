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

	/**
	 * Rota do servidor (ZC_NOTIFY_MOVE / PLAYERMOVE com a origem): parte da posicao atual em float (sem estalo),
	 * mas se o cliente estiver a mais de 16 celulas da origem do servidor, salta para ela antes (docs/ref/01 3.3/3.4).
	 */
	void ServerWalk(const FIntPoint& From, const FIntPoint& Dest, int32 MsPerCell);

	/**
	 * Parada do servidor (ZC_STOPMOVE). Celula ja passada, atual ou a proxima: ignora o "passo para tras" e so
	 * termina o passo em andamento; mais a frente na rota: para la; fora da rota: anda ate ela, ou salta se > 16.
	 */
	void ServerStop(const FIntPoint& Cell);

	/** Para no lugar (ataque, teleporte). */
	void Stop();

	bool IsMoving() const { return Path.Num() > 0; }

	/** Celula para onde o passo atual vai (ou a celula atual, parado). */
	FIntPoint GetNextCell() const;

	/** Celula N passos a frente na rota (0 = a do passo atual); no fim da rota, o destino. */
	FIntPoint GetCellAhead(int32 N) const
	{
		return Path.IsValidIndex(N) ? Path[N] : (Path.Num() > 0 ? Path.Last() : GetCurrentCell());
	}

	/** Celula onde o dono esta agora. */
	FIntPoint GetCurrentCell() const;

	/** Distancia (cm) dos pes ao centro da capsula do dono: o ator fica a essa altura do chao. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Myth|Move")
	float FeetOffset = 0.f;

	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

	/** Celula andavel? (existe chao do .gat nela) */
	static bool IsWalkable(const UWorld* World, const FIntPoint& Cell);

	/**
	 * Altura do chao do .gat no ponto (Z), ou falso se nao ha chao. Com NearZ, procura so perto dessa altura
	 * (degraus de ate StepUp): copas de arvore, postes e telhados acima nao viram "chao".
	 */
	static bool GroundZ(const UWorld* World, const FVector& At, float& OutZ, const float* NearZ = nullptr);

	/** Quanto (cm) o chao pode subir de uma vez seguindo o caminho (calcadas, escadas). */
	static constexpr float StepUp = 120.f;

	/** Caminho de From ate To (sem incluir From); vazio se nao houver. Reta primeiro, A* se ela bater em algo. */
	static TArray<FIntPoint> FindPath(const UWorld* World, const FIntPoint& From, const FIntPoint& To);

	/** Distancia maxima (celulas) entre cliente e servidor antes de saltar em vez de andar. */
	static constexpr int32 MaxDesyncCells = 16;

private:
	TArray<FIntPoint> Path;
	/** Ultimas celulas alcancadas (a mais recente no fim): um STOPMOVE atrasado para uma delas e ignorado. */
	TArray<FIntPoint> Passed;
	FVector SegmentStart = FVector::ZeroVector;
	float SegmentTime = 0.f;
	float SegmentDuration = 0.f;
	int32 MsPerCell = 150;

	void BeginSegment();
	void SetOwnerVelocity(const FVector& Velocity);
	void SnapToCell(const FIntPoint& Cell);
	static TArray<FIntPoint> StraightPath(const UWorld* World, const FIntPoint& From, const FIntPoint& To);
};

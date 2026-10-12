#include "Sprites/MythFalcon.h"

#include "Sprites/MythRoSpriteComponent.h"

#include "Engine/World.h"

namespace
{
	constexpr float CellUU = 50.f;          // celula do mapa em unidades do mundo
	constexpr float FollowStart = 2.f * CellUU; // roBrowser entitiesWalkProcess: range 2
	constexpr float FollowStop = 1.f * CellUU;  // anda ate range - 1
	constexpr float FollowSpeed = 420.f;    // um pouco mais rapido que o dono (velocidade do dono - 50 ms por celula)
	constexpr float AttackSpeed = 50.f * 1000.f / 35.f; // Ataque Aereo: 35 ms por celula
	constexpr float HoverHeight = 230.f;    // uma altura de personagem acima da cabeca do dono, para nao cobri-lo
	constexpr float SideOffset = 30.f;      // ao parar, fica um pouco ao lado do dono e nao em cima dele
}

AMythFalcon::AMythFalcon()
{
	PrimaryActorTick.bCanEverTick = true;
	Root = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
	SetRootComponent(Root);
	Sprite = CreateDefaultSubobject<UMythRoSpriteComponent>(TEXT("Sprite"));
	Sprite->SetupAttachment(Root);
	Sprite->MonsterSpriteFolder = TEXT("이팩트"); // 이팩트
	Sprite->MonsterSpriteName = TEXT("매");
	Sprite->bPlayActSounds = false;
	Sprite->bBlobShadow = false;      // voando: sem sombra no chao (roBrowser: hideShadow)
	Sprite->bCastSpriteShadow = false;
}

AMythFalcon* AMythFalcon::Spawn(AActor* Master, int32 JobClass)
{
	if (!Master || !Master->GetWorld())
	{
		return nullptr;
	}
	FActorSpawnParameters Params;
	Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
	const FVector Start = Master->GetActorLocation() + FVector(0.f, 0.f, HoverHeight);
	AMythFalcon* Falcon = Master->GetWorld()->SpawnActorDeferred<AMythFalcon>(AMythFalcon::StaticClass(), FTransform(Start), Master, nullptr, ESpawnActorCollisionHandlingMethod::AlwaysSpawn);
	if (!Falcon)
	{
		return nullptr;
	}
	Falcon->Master = Master;
	// roBrowser DB: 11/4034 (Cacador, bebe) -> 매; 4012 (Atirador de Elite) -> 매2
	Falcon->Sprite->MonsterSpriteName = JobClass == 4012 ? TEXT("매2") : TEXT("매");
	Falcon->FinishSpawning(FTransform(Start));
	return Falcon;
}

void AMythFalcon::AttackTo(const FVector& Target)
{
	// Passa um pouco do alvo (overshoot) antes de voltar, como o calculateOverShot do roBrowser
	const FVector From = GetActorLocation();
	FVector Dir = Target - From;
	Dir.Z = 0.f;
	AttackPoint = Target + Dir.GetSafeNormal() * CellUU * 2.f;
	AttackPoint.Z = Target.Z + HoverHeight * 0.6f;
	bAttacking = true;
	bReturning = false;
	bFollowing = false;
}

void AMythFalcon::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);
	AActor* Lord = Master.Get();
	if (!Lord)
	{
		Destroy();
		return;
	}
	const FVector Here = GetActorLocation();
	const FVector MasterAt = Lord->GetActorLocation();
	FVector Goal;
	float Speed;
	if (bAttacking)
	{
		Goal = bReturning ? MasterAt + FVector(0.f, 0.f, HoverHeight) : AttackPoint;
		Speed = AttackSpeed;
	}
	else
	{
		const float Dist = FVector::Dist2D(Here, MasterAt);
		if (!bFollowing && Dist > FollowStart)
		{
			bFollowing = true;
		}
		Goal = FVector(MasterAt.X, MasterAt.Y, MasterAt.Z + HoverHeight);
		Speed = FollowSpeed;
		if (bFollowing && Dist <= FollowStop)
		{
			bFollowing = false;
		}
	}

	FVector Next = Here;
	const bool bMove = bAttacking || bFollowing;
	if (bMove)
	{
		FVector Delta = Goal - Here;
		const float Len2D = FVector2D(Delta.X, Delta.Y).Size();
		const float Step = Speed * DeltaSeconds;
		if (Len2D <= Step || Len2D < 1.f)
		{
			Next.X = Goal.X;
			Next.Y = Goal.Y;
			if (bAttacking)
			{
				if (bReturning)
				{
					bAttacking = false; // de volta ao dono
				}
				bReturning = true;
			}
		}
		else
		{
			Next.X += Delta.X / Len2D * Step;
			Next.Y += Delta.Y / Len2D * Step;
		}
		Sprite->FaceLocation(FVector(Goal.X, Goal.Y, Here.Z));
	}
	else if (FVector::Dist2D(Here, MasterAt) < SideOffset * 0.5f)
	{
		// Parado em cima do dono: desliza para o lado para nao cobrir o personagem
		Next += FVector(SideOffset * DeltaSeconds * 2.f, 0.f, 0.f);
	}
	// Altura suave (o dono sobe e desce escadas)
	Next.Z = FMath::FInterpTo(Here.Z, (bAttacking ? Goal.Z : MasterAt.Z + HoverHeight), DeltaSeconds, 6.f);
	SetActorLocation(Next);
	// Batendo as asas (acao 1) so no ataque; seguindo o dono ele plana (acao 0), como no roBrowser
	Root->ComponentVelocity = bAttacking ? (Next - Here) / FMath::Max(DeltaSeconds, 0.001f) : FVector::ZeroVector;
}

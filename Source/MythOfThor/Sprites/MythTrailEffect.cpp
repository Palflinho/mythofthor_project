#include "Sprites/MythTrailEffect.h"

#include "UI/MythRoUITextures.h"
#include "World/MythCellMoverComponent.h"

#include "Dom/JsonObject.h"
#include "Engine/World.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "ProceduralMeshComponent.h"

namespace
{
	constexpr float TrailCell = 50.f; // 1 celula do RO em uu

	/** Numero, ou faixa [min, max] sorteada (os campos do roBrowser aceitam os dois). */
	float RoNum(const TSharedPtr<FJsonObject>& O, const TCHAR* Key, float Default)
	{
		double V = 0.0;
		if (O->TryGetNumberField(Key, V))
		{
			return static_cast<float>(V);
		}
		const TArray<TSharedPtr<FJsonValue>>* Range = nullptr;
		if (O->TryGetArrayField(Key, Range) && Range->Num() >= 2)
		{
			return FMath::FRandRange(static_cast<float>((*Range)[0]->AsNumber()), static_cast<float>((*Range)[1]->AsNumber()));
		}
		return Default;
	}

	float GroundAt(const UWorld* World, const FVector& At)
	{
		float Z = At.Z;
		const float Near = At.Z;
		return UMythCellMoverComponent::GroundZ(World, At, Z, &Near) ? Z : At.Z;
	}
}

// ------------------------------------------------------------------------------------------------ TRAIL

AMythTrailEffect::AMythTrailEffect()
{
	PrimaryActorTick.bCanEverTick = true;
	SetRootComponent(CreateDefaultSubobject<USceneComponent>(TEXT("Root")));
}

bool AMythTrailEffect::Spawn(UObject* WorldContext, const TSharedPtr<FJsonObject>& Part, const FVector& From, const FVector& To)
{
	UWorld* World = WorldContext ? WorldContext->GetWorld() : nullptr;
	const TArray<TSharedPtr<FJsonValue>>* SpawnList = nullptr;
	if (!World || !Part.IsValid() || !Part->TryGetArrayField(TEXT("spawn"), SpawnList) || SpawnList->Num() == 0)
	{
		return false;
	}
	FActorSpawnParameters P;
	P.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
	AMythTrailEffect* T = World->SpawnActor<AMythTrailEffect>(From, FRotator::ZeroRotator, P);
	if (!T)
	{
		return false;
	}
	T->Def = Part;
	T->Start = From;
	const FVector2D D(To.X - From.X, To.Y - From.Y);
	T->TargetDistance = D.Size();
	T->Dir = T->TargetDistance > 1.f ? D / T->TargetDistance : FVector2D(0.f, -1.f);
	T->Speed = RoNum(Part, TEXT("speed"), 24.f);
	T->Interval = FMath::Max(RoNum(Part, TEXT("interval"), 17.f), 5.f) / 1000.f;
	T->StartOffset = RoNum(Part, TEXT("startOffset"), 0.f);
	T->Overshoot = RoNum(Part, TEXT("overshoot"), 0.f);
	bool bStop = false;
	T->bStopAtTarget = Part->TryGetBoolField(TEXT("stopAtTarget"), bStop) && bStop;
	T->Life = FMath::Max(RoNum(Part, TEXT("duration"), 2500.f), 100.f) / 1000.f;
	return true;
}

void AMythTrailEffect::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);
	Time += DeltaSeconds;
	const float MaxCells = bStopAtTarget ? TargetDistance / TrailCell + Overshoot : TNumericLimits<float>::Max();
	while (NextSpawn <= Time)
	{
		const float Cells = StartOffset + NextSpawn * Speed;
		if (Cells > MaxCells || NextSpawn >= Life)
		{
			Destroy();
			return;
		}
		SpawnAt(Cells);
		NextSpawn += Interval;
	}
}

void AMythTrailEffect::SpawnAt(float DistanceCells)
{
	const TArray<TSharedPtr<FJsonValue>>* SpawnList = nullptr;
	if (!Def->TryGetArrayField(TEXT("spawn"), SpawnList))
	{
		return;
	}
	const float Angle = FMath::FRandRange(0.f, 2.f * PI);
	const float Radius = RoNum(Def, TEXT("spread"), 0.f) * TrailCell;
	FVector At = Start + FVector(Dir.X, Dir.Y, 0.f) * DistanceCells * TrailCell + FVector(FMath::Cos(Angle), FMath::Sin(Angle), 0.f) * Radius;
	At.Z = GroundAt(GetWorld(), At);
	for (const TSharedPtr<FJsonValue>& V : *SpawnList)
	{
		const TSharedPtr<FJsonObject> Sub = V->AsObject();
		if (Sub.IsValid() && Sub->GetStringField(TEXT("type")) == TEXT("QuadHorn"))
		{
			AMythQuadHorn::Spawn(this, Sub, At);
		}
	}
}

// ------------------------------------------------------------------------------------------------ QuadHorn

AMythQuadHorn::AMythQuadHorn()
{
	PrimaryActorTick.bCanEverTick = true;
	Mesh = CreateDefaultSubobject<UProceduralMeshComponent>(TEXT("Mesh"));
	SetRootComponent(Mesh);
	Mesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	Mesh->SetCastShadow(false);
}

bool AMythQuadHorn::Spawn(UObject* WorldContext, const TSharedPtr<FJsonObject>& Part, const FVector& Ground)
{
	UWorld* World = WorldContext ? WorldContext->GetWorld() : nullptr;
	FString File;
	if (!World || !Part.IsValid() || !Part->TryGetStringField(TEXT("textureFile"), File))
	{
		Part->TryGetStringField(TEXT("file"), File);
	}
	UTexture2D* Tex = File.IsEmpty() ? nullptr : FMythRoUITextures::Get(TEXT("../") + File); // data/texture/<arquivo>
	if (!Tex && File.EndsWith(TEXT(".tga")))
	{
		Tex = FMythRoUITextures::Get(TEXT("../") + File.LeftChop(4) + TEXT(".bmp"));
	}
	UMaterialInterface* Translucent = LoadObject<UMaterialInterface>(nullptr, TEXT("/Game/Myth/M_StrTranslucent.M_StrTranslucent"));
	if (!World || !Tex || !Translucent)
	{
		return false;
	}
	FActorSpawnParameters P;
	P.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
	AMythQuadHorn* H = World->SpawnActor<AMythQuadHorn>(Ground, FRotator::ZeroRotator, P);
	if (!H)
	{
		return false;
	}
	H->Material = UMaterialInstanceDynamic::Create(Translucent, H);
	H->Material->SetTextureParameterValue(TEXT("Tex"), Tex);
	H->Material->SetScalarParameterValue(TEXT("Brightness"), 1.4f);
	H->Mesh->SetMaterial(0, H->Material);
	H->Height = FMath::Max(RoNum(Part, TEXT("height"), 1.f), 0.01f) * TrailCell;
	H->Bottom = RoNum(Part, TEXT("bottomSize"), 0.2f) * TrailCell;
	H->OffsetZ = RoNum(Part, TEXT("offsetZ"), 0.f) * TrailCell;
	H->Rise = RoNum(Part, TEXT("riseDistance"), 0.f) * TrailCell;
	H->Tilt = FRotator(RoNum(Part, TEXT("rotateX"), 0.f), RoNum(Part, TEXT("rotateY"), 0.f), RoNum(Part, TEXT("rotateZ"), 0.f));
	H->Animation = static_cast<int32>(RoNum(Part, TEXT("animation"), 0.f));
	H->AnimSeconds = FMath::Max(RoNum(Part, TEXT("animationSpeed"), 100.f), 1.f) / 1000.f;
	H->Duration = FMath::Max(RoNum(Part, TEXT("duration"), 670.f), 30.f) / 1000.f;
	H->FadeOut = RoNum(Part, TEXT("fadeOut"), 0.f) / 1000.f;
	const TArray<TSharedPtr<FJsonValue>>* Color = nullptr;
	if (Part->TryGetArrayField(TEXT("color"), Color) && Color->Num() >= 4)
	{
		H->Alpha = static_cast<float>((*Color)[3]->AsNumber());
	}
	H->SetActorRotation(FRotator(0.f, H->Tilt.Yaw, 0.f));
	H->Build(H->Animation == 1 ? 0.f : H->Height, H->Animation == 4 ? H->OffsetZ - H->Rise : H->OffsetZ, H->Alpha);
	return true;
}

void AMythQuadHorn::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);
	Time += DeltaSeconds;
	if (Time >= Duration)
	{
		Destroy();
		return;
	}
	const float P = FMath::Clamp(Time / AnimSeconds, 0.f, 1.f);
	float CurHeight = Height;
	float CurOffset = OffsetZ;
	switch (Animation)
	{
	case 1: CurHeight = Height * P; break;                                   // cresce
	case 2: CurOffset = OffsetZ * P; break;                                  // sobe ate o offset
	case 3: CurOffset = FMath::Min(Time / AnimSeconds * TrailCell, Height * 0.5f); break; // sobe passando do chao
	case 4: CurOffset = OffsetZ - Rise * (1.f - (1.f - (1.f - P) * (1.f - P))); break;  // sai de baixo, desacelerando
	default: break;
	}
	float A = Alpha;
	if (FadeOut > 0.f)
	{
		A *= FMath::Clamp((Duration - Time) / FadeOut, 0.f, 1.f);
	}
	Build(CurHeight, CurOffset, A);
}

void AMythQuadHorn::Build(float CurHeight, float CurOffsetZ, float CurAlpha)
{
	// Mesma piramide do QuadHorn.js: ponta no alto, base quadrada de meia largura Bottom; a base fica 0,1 da altura abaixo
	// do chao (+offsetZ) e a ponta 1,9 acima, como o vertex shader do roBrowser. Inclinacao rotateX, giro rotateY.
	const float BaseZ = CurOffsetZ - 0.1f * CurHeight;
	const FVector Apex(0.f, 0.f, BaseZ + 2.f * CurHeight);
	const FVector B[4] = { FVector(-Bottom, Bottom, BaseZ), FVector(Bottom, Bottom, BaseZ), FVector(Bottom, -Bottom, BaseZ), FVector(-Bottom, -Bottom, BaseZ) };
	const FRotator Lean(Tilt.Pitch, 0.f, 0.f);
	TArray<FVector> V;
	TArray<int32> Tri;
	TArray<FVector2D> UV;
	TArray<FLinearColor> Col;
	static const FVector2D FaceUV[4][3] = { { {0,0},{1,0},{1,1} }, { {0,0},{1,1},{0,1} }, { {1,0},{1,1},{0,1} }, { {1,0},{0,1},{0,0} } };
	for (int32 f = 0; f < 4; ++f)
	{
		const FVector Pts[3] = { Apex, B[f], B[(f + 1) % 4] };
		for (int32 k = 0; k < 3; ++k)
		{
			Tri.Add(V.Num());
			V.Add(Lean.RotateVector(Pts[k]));
			UV.Add(FaceUV[f][k]);
			Col.Add(FLinearColor(1.f, 1.f, 1.f, CurAlpha));
		}
		// As duas faces (o material nao e de dois lados)
		const int32 Base = V.Num() - 3;
		Tri.Append({ Base, Base + 2, Base + 1 });
	}
	Mesh->CreateMeshSection_LinearColor(0, V, Tri, TArray<FVector>(), UV, Col, TArray<FProcMeshTangent>(), false);
	Mesh->SetMaterial(0, Material);
}

// ------------------------------------------------------------------------------------------------ circulo de conjuracao

AMythCastRing::AMythCastRing()
{
	PrimaryActorTick.bCanEverTick = true;
	Mesh = CreateDefaultSubobject<UProceduralMeshComponent>(TEXT("Mesh"));
	SetRootComponent(Mesh);
	Mesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	Mesh->SetCastShadow(false);
}

AMythCastRing* AMythCastRing::Spawn(UObject* WorldContext, bool bInLockOn, const FVector& InCenter, float InSizeCells, float InDurationSec, AActor* Follow)
{
	UWorld* World = WorldContext ? WorldContext->GetWorld() : nullptr;
	UTexture2D* Tex = FMythRoUITextures::Get(bInLockOn ? TEXT("../effect/lockon128.tga") : TEXT("../effect/magic_target.tga"));
	// O circulo magico tem fundo preto (soma de luz); o alvo travado tem canal alfa (translucido)
	UMaterialInterface* Additive = LoadObject<UMaterialInterface>(nullptr, bInLockOn ? TEXT("/Game/Myth/M_StrTranslucent.M_StrTranslucent") : TEXT("/Game/Myth/M_StrAdditive.M_StrAdditive"));
	if (!World || !Tex || !Additive)
	{
		return nullptr;
	}
	FActorSpawnParameters P;
	P.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
	AMythCastRing* R = World->SpawnActor<AMythCastRing>(InCenter, FRotator::ZeroRotator, P);
	if (!R)
	{
		return nullptr;
	}
	R->bLockOn = bInLockOn;
	R->Center = InCenter;
	R->SizeCells = FMath::Max(InSizeCells, 1.f);
	R->Duration = FMath::Max(InDurationSec, 0.2f);
	R->FollowActor = Follow;
	R->Material = UMaterialInstanceDynamic::Create(Additive, R);
	R->Material->SetTextureParameterValue(TEXT("Tex"), Tex);
	R->Material->SetScalarParameterValue(TEXT("Brightness"), bInLockOn ? 1.5f : 2.5f);
	R->Mesh->SetMaterial(0, R->Material);
	R->Rebuild(R->SizeCells * TrailCell * 0.5f, 0.f, 1.f);
	return R;
}

void AMythCastRing::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);
	Time += DeltaSeconds;
	if (Time >= Duration)
	{
		Destroy();
		return;
	}
	if (AActor* F = FollowActor.Get())
	{
		Center = F->GetActorLocation();
	}
	if (bLockOn)
	{
		// Fecha de 4 celulas ate o tamanho do alvo em 250 ms, depois gira (250 graus/s) e pisca
		const float Close = FMath::Clamp(Time / 0.25f, 0.f, 1.f);
		const float Half = (FMath::Lerp(5.f, 2.f, Close)) * TrailCell * 0.5f;
		const float Pulse = 0.55f + 0.45f * FMath::Abs(FMath::Cos(Time * 2.f * PI / 0.4f));
		Rebuild(Half, FMath::DegreesToRadians(Time * 250.f), Pulse);
	}
	else
	{
		Rebuild(SizeCells * TrailCell * 0.5f, FMath::DegreesToRadians(Time * 40.f), 1.f);
	}
}

void AMythCastRing::Rebuild(float HalfSizeUU, float AngleRad, float Alpha)
{
	// Grade colada ao terreno (uma subdivisao por meia celula); a textura gira dentro do quadrado (a rotacao fica nas UVs)
	const int32 Steps = FMath::Clamp(FMath::CeilToInt(HalfSizeUU * 2.f / (TrailCell * 0.5f)), 2, 28);
	TArray<FVector> V;
	TArray<FVector2D> UV;
	TArray<FLinearColor> Col;
	TArray<int32> Tri;
	SetActorLocation(FVector(Center.X, Center.Y, GetActorLocation().Z)); // antes de calcular a malha: os vertices sao relativos ao ator
	const float C = FMath::Cos(AngleRad), S = FMath::Sin(AngleRad);
	for (int32 iy = 0; iy <= Steps; ++iy)
	{
		for (int32 ix = 0; ix <= Steps; ++ix)
		{
			const float U = static_cast<float>(ix) / Steps, W = static_cast<float>(iy) / Steps;
			const FVector P(Center.X + (U - 0.5f) * 2.f * HalfSizeUU, Center.Y + (W - 0.5f) * 2.f * HalfSizeUU, Center.Z);
			const float Z = GroundAt(GetWorld(), P) + 14.f; // acima do chao renderizado (a colisao do .gat fica um pouco abaixo da malha visivel)
			V.Add(FVector(P.X, P.Y, Z) - GetActorLocation());
			const float DU = U - 0.5f, DW = W - 0.5f;
			UV.Add(FVector2D(0.5f + DU * C - DW * S, 0.5f + DU * S + DW * C));
			Col.Add(FLinearColor(1.f, 1.f, 1.f, Alpha));
		}
	}
	for (int32 iy = 0; iy < Steps; ++iy)
	{
		for (int32 ix = 0; ix < Steps; ++ix)
		{
			const int32 A = iy * (Steps + 1) + ix;
			Tri.Append({ A, A + Steps + 1, A + 1, A + 1, A + Steps + 1, A + Steps + 2 });
			Tri.Append({ A, A + 1, A + Steps + 1, A + 1, A + Steps + 2, A + Steps + 1 }); // verso (material de uma face so)
		}
	}
	Mesh->CreateMeshSection_LinearColor(0, V, Tri, TArray<FVector>(), UV, Col, TArray<FProcMeshTangent>(), false);
	Mesh->SetMaterial(0, Material); // depois de criar a secao: antes dela o slot nao existe e o material se perde
}

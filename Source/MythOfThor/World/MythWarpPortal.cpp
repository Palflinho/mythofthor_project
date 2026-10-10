#include "World/MythWarpPortal.h"

#include "UI/MythRoUITextures.h"

#include "Components/PointLightComponent.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "ProceduralMeshComponent.h"

namespace
{
	constexpr int32 RingSegments = 24;
}

AMythWarpPortal::AMythWarpPortal()
{
	PrimaryActorTick.bCanEverTick = true;
	Mesh = CreateDefaultSubobject<UProceduralMeshComponent>(TEXT("Mesh"));
	SetRootComponent(Mesh);
	Mesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	Mesh->SetCastShadow(false);

	Light = CreateDefaultSubobject<UPointLightComponent>(TEXT("Light"));
	Light->SetupAttachment(Mesh);
	Light->SetRelativeLocation(FVector(0.f, 0.f, 50.f));
	Light->SetIntensityUnits(ELightUnits::Candelas);
	Light->SetIntensity(18.f);
	Light->SetLightColor(FLinearColor(0.35f, 0.6f, 1.f));
	Light->SetAttenuationRadius(320.f);
	Light->SetCastShadows(false); // varios portais por mapa: sem sombra, so o brilho no chao
}

void AMythWarpPortal::BeginPlay()
{
	Super::BeginPlay();
	if (UMaterialInterface* Additive = LoadObject<UMaterialInterface>(nullptr, TEXT("/Game/Myth/M_StrAdditive.M_StrAdditive")))
	{
		RingMaterial = UMaterialInstanceDynamic::Create(Additive, this);
		RingMaterial->SetTextureParameterValue(TEXT("Tex"), FMythRoUITextures::Get(TEXT("../effect/ring_blue.tga")));
		RingMaterial->SetScalarParameterValue(TEXT("Brightness"), 4.f);
	}
	BuildRings();
}

void AMythWarpPortal::BuildRings()
{
	// Cada anel e uma faixa cilindrica (U da textura em volta, V na altura); a altura/alpha muda a cada quadro
	for (int32 r = 0; r < RingCount; ++r)
	{
		const float Phase = static_cast<float>(r) / RingCount;
		const float T = FMath::Fmod(Time / CycleSeconds + Phase, 1.f);
		const float Z = T * RiseHeight;
		const float Rad = Radius * (1.f + 0.25f * T);
		const float Alpha = FMath::Sin(T * PI); // nasce e some suave
		const float Spin = Time * 40.f + r * 37.f;
		TArray<FVector> Verts;
		TArray<FVector2D> UVs;
		TArray<FColor> Colors;
		TArray<int32> Tris;
		const FColor C = FLinearColor(1.f, 1.f, 1.f, Alpha).ToFColor(false);
		for (int32 s = 0; s <= RingSegments; ++s)
		{
			const float A = FMath::DegreesToRadians(Spin + 360.f * s / RingSegments);
			const FVector Dir(FMath::Cos(A), FMath::Sin(A), 0.f);
			Verts.Add(Dir * Rad + FVector(0.f, 0.f, Z));
			Verts.Add(Dir * Rad + FVector(0.f, 0.f, Z + BandHeight));
			UVs.Add(FVector2D(2.f * s / RingSegments, 1.f));
			UVs.Add(FVector2D(2.f * s / RingSegments, 0.f));
			Colors.Add(C);
			Colors.Add(C);
			if (s < RingSegments)
			{
				const int32 i = s * 2;
				Tris.Append({ i, i + 2, i + 1, i + 1, i + 2, i + 3 });
			}
		}
		if (!bCreated)
		{
			Mesh->CreateMeshSection(r, Verts, Tris, TArray<FVector>(), UVs, Colors, TArray<FProcMeshTangent>(), false);
			if (RingMaterial)
			{
				Mesh->SetMaterial(r, RingMaterial); // material dos efeitos: aditivo e dos dois lados
			}
		}
		else
		{
			Mesh->UpdateMeshSection(r, Verts, TArray<FVector>(), UVs, Colors, TArray<FProcMeshTangent>());
		}
	}
	bCreated = true;
}

void AMythWarpPortal::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);
	Time += DeltaSeconds;
	BuildRings();
	Light->SetIntensity(14.f + 6.f * FMath::Sin(Time * 3.f)); // pulso leve
}

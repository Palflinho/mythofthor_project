#include "Sprites/MythSpiritBalls.h"

#include "UI/MythRoUITextures.h"

#include "Camera/PlayerCameraManager.h"
#include "Components/CapsuleComponent.h"
#include "Engine/World.h"
#include "GameFramework/Character.h"
#include "GameFramework/PlayerController.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "ProceduralMeshComponent.h"

namespace
{
	constexpr float SbCell = 50.f;          // 1 unidade do RO = 1 celula = 50 uu
	constexpr float SbOrbitRadius = 1.f * SbCell;
	constexpr float SbOrbitHeight = 2.f * SbCell;
	constexpr float SbDegreesPerSecond = 1000.f / 64.f; // o cliente soma tick/64 graus
	FColor SbRgba(float R, float G, float B, float A) { return FLinearColor(R, G, B, A).ToFColor(false); }
}

AMythSpiritBalls::AMythSpiritBalls()
{
	PrimaryActorTick.bCanEverTick = true;
	USceneComponent* Root = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
	SetRootComponent(Root);
	Mesh = CreateDefaultSubobject<UProceduralMeshComponent>(TEXT("Mesh"));
	Mesh->SetupAttachment(Root);
	Mesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	Mesh->SetCastShadow(false);
}

void AMythSpiritBalls::Ensure(AActor* Owner, int32 NewCount)
{
	if (!Owner || !Owner->GetWorld())
	{
		return;
	}
	TArray<AActor*> Attached;
	Owner->GetAttachedActors(Attached);
	AMythSpiritBalls* Existing = nullptr;
	for (AActor* A : Attached)
	{
		if (AMythSpiritBalls* B = Cast<AMythSpiritBalls>(A))
		{
			Existing = B;
			break;
		}
	}
	if (NewCount <= 0)
	{
		if (Existing)
		{
			Existing->Destroy();
		}
		return;
	}
	if (Existing)
	{
		Existing->Count = FMath::Min(NewCount, 15);
		return;
	}
	float HalfHeight = 90.f;
	if (const ACharacter* Char = Cast<ACharacter>(Owner); Char && Char->GetCapsuleComponent())
	{
		HalfHeight = Char->GetCapsuleComponent()->GetScaledCapsuleHalfHeight();
	}
	FActorSpawnParameters P;
	P.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
	if (AMythSpiritBalls* Balls = Owner->GetWorld()->SpawnActor<AMythSpiritBalls>(Owner->GetActorLocation() - FVector(0.f, 0.f, HalfHeight), FRotator::ZeroRotator, P))
	{
		Balls->AttachToActor(Owner, FAttachmentTransformRules::KeepWorldTransform);
		Balls->Count = FMath::Min(NewCount, 15);
		UTexture2D* Tex = FMythRoUITextures::Get(TEXT("../effect/thunder_center.bmp"));
		UMaterialInterface* Additive = LoadObject<UMaterialInterface>(nullptr, TEXT("/Game/Myth/M_StrAdditive.M_StrAdditive"));
		if (Tex && Additive)
		{
			Balls->Material = UMaterialInstanceDynamic::Create(Additive, Balls);
			Balls->Material->SetTextureParameterValue(TEXT("Tex"), Tex);
			Balls->Material->SetScalarParameterValue(TEXT("Brightness"), 3.f);
		}
	}
}

void AMythSpiritBalls::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);
	Age += DeltaSeconds;
	Fade = FMath::Min(Fade + DeltaSeconds * 0.5f, 1.f); // o brilho entra em ~2 s (alpha +0,005 por quadro)
	FVector Right = FVector::RightVector;
	FVector Up = FVector::UpVector;
	if (const APlayerController* PC = GetWorld() ? GetWorld()->GetFirstPlayerController() : nullptr; PC && PC->PlayerCameraManager)
	{
		const FRotationMatrix M(PC->PlayerCameraManager->GetCameraRotation());
		Right = M.GetUnitAxis(EAxis::Y);
		Up = M.GetUnitAxis(EAxis::Z);
	}
	TArray<FVector> Verts;
	TArray<FVector2D> UVs;
	TArray<FColor> Colors;
	TArray<int32> Tris;
	auto Quad = [&](const FVector& Center, float HalfSize, const FColor& C)
	{
		const int32 i = Verts.Num();
		const FVector HX = Right * HalfSize, HY = Up * HalfSize;
		Verts.Append({ Center - HX + HY, Center + HX + HY, Center - HX - HY, Center + HX - HY });
		UVs.Append({ FVector2D(0.f, 0.f), FVector2D(1.f, 0.f), FVector2D(0.f, 1.f), FVector2D(1.f, 1.f) });
		Colors.Append({ C, C, C, C });
		Tris.Append({ i, i + 1, i + 2, i + 1, i + 3, i + 2, i, i + 2, i + 1, i + 1, i + 2, i + 3 });
	};
	// Como o cliente: i de Count ate 1; a esfera i fica no angulo (i mod 5) * 72 graus; i > 10 so halo largo, i > 5 halo medio
	for (int32 i = Count; i > 0; --i)
	{
		const float Angle = FMath::DegreesToRadians((i % 5) * 72.f - Age * SbDegreesPerSecond);
		const FVector Center(FMath::Cos(Angle) * SbOrbitRadius, FMath::Sin(Angle) * SbOrbitRadius, SbOrbitHeight);
		if (i > 10)
		{
			Quad(Center, 0.55f * SbCell, SbRgba(0.f, 0.f, 1.f, 0.2f * Fade));
		}
		else if (i > 5)
		{
			Quad(Center, 0.35f * SbCell, SbRgba(0.f, 0.f, 1.f, 0.6f * Fade));
		}
		else
		{
			Quad(Center, 0.25f * SbCell, SbRgba(0.f, 0.f, 1.f, 1.f * Fade));
			Quad(Center, 0.15f * SbCell, SbRgba(0.8f, 0.8f, 1.f, 1.f * Fade));
		}
	}
	Mesh->ClearMeshSection(0);
	if (!Verts.IsEmpty())
	{
		Mesh->CreateMeshSection(0, Verts, Tris, TArray<FVector>(), UVs, Colors, TArray<FProcMeshTangent>(), false);
		if (Material) { Mesh->SetMaterial(0, Material); }
	}
}

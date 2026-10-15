#include "Sprites/MythAura99.h"

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
	int32 GAuraMode = 2;           // 0 desligada, 1 so particulas, 2 completa (docs/ref/13 2.3)
	constexpr float Un = 10.f;     // 1 unidade do RO = 10 uu (celula = 5 unidades = 50 uu)
	constexpr float TickSeconds = 0.025f; // o cliente atualiza o efeito a cada 25 ms
	constexpr int32 Slices = 21;   // subdivisoes de cada faixa espiral
	constexpr float ArcDegrees = 315.f;

	FColor Rgba(float R, float G, float B, float A) { return FLinearColor(R, G, B, A).ToFColor(false); }
}

AMythAura99::AMythAura99()
{
	PrimaryActorTick.bCanEverTick = true;
	USceneComponent* Root = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
	SetRootComponent(Root);
	auto MakeMesh = [this, Root](const TCHAR* Name)
	{
		UProceduralMeshComponent* M = CreateDefaultSubobject<UProceduralMeshComponent>(Name);
		M->SetupAttachment(Root);
		M->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		M->SetCastShadow(false);
		return M;
	};
	RibbonMesh = MakeMesh(TEXT("Ribbons"));
	GroundMesh = MakeMesh(TEXT("Ground"));
	BubbleMesh = MakeMesh(TEXT("Bubbles"));
}

int32 AMythAura99::GetMode() { return GAuraMode; }
void AMythAura99::SetMode(int32 NewMode) { GAuraMode = FMath::Clamp(NewMode, 0, 2); }

UMaterialInstanceDynamic* AMythAura99::MakeMaterial(UObject* Outer, const FString& TextureFile, float Brightness)
{
	UTexture2D* Tex = FMythRoUITextures::Get(TEXT("../effect/") + TextureFile + TEXT(".tga"));
	if (!Tex)
	{
		Tex = FMythRoUITextures::Get(TEXT("../effect/") + TextureFile + TEXT(".bmp"));
	}
	UMaterialInterface* Additive = LoadObject<UMaterialInterface>(nullptr, TEXT("/Game/Myth/M_StrAdditive.M_StrAdditive"));
	if (!Tex || !Additive)
	{
		return nullptr;
	}
	UMaterialInstanceDynamic* M = UMaterialInstanceDynamic::Create(Additive, Outer);
	M->SetTextureParameterValue(TEXT("Tex"), Tex);
	M->SetScalarParameterValue(TEXT("Brightness"), Brightness);
	return M;
}

void AMythAura99::Ensure(AActor* Owner, bool bWanted)
{
	if (!Owner || !Owner->GetWorld())
	{
		return;
	}
	TArray<AActor*> Attached;
	Owner->GetAttachedActors(Attached);
	AMythAura99* Existing = nullptr;
	for (AActor* A : Attached)
	{
		if (AMythAura99* Aura = Cast<AMythAura99>(A))
		{
			Existing = Aura;
			break;
		}
	}
	if (!bWanted)
	{
		if (Existing)
		{
			Existing->Destroy();
		}
		return;
	}
	if (Existing)
	{
		return;
	}
	float HalfHeight = 90.f;
	if (const ACharacter* Char = Cast<ACharacter>(Owner); Char && Char->GetCapsuleComponent())
	{
		HalfHeight = Char->GetCapsuleComponent()->GetScaledCapsuleHalfHeight();
	}
	FActorSpawnParameters P;
	P.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
	if (AMythAura99* Aura = Owner->GetWorld()->SpawnActor<AMythAura99>(Owner->GetActorLocation() - FVector(0.f, 0.f, HalfHeight), FRotator::ZeroRotator, P))
	{
		Aura->AttachToActor(Owner, FAttachmentTransformRules::KeepWorldTransform);
		Aura->RibbonMat = MakeMaterial(Aura, TEXT("ring_blue"), 6.f);
		Aura->GroundMat = MakeMaterial(Aura, TEXT("pikapika2"), 1.2f);
		Aura->BubbleMat = MakeMaterial(Aura, TEXT("whitelight"), 5.f);
		for (int32 Column = 0; Column < 4; ++Column)
		{
			for (int32 Anchor = 0; Anchor < 4; ++Anchor)
			{
				FBubble& B = Aura->Bubbles.AddDefaulted_GetRef();
				// Quatro ancoras por coluna com os sinais de deriva do cliente (+,+) (-,-) (+,-) (-,+)
				B.SignX = (Anchor == 0 || Anchor == 2) ? 1.f : -1.f;
				B.SignZ = (Anchor == 0 || Anchor == 3) ? 1.f : -1.f;
				B.Column = Column * 90.f;
				B.Height = FMath::FRandRange(0.f, 100.f);
				B.PhaseA = B.TargetA = FMath::FRandRange(0.f, 360.f);
				B.PhaseB = B.TargetB = FMath::FRandRange(0.f, 360.f);
			}
		}
	}
}

void AMythAura99::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);
	Age += DeltaSeconds;
	const int32 Mode = GAuraMode;
	RibbonMesh->SetVisibility(Mode >= 2);
	GroundMesh->SetVisibility(Mode >= 2);
	BubbleMesh->SetVisibility(Mode >= 1);

	FVector Right = FVector::RightVector;
	FVector Up = FVector::UpVector;
	if (const APlayerController* PC = GetWorld() ? GetWorld()->GetFirstPlayerController() : nullptr; PC && PC->PlayerCameraManager)
	{
		const FRotationMatrix M(PC->PlayerCameraManager->GetCameraRotation());
		Right = M.GetUnitAxis(EAxis::Y);
		Up = M.GetUnitAxis(EAxis::Z);
	}
	if (Mode >= 2)
	{
		BuildRibbons(Right, Up);
		BuildGround();
	}
	if (Mode >= 1)
	{
		BuildBubbles(Right, Up);
	}
}

void AMythAura99::BuildRibbons(const FVector&, const FVector&)
{
	// EF_LEVEL99: 3 faixas de 21 subdivisoes e 315 graus, em cone aberto para fora (SwirlingAura)
	TArray<FVector> Verts;
	TArray<FVector2D> UVs;
	TArray<FColor> Colors;
	TArray<int32> Tris;
	const float Ticks = Age / TickSeconds;
	const float Process = FMath::Min(Ticks, 90.f);
	const float RiseGrow = FMath::Sin(FMath::DegreesToRadians(Process)); // subida inicial de 2,25 s
	const FColor C = Rgba(0.392f, 0.392f, 1.f, 0.4706f);
	for (int32 Band = 0; Band < 3; ++Band)
	{
		const float RotStart = Band * 90.f;
		const float MaxHeight = (15.f - 2.f * Band) * Un;
		const float Distance = (3.9f + 0.2f * Band) * 0.6f * Un;
		const float RiseAngle = FMath::DegreesToRadians(55.f - 5.f * Band);
		const float Spin = Ticks * (Band + 3.f); // graus por tick de 25 ms
		const int32 First = Verts.Num();
		for (int32 k = 0; k < Slices; ++k)
		{
			const float Theta = FMath::DegreesToRadians(RotStart + Spin + k * (ArcDegrees / (Slices - 1)));
			const float SinLimit = FMath::DegreesToRadians(90.f + (k - 10) * 9.f);
			const float H = FMath::Max(0.f, MaxHeight * FMath::Sin(SinLimit) * RiseGrow);
			const float Rx = FMath::Cos(RiseAngle) * H;
			const float Rz = FMath::Sin(RiseAngle) * H;
			Verts.Add(FVector(Distance * FMath::Cos(Theta), Distance * FMath::Sin(Theta), 0.f));
			Verts.Add(FVector((Distance + Rx) * FMath::Cos(Theta), (Distance + Rx) * FMath::Sin(Theta), Rz));
			UVs.Add(FVector2D(static_cast<float>(k) / (Slices - 1), 1.f));
			UVs.Add(FVector2D(static_cast<float>(k) / (Slices - 1), 0.f));
			Colors.Add(C);
			Colors.Add(C);
			if (k < Slices - 1)
			{
				const int32 i = First + k * 2;
				Tris.Append({ i, i + 2, i + 1, i + 1, i + 2, i + 3, i, i + 1, i + 2, i + 1, i + 3, i + 2 }); // dos dois lados
			}
		}
	}
	if (RibbonMesh->GetNumSections() == 0)
	{
		RibbonMesh->CreateMeshSection(0, Verts, Tris, TArray<FVector>(), UVs, Colors, TArray<FProcMeshTangent>(), false);
		if (RibbonMat) { RibbonMesh->SetMaterial(0, RibbonMat); }
	}
	else
	{
		RibbonMesh->UpdateMeshSection(0, Verts, TArray<FVector>(), UVs, Colors, TArray<FProcMeshTangent>());
	}
}

void AMythAura99::BuildGround()
{
	// EF_LEVEL99_2: dois quads de chao que respiram em oposicao (115..130), o segundo girado 23 graus (GroundAura)
	TArray<FVector> Verts;
	TArray<FVector2D> UVs;
	TArray<FColor> Colors;
	TArray<int32> Tris;
	const float PhaseRad = FMath::DegreesToRadians(Age / TickSeconds * 3.f); // +3 graus por tick: ciclo de 3 s
	const float Breath = FMath::Cos(PhaseRad);                              // -1..1
	const float Mid = 122.5f, Amp = 7.5f;
	const float UnitToUU = 1.4f;                                            // tamanho do cliente (115..130) para uu
	const FColor C = Rgba(1.f, 1.f, 1.f, 0.8f);
	for (int32 Ring = 0; Ring < 2; ++Ring)
	{
		const float Size = (Mid + (Ring == 0 ? -Amp : Amp) * Breath) * UnitToUU * 0.5f;
		const float Rot = FMath::DegreesToRadians(Ring == 0 ? 0.f : 23.f);
		const float Cs = FMath::Cos(Rot), Sn = FMath::Sin(Rot);
		const int32 i = Verts.Num();
		const FVector2D Corners[4] = { {-1.f, -1.f}, {1.f, -1.f}, {-1.f, 1.f}, {1.f, 1.f} };
		for (const FVector2D& Corner : Corners)
		{
			const float X = Corner.X * Size, Y = Corner.Y * Size;
			Verts.Add(FVector(X * Cs - Y * Sn, X * Sn + Y * Cs, 8.f)); // +8 cm acima do chao (sem Z-fighting)
			UVs.Add(FVector2D((Corner.X + 1.f) * 0.5f, (Corner.Y + 1.f) * 0.5f));
			Colors.Add(C);
		}
		Tris.Append({ i, i + 1, i + 2, i + 1, i + 3, i + 2, i, i + 2, i + 1, i + 1, i + 2, i + 3 });
	}
	if (GroundMesh->GetNumSections() == 0)
	{
		GroundMesh->CreateMeshSection(0, Verts, Tris, TArray<FVector>(), UVs, Colors, TArray<FProcMeshTangent>(), false);
		if (GroundMat) { GroundMesh->SetMaterial(0, GroundMat); }
	}
	else
	{
		GroundMesh->UpdateMeshSection(0, Verts, TArray<FVector>(), UVs, Colors, TArray<FProcMeshTangent>());
	}
}

void AMythAura99::BuildBubbles(const FVector& Right, const FVector& Up)
{
	// EF_LEVEL99_3: 4 colunas x 4 ancoras = 16 brilhos que sobem balancando e renascem na base (Level99Bubble)
	TArray<FVector> Verts;
	TArray<FVector2D> UVs;
	TArray<FColor> Colors;
	TArray<int32> Tris;
	const float Dt = FMath::Max(GetWorld() ? GetWorld()->GetDeltaSeconds() : 0.016f, 0.0001f);
	constexpr float Rise = 70.f;       // uu/s (0,15 un por tick de 25 ms, ~1 cm/quadro)
	constexpr float TopHeight = 195.f; // acima da cabeca
	constexpr float Drift = 6.f;       // uu (K_drift 0,15 un)
	constexpr float Radius = 9.f;      // meia-largura do brilho em uu
	for (FBubble& B : Bubbles)
	{
		B.Height += Rise * Dt;
		if (B.Height > TopHeight)
		{
			B.Height = FMath::FRandRange(0.f, 100.f); // renasce na base com fases novas
			B.TargetA = FMath::FRandRange(0.f, 360.f);
			B.TargetB = FMath::FRandRange(0.f, 360.f);
		}
		B.PhaseA = FMath::FInterpConstantTo(B.PhaseA, B.TargetA, Dt, 100.f);
		B.PhaseB = FMath::FInterpConstantTo(B.PhaseB, B.TargetB, Dt, 100.f);
		const float ColRad = FMath::DegreesToRadians(B.Column);
		const FVector Center(FMath::Cos(ColRad) * 35.f + B.SignX * Drift * FMath::Sin(FMath::DegreesToRadians(B.PhaseA)),
			FMath::Sin(ColRad) * 35.f + B.SignZ * Drift * FMath::Sin(FMath::DegreesToRadians(B.PhaseB)), B.Height);
		// Fade-in na base, pleno no tronco, fade-out na cabeca
		const float Alpha = FMath::Clamp(B.Height / 25.f, 0.f, 1.f) * FMath::Clamp((TopHeight - B.Height) / 55.f, 0.f, 1.f) * (250.f / 255.f);
		const FColor C = Rgba(0.314f, 0.314f, 1.f, Alpha);
		const int32 i = Verts.Num();
		const FVector HX = Right * Radius, HY = Up * Radius;
		Verts.Append({ Center - HX + HY, Center + HX + HY, Center - HX - HY, Center + HX - HY });
		UVs.Append({ FVector2D(0.f, 0.f), FVector2D(1.f, 0.f), FVector2D(0.f, 1.f), FVector2D(1.f, 1.f) });
		Colors.Append({ C, C, C, C });
		Tris.Append({ i, i + 1, i + 2, i + 1, i + 3, i + 2 });
	}
	if (BubbleMesh->GetNumSections() == 0)
	{
		BubbleMesh->CreateMeshSection(0, Verts, Tris, TArray<FVector>(), UVs, Colors, TArray<FProcMeshTangent>(), false);
		if (BubbleMat) { BubbleMesh->SetMaterial(0, BubbleMat); }
	}
	else
	{
		BubbleMesh->UpdateMeshSection(0, Verts, TArray<FVector>(), UVs, Colors, TArray<FProcMeshTangent>());
	}
}

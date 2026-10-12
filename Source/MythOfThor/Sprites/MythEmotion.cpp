#include "Sprites/MythEmotion.h"

#include "Sprites/MythRoSpriteComponent.h"
#include "Sprites/RoSpriteFormats.h"

#include "Camera/PlayerCameraManager.h"
#include "Components/CapsuleComponent.h"
#include "Engine/Texture2D.h"
#include "Engine/World.h"
#include "GameFramework/Character.h"
#include "GameFramework/PlayerController.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Misc/FileHelper.h"
#include "ProceduralMeshComponent.h"

namespace
{
	constexpr float FrameSeconds = 0.05f;   // 50 ms por quadro (emotion.act)
	constexpr float TotalSeconds = 2.5f;    // docs/ref/18 2.3
	constexpr int32 MaxLayers = 8;

	struct FEmotionData
	{
		FRoSpr Spr;
		FRoAct Act;
		bool bLoaded = false;
		bool bOk = false;
		TMap<int32, TObjectPtr<UTexture2D>> Textures; // imagem do .spr -> textura (mantida viva)
	};

	FEmotionData& EmotionAssets()
	{
		static FEmotionData D;
		if (!D.bLoaded)
		{
			D.bLoaded = true;
			const FString Base = FPaths::Combine(UMythRoSpriteComponent::GetAssetsRoot(), TEXT("data"), TEXT("sprite"), TEXT("이팩트"), TEXT("emotion"));
			TArray<uint8> SprBytes, ActBytes;
			D.bOk = FFileHelper::LoadFileToArray(SprBytes, *(Base + TEXT(".spr"))) && FFileHelper::LoadFileToArray(ActBytes, *(Base + TEXT(".act")))
				&& D.Spr.Load(SprBytes) && D.Act.Load(ActBytes);
		}
		return D;
	}

	UTexture2D* TextureFor(FEmotionData& D, int32 SprIndex, FIntPoint& OutSize)
	{
		const FRoSprImage* Img = D.Spr.Find(SprIndex, 0);
		if (!Img || Img->Width <= 0 || Img->Height <= 0)
		{
			return nullptr;
		}
		OutSize = FIntPoint(Img->Width, Img->Height);
		if (TObjectPtr<UTexture2D>* Found = D.Textures.Find(SprIndex))
		{
			return Found->Get();
		}
		UTexture2D* Tex = UTexture2D::CreateTransient(Img->Width, Img->Height, PF_B8G8R8A8);
		if (!Tex)
		{
			return nullptr;
		}
		Tex->AddToRoot(); // cache de processo: as emocoes se repetem
		Tex->Filter = TF_Bilinear;
		Tex->SRGB = true;
		Tex->NeverStream = true;
		FTexture2DMipMap& Mip = Tex->GetPlatformData()->Mips[0];
		void* Dest = Mip.BulkData.Lock(LOCK_READ_WRITE);
		FMemory::Memcpy(Dest, Img->Pixels.GetData(), Img->Pixels.Num() * sizeof(FColor));
		Mip.BulkData.Unlock();
		Tex->UpdateResource();
		D.Textures.Add(SprIndex, Tex);
		return Tex;
	}
}

AMythEmotion::AMythEmotion()
{
	PrimaryActorTick.bCanEverTick = true;
	USceneComponent* Root = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
	SetRootComponent(Root);
	Mesh = CreateDefaultSubobject<UProceduralMeshComponent>(TEXT("Mesh"));
	Mesh->SetupAttachment(Root);
	Mesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	Mesh->SetCastShadow(false);
}

int32 AMythEmotion::TypeFromOrder(int32 Order)
{
	static const int32 Table[65] = { -1, 0, 1, 2, 3, 14, 4, 5, 6, 7, 8, 9, 11, 10, 12, 15, 16, 17, 18, 19, 20, 21, 22, 23, 24, 25, 26,
		27, 28, 29, 30, 31, 32, 33, 36, 37, 38, 39, 40, 41, 42, 43, 44, 45, 46, 47, 52, 53, 54, 55, 56, 57, 65, 68, 69, 71, 72, 73, 74,
		75, 76, 77, 78, 79, 80 };
	return Order >= 1 && Order <= 64 ? Table[Order] : -1;
}

UTexture2D* AMythEmotion::IconFor(int32 Type)
{
	static TMap<int32, TObjectPtr<UTexture2D>> Icons;
	if (TObjectPtr<UTexture2D>* Found = Icons.Find(Type))
	{
		return Found->Get();
	}
	FEmotionData& D = EmotionAssets();
	if (!D.bOk || !D.Act.Actions.IsValidIndex(Type) || D.Act.Actions[Type].Frames.Num() == 0)
	{
		Icons.Add(Type, nullptr);
		return nullptr;
	}
	const FRoActAction& Act = D.Act.Actions[Type];
	const FRoActFrame& Frame = Act.Frames[FMath::Min(Act.Frames.Num() - 1, Act.Frames.Num() / 2)];
	constexpr int32 N = 48;
	TArray<FColor> Canvas;
	Canvas.Init(FColor(0, 0, 0, 0), N * N);
	for (const FRoActLayer& L : Frame.Layers)
	{
		const FRoSprImage* Img = L.SprIndex >= 0 ? D.Spr.Find(L.SprIndex, L.SprType) : nullptr;
		if (!Img || Img->Width <= 0)
		{
			continue;
		}
		const int32 X0 = N / 2 + L.Offset.X - Img->Width / 2;
		const int32 Y0 = N / 2 + L.Offset.Y - Img->Height / 2;
		for (int32 y = 0; y < Img->Height; ++y)
		{
			for (int32 x = 0; x < Img->Width; ++x)
			{
				const int32 Dx = X0 + (L.bMirror ? Img->Width - 1 - x : x);
				const int32 Dy = Y0 + y;
				const FColor S = Img->Pixels[y * Img->Width + x];
				if (S.A == 0 || Dx < 0 || Dy < 0 || Dx >= N || Dy >= N)
				{
					continue;
				}
				Canvas[Dy * N + Dx] = S;
			}
		}
	}
	UTexture2D* Tex = UTexture2D::CreateTransient(N, N, PF_B8G8R8A8);
	if (!Tex)
	{
		return nullptr;
	}
	Tex->AddToRoot();
	Tex->Filter = TF_Nearest;
	Tex->SRGB = true;
	Tex->NeverStream = true;
	FTexture2DMipMap& Mip = Tex->GetPlatformData()->Mips[0];
	FMemory::Memcpy(Mip.BulkData.Lock(LOCK_READ_WRITE), Canvas.GetData(), Canvas.Num() * sizeof(FColor));
	Mip.BulkData.Unlock();
	Tex->UpdateResource();
	Icons.Add(Type, Tex);
	return Tex;
}

void AMythEmotion::Show(AActor* Owner, int32 Type)
{
	FEmotionData& D = EmotionAssets();
	if (!Owner || !Owner->GetWorld() || !D.bOk || !D.Act.Actions.IsValidIndex(Type) || D.Act.Actions[Type].Frames.Num() == 0)
	{
		return;
	}
	TArray<AActor*> Attached;
	Owner->GetAttachedActors(Attached);
	for (AActor* A : Attached)
	{
		if (Cast<AMythEmotion>(A))
		{
			A->Destroy(); // uma por vez
		}
	}
	float Half = 90.f;
	if (const ACharacter* Char = Cast<ACharacter>(Owner); Char && Char->GetCapsuleComponent())
	{
		Half = Char->GetCapsuleComponent()->GetScaledCapsuleHalfHeight();
	}
	FActorSpawnParameters P;
	P.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
	if (AMythEmotion* E = Owner->GetWorld()->SpawnActor<AMythEmotion>(Owner->GetActorLocation(), FRotator::ZeroRotator, P))
	{
		E->Action = Type;
		E->HeadHeight = Half * 0.9f + 38.f; // acima da cabeca (a origem do ator fica no centro da capsula)
		E->AttachToActor(Owner, FAttachmentTransformRules::KeepWorldTransform);
		UMaterialInterface* Base = LoadObject<UMaterialInterface>(nullptr, TEXT("/Paper2D/MaskedUnlitSpriteMaterial.MaskedUnlitSpriteMaterial"));
		for (int32 i = 0; i < MaxLayers && Base; ++i)
		{
			E->Materials.Add(UMaterialInstanceDynamic::Create(Base, E));
		}
	}
}

void AMythEmotion::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);
	Age += DeltaSeconds;
	if (Age >= TotalSeconds)
	{
		Destroy();
		return;
	}
	FEmotionData& D = EmotionAssets();
	const FRoActAction& Act = D.Act.Actions[Action];
	const int32 FrameIdx = FMath::Min(FMath::FloorToInt(Age / FrameSeconds), Act.Frames.Num() - 1);
	const FRoActFrame& Frame = Act.Frames[FrameIdx];

	FVector Right = FVector::RightVector;
	FVector Up = FVector::UpVector;
	FVector Toward = FVector::BackwardVector; // para a camera: camadas empilhadas nao brigam pela mesma profundidade
	if (const APlayerController* PC = GetWorld() ? GetWorld()->GetFirstPlayerController() : nullptr; PC && PC->PlayerCameraManager)
	{
		const FRotationMatrix M(PC->PlayerCameraManager->GetCameraRotation());
		Right = M.GetUnitAxis(EAxis::Y);
		Up = M.GetUnitAxis(EAxis::Z);
		Toward = -M.GetUnitAxis(EAxis::X);
	}
	// A emocao fica acima e um pouco a direita da cabeca
	const FVector Anchor = GetActorLocation();
	const FVector Base = Anchor + FVector(0.f, 0.f, HeadHeight) + Right * 28.f;
	const float UnitsPerPixel = 2.f;

	Mesh->ClearAllMeshSections();
	int32 Section = 0;
	for (const FRoActLayer& L : Frame.Layers)
	{
		if (Section >= MaxLayers || !Materials.IsValidIndex(Section))
		{
			break;
		}
		if (L.SprIndex < 0)
		{
			continue; // camada vazia: as seguintes ainda valem
		}
		FIntPoint Size;
		UTexture2D* Tex = TextureFor(D, L.SprIndex, Size);
		if (!Tex)
		{
			continue;
		}
		Materials[Section]->SetTextureParameterValue(TEXT("SpriteTexture"), Tex);
		const float HW = Size.X * 0.5f * UnitsPerPixel * L.Scale.X;
		const float HH = Size.Y * 0.5f * UnitsPerPixel * L.Scale.Y;
		const FVector Center = Base + Right * (L.Offset.X * UnitsPerPixel) - Up * (L.Offset.Y * UnitsPerPixel) + Toward * (Section * 1.5f) - GetActorLocation();
		const float Flip = L.bMirror ? -1.f : 1.f;
		TArray<FVector> Verts = { Center - Right * HW * Flip + Up * HH, Center + Right * HW * Flip + Up * HH, Center - Right * HW * Flip - Up * HH, Center + Right * HW * Flip - Up * HH };
		TArray<FVector2D> UVs = { FVector2D(0.f, 0.f), FVector2D(1.f, 0.f), FVector2D(0.f, 1.f), FVector2D(1.f, 1.f) };
		TArray<FColor> Colors = { FColor::White, FColor::White, FColor::White, FColor::White };
		TArray<int32> Tris = { 0, 1, 2, 1, 3, 2, 0, 2, 1, 1, 2, 3 };
		Mesh->CreateMeshSection(Section, Verts, Tris, TArray<FVector>(), UVs, Colors, TArray<FProcMeshTangent>(), false);
		Mesh->SetMaterial(Section, Materials[Section]);
		++Section;
	}
}

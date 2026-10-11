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
	constexpr int32 MaxLayers = 3;

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
	if (const APlayerController* PC = GetWorld() ? GetWorld()->GetFirstPlayerController() : nullptr; PC && PC->PlayerCameraManager)
	{
		const FRotationMatrix M(PC->PlayerCameraManager->GetCameraRotation());
		Right = M.GetUnitAxis(EAxis::Y);
		Up = M.GetUnitAxis(EAxis::Z);
	}
	// A emocao fica acima e um pouco a direita da cabeca
	const FVector Anchor = GetActorLocation();
	const FVector Base = Anchor + FVector(0.f, 0.f, HeadHeight) + Right * 28.f;
	const float UnitsPerPixel = 2.f;

	Mesh->ClearAllMeshSections();
	int32 Section = 0;
	for (const FRoActLayer& L : Frame.Layers)
	{
		if (Section >= MaxLayers || L.SprIndex < 0 || !Materials.IsValidIndex(Section))
		{
			break;
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
		const FVector Center = Base + Right * (L.Offset.X * UnitsPerPixel) - Up * (L.Offset.Y * UnitsPerPixel) - GetActorLocation();
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

#include "Sprites/MythSprEffect.h"

#include "Sprites/MythRoSpriteComponent.h"
#include "Sprites/MythStrEffect.h"
#include "Sprites/RoSpriteFormats.h"

#include "Camera/PlayerCameraManager.h"
#include "Components/CapsuleComponent.h"
#include "Engine/Texture2D.h"
#include "GameFramework/Character.h"
#include "Engine/World.h"
#include "GameFramework/PlayerController.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Misc/FileHelper.h"
#include "ProceduralMeshComponent.h"

namespace
{
	constexpr int32 SprMaxLayers = 32;

	struct FSprEffectData
	{
		FRoSpr Spr;
		FRoAct Act;
		bool bOk = false;
		TMap<int32, TObjectPtr<UTexture2D>> Textures;
		TMap<int32, bool> Opaque; // imagem sem nenhum pixel transparente: fundo preto = aditivo
	};

	TMap<FString, FSprEffectData>& SprEffectCache()
	{
		static TMap<FString, FSprEffectData> Cache;
		return Cache;
	}

	FString ResolveSprPath(const FString& File)
	{
		const FString Root = FPaths::Combine(UMythRoSpriteComponent::GetAssetsRoot(), TEXT("data"), TEXT("sprite"));
		const FString Candidates[] = { FPaths::Combine(Root, TEXT("이팩트"), File), FPaths::Combine(Root, File) };
		for (const FString& C : Candidates)
		{
			if (FPaths::FileExists(C + TEXT(".spr")) && FPaths::FileExists(C + TEXT(".act")))
			{
				return C;
			}
		}
		return FString();
	}

	FSprEffectData* LoadSprEffect(const FString& File)
	{
		if (FSprEffectData* Found = SprEffectCache().Find(File))
		{
			return Found->bOk ? Found : nullptr;
		}
		FSprEffectData& D = SprEffectCache().Add(File);
		const FString Base = ResolveSprPath(File);
		TArray<uint8> SprBytes, ActBytes;
		D.bOk = !Base.IsEmpty() && FFileHelper::LoadFileToArray(SprBytes, *(Base + TEXT(".spr"))) && FFileHelper::LoadFileToArray(ActBytes, *(Base + TEXT(".act")))
			&& D.Spr.Load(SprBytes) && D.Act.Load(ActBytes) && D.Act.Actions.Num() > 0 && D.Act.Actions[0].Frames.Num() > 0;
		return D.bOk ? &D : nullptr;
	}

	UTexture2D* SprEffectTexture(FSprEffectData& D, int32 Index, int32 Type, FIntPoint& OutSize)
	{
		const FRoSprImage* Img = D.Spr.Find(Index, Type);
		if (!Img || Img->Width <= 0 || Img->Height <= 0)
		{
			return nullptr;
		}
		OutSize = FIntPoint(Img->Width, Img->Height);
		const int32 CacheKey = Index * 2 + Type;
		if (TObjectPtr<UTexture2D>* Found = D.Textures.Find(CacheKey))
		{
			return Found->Get();
		}
		UTexture2D* Tex = UTexture2D::CreateTransient(Img->Width, Img->Height, PF_B8G8R8A8);
		if (!Tex)
		{
			return nullptr;
		}
		bool bAnyTransparent = false;
		for (const FColor& Px : Img->Pixels)
		{
			if (Px.A < 250) { bAnyTransparent = true; break; }
		}
		D.Opaque.Add(CacheKey, !bAnyTransparent);
		Tex->AddToRoot();
		Tex->Filter = TF_Bilinear;
		Tex->SRGB = true;
		Tex->NeverStream = true;
		FTexture2DMipMap& Mip = Tex->GetPlatformData()->Mips[0];
		FMemory::Memcpy(Mip.BulkData.Lock(LOCK_READ_WRITE), Img->Pixels.GetData(), Img->Pixels.Num() * sizeof(FColor));
		Mip.BulkData.Unlock();
		Tex->UpdateResource();
		D.Textures.Add(CacheKey, Tex);
		return Tex;
	}
}

AMythSprEffect::AMythSprEffect()
{
	PrimaryActorTick.bCanEverTick = true;
	USceneComponent* Root = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
	SetRootComponent(Root);
	Mesh = CreateDefaultSubobject<UProceduralMeshComponent>(TEXT("Mesh"));
	Mesh->SetupAttachment(Root);
	Mesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	Mesh->SetCastShadow(false);
}

AMythSprEffect* AMythSprEffect::Play(UObject* WorldContext, const FString& SprFile, const FVector& Location, AActor* Follow, bool bRepeat, float DurationSec)
{
	UWorld* World = WorldContext ? WorldContext->GetWorld() : nullptr;
	FSprEffectData* D = World && !SprFile.IsEmpty() ? LoadSprEffect(SprFile) : nullptr;
	if (!D)
	{
		return nullptr;
	}
	FActorSpawnParameters P;
	P.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
	AMythSprEffect* E = World->SpawnActor<AMythSprEffect>(Location, FRotator::ZeroRotator, P);
	if (!E)
	{
		return nullptr;
	}
	const FRoActAction& Action = D->Act.Actions[0];
	const float Cycle = FMath::Max(0.05f, Action.Frames.Num() * Action.IntervalMs / 1000.f);
	E->Key = SprFile;
	E->bLoop = bRepeat;
	E->Length = bRepeat ? (DurationSec > 0.f ? DurationSec : 2.f) : Cycle;
	if (Follow)
	{
		E->FollowActor = Follow;
	}
	UMaterialInterface* Translucent = LoadObject<UMaterialInterface>(nullptr, TEXT("/Game/Myth/M_StrTranslucent.M_StrTranslucent"));
	UMaterialInterface* Additive = LoadObject<UMaterialInterface>(nullptr, TEXT("/Game/Myth/M_StrAdditive.M_StrAdditive"));
	for (int32 i = 0; i < SprMaxLayers && Translucent && Additive; ++i)
	{
		UMaterialInstanceDynamic* T = UMaterialInstanceDynamic::Create(Translucent, E);
		T->SetScalarParameterValue(TEXT("Brightness"), 1.f);
		E->Materials.Add(T);
		UMaterialInstanceDynamic* A = UMaterialInstanceDynamic::Create(Additive, E);
		A->SetScalarParameterValue(TEXT("Brightness"), 3.f);
		E->AdditiveMaterials.Add(A);
	}
	return E;
}

AMythSprEffect* AMythSprEffect::Attach(AActor* Owner, const FString& SprFile, bool bAboveHead, float SideOffsetUU, float UpOffset)
{
	if (!Owner)
	{
		return nullptr;
	}
	AMythSprEffect* E = Play(Owner, SprFile, AMythStrEffect::FeetOf(Owner), Owner, true, 100000.f);
	if (E && bAboveHead)
	{
		float Half = 90.f;
		if (const ACharacter* Char = Cast<ACharacter>(Owner); Char && Char->GetCapsuleComponent())
		{
			Half = Char->GetCapsuleComponent()->GetScaledCapsuleHalfHeight();
		}
		E->HeightOffset = Half * 1.7f;
	}
	if (E)
	{
		E->HeightOffset += UpOffset;
		E->bMasked = SprFile == TEXT("매"); // falcao: sprite solido, nao brilho
		E->SideOffset = SideOffsetUU;
	}
	return E;
}

void AMythSprEffect::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);
	Age += DeltaSeconds;
	FSprEffectData* D = LoadSprEffect(Key);
	if (!D || Age >= Length)
	{
		Destroy();
		return;
	}
	if (FollowActor.IsValid())
	{
		FVector CamRight = FVector::RightVector;
		if (const APlayerController* SidePC = GetWorld() ? GetWorld()->GetFirstPlayerController() : nullptr; SidePC && SidePC->PlayerCameraManager)
		{
			CamRight = FRotationMatrix(SidePC->PlayerCameraManager->GetCameraRotation()).GetUnitAxis(EAxis::Y);
		}
		SetActorLocation(AMythStrEffect::FeetOf(FollowActor.Get()) + FVector(0.f, 0.f, HeightOffset) + CamRight * SideOffset);
	}
	const FRoActAction& Act = D->Act.Actions[0];
	const int32 Count = Act.Frames.Num();
	const int32 Step = FMath::FloorToInt(Age * 1000.f / FMath::Max(Act.IntervalMs, 1.f));
	const FRoActFrame& Frame = Act.Frames[bLoop ? Step % Count : FMath::Min(Step, Count - 1)];

	FVector Right = FVector::RightVector;
	FVector Up = FVector::UpVector;
	if (const APlayerController* PC = GetWorld() ? GetWorld()->GetFirstPlayerController() : nullptr; PC && PC->PlayerCameraManager)
	{
		const FRotationMatrix M(PC->PlayerCameraManager->GetCameraRotation());
		Right = M.GetUnitAxis(EAxis::Y);
		Up = M.GetUnitAxis(EAxis::Z);
	}
	const float UnitsPerPixel = 2.f;
	Mesh->ClearAllMeshSections();
	int32 Section = 0;
	for (const FRoActLayer& L : Frame.Layers)
	{
		if (Section >= SprMaxLayers || L.SprIndex < 0 || !Materials.IsValidIndex(Section))
		{
			continue;
		}
		FIntPoint Size;
		UTexture2D* Tex = SprEffectTexture(*D, L.SprIndex, L.SprType, Size);
		if (!Tex)
		{
			continue;
		}
		UMaterialInstanceDynamic* Mat = nullptr;
		if (bMasked)
		{
			if (MaskedMaterials.Num() <= Section)
			{
				if (UMaterialInterface* MaskBase = LoadObject<UMaterialInterface>(nullptr, TEXT("/Paper2D/MaskedUnlitSpriteMaterial.MaskedUnlitSpriteMaterial")))
				{
					MaskedMaterials.Add(UMaterialInstanceDynamic::Create(MaskBase, this));
				}
			}
			if (MaskedMaterials.IsValidIndex(Section))
			{
				Mat = MaskedMaterials[Section];
				Mat->SetTextureParameterValue(TEXT("SpriteTexture"), Tex);
			}
		}
		if (!Mat)
		{
			const bool bAdd = D->Opaque.FindRef(L.SprIndex * 2 + L.SprType);
			Mat = bAdd ? AdditiveMaterials[Section] : Materials[Section];
			Mat->SetTextureParameterValue(TEXT("Tex"), Tex);
		}
		const float HW = Size.X * 0.5f * UnitsPerPixel * L.Scale.X;
		const float HH = Size.Y * 0.5f * UnitsPerPixel * L.Scale.Y;
		// Origem do .act = pes; o centro da imagem fica em Offset (Y para baixo); puxado para a camera para ficar na frente
		const FVector Center = Right * (L.Offset.X * UnitsPerPixel) - Up * (L.Offset.Y * UnitsPerPixel) + Up * 10.f;
		const float Flip = L.bMirror ? -1.f : 1.f;
		const FLinearColor Tint(L.Color.R / 255.f, L.Color.G / 255.f, L.Color.B / 255.f, L.Color.A / 255.f);
		const FColor VC = Tint.ToFColor(false);
		TArray<FVector> Verts = { Center - Right * HW * Flip + Up * HH, Center + Right * HW * Flip + Up * HH, Center - Right * HW * Flip - Up * HH, Center + Right * HW * Flip - Up * HH };
		TArray<FVector2D> UVs = { FVector2D(0.f, 0.f), FVector2D(1.f, 0.f), FVector2D(0.f, 1.f), FVector2D(1.f, 1.f) };
		TArray<FColor> Colors = { VC, VC, VC, VC };
		TArray<int32> Tris = { 0, 1, 2, 1, 3, 2, 0, 2, 1, 1, 2, 3 };
		Mesh->CreateMeshSection(Section, Verts, Tris, TArray<FVector>(), UVs, Colors, TArray<FProcMeshTangent>(), false);
		Mesh->SetMaterial(Section, Mat);
		++Section;
	}
}

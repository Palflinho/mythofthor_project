#include "Sprites/MythProcEffect.h"

#include "Sprites/MythStrEffect.h"
#include "Sprites/MythRoSpriteComponent.h"
#include "Sprites/RoSpriteFormats.h"
#include "UI/MythRoUITextures.h"

#include "Camera/PlayerCameraManager.h"
#include "Dom/JsonObject.h"
#include "Engine/World.h"
#include "GameFramework/PlayerController.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "ProceduralMeshComponent.h"

namespace
{
	constexpr float ProcCell = 50.f;      // 1 celula do RO
	constexpr int32 ProcSegments = 28;

	float Num(const TSharedPtr<FJsonObject>& O, const TCHAR* Key, float Default)
	{
		double V = 0.0;
		if (O->TryGetNumberField(Key, V))
		{
			return static_cast<float>(V);
		}
		// Faixa [min, max] (os campos *Rand do cliente): sorteia
		const TArray<TSharedPtr<FJsonValue>>* Range;
		if (O->TryGetArrayField(Key, Range) && Range->Num() >= 2)
		{
			return FMath::FRandRange(static_cast<float>((*Range)[0]->AsNumber()), static_cast<float>((*Range)[1]->AsNumber()));
		}
		return Default;
	}

	bool Flag(const TSharedPtr<FJsonObject>& O, const TCHAR* Key)
	{
		bool B = false;
		return O->TryGetBoolField(Key, B) && B;
	}

	/** "ring_yellow" -> effect/ring_yellow.tga; "effect/pok3.tga" fica; caminho relativo a data/texture. */
	UTexture2D* LoadEffectTexture(const FString& File)
	{
		if (File.IsEmpty() || File.EndsWith(TEXT(".spr"), ESearchCase::IgnoreCase) || File.EndsWith(TEXT(".str"), ESearchCase::IgnoreCase))
		{
			return nullptr; // sprites/str tem outro caminho (AMythStrEffect)
		}
		FString Path = File.Replace(TEXT("\\"), TEXT("/"));
		if (Path.StartsWith(TEXT("data/texture/"), ESearchCase::IgnoreCase))
		{
			Path.RightChopInline(13);
		}
		if (!Path.Contains(TEXT("/")))
		{
			Path = TEXT("effect/") + Path;
		}
		if (!Path.Contains(TEXT(".")))
		{
			Path += TEXT(".tga");
		}
		UTexture2D* Tex = FMythRoUITextures::Get(TEXT("../") + Path); // FMythRoUITextures parte de texture/유저인터페이스
		if (!Tex && Path.EndsWith(TEXT(".tga")))
		{
			Tex = FMythRoUITextures::Get(TEXT("../") + Path.LeftChop(4) + TEXT(".bmp"));
		}
		return Tex;
	}

	/** "data/sprite/npc/skel_archer_arrow": primeira imagem da primeira acao do .spr/.act do cliente, como textura. */
	UTexture2D* LoadSpriteImage(const FString& File, FVector2D& OutPixels)
	{
		const FString Base = FPaths::Combine(UMythRoSpriteComponent::GetAssetsRoot(), File);
		TArray<uint8> SprBytes, ActBytes;
		FRoSpr Spr;
		FRoAct Act;
		if (!FFileHelper::LoadFileToArray(SprBytes, *(Base + TEXT(".spr"))) || !FFileHelper::LoadFileToArray(ActBytes, *(Base + TEXT(".act")))
			|| !Spr.Load(SprBytes) || !Act.Load(ActBytes) || Act.Actions.Num() == 0 || Act.Actions[0].Frames.Num() == 0)
		{
			return nullptr;
		}
		for (const FRoActLayer& L : Act.Actions[0].Frames[0].Layers)
		{
			const FRoSprImage* Img = L.SprIndex >= 0 ? Spr.Find(L.SprIndex, L.SprType) : nullptr;
			if (!Img || Img->Width <= 0 || Img->Height <= 0)
			{
				continue;
			}
			UTexture2D* Tex = UTexture2D::CreateTransient(Img->Width, Img->Height, PF_B8G8R8A8);
			if (!Tex)
			{
				return nullptr;
			}
			Tex->Filter = TF_Bilinear;
			Tex->SRGB = true;
			Tex->NeverStream = true;
			FTexture2DMipMap& Mip = Tex->GetPlatformData()->Mips[0];
			void* Data = Mip.BulkData.Lock(LOCK_READ_WRITE);
			FMemory::Memcpy(Data, Img->Pixels.GetData(), Img->Pixels.Num() * sizeof(FColor));
			Mip.BulkData.Unlock();
			Tex->UpdateResource();
			OutPixels = FVector2D(Img->Width, Img->Height);
			return Tex;
		}
		return nullptr;
	}
}

AMythProcEffect::AMythProcEffect()
{
	PrimaryActorTick.bCanEverTick = true;
	Mesh = CreateDefaultSubobject<UProceduralMeshComponent>(TEXT("Mesh"));
	SetRootComponent(Mesh);
	Mesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	Mesh->SetCastShadow(false);
}

bool AMythProcEffect::Spawn(UObject* WorldContext, const TSharedPtr<FJsonObject>& Part, AActor* Target, const FVector& Feet, AActor* Source, AActor** OutActor)
{
	UWorld* World = WorldContext ? WorldContext->GetWorld() : nullptr;
	if (!World || !Part.IsValid())
	{
		return false;
	}
	const FString Type = Part->GetStringField(TEXT("type"));
	if (Type != TEXT("CYLINDER") && Type != TEXT("3D") && Type != TEXT("2D"))
	{
		return false;
	}
	const FString FileName = Part->GetStringField(TEXT("file"));
	// Projeteis de sprite (flecha: data/sprite/npc/skel_archer_arrow): uma imagem do .spr, nao uma textura de efeito
	const bool bSprite = FileName.StartsWith(TEXT("data/sprite/"), ESearchCase::IgnoreCase);
	FVector2D ImagePixels = FVector2D::ZeroVector;
	UTexture2D* Tex = bSprite ? LoadSpriteImage(FileName, ImagePixels) : LoadEffectTexture(FileName);
	if (!Tex)
	{
		UE_LOG(LogTemp, Verbose, TEXT("MythProcEffect: no texture '%s'"), *Part->GetStringField(TEXT("file")));
		return false;
	}
	FActorSpawnParameters P;
	P.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
	AMythProcEffect* E = World->SpawnActor<AMythProcEffect>(Feet, FRotator::ZeroRotator, P);
	if (!E)
	{
		return false;
	}
	E->Origin = Feet;
	E->bSpriteImage = bSprite;
	E->ImagePixels = ImagePixels;
	E->Follow = Flag(Part, TEXT("attached")) ? Target : nullptr;
	if (Source)
	{
		E->SourcePos = AMythStrEffect::FeetOf(Source);
		E->bHasSource = true;
	}
	if (bSprite)
	{
		// Sprite opaco (sem brilho aditivo): o mesmo material mascarado sem luz dos sprites do jogo
		if (UMaterialInterface* Masked = LoadObject<UMaterialInterface>(nullptr, TEXT("/Paper2D/MaskedUnlitSpriteMaterial.MaskedUnlitSpriteMaterial")))
		{
			E->Material = UMaterialInstanceDynamic::Create(Masked, E);
			E->Material->SetTextureParameterValue(TEXT("SpriteTexture"), Tex);
		}
	}
	else if (UMaterialInterface* Additive = LoadObject<UMaterialInterface>(nullptr, TEXT("/Game/Myth/M_StrAdditive.M_StrAdditive")))
	{
		E->Material = UMaterialInstanceDynamic::Create(Additive, E);
		E->Material->SetTextureParameterValue(TEXT("Tex"), Tex);
		E->Material->SetScalarParameterValue(TEXT("Brightness"), 4.f);
	}
	if (!E->Setup(Part))
	{
		E->Destroy();
		return false;
	}
	if (OutActor)
	{
		*OutActor = E;
	}
	return true;
}

bool AMythProcEffect::Setup(const TSharedPtr<FJsonObject>& Part)
{
	Type = Part->GetStringField(TEXT("type"));
	Duration = FMath::Max(Num(Part, TEXT("duration"), Num(Part, TEXT("durationRand"), 1000.f)), 30.f) / 1000.f;
	Delay = Num(Part, TEXT("delayLate"), Num(Part, TEXT("delayOffset"), 0.f)) / 1000.f;
	AlphaMax = FMath::Clamp(Num(Part, TEXT("alphaMax"), 1.f), 0.f, 1.f);
	bFade = Flag(Part, TEXT("fade"));
	bFadeIn = Flag(Part, TEXT("fadeIn"));
	bFadeOut = Flag(Part, TEXT("fadeOut"));
	bRotate = Flag(Part, TEXT("rotate"));
	bRepeat = Flag(Part, TEXT("repeat"));
	Animation = static_cast<int32>(Num(Part, TEXT("animation"), 0.f));
	Tint = FLinearColor(Num(Part, TEXT("red"), 1.f), Num(Part, TEXT("green"), 1.f), Num(Part, TEXT("blue"), 1.f), 1.f);
	PosZ = Num(Part, TEXT("posz"), Num(Part, TEXT("posZ"), 0.f)) * ProcCell;

	if (Type == TEXT("CYLINDER"))
	{
		BottomRadius = Num(Part, TEXT("bottomSize"), 1.f) * ProcCell;
		TopRadius = Num(Part, TEXT("topSize"), 1.f) * ProcCell;
		Height = FMath::Max(Num(Part, TEXT("height"), 1.f) * ProcCell, 0.f);
		BuildCylinder(0.f);
		if (Material)
		{
			Mesh->SetMaterial(0, Material);
		}
		Mesh->SetVisibility(Delay <= 0.f);
		return true;
	}

	// 3D / 2D: quads voltados para a camera. 3D: tamanho em centesimos de celula; 2D: pixels (~1,5 uu na camera do RO)
	const FString FileStr = Part->GetStringField(TEXT("file")).ToLower();
	const bool bMagicSpear = FileStr.Contains(TEXT("icearrow")) || FileStr.Contains(TEXT("firearrow")) || FileStr.Contains(TEXT("arrow")) || FileStr.Contains(TEXT("spear"));
	const float Unit = (Type == TEXT("2D") ? 1.5f : ProcCell / 100.f) * (bMagicSpear ? 3.2f : 1.0f);
	const float Size = Num(Part, TEXT("size"), 100.f);
	SizeStart = FVector2D(Num(Part, TEXT("sizeStartX"), Num(Part, TEXT("sizeRandStartX"), Num(Part, TEXT("sizeX"), Num(Part, TEXT("sizeStart"), Size)))),
		Num(Part, TEXT("sizeStartY"), Num(Part, TEXT("sizeRandStartY"), Num(Part, TEXT("sizeY"), Num(Part, TEXT("sizeStart"), Size))))) * Unit;
	SizeEnd = FVector2D(Num(Part, TEXT("sizeEndX"), Num(Part, TEXT("sizeRandEndX"), Num(Part, TEXT("sizeX"), Num(Part, TEXT("sizeEnd"), SizeStart.X / Unit)))),
		Num(Part, TEXT("sizeEndY"), Num(Part, TEXT("sizeRandEndY"), Num(Part, TEXT("sizeY"), Num(Part, TEXT("sizeEnd"), SizeStart.Y / Unit))))) * Unit;
	if (bSpriteImage)
	{
		SizeStart = SizeEnd = ImagePixels * 2.f; // 1 px = 2 cm, como o resto dos sprites
	}
	ZStart = Num(Part, TEXT("poszStart"), Num(Part, TEXT("posz"), 1.f)) * ProcCell;
	ZEnd = Num(Part, TEXT("poszEnd"), ZStart / ProcCell) * ProcCell;
	bToSrc = Flag(Part, TEXT("toSrc")) && bHasSource;
	bCircle = Part->HasField(TEXT("circlePattern")) || (Type == TEXT("2D") && Part->HasField(TEXT("sizeRandEndY")));
	CircleInner = 2.2f * ProcCell;
	CircleOuter = 5.5f * ProcCell;

	const int32 Count = FMath::Clamp(static_cast<int32>(Num(Part, TEXT("duplicate"), bCircle ? 8.f : 1.f)), 1, 40);
	const float Between = Num(Part, TEXT("timeBetweenDupli"), 0.f) / 1000.f;
	const bool bSwarm = Count > 1 && !bCircle && !bToSrc;
	for (int32 i = 0; i < Count; ++i)
	{
		FCopy& C = Copies.AddDefaulted_GetRef();
		C.Delay = Delay + i * Between;
		C.Angle = bCircle ? 360.f * i / Count + FMath::FRandRange(0.f, 35.f) : FMath::FRandRange(0.f, 360.f);
		if (bSwarm)
		{
			// Enxame (centelhas da cura, bolhas): espalhado em volta do alvo
			const float R = FMath::FRandRange(0.2f, 1.f) * ProcCell;
			C.Offset = FVector(FMath::Cos(FMath::DegreesToRadians(C.Angle)) * R, FMath::Sin(FMath::DegreesToRadians(C.Angle)) * R, 0.f);
		}
	}
	if (bSwarm && FMath::IsNearlyEqual(ZStart, ZEnd))
	{
		ZStart = 0.f; // particulas soltas sobem do chao ate a altura do peito
		ZEnd = 2.f * ProcCell;
	}
	if (Part->HasField(TEXT("angle")) && !bToSrc && ZStart > ZEnd)
	{
		bHasSource = true; // lanca caindo em diagonal do ceu (Lancas de Fogo/Gelo)
		bToSrc = true;
		SourcePos = Origin + FVector(-0.4f * (ZStart - ZEnd), 0.f, 0.f);
	}
	if (Material)
	{
		Mesh->SetMaterial(0, Material);
	}
	BuildQuads();
	return true;
}

float AMythProcEffect::AlphaAt(float T) const
{
	float A = AlphaMax;
	if (bFade || bFadeOut)
	{
		A *= T > 0.6f ? 1.f - (T - 0.6f) / 0.4f : 1.f;
	}
	if (bFade || bFadeIn)
	{
		A *= T < 0.15f ? T / 0.15f : 1.f;
	}
	return FMath::Clamp(A, 0.f, 1.f);
}

void AMythProcEffect::BuildCylinder(float T)
{
	// animation 4: abre do centro para fora; 2: fecha; 3: sobe; 1: a textura corre para cima
	float RadiusScale = 1.f;
	float HeightScale = 1.f;
	float VScroll = 0.f;
	switch (Animation)
	{
	case 4: RadiusScale = 1.f - FMath::Square(1.f - T); break;
	case 2: RadiusScale = 1.f - T; break;
	case 3: HeightScale = T; break;
	case 1: VScroll = T * 2.f; break;
	default: break;
	}
	TArray<FVector> Verts;
	TArray<FVector2D> UVs;
	TArray<FColor> Colors;
	TArray<int32> Tris;
	const FColor C = FLinearColor(Tint.R, Tint.G, Tint.B, AlphaAt(T)).ToFColor(false);
	for (int32 s = 0; s <= ProcSegments; ++s)
	{
		const float A = 2.f * PI * s / ProcSegments;
		const FVector Dir(FMath::Cos(A), FMath::Sin(A), 0.f);
		Verts.Add(Dir * BottomRadius * RadiusScale + FVector(0.f, 0.f, PosZ));
		Verts.Add(Dir * TopRadius * RadiusScale + FVector(0.f, 0.f, PosZ + Height * HeightScale));
		UVs.Add(FVector2D(static_cast<float>(s) / ProcSegments, 1.f + VScroll));
		UVs.Add(FVector2D(static_cast<float>(s) / ProcSegments, 0.f + VScroll));
		Colors.Add(C);
		Colors.Add(C);
		if (s < ProcSegments)
		{
			const int32 i = s * 2;
			Tris.Append({ i, i + 2, i + 1, i + 1, i + 2, i + 3 });
		}
	}
	if (Mesh->GetNumSections() == 0)
	{
		Mesh->CreateMeshSection(0, Verts, Tris, TArray<FVector>(), UVs, Colors, TArray<FProcMeshTangent>(), false);
	}
	else
	{
		Mesh->UpdateMeshSection(0, Verts, TArray<FVector>(), UVs, Colors, TArray<FProcMeshTangent>());
	}
}

void AMythProcEffect::BuildQuads()
{
	// Quads no plano da camera (como os billboards do cliente); 2D em leque: o lado comprido aponta para fora
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
	const FVector Base = GetActorLocation();
	for (const FCopy& Copy : Copies)
	{
		const float T = Duration > 0.f ? (Time - Copy.Delay) / Duration : 1.f;
		if (T < 0.f || T > 1.f)
		{
			continue;
		}
		const FVector2D Size = FMath::Lerp(SizeStart, SizeEnd, T);
		FVector Center;
		FVector AxisX = Right;
		FVector AxisY = Up;
		if (bCircle)
		{
			const float Rad = FMath::DegreesToRadians(Copy.Angle);
			const FVector Radial = Right * FMath::Cos(Rad) + Up * FMath::Sin(Rad);
			Center = Origin + FVector(0.f, 0.f, ProcCell) + Radial * FMath::Lerp(CircleInner, CircleOuter, T) * 0.35f;
			AxisY = Radial;
			AxisX = FVector::CrossProduct(Radial, Right ^ Up).GetSafeNormal();
		}
		else if (bToSrc)
		{
			const FVector From = SourcePos + FVector(0.f, 0.f, ZStart);
			const FVector To = Origin + FVector(0.f, 0.f, ZEnd > 0.f ? ZEnd : ProcCell);
			Center = FMath::Lerp(From, To, T);
			if (bSpriteImage)
			{
				// A ponta (parte de baixo da imagem) aponta, na tela, para onde a flecha vai (rotateToTarget)
				const FVector Delta = To - From;
				const float DX = FVector::DotProduct(Delta, Right);
				const float DY = FVector::DotProduct(Delta, Up);
				if (FMath::Square(DX) + FMath::Square(DY) > 1.f)
				{
					AxisY = -(Right * DX + Up * DY).GetSafeNormal();
					AxisX = (Right * DY - Up * DX).GetSafeNormal();
				}
			}
		}
		else
		{
			Center = Origin + Copy.Offset + FVector(0.f, 0.f, FMath::Lerp(ZStart, ZEnd, T));
		}
		Center -= Base; // a malha e local ao ator
		const FVector HX = AxisX * Size.X * 0.5f;
		const FVector HY = AxisY * Size.Y * 0.5f;
		const int32 i = Verts.Num();
		Verts.Append({ Center - HX + HY, Center + HX + HY, Center - HX - HY, Center + HX - HY });
		UVs.Append({ FVector2D(0.f, 0.f), FVector2D(1.f, 0.f), FVector2D(0.f, 1.f), FVector2D(1.f, 1.f) });
		const FColor C = FLinearColor(Tint.R, Tint.G, Tint.B, bSpriteImage ? 1.f : AlphaAt(T)).ToFColor(false);
		Colors.Append({ C, C, C, C });
		Tris.Append({ i, i + 1, i + 2, i + 1, i + 3, i + 2 });
	}
	Mesh->ClearMeshSection(0);
	if (Verts.Num() > 0)
	{
		Mesh->CreateMeshSection(0, Verts, Tris, TArray<FVector>(), UVs, Colors, TArray<FProcMeshTangent>(), false);
		if (Material)
		{
			Mesh->SetMaterial(0, Material);
		}
	}
}

void AMythProcEffect::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);
	Time += DeltaSeconds;
	if (const AActor* Target = Follow.Get())
	{
		Origin = AMythStrEffect::FeetOf(Target);
	}
	SetActorLocation(Origin);
	float LastEnd = Delay + Duration;
	for (const FCopy& C : Copies)
	{
		LastEnd = FMath::Max(LastEnd, C.Delay + Duration);
	}
	if (Time >= LastEnd)
	{
		if (bRepeat)
		{
			Time = 0.f;
		}
		else
		{
			Destroy();
			return;
		}
	}
	if (Type == TEXT("CYLINDER"))
	{
		const float T = FMath::Clamp((Time - Delay) / Duration, 0.f, 1.f);
		Mesh->SetVisibility(Time >= Delay);
		if (bRotate)
		{
			Spin += DeltaSeconds * 180.f;
			SetActorRotation(FRotator(0.f, Spin, 0.f));
		}
		BuildCylinder(T);
		return;
	}
	BuildQuads();
}

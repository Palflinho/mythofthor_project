#include "Sprites/MythProcEffect.h"

#include "Sprites/MythStrEffect.h"
#include "Sprites/MythRoSpriteComponent.h"
#include "Sprites/RoSpriteFormats.h"
#include "UI/MythRoUITextures.h"

#include "Camera/PlayerCameraManager.h"
#include "Dom/JsonObject.h"
#include "Dom/JsonValue.h"
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

	/** "data/sprite/npc/skel_archer_arrow" ou "fireball": primeira imagem da primeira acao do .spr/.act do cliente, como textura. */
	UTexture2D* LoadSpriteImage(const FString& File, FVector2D& OutPixels)
	{
		FString Clean = File.Replace(TEXT("\\"), TEXT("/"));
		Clean.RemoveFromEnd(TEXT(".spr"), ESearchCase::IgnoreCase);
		TArray<FString> Candidates;
		if (Clean.StartsWith(TEXT("data/sprite/"), ESearchCase::IgnoreCase))
		{
			Candidates.Add(FPaths::Combine(UMythRoSpriteComponent::GetAssetsRoot(), Clean));
		}
		else if (Clean.StartsWith(TEXT("../"), ESearchCase::IgnoreCase))
		{
			Candidates.Add(FPaths::Combine(UMythRoSpriteComponent::GetAssetsRoot(), TEXT("data/sprite"), Clean.Mid(3)));
		}
		else
		{
			Candidates.Add(FPaths::Combine(UMythRoSpriteComponent::GetAssetsRoot(), TEXT("data/sprite/\xC774\xD329\xD2B8"), Clean));
			Candidates.Add(FPaths::Combine(UMythRoSpriteComponent::GetAssetsRoot(), TEXT("data/sprite/npc"), Clean));
		}

		for (const FString& Base : Candidates)
		{
			TArray<uint8> SprBytes, ActBytes;
			FRoSpr Spr;
			FRoAct Act;
			if (!FFileHelper::LoadFileToArray(SprBytes, *(Base + TEXT(".spr"))) || !FFileHelper::LoadFileToArray(ActBytes, *(Base + TEXT(".act")))
				|| !Spr.Load(SprBytes) || !Act.Load(ActBytes) || Act.Actions.Num() == 0 || Act.Actions[0].Frames.Num() == 0)
			{
				continue;
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

	FString FileName;
	Part->TryGetStringField(TEXT("file"), FileName);
	if (FileName.IsEmpty())
	{
		Part->TryGetStringField(TEXT("textureName"), FileName);
	}

	TArray<UTexture2D*> LoadedFrames;
	const TArray<TSharedPtr<FJsonValue>>* FileList = nullptr;
	if (Part->TryGetArrayField(TEXT("fileList"), FileList) && FileList)
	{
		for (const TSharedPtr<FJsonValue>& Val : *FileList)
		{
			const FString Entry = Val->AsString();
			if (UTexture2D* FTex = LoadEffectTexture(Entry))
			{
				LoadedFrames.Add(FTex);
				if (FileName.IsEmpty())
				{
					FileName = Entry;
				}
			}
		}
	}

	FString SpriteName;
	Part->TryGetStringField(TEXT("spriteName"), SpriteName);
	if (SpriteName.IsEmpty())
	{
		Part->TryGetStringField(TEXT("absoluteSpriteName"), SpriteName);
	}

	const bool bSprite = FileName.StartsWith(TEXT("data/sprite/"), ESearchCase::IgnoreCase) || (FileName.IsEmpty() && !SpriteName.IsEmpty());
	FVector2D ImagePixels = FVector2D::ZeroVector;
	UTexture2D* Tex = nullptr;
	if (LoadedFrames.Num() > 0)
	{
		Tex = LoadedFrames[0];
	}
	else if (bSprite)
	{
		Tex = LoadSpriteImage(!FileName.IsEmpty() ? FileName : SpriteName, ImagePixels);
	}
	else
	{
		Tex = LoadEffectTexture(FileName);
	}

	if (!Tex)
	{
		UE_LOG(LogTemp, Verbose, TEXT("MythProcEffect: no texture '%s'"), *FileName);
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
	for (UTexture2D* FT : LoadedFrames)
	{
		E->FrameTextures.Add(FT);
	}
	E->FrameDelaySec = FMath::Max(Num(Part, TEXT("frameDelay"), 35.f), 10.f) / 1000.f;
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
		E->Material->SetScalarParameterValue(TEXT("Brightness"), 4.5f);
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
	Delay = Num(Part, TEXT("delayLate"), Num(Part, TEXT("delayOffset"), Num(Part, TEXT("delayStart"), 0.f))) / 1000.f;
	AlphaMax = FMath::Clamp(Num(Part, TEXT("alphaMax"), 1.f), 0.f, 1.f);
	bFade = Flag(Part, TEXT("fade"));
	bFadeIn = Flag(Part, TEXT("fadeIn"));
	bFadeOut = Flag(Part, TEXT("fadeOut"));
	bRotate = Flag(Part, TEXT("rotate"));
	bRepeat = Flag(Part, TEXT("repeat"));
	bRotateToTarget = Flag(Part, TEXT("rotateToTarget"));
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

	// 3D / 2D: quads voltados para a camera.
	FString FileStr;
	Part->TryGetStringField(TEXT("file"), FileStr);
	FileStr = FileStr.ToLower();
	bMagicSpear = FileStr.Contains(TEXT("icearrow")) || FileStr.Contains(TEXT("firearrow")) || (FrameTextures.Num() > 0 && Part->HasField(TEXT("angle")));
	if (FileStr.Contains(TEXT("icearrow")))
	{
		// icearrow.tga tem RGB=(1,1,1) puro e silhueta no Alpha: aplica tom cristalino de gelo azul-ciano
		Tint = FLinearColor(0.42f, 0.85f, 1.0f, 1.f);
	}

	const float Unit = (Type == TEXT("2D") ? 1.5f : ProcCell / 100.f);
	const float Size = Num(Part, TEXT("size"), 100.f);
	SizeStart = FVector2D(Num(Part, TEXT("sizeStartX"), Num(Part, TEXT("sizeRandStartX"), Num(Part, TEXT("sizeX"), Num(Part, TEXT("sizeStart"), Size)))),
		Num(Part, TEXT("sizeStartY"), Num(Part, TEXT("sizeRandStartY"), Num(Part, TEXT("sizeY"), Num(Part, TEXT("sizeStart"), Size))))) * Unit;
	SizeEnd = FVector2D(Num(Part, TEXT("sizeEndX"), Num(Part, TEXT("sizeRandEndX"), Num(Part, TEXT("sizeX"), Num(Part, TEXT("sizeEnd"), SizeStart.X / Unit)))),
		Num(Part, TEXT("sizeEndY"), Num(Part, TEXT("sizeRandEndY"), Num(Part, TEXT("sizeY"), Num(Part, TEXT("sizeEnd"), SizeStart.Y / Unit))))) * Unit;
	if (bSpriteImage)
	{
		SizeStart = SizeEnd = ImagePixels * 2.f; // 1 px = 2 cm, como o resto dos sprites
	}
	else if (bMagicSpear)
	{
		// Lancas de Gelo e Fogo: formato alongado de lanca (X = comprimento ao longo da queda, Y = espessura)
		SizeStart = SizeEnd = FVector2D(220.f, 88.f);
		Duration = 0.22f; // tempo exato de queda ate o impacto do golpe (sincronizado com LandHandle = 0.22s)
	}

	ZStart = Num(Part, TEXT("poszStart"), Num(Part, TEXT("posz"), Num(Part, TEXT("zOffsetStart"), 1.f))) * ProcCell;
	ZEnd = Num(Part, TEXT("poszEnd"), Num(Part, TEXT("zOffsetEnd"), ZStart / ProcCell)) * ProcCell;
	bFromSrc = Flag(Part, TEXT("fromSrc")) && bHasSource;
	bToSrc = (Flag(Part, TEXT("toSrc")) || bFromSrc) && bHasSource;
	bCircle = Part->HasField(TEXT("circlePattern")) || (Type == TEXT("2D") && Part->HasField(TEXT("sizeRandEndY")));
	CircleInner = 2.2f * ProcCell;
	CircleOuter = 5.5f * ProcCell;

	if (Part->HasField(TEXT("angle")))
	{
		FixedAngle = Num(Part, TEXT("angle"), 0.f);
		bHasFixedAngle = true;
	}

	const bool bFallingSpear = (bMagicSpear || (bHasFixedAngle && !bToSrc && ZStart > ZEnd));
	if (bFallingSpear)
	{
		bHasSource = true;
		bToSrc = true;
		bMagicSpear = true;
		ZStart = FMath::Max(ZStart, 10.5f * ProcCell); // cai bem do alto (525 uu)
		ZEnd = 0.8f * ProcCell;
	}

	// Cada golpe do multihit ja dispara um BeforeHit proprio: 1 lanca por golpe (ou duplicate explicito se houver no JSON)
	int32 Count = FMath::Clamp(static_cast<int32>(Num(Part, TEXT("duplicate"), bCircle ? 8.f : 1.f)), 1, 40);
	float Between = Num(Part, TEXT("timeBetweenDupli"), bFallingSpear ? 220.f : 0.f) / 1000.f;
	const bool bHasRadialEnd = Part->HasField(TEXT("posxEndRand")) || Part->HasField(TEXT("posyEndRand")) || Part->HasField(TEXT("posxEnd")) || Part->HasField(TEXT("posyEnd"));
	const bool bSwarm = Count > 1 && !bCircle && !bToSrc && !bHasRadialEnd;

	for (int32 i = 0; i < Count; ++i)
	{
		FCopy& C = Copies.AddDefaulted_GetRef();
		C.Delay = Delay + i * Between;
		C.Angle = bCircle ? 360.f * i / Count + FMath::FRandRange(0.f, 35.f) : FMath::FRandRange(0.f, 360.f);
		if (bFallingSpear)
		{
			// Leve espalhamento aleatorio ao redor do alvo para que as lancas consecutivas do multihit nao caiam empilhadas no mesmo pixel
			const float SpreadX = FMath::FRandRange(-42.f, 42.f);
			const float SpreadY = FMath::FRandRange(-34.f, 34.f);
			C.OffsetEnd = FVector(SpreadX, SpreadY, 0.f);
			C.OffsetStart = FVector(SpreadX, SpreadY, 0.f);
		}
		else if (bHasRadialEnd)
		{
			// Explosao radial (ex.: Punho Supremo de Asura ef 266 com posxEndRand/posyEndRand = 40)
			const float SX = Num(Part, TEXT("posxStart"), 0.f) * ProcCell * 0.15f;
			const float SY = Num(Part, TEXT("posyStart"), 0.f) * ProcCell * 0.15f;
			float EX = Num(Part, TEXT("posxEnd"), 0.f) * ProcCell * 0.25f;
			float EY = Num(Part, TEXT("posyEnd"), 0.f) * ProcCell * 0.25f;
			if (Part->HasField(TEXT("posxEndRand")) || Part->HasField(TEXT("posyEndRand")))
			{
				const float Rad = FMath::DegreesToRadians(360.f * i / FMath::Max(1, Count) + FMath::FRandRange(-12.f, 12.f));
				const float MaxR = FMath::Max(Num(Part, TEXT("posxEndRand"), 20.f), Num(Part, TEXT("posyEndRand"), 20.f)) * ProcCell * 0.16f;
				const float R = FMath::FRandRange(MaxR * 0.55f, MaxR);
				EX = FMath::Cos(Rad) * R;
				EY = FMath::Sin(Rad) * R;
				C.Angle = FMath::RadiansToDegrees(Rad);
			}
			C.OffsetStart = FVector(SX, SY, 0.f);
			C.OffsetEnd = FVector(EX, EY, 0.f);
		}
		else if (bSwarm)
		{
			// Enxame (centelhas da cura, bolhas): espalhado em volta do alvo
			const float R = FMath::FRandRange(0.2f, 1.f) * ProcCell;
			C.OffsetStart = C.OffsetEnd = FVector(FMath::Cos(FMath::DegreesToRadians(C.Angle)) * R, FMath::Sin(FMath::DegreesToRadians(C.Angle)) * R, 0.f);
		}
	}
	if (bSwarm && FMath::IsNearlyEqual(ZStart, ZEnd))
	{
		ZStart = 0.f; // particulas soltas sobem do chao ate a altura do peito
		ZEnd = 2.f * ProcCell;
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
	// Quads no plano da camera (como os billboards do cliente)
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
			FVector From = SourcePos + FVector(0.f, 0.f, ZStart) + Copy.OffsetStart;
			FVector To = Origin + FVector(0.f, 0.f, ZEnd > 0.f ? ZEnd : ProcCell) + Copy.OffsetEnd;
			if (bMagicSpear)
			{
				// Lancas de Gelo e Fogo caem do alto em diagonal no espaco da tela (-Right * 0.32 + Up),
				// garantindo que sempre apontem de cima para baixo em direcao ao alvo independente do angulo da camera!
				const float DropHeight = FMath::Max(ZStart - ZEnd, 450.f);
				To = Origin + Copy.OffsetEnd + FVector(0.f, 0.f, ProcCell * 0.8f);
				From = To - Right * (DropHeight * 0.32f) + FVector(0.f, 0.f, DropHeight);
			}
			Center = FMath::Lerp(From, To, T);
			const FVector Delta = To - From;
			const float DX = FVector::DotProduct(Delta, Right);
			const float DY = FVector::DotProduct(Delta, Up);
			if (FMath::Square(DX) + FMath::Square(DY) > 1.f)
			{
				const FVector ScreenDir = (Right * DX + Up * DY).GetSafeNormal();
				const FVector ScreenPerp = (-Right * DY + Up * DX).GetSafeNormal();
				if (bSpriteImage && !bMagicSpear)
				{
					// Flecha de sprite (.spr): desenhada em pe com a ponta para baixo (-Y)
					AxisY = -ScreenDir;
					AxisX = -ScreenPerp;
				}
				else
				{
					// Texturas horizontais (icearrow.tga, firearrow, thunder_ball): ponta em +X (U=1) aponta direto para o alvo!
					AxisX = ScreenDir;
					AxisY = ScreenPerp;
				}
			}
		}
		else
		{
			const FVector CurOffset = FMath::Lerp(Copy.OffsetStart, Copy.OffsetEnd, T);
			// Se tem deslocamento radial (ex.: Asura Strike ef 266), projeta no plano da camera (Right/Up)
			if (!Copy.OffsetEnd.IsNearlyZero())
			{
				const FVector RadialDir = (Right * Copy.OffsetEnd.X + Up * Copy.OffsetEnd.Y).GetSafeNormal();
				const float RadialDist = FMath::Lerp(Copy.OffsetStart.Size(), Copy.OffsetEnd.Size(), T);
				Center = Origin + FVector(0.f, 0.f, FMath::Lerp(ZStart, ZEnd, T) + ProcCell * 0.9f) + RadialDir * RadialDist;
				if (bRotateToTarget && !RadialDir.IsNearlyZero())
				{
					AxisY = RadialDir;
					AxisX = FVector::CrossProduct(RadialDir, Right ^ Up).GetSafeNormal();
				}
			}
			else
			{
				Center = Origin + CurOffset + FVector(0.f, 0.f, FMath::Lerp(ZStart, ZEnd, T));
				if (bHasFixedAngle)
				{
					const float Rad = FMath::DegreesToRadians(FixedAngle + (bRotate ? Time * 180.f : 0.f));
					const float C = FMath::Cos(Rad), S = FMath::Sin(Rad);
					AxisX = Right * C - Up * S;
					AxisY = Right * S + Up * C;
				}
			}
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

	// Anima quadros de fileList (ex.: Lancas de Fogo 불화살1..6.tga, Trovao de Jupiter thunder_ball_a..f.bmp)
	if (FrameTextures.Num() > 1 && Material && FrameDelaySec > 0.f)
	{
		const int32 FrameIdx = static_cast<int32>(Time / FrameDelaySec) % FrameTextures.Num();
		if (FrameIdx != LastFrameIdx && FrameTextures[FrameIdx])
		{
			LastFrameIdx = FrameIdx;
			Material->SetTextureParameterValue(bSpriteImage ? TEXT("SpriteTexture") : TEXT("Tex"), FrameTextures[FrameIdx]);
		}
	}

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

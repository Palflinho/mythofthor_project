#include "Sprites/MythStrEffect.h"

#include "Sprites/MythRoSpriteComponent.h"
#include "UI/MythRoUITextures.h"

#include "Camera/PlayerCameraManager.h"
#include "Components/CapsuleComponent.h"
#include "GameFramework/Character.h"
#include "Components/PointLightComponent.h"
#include "Engine/World.h"
#include "GameFramework/PlayerController.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "ProceduralMeshComponent.h"

namespace
{
	// Origem do efeito nas coordenadas de tela do .str: (320, 320) fica meia celula (0,5 x 35 px) abaixo dos pes,
	// como o cliente posiciona os .str; entao os pes ficam em y = 302,5
	constexpr float StrOriginX = 320.f;
	constexpr float StrOriginY = 320.f - 0.5f * 35.f;

	TMap<FString, TSharedPtr<FRoStr>>& StrCache()
	{
		static TMap<FString, TSharedPtr<FRoStr>> Map;
		return Map;
	}

	bool IsAdditive(uint32 DstBlend)
	{
		return DstBlend == 2 || DstBlend == 7; // D3DBLEND_ONE / DESTALPHA: soma de luz
	}
}

// ------------------------------------------------------------------------------------------------ leitura

bool FRoStr::Load(const FString& Path)
{
	TArray<uint8> Data;
	if (!FFileHelper::LoadFileToArray(Data, *Path) || Data.Num() < 36 || FMemory::Memcmp(Data.GetData(), "STRM", 4) != 0)
	{
		return false;
	}
	int32 Off = 4;
	auto U32 = [&Data, &Off]() { uint32 V = 0; if (Off + 4 <= Data.Num()) { FMemory::Memcpy(&V, Data.GetData() + Off, 4); } Off += 4; return V; };
	auto F32 = [&Data, &Off]() { float V = 0.f; if (Off + 4 <= Data.Num()) { FMemory::Memcpy(&V, Data.GetData() + Off, 4); } Off += 4; return V; };

	U32(); // versao (0x94)
	Fps = FMath::Max<int32>(1, U32());
	MaxKey = U32();
	const uint32 LayerCount = U32();
	Off += 16; // reservado
	Layers.SetNum(LayerCount);
	for (FRoStrLayer& Layer : Layers)
	{
		const int32 TexCount = static_cast<int32>(U32());
		for (int32 t = 0; t < TexCount && Off + 128 <= Data.Num(); ++t)
		{
			const ANSICHAR* Name = reinterpret_cast<const ANSICHAR*>(Data.GetData() + Off);
			Layer.Textures.Add(FString(UTF8_TO_TCHAR(Name)));
			Off += 128;
		}
		const int32 KeyCount = static_cast<int32>(U32());
		Layer.Keys.SetNum(FMath::Max(0, KeyCount));
		for (FRoStrKey& K : Layer.Keys)
		{
			K.Frame = static_cast<int32>(U32());
			K.Type = U32();
			K.Pos.X = F32();
			K.Pos.Y = F32();
			for (float& V : K.UV) { V = F32(); }
			for (float& V : K.XY) { V = F32(); }
			K.AniFrame = F32();
			K.AniType = U32();
			K.Delay = F32();
			K.Angle = F32();
			K.Color.R = F32();
			K.Color.G = F32();
			K.Color.B = F32();
			K.Color.A = F32();
			K.SrcBlend = U32();
			K.DstBlend = U32();
			U32(); // preset
		}
		if (Off > Data.Num())
		{
			return false;
		}
	}
	return true;
}

// ------------------------------------------------------------------------------------------------ ator

AMythStrEffect::AMythStrEffect()
{
	PrimaryActorTick.bCanEverTick = true;
	PrimaryActorTick.TickGroup = TG_PostUpdateWork; // depois da camera do quadro
	Mesh = CreateDefaultSubobject<UProceduralMeshComponent>(TEXT("Mesh"));
	SetRootComponent(Mesh);
	Mesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	Mesh->SetCastShadow(false);
	Mesh->bUseAsyncCooking = false;

	Light = CreateDefaultSubobject<UPointLightComponent>(TEXT("Light"));
	Light->SetupAttachment(Mesh);
	Light->SetMobility(EComponentMobility::Movable);
	Light->SetIntensityUnits(ELightUnits::Candelas);
	Light->SetIntensity(0.f);
	Light->SetAttenuationRadius(600.f);
	Light->SetSourceRadius(30.f);
	Light->SetCastShadows(true); // a luz do efeito projeta as sombras dos personagens em volta (HD-2D)
	Light->SetVisibility(false);
}

FString AMythStrEffect::TexturePath(const FString& Name) const
{
	// FMythRoUITextures le a partir de data/texture/유저인터페이스: ".." volta para data/texture
	return FString::Printf(TEXT("../effect/%s%s"), *Folder, *Name);
}

bool AMythStrEffect::Setup(const FString& StrFile)
{
	FString Rel = StrFile.Replace(TEXT("\\"), TEXT("/"));
	Rel.RemoveFromEnd(TEXT(".str"));
	TSharedPtr<FRoStr>& Cached = StrCache().FindOrAdd(Rel.ToLower());
	if (!Cached.IsValid())
	{
		TSharedPtr<FRoStr> Loaded = MakeShared<FRoStr>();
		const FString Path = FPaths::Combine(UMythRoSpriteComponent::GetAssetsRoot(), TEXT("data"), TEXT("texture"), TEXT("effect"), Rel + TEXT(".str"));
		if (!Loaded->Load(Path))
		{
			UE_LOG(LogTemp, Warning, TEXT("MythStrEffect: could not read %s"), *Path);
			return false;
		}
		Cached = Loaded;
	}
	Str = Cached;
	const int32 Slash = Rel.Find(TEXT("/"), ESearchCase::IgnoreCase, ESearchDir::FromEnd);
	Folder = Slash != INDEX_NONE ? Rel.Left(Slash + 1) : FString();

	UMaterialInterface* Additive = LoadObject<UMaterialInterface>(nullptr, TEXT("/Game/Myth/M_StrAdditive.M_StrAdditive"));
	UMaterialInterface* Translucent = LoadObject<UMaterialInterface>(nullptr, TEXT("/Game/Myth/M_StrTranslucent.M_StrTranslucent"));
	if (!Additive || !Translucent)
	{
		UE_LOG(LogTemp, Warning, TEXT("MythStrEffect: materiais M_StrAdditive/M_StrTranslucent ausentes (tools/ue_setup_weather.py)"));
		return false;
	}
	for (const FRoStrLayer& Layer : Str->Layers)
	{
		const bool bAdd = Layer.Keys.Num() > 0 && IsAdditive(Layer.Keys[0].DstBlend);
		UMaterialInstanceDynamic* MID = UMaterialInstanceDynamic::Create(bAdd ? Additive : Translucent, this);
		MID->SetScalarParameterValue(TEXT("Brightness"), bAdd ? AdditiveBrightness : TranslucentBrightness);
		LayerMaterials.Add(MID);
	}
	SectionCreated.Init(false, Str->Layers.Num());
	return true;
}

FVector AMythStrEffect::FeetOf(const AActor* Unit)
{
	if (!Unit)
	{
		return FVector::ZeroVector;
	}
	if (const ACharacter* C = Cast<ACharacter>(Unit); C && C->GetCapsuleComponent())
	{
		return C->GetActorLocation() - FVector(0.f, 0.f, C->GetCapsuleComponent()->GetScaledCapsuleHalfHeight());
	}
	return Unit->GetActorLocation();
}

AMythStrEffect* AMythStrEffect::Play(UObject* WorldContext, const FString& StrFile, const FVector& Location, AActor* InFollow)
{
	UWorld* World = WorldContext ? WorldContext->GetWorld() : nullptr;
	if (!World || StrFile.IsEmpty())
	{
		return nullptr;
	}
	FActorSpawnParameters Params;
	Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
	AMythStrEffect* Effect = World->SpawnActor<AMythStrEffect>(Location, FRotator::ZeroRotator, Params);
	if (!Effect)
	{
		return nullptr;
	}
	if (!Effect->Setup(StrFile))
	{
		Effect->Destroy();
		return nullptr;
	}
	Effect->Follow = InFollow;
	Effect->Origin = Location;
	return Effect;
}

bool AMythStrEffect::EvalLayer(const FRoStrLayer& Layer, float Frame, FRoStrKey& Out) const
{
	// Ultima pose absoluta (tipo 0) ate este quadro
	int32 From = INDEX_NONE;
	for (int32 i = 0; i < Layer.Keys.Num(); ++i)
	{
		if (Layer.Keys[i].Frame > Frame)
		{
			break;
		}
		if (Layer.Keys[i].Type == 0)
		{
			From = i;
		}
	}
	if (From == INDEX_NONE)
	{
		return false;
	}
	const FRoStrKey& A = Layer.Keys[From];
	Out = A;
	const FRoStrKey* B = Layer.Keys.IsValidIndex(From + 1) ? &Layer.Keys[From + 1] : nullptr;
	const float TexCount = FMath::Max(1, Layer.Textures.Num());
	if (B && B->Type == 1)
	{
		// Variacao por quadro somada desde a pose
		const float D = Frame - A.Frame;
		Out.Pos = A.Pos + B->Pos * D;
		for (int32 i = 0; i < 8; ++i)
		{
			Out.XY[i] = A.XY[i] + B->XY[i] * D;
			Out.UV[i] = A.UV[i] + B->UV[i] * D;
		}
		Out.Angle = A.Angle + B->Angle * D;
		Out.Color = A.Color + B->Color * D;
		switch (A.AniType)
		{
		case 1: Out.AniFrame = A.AniFrame + B->AniFrame * D; break;
		case 2: Out.AniFrame = FMath::Min(A.AniFrame + B->Delay * D, TexCount - 1.f); break;
		case 3: Out.AniFrame = FMath::Fmod(A.AniFrame + B->Delay * D, TexCount); break;
		case 4: Out.AniFrame = FMath::Fmod(A.AniFrame - B->Delay * D + TexCount * 100.f, TexCount); break;
		default: break;
		}
		// A variacao vale ate a proxima pose absoluta; depois da ultima chave a camada segue ate o fim do efeito
		return true;
	}
	if (B && B->Type == 0 && B->Frame > A.Frame)
	{
		const float T = (Frame - A.Frame) / static_cast<float>(B->Frame - A.Frame);
		Out.Pos = FMath::Lerp(A.Pos, B->Pos, T);
		for (int32 i = 0; i < 8; ++i)
		{
			Out.XY[i] = FMath::Lerp(A.XY[i], B->XY[i], T);
			Out.UV[i] = FMath::Lerp(A.UV[i], B->UV[i], T);
		}
		Out.Angle = FMath::Lerp(A.Angle, B->Angle, T);
		Out.Color = FMath::Lerp(A.Color, B->Color, T);
		return true;
	}
	// Ultima chave da camada: so aparece no proprio quadro
	return Frame - A.Frame < 1.f;
}

UTexture2D* AMythStrEffect::LayerTexture(const FRoStrLayer& Layer, float AniFrame) const
{
	if (Layer.Textures.IsEmpty())
	{
		return nullptr;
	}
	const int32 Index = FMath::Clamp(FMath::FloorToInt(AniFrame), 0, Layer.Textures.Num() - 1);
	return FMythRoUITextures::Get(TexturePath(Layer.Textures[Index]));
}

void AMythStrEffect::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);
	if (!Str.IsValid())
	{
		Destroy();
		return;
	}
	Elapsed += DeltaSeconds;
	const float Frame = Elapsed * Str->Fps;
	if (Frame > Str->MaxKey + 1)
	{
		Destroy();
		return;
	}
	if (Follow.IsValid())
	{
		Origin = FeetOf(Follow.Get()) + FVector(0.f, 0.f, HeadOffset);
	}

	// Camera: quads paralelos a tela, puxados pela linha de visao para ficar na frente do sprite
	FVector CamLoc = Origin + FVector(0.f, -1000.f, 1000.f);
	FRotator CamRot(-45.f, 90.f, 0.f);
	if (const APlayerController* PC = GetWorld()->GetFirstPlayerController(); PC && PC->PlayerCameraManager)
	{
		CamLoc = PC->PlayerCameraManager->GetCameraLocation();
		CamRot = PC->PlayerCameraManager->GetCameraRotation();
	}
	const FRotationMatrix CamM(CamRot);
	const FVector Right = CamM.GetUnitAxis(EAxis::Y);
	const FVector Up = CamM.GetUnitAxis(EAxis::Z);
	const FVector ToOrigin = Origin - CamLoc;
	const float Dist = ToOrigin.Size();
	const float Scale = Dist > CameraDepthOffset + 1.f ? (Dist - CameraDepthOffset) / Dist : 1.f;
	const FVector Center = Origin - ToOrigin.GetSafeNormal() * CameraDepthOffset;
	SetActorLocation(Center);
	const float Px = UnitsPerPixel * Scale;

	FLinearColor LightSum(0.f, 0.f, 0.f, 0.f);
	float LightWeight = 0.f;
	for (int32 L = 0; L < Str->Layers.Num(); ++L)
	{
		const FRoStrLayer& Layer = Str->Layers[L];
		FRoStrKey K;
		UTexture2D* Tex = nullptr;
		const bool bVisible = EvalLayer(Layer, Frame, K) && K.Color.A > 1.f && (Tex = LayerTexture(Layer, K.AniFrame)) != nullptr;
		if (!bVisible)
		{
			if (SectionCreated[L])
			{
				Mesh->SetMeshSectionVisible(L, false);
			}
			continue;
		}
		// Cantos em pixels de tela (y para baixo) -> mundo, girados por Angle no plano da tela
		const float Rad = FMath::DegreesToRadians(K.Angle); // graus, horario na tela
		const float C = FMath::Cos(Rad), S = FMath::Sin(Rad);
		TArray<FVector> Verts;
		TArray<FVector2D> UVs;
		// UV = retangulo da textura (u0, v0, u1, v1); os 4 ultimos valores repetem e nao sao usados
		const bool bUVSet = K.UV[2] != K.UV[0] && K.UV[3] != K.UV[1];
		const float U0 = bUVSet ? K.UV[0] : 0.f, V0 = bUVSet ? K.UV[1] : 0.f, U1 = bUVSet ? K.UV[2] : 1.f, V1 = bUVSet ? K.UV[3] : 1.f;
		const FVector2D CornerUV[4] = { {U0, V0}, {U1, V0}, {U1, V1}, {U0, V1} }; // cantos: sup. esq., sup. dir., inf. dir., inf. esq.
		for (int32 c = 0; c < 4; ++c)
		{
			const float X = K.XY[c] * C - K.XY[4 + c] * S;
			const float Y = K.XY[c] * S + K.XY[4 + c] * C;
			const float ScreenX = K.Pos.X - StrOriginX + X;
			const float ScreenY = K.Pos.Y - StrOriginY + Y;
			Verts.Add(Right * (ScreenX * Px) - Up * (ScreenY * Px));
			UVs.Add(CornerUV[c]);
		}
		const FLinearColor Tint(K.Color.R / 255.f, K.Color.G / 255.f, K.Color.B / 255.f, FMath::Clamp(K.Color.A / 255.f, 0.f, 1.f));
		const FColor VColor = Tint.ToFColor(false);
		const TArray<FColor> Colors = { VColor, VColor, VColor, VColor };
		const TArray<int32> Tris = { 0, 1, 2, 0, 2, 3 };
		const TArray<FVector> Normals = { -CamM.GetUnitAxis(EAxis::X), -CamM.GetUnitAxis(EAxis::X), -CamM.GetUnitAxis(EAxis::X), -CamM.GetUnitAxis(EAxis::X) };
		if (!SectionCreated[L])
		{
			Mesh->CreateMeshSection(L, Verts, Tris, Normals, UVs, Colors, TArray<FProcMeshTangent>(), false);
			Mesh->SetMaterial(L, LayerMaterials[L]);
			SectionCreated[L] = true;
		}
		else
		{
			Mesh->UpdateMeshSection(L, Verts, Normals, UVs, Colors, TArray<FProcMeshTangent>());
			Mesh->SetMeshSectionVisible(L, true);
		}
		LayerMaterials[L]->SetTextureParameterValue(TEXT("Tex"), Tex);

		// Luz: cor media da textura x cor da camada, pesada pelo alfa (camadas aditivas brilham mais)
		const FString TexName = TexturePath(Layer.Textures[FMath::Clamp(FMath::FloorToInt(K.AniFrame), 0, Layer.Textures.Num() - 1)]);
		const float W = Tint.A * (IsAdditive(K.DstBlend) ? 1.f : 0.4f);
		LightSum += FMythRoUITextures::AverageColor(TexName) * Tint * W;
		LightWeight += W;
	}

	if (!bLoggedFirstFrame && LightWeight > 0.f)
	{
		bLoggedFirstFrame = true;
		UE_LOG(LogTemp, Verbose, TEXT("MythStrEffect: frame %.1f of %d with visible layers (%d layers, light %.2f)"), Frame, Str->MaxKey, Str->Layers.Num(), LightWeight);
	}
	if (LightWeight > 0.01f)
	{
		FLinearColor Col = LightSum / LightWeight;
		const float Peak = FMath::Max3(Col.R, Col.G, Col.B);
		Col = Peak > 0.f ? Col / Peak : FLinearColor::White; // cor saturada; o brilho vem da intensidade
		Light->SetLightColor(Col);
		Light->SetIntensity(LightIntensity * FMath::Clamp(LightWeight, 0.f, 2.f) * 0.5f);
		Light->SetWorldLocation(Origin + FVector(0.f, 0.f, 70.f));
		Light->SetVisibility(true);
	}
	else
	{
		Light->SetVisibility(false);
	}
}

// ------------------------------------------------------------------------------------------------ clarao

AMythLightFlash::AMythLightFlash()
{
	PrimaryActorTick.bCanEverTick = true;
	Light = CreateDefaultSubobject<UPointLightComponent>(TEXT("Light"));
	SetRootComponent(Light);
	Light->SetMobility(EComponentMobility::Movable);
	Light->SetIntensityUnits(ELightUnits::Candelas);
	Light->SetIntensity(0.f);
	Light->SetAttenuationRadius(550.f);
	Light->SetSourceRadius(25.f);
	Light->SetCastShadows(true);
}

AMythLightFlash* AMythLightFlash::Flash(UObject* WorldContext, const FVector& Location, const FLinearColor& Color, float Intensity, float InDuration, AActor* InFollow)
{
	UWorld* World = WorldContext ? WorldContext->GetWorld() : nullptr;
	if (!World)
	{
		return nullptr;
	}
	FActorSpawnParameters Params;
	Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
	AMythLightFlash* F = World->SpawnActor<AMythLightFlash>(Location, FRotator::ZeroRotator, Params);
	if (F)
	{
		F->Light->SetLightColor(Color);
		F->Peak = Intensity;
		F->Duration = FMath::Max(0.1f, InDuration);
		F->Follow = InFollow;
	}
	return F;
}

void AMythLightFlash::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);
	Elapsed += DeltaSeconds;
	if (Elapsed >= Duration)
	{
		Destroy();
		return;
	}
	if (Follow.IsValid())
	{
		SetActorLocation(Follow->GetActorLocation() + FVector(0.f, 0.f, 25.f));
	}
	// Sobe em 12% do tempo e apaga com curva suave
	const float T = Elapsed / Duration;
	const float Env = T < 0.12f ? T / 0.12f : FMath::Square(1.f - (T - 0.12f) / 0.88f);
	Light->SetIntensity(Peak * Env);
}

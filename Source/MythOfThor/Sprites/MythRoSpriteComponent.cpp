#include "Sprites/MythRoSpriteComponent.h"

#include "World/MythDayNightActor.h"
#include "World/MythNightLight.h"
#include "Sprites/MythSprEffect.h"
#include "Sprites/MythRoSound.h"
#include "Components/PointLightComponent.h"
#include "EngineUtils.h"
#include "Camera/PlayerCameraManager.h"
#include "Engine/Texture2D.h"
#include "GameFramework/Actor.h"
#include "GameFramework/PlayerController.h"
#include "HAL/PlatformMisc.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Misc/ConfigCacheIni.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "Dom/JsonObject.h"
#include "Serialization/JsonReader.h"
#include "Serialization/JsonSerializer.h"

namespace
{
	// Caminhos do GRF em coreano, escritos como codigos Unicode para nao depender da codificacao do fonte.
	const TCHAR* HumanDir = TEXT("인간족");   // 인간족
	const TCHAR* BodyDir = TEXT("몸통");          // 몸통
	const TCHAR* HeadDir = TEXT("머리통");    // 머리통
	const TCHAR* HairPalDir = TEXT("머리");       // 머리
	const TCHAR* MaleTag = TEXT("남");                // 남
	const TCHAR* FemaleTag = TEXT("여");              // 여
	const TCHAR* MonsterDir = TEXT("몬스터"); // 몬스터
	const TCHAR* AccessoryDir = TEXT("악세사리"); // 악세사리
	const TCHAR* ShieldDir = TEXT("방패");             // 방패
	const TCHAR* RobeDir = TEXT("로브");   // 로브
	const TCHAR* EffectSpriteDir = TEXT("이팩트"); // 이팩트 (o carrinho fica aqui)
	const TCHAR* CartName = TEXT("손수레");   // 손수레

	/** Config/equip_sprites.json (tools/gen_equip_sprites.py): nomes dos sprites de equipamento do cliente. */
	struct FEquipTables
	{
		TMap<int32, FString> Hats;
		TMap<int32, FString> Weapons;
		TMap<int32, FString> Shields;
		TMap<int32, FString> WeaponJobs;
	};

	const FEquipTables& EquipTables()
	{
		static FEquipTables T;
		static bool bLoaded = false;
		if (bLoaded)
		{
			return T;
		}
		bLoaded = true;
		FString Text;
		TSharedPtr<FJsonObject> Root;
		if (!FFileHelper::LoadFileToString(Text, *FPaths::Combine(FPaths::ProjectConfigDir(), TEXT("equip_sprites.json")))
			|| !FJsonSerializer::Deserialize(TJsonReaderFactory<>::Create(Text), Root) || !Root.IsValid())
		{
			UE_LOG(LogTemp, Warning, TEXT("MythRoSprite: Config/equip_sprites.json ausente (tools/gen_equip_sprites.py)"));
			return T;
		}
		auto Fill = [&Root](const TCHAR* Key, TMap<int32, FString>& Out)
		{
			const TSharedPtr<FJsonObject>* Obj = nullptr;
			if (Root->TryGetObjectField(Key, Obj))
			{
				for (const auto& Pair : (*Obj)->Values)
				{
					Out.Add(FCString::Atoi(*Pair.Key), Pair.Value->AsString());
				}
			}
		};
		Fill(TEXT("hats"), T.Hats);
		Fill(TEXT("weapons"), T.Weapons);
		Fill(TEXT("shields"), T.Shields);
		Fill(TEXT("weapon_jobs"), T.WeaponJobs);
		return T;
	}

	/** Config/robe_sprites.json (tools/gen_robe_sprites.py): id visual da capa -> nome da pasta em data/sprite/로브. */
	const TMap<int32, FString>& RobeNames()
	{
		static TMap<int32, FString> Names;
		static bool bLoaded = false;
		if (!bLoaded)
		{
			bLoaded = true;
			FString Text;
			TSharedPtr<FJsonObject> Root;
			const TSharedPtr<FJsonObject>* Obj = nullptr;
			if (FFileHelper::LoadFileToString(Text, *FPaths::Combine(FPaths::ProjectConfigDir(), TEXT("robe_sprites.json")))
				&& FJsonSerializer::Deserialize(TJsonReaderFactory<>::Create(Text), Root) && Root.IsValid() && Root->TryGetObjectField(TEXT("robes"), Obj))
			{
				for (const auto& Pair : (*Obj)->Values)
				{
					Names.Add(FCString::Atoi(*Pair.Key), Pair.Value->AsString());
				}
			}
			else
			{
				UE_LOG(LogTemp, Warning, TEXT("MythRoSprite: Config/robe_sprites.json ausente (tools/gen_robe_sprites.py)"));
			}
		}
		return Names;
	}

	// Grupos de acao do .act (acao = grupo * 8 + direcao), iguais ao cliente original.
	// Jogador: 0 parado, 1 andando, 2 sentado, 3 pegando, 4 em guarda, 5 ataque, 6 dano, 7 congelado, 8 morto...
	constexpr int32 GroupIdle = 0;
	constexpr int32 GroupWalk = 1;
	constexpr int32 GroupSit = 2;
	constexpr int32 GroupPickup = 3;
	constexpr int32 GroupReady = 4;
	constexpr float ReadyFightSeconds = 4.f;
	constexpr int32 GroupAttack = 5;
	constexpr int32 GroupHurt = 6;
	constexpr int32 GroupFrozen = 7;
	constexpr int32 GroupDead = 8;
	// Monstro: 0 parado, 1 andando, 2 ataque, 3 dano, 4 morte.
	constexpr int32 MobGroupAttack = 2;
	constexpr int32 MobGroupHurt = 3;
	constexpr int32 MobGroupDead = 4;

	// Materiais de sprite do Paper2D: textura "SpriteTexture" x cor do vertice, mascarados (com e sem iluminacao).
	const TCHAR* LitMaterialPath = TEXT("/Paper2D/MaskedLitSpriteMaterial.MaskedLitSpriteMaterial");
	const TCHAR* UnlitMaterialPath = TEXT("/Paper2D/MaskedUnlitSpriteMaterial.MaskedUnlitSpriteMaterial");
}

UMythRoSpriteComponent::UMythRoSpriteComponent(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	PrimaryComponentTick.bCanEverTick = true;
	PrimaryComponentTick.TickGroup = TG_PostPhysics; // depois do movimento e da camera do quadro
	SetCollisionEnabled(ECollisionEnabled::NoCollision);
	CastShadow = false;
	bUseAsyncCooking = true;
	JobSpriteName = TEXT("초보자"); // 초보자 (Aprendiz)
}

FString UMythRoSpriteComponent::GetAssetsRoot()
{
	FString Root;
	if (GConfig && GConfig->GetString(TEXT("Myth"), TEXT("AssetsRoot"), Root, GGameIni) && !Root.IsEmpty())
	{
		return Root;
	}
	const FString Env = FPlatformMisc::GetEnvironmentVariable(TEXT("MYTH_ASSETS"));
	return Env.IsEmpty() ? FString() : FPaths::Combine(Env, TEXT("grf"));
}

void UMythRoSpriteComponent::BeginPlay()
{
	Super::BeginPlay();
	FeetRelativeLocation = GetRelativeLocation();
	ReloadSprites();

	if (bCastSpriteShadow && GetOwner())
	{
		ShadowMesh = NewObject<UProceduralMeshComponent>(GetOwner(), TEXT("RoSpriteShadow"));
		ShadowMesh->SetupAttachment(GetAttachParent());
		ShadowMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		ShadowMesh->SetRenderInMainPass(false);   // invisivel na imagem...
		ShadowMesh->SetCastShadow(true);           // ...mas projeta a sombra
		ShadowMesh->bCastDynamicShadow = true;
		ShadowMesh->RegisterComponent();
	}
	if (bBlobShadow)
	{
		CreateBlobShadow();
	}
	DayNight = AMythDayNightActor::Find(this);
	if (const UWorld* World = GetWorld())
	{
		for (TActorIterator<AMythNightLight> It(World); It; ++It)
		{
			NearbyLamps.Add(*It);
		}
	}
}

void UMythRoSpriteComponent::UpdateTint(const FVector& Feet, float DeltaTime)
{
	if (!bTintLighting || bLitByScene)
	{
		CurrentTint = FLinearColor::White;
		return;
	}
	if (!DayNight.IsValid())
	{
		DayNight = AMythDayNightActor::Find(this);
	}
	const float Night = DayNight.IsValid() ? DayNight->GetNightFactor() : 0.f;

	// Sombra "neles": a cada 0,2 s, um raio dos pes em direcao ao sol (canal Camera = so a malha visual do mapa).
	SunTraceTimer -= DeltaTime;
	if (SunTraceTimer <= 0.f && DayNight.IsValid() && GetWorld())
	{
		SunTraceTimer = 0.2f;
		const FVector Start = Feet + FVector(0.f, 0.f, 60.f);
		const FVector End = Start - DayNight->GetLightDirection() * 5000.f;
		FCollisionQueryParams Params(SCENE_QUERY_STAT(MythSpriteSunTrace), false, GetOwner());
		FHitResult Hit;
		const bool bBlocked = GetWorld()->LineTraceSingleByChannel(Hit, Start, End, ECC_Camera, Params);
		SunVisibility = bBlocked ? 0.f : 1.f;
	}

	// Dia: branco (ou escurecido sob um predio); noite: azulado. Postes somam luz quente pela distancia.
	const float DayShade = FMath::Lerp(ShadowedTint, 1.f, SunVisibility);
	FLinearColor Target = FMath::Lerp(FLinearColor::White * DayShade, NightTint, Night);
	for (const TWeakObjectPtr<AMythNightLight>& Lamp : NearbyLamps)
	{
		if (!Lamp.IsValid() || !Lamp->Light)
		{
			continue;
		}
		const float Radius = Lamp->Light->AttenuationRadius;
		const float Dist = FVector::Dist(Lamp->GetActorLocation(), Feet);
		if (Dist >= Radius || Lamp->NightIntensity <= 0.f)
		{
			continue;
		}
		const float Falloff = FMath::Square(1.f - Dist / Radius);
		const float On = Lamp->Light->Intensity / Lamp->NightIntensity;
		Target += Lamp->Light->GetLightColor() * (Falloff * On * LampTintStrength);
	}
	Target.R = FMath::Clamp(Target.R, 0.f, 1.15f);
	Target.G = FMath::Clamp(Target.G, 0.f, 1.15f);
	Target.B = FMath::Clamp(Target.B, 0.f, 1.15f);
	Target.A = 1.f;
	// Transicao suave (entrar/sair da sombra de um predio, acender dos postes)
	const float Alpha = FMath::Clamp(DeltaTime * 5.f, 0.f, 1.f);
	CurrentTint = FMath::Lerp(CurrentTint, Target, Alpha);
}

void UMythRoSpriteComponent::CreateBlobShadow()
{
	const FString Base = FPaths::Combine(GetAssetsRoot(), TEXT("data"), TEXT("sprite"), TEXT("shadow"));
	if (!GetOwner() || !LoadSource(BlobSource, Base + TEXT(".spr"), Base + TEXT(".act"), FString()) || BlobSource.Spr.Indexed.Num() == 0)
	{
		return;
	}
	// A sombra do RO e uma elipse semitransparente: material translucido, alpha pela cor do vertice.
	if (UMaterialInterface* Translucent = LoadObject<UMaterialInterface>(nullptr, TEXT("/Paper2D/TranslucentUnlitSpriteMaterial.TranslucentUnlitSpriteMaterial")))
	{
		BlobSource.Material = UMaterialInstanceDynamic::Create(Translucent, this);
		BlobSource.Material->SetTextureParameterValue(TEXT("SpriteTexture"), BlobSource.Atlas);
	}
	BlobMesh = NewObject<UProceduralMeshComponent>(GetOwner(), TEXT("RoSpriteBlobShadow"));
	BlobMesh->SetupAttachment(GetAttachParent());
	BlobMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	BlobMesh->SetCastShadow(false);
	BlobMesh->RegisterComponent();

	const FRoSprImage& Img = BlobSource.Spr.Indexed[0];
	const FBox2f UV = BlobSource.IndexedUV[0];
	const float HalfW = Img.Width * UnitsPerPixel * BlobShadowScale * 0.5f;
	const float HalfH = Img.Height * UnitsPerPixel * BlobShadowScale * 0.5f;
	const TArray<FVector> Verts = { {-HalfH, -HalfW, 0.f}, {-HalfH, HalfW, 0.f}, {HalfH, HalfW, 0.f}, {HalfH, -HalfW, 0.f} };
	const TArray<FVector2D> Tex = { {UV.Min.X, UV.Min.Y}, {UV.Max.X, UV.Min.Y}, {UV.Max.X, UV.Max.Y}, {UV.Min.X, UV.Max.Y} };
	const uint8 BlobAlpha = 165; // o shadow.act pede 128 sobre o fundo do RO; aqui o chao HD (Lumen) e mais claro
	const TArray<FColor> Colors = { FColor(255, 255, 255, BlobAlpha), FColor(255, 255, 255, BlobAlpha), FColor(255, 255, 255, BlobAlpha), FColor(255, 255, 255, BlobAlpha) };
	const TArray<FVector> Normals = { FVector::UpVector, FVector::UpVector, FVector::UpVector, FVector::UpVector };
	const TArray<int32> Tris = { 0, 1, 2, 0, 2, 3, 0, 2, 1, 0, 3, 2 };
	BlobMesh->CreateMeshSection(0, Verts, Tris, Normals, Tex, Colors, TArray<FProcMeshTangent>(), false);
	BlobMesh->SetMaterial(0, BlobSource.Material);
}

void UMythRoSpriteComponent::PlayFrameEvent(const FRoActFrame& Frame, int32 ActionIndex, int32 FrameIndex)
{
	const FIntPoint Key(ActionIndex, FrameIndex);
	if (Key == LastSoundFrame)
	{
		return;
	}
	LastSoundFrame = Key;
	if (!bPlayActSounds || !Body.Act.Events.IsValidIndex(Frame.EventId))
	{
		return;
	}
	// Eventos do .act: nomes de .wav tocam o som; "atk" (jogadores) toca o balanco da arma equipada (WeaponSoundTable)
	const FString& Event = Body.Act.Events[Frame.EventId];
	if (!GetOwner())
	{
		return;
	}
	if (Event.EndsWith(TEXT(".wav"), ESearchCase::IgnoreCase))
	{
		UMythRoSound::PlayWavAtLocation(this, Event, GetOwner()->GetActorLocation());
	}
	else if (Event.Equals(TEXT("atk"), ESearchCase::IgnoreCase) && !IsMonster())
	{
		// A arma vem como o "view" do item; nas armas comuns ele e o proprio tipo (W_DAGGER 1 ... W_2HSTAFF 23)
		const int32 WeaponType = Weapon > 0 && Weapon <= 23 ? Weapon : (Weapon > 0 ? 2 : 0);
		UMythRoSound::PlayWavAtLocation(this, UMythRoSound::WeaponSwingWav(WeaponType), GetOwner()->GetActorLocation());
	}
}

void UMythRoSpriteComponent::OnComponentDestroyed(bool bDestroyingHierarchy)
{
	if (ShadowMesh)
	{
		ShadowMesh->DestroyComponent();
		ShadowMesh = nullptr;
	}
	if (BlobMesh)
	{
		BlobMesh->DestroyComponent();
		BlobMesh = nullptr;
	}
	Super::OnComponentDestroyed(bDestroyingHierarchy);
}

void UMythRoSpriteComponent::UpdateShadowTransform(const FVector& Feet)
{
	if (!ShadowMesh)
	{
		return;
	}
	if (!DayNight.IsValid())
	{
		DayNight = AMythDayNightActor::Find(this);
	}
	// Plano em pe, de frente para a luz (so o yaw da luz importa): a silhueta inteira vira sombra no chao.
	FVector LightDir = DayNight.IsValid() ? DayNight->GetLightDirection() : FVector(0.3f, 0.2f, -1.f);

	// Comprimento da sombra no chao = altura do sprite / tan(elevacao da luz). Perto do meio-dia ela fica do
	// tamanho do blob: a copia em pe e achatada ate sumir (fica so o blob) e cresce de novo quando o sol baixa.
	const float Horizontal = FVector2D(LightDir.X, LightDir.Y).Size();
	const float Down = FMath::Max(-LightDir.Z, KINDA_SMALL_NUMBER);
	const FBox& Frame = GetFrameLocalBounds();
	const float Height = Frame.IsValid ? FMath::Max(Frame.GetSize().Z, 40.f) : 90.f;
	const float ShadowLength = Height * Horizontal / Down;
	const float Fade = ShadowFadeLength > 0.f ? FMath::Clamp((ShadowLength - ShadowFadeLength) / ShadowFadeLength, 0.f, 1.f) : 1.f;
	// Sprites baixos (Poring, Fabre...) projetam uma mancha estranha: ficam so com a sombra redonda, como no RO
	const bool bShowCast = bCastSpriteShadow && Fade > 0.02f && Height >= MinCastShadowHeight;
	if (ShadowMesh->CastShadow != bShowCast)
	{
		ShadowMesh->SetCastShadow(bShowCast);
	}
	ShadowMesh->SetWorldScale3D(FVector(1.f, 1.f, FMath::Max(Fade, 0.02f)));

	LightDir.Z = 0.f;
	if (!LightDir.Normalize())
	{
		LightDir = FVector::ForwardVector; // luz a pino: qualquer orientacao serve
	}
	const FVector Right = FVector::CrossProduct(FVector::UpVector, LightDir).GetSafeNormal();
	// A base fica um pouco abaixo do chao: a sombra nasce embaixo do centro da elipse, que esconde a emenda
	ShadowMesh->SetWorldLocationAndRotation(Feet - FVector(0.f, 0.f, CastShadowSink), FRotationMatrix::MakeFromXZ(Right, FVector::UpVector).Rotator());
}

void UMythRoSpriteComponent::ReloadSprites()
{
	const FString Root = GetAssetsRoot();
	if (IsMonster())
	{
		const FString Folder = MonsterSpriteFolder.IsEmpty() ? FString(MonsterDir) : MonsterSpriteFolder;
		const FString Base = FPaths::Combine(Root, TEXT("data"), TEXT("sprite"), Folder, MonsterSpriteName);
		Head = FMythRoSpriteSource();
		if (!LoadSource(Body, Base + TEXT(".spr"), Base + TEXT(".act"), FString()))
		{
			UE_LOG(LogTemp, Warning, TEXT("MythRoSprite: falha ao carregar o monstro '%s' (raiz '%s')."), *MonsterSpriteName, *Root);
		}
		return;
	}

	const FString Sex = bMale ? MaleTag : FemaleTag;
	const FString Sprite = FPaths::Combine(Root, TEXT("data"), TEXT("sprite"), HumanDir);

	const FString BodyBase = FPaths::Combine(Sprite, BodyDir, Sex, FString::Printf(TEXT("%s_%s"), *JobSpriteName, *Sex));
	const FString HeadBase = FPaths::Combine(Sprite, HeadDir, Sex, FString::Printf(TEXT("%d_%s"), HairStyle, *Sex));
	const FString Palette = HairColor > 0
		? FPaths::Combine(Root, TEXT("data"), TEXT("palette"), HairPalDir, FString::Printf(TEXT("%s%d_%s_%d.pal"), HairPalDir, HairStyle, *Sex, HairColor))
		: FString();

	const bool bBody = LoadSource(Body, BodyBase + TEXT(".spr"), BodyBase + TEXT(".act"), FString());
	const bool bHead = LoadSource(Head, HeadBase + TEXT(".spr"), HeadBase + TEXT(".act"), Palette);
	LoadEquipment(Root, Sex);
	if (!bBody || !bHead)
	{
		UE_LOG(LogTemp, Warning, TEXT("MythRoSprite: falha ao carregar sprites (raiz '%s'). Corpo=%d Cabeca=%d. Configure [Myth] AssetsRoot no DefaultGame.ini."),
			*Root, bBody, bHead);
	}
}

void UMythRoSpriteComponent::LoadEquipment(const FString& Root, const FString& Sex)
{
	// Caminhos do cliente original (iguais ao roBrowser DB.getHatPath/getWeaponPath/getShieldPath)
	const FEquipTables& T = EquipTables();
	const FString SpriteRoot = FPaths::Combine(Root, TEXT("data"), TEXT("sprite"));
	auto Load = [this](FMythRoSpriteSource& Source, const FString& Base)
	{
		Source = FMythRoSpriteSource();
		if (!Base.IsEmpty() && FPaths::FileExists(Base + TEXT(".spr")))
		{
			const bool bOk = LoadSource(Source, Base + TEXT(".spr"), Base + TEXT(".act"), FString());
			UE_LOG(LogTemp, Verbose, TEXT("MythRoSprite: equipamento %s: %s (%d acoes)"), bOk ? TEXT("ok") : TEXT("falhou"), *Base, Source.Act.Actions.Num());
		}
		else if (!Base.IsEmpty())
		{
			UE_LOG(LogTemp, Verbose, TEXT("MythRoSprite: equipamento sem sprite: %s"), *Base);
		}
	};
	auto HatBase = [&](int32 View)
	{
		const FString* Name = View > 0 ? T.Hats.Find(View) : nullptr;
		return Name ? FPaths::Combine(SpriteRoot, AccessoryDir, Sex, Sex + *Name) : FString();
	};
	// O RO nao desenha duas vezes o mesmo acessorio que ocupa mais de um lugar (ex.: topo+meio)
	Load(HatBottom, HatBase(HeadBottom));
	Load(HatMid, HeadMid != HeadTop && HeadMid != HeadBottom ? HatBase(HeadMid) : FString());
	Load(HatTop, HeadTop != HeadBottom ? HatBase(HeadTop) : FString());

	const FString* JobFolder = T.WeaponJobs.Find(JobClass);
	const FString Job = JobFolder ? *JobFolder : FString(TEXT("초보자"));
	auto WeaponBase = [&](int32 View)
	{
		if (View <= 0)
		{
			return FString();
		}
		const FString* Name = T.Weapons.Find(View);
		return FPaths::Combine(SpriteRoot, HumanDir, Job, FString::Printf(TEXT("%s_%s%s"), *Job, *Sex, Name ? **Name : *FString::Printf(TEXT("_%d"), View)));
	};
	Load(WeaponSource, WeaponBase(Weapon));
	// Escudo: ids fora de 2100-2200 acima de 500 sao a arma da mao esquerda (Mercenario com duas armas)
	if (Shield > 500 && (Shield < 2100 || Shield > 2200))
	{
		Load(ShieldSource, WeaponBase(Shield));
	}
	else if (Shield > 0)
	{
		// Escudos com nome: <classe>_<sexo>_<nome> (가드, 버클러...); os demais: <classe>_<sexo>_<id>_방패
		const FString* Name = T.Shields.Find(Shield);
		const FString Dir = FPaths::Combine(SpriteRoot, ShieldDir, Job);
		FString Base = Name ? FPaths::Combine(Dir, FString::Printf(TEXT("%s_%s_%s"), *Job, *Sex, **Name))
			: FPaths::Combine(Dir, FString::Printf(TEXT("%s_%s_%d_%s"), *Job, *Sex, Shield, ShieldDir));
		if (!FPaths::FileExists(Base + TEXT(".spr")) && T.Shields.Contains(1))
		{
			Base = FPaths::Combine(Dir, FString::Printf(TEXT("%s_%s_%s"), *Job, *Sex, *T.Shields[1])); // sem sprite: guarda, como o cliente
		}
		Load(ShieldSource, Base);
	}
	else
	{
		ShieldSource = FMythRoSpriteSource();
	}

	// Carrinho: data/sprite/이팩트/손수레<1..5>, 8 acoes (uma por direcao) de 6 quadros
	Load(CartSource, CartType > 0 ? FPaths::Combine(SpriteRoot, EffectSpriteDir, FString::Printf(TEXT("%s%d"), CartName, CartType)) : FString());

	// Capa/asas/mochila: data/sprite/로브/<nome>/<sexo>/<sprite da classe>_<sexo> (o mesmo nome do corpo da classe)
	const FString* RobeName = Robe > 0 ? RobeNames().Find(Robe) : nullptr;
	Load(RobeSource, RobeName ? FPaths::Combine(SpriteRoot, RobeDir, *RobeName, Sex, FString::Printf(TEXT("%s_%s"), *JobSpriteName, *Sex)) : FString());
}

bool UMythRoSpriteComponent::LoadSource(FMythRoSpriteSource& Source, const FString& SprPath, const FString& ActPath, const FString& PalettePath)
{
	Source = FMythRoSpriteSource();
	TArray<uint8> SprBytes, ActBytes, PalBytes;
	if (!FFileHelper::LoadFileToArray(SprBytes, *SprPath) || !FFileHelper::LoadFileToArray(ActBytes, *ActPath))
	{
		UE_LOG(LogTemp, Warning, TEXT("MythRoSprite: arquivo nao encontrado: %s"), *SprPath);
		return false;
	}

	TArray<FColor> Palette;
	const bool bPalette = !PalettePath.IsEmpty() && FFileHelper::LoadFileToArray(PalBytes, *PalettePath) && FRoSpr::LoadPalette(PalBytes, Palette);
	if (!Source.Spr.Load(SprBytes, bPalette ? &Palette : nullptr) || !Source.Act.Load(ActBytes))
	{
		UE_LOG(LogTemp, Warning, TEXT("MythRoSprite: arquivo invalido: %s"), *SprPath);
		return false;
	}
	BuildAtlas(Source);

	if (UMaterialInterface* Base = LoadObject<UMaterialInterface>(nullptr, bLitByScene ? LitMaterialPath : UnlitMaterialPath))
	{
		Source.Material = UMaterialInstanceDynamic::Create(Base, this);
		Source.Material->SetTextureParameterValue(TEXT("SpriteTexture"), Source.Atlas);
	}
	return Source.IsValid() && Source.Material != nullptr;
}

void UMythRoSpriteComponent::BuildAtlas(FMythRoSpriteSource& Source)
{
	// Empacota todas as imagens em prateleiras (1 px de margem), indexadas e depois RGBA.
	constexpr int32 MaxWidth = 1024;
	TArray<const FRoSprImage*> Images;
	for (const FRoSprImage& Img : Source.Spr.Indexed) { Images.Add(&Img); }
	for (const FRoSprImage& Img : Source.Spr.Rgba) { Images.Add(&Img); }

	TArray<FIntPoint> Positions;
	Positions.SetNum(Images.Num());
	// Margem de 2 px (e 1 px antes da primeira): com filtro bilinear uma imagem nao "vaza" na vizinha
	int32 X = 1, Y = 1, ShelfH = 0, UsedW = 1;
	for (int32 i = 0; i < Images.Num(); ++i)
	{
		const FIntPoint Size(Images[i]->Width + 2, Images[i]->Height + 2);
		if (X + Size.X > MaxWidth)
		{
			X = 1;
			Y += ShelfH;
			ShelfH = 0;
		}
		Positions[i] = FIntPoint(X, Y);
		X += Size.X;
		UsedW = FMath::Max(UsedW, X);
		ShelfH = FMath::Max(ShelfH, Size.Y);
	}
	const int32 W = FMath::Max(UsedW, 1);
	const int32 H = FMath::Max(Y + ShelfH, 1);

	TArray<FColor> Pixels;
	Pixels.SetNumZeroed(W * H);
	for (int32 i = 0; i < Images.Num(); ++i)
	{
		const FRoSprImage& Img = *Images[i];
		for (int32 Row = 0; Row < Img.Height; ++Row)
		{
			FMemory::Memcpy(&Pixels[(Positions[i].Y + Row) * W + Positions[i].X], &Img.Pixels[Row * Img.Width], Img.Width * sizeof(FColor));
		}
		const FBox2f UV(FVector2f(float(Positions[i].X) / W, float(Positions[i].Y) / H),
		                FVector2f(float(Positions[i].X + Img.Width) / W, float(Positions[i].Y + Img.Height) / H));
		(i < Source.Spr.Indexed.Num() ? Source.IndexedUV : Source.RgbaUV).Add(UV);
	}

	UTexture2D* Tex = UTexture2D::CreateTransient(W, H, PF_B8G8R8A8);
	if (!Tex)
	{
		return;
	}
	// O cliente original desenha os sprites com filtro bilinear (suave); "nearest" serrilha em escala fracionaria
	Tex->Filter = bSmoothSprites ? TF_Bilinear : TF_Nearest;
	Tex->SRGB = true;
	Tex->NeverStream = true;
	Tex->LODGroup = TEXTUREGROUP_Pixels2D;
	FTexture2DMipMap& Mip = Tex->GetPlatformData()->Mips[0];
	void* Data = Mip.BulkData.Lock(LOCK_READ_WRITE);
	FMemory::Memcpy(Data, Pixels.GetData(), Pixels.Num() * sizeof(FColor)); // FColor ja e BGRA na memoria
	Mip.BulkData.Unlock();
	Tex->UpdateResource();
	Source.Atlas = Tex;
}

// ------------------------------------------------------------------------------------------- estado

namespace
{
	// OPT3 do rAthena (StatusState.js do roBrowser, EntityState.js): o bit de cada skill e a cor que ela da ao sprite
	int32 VirtueBitFromEfst(int32 Efst)
	{
		switch (Efst)
		{
		case 2: case 68: case 105: case 161: return 0x1;          // Aceleracao (QUICKEN)
		case 25: case 188: case 154: return 0x2;                  // Overthrust, Overthrust Max, SWOO
		case 31: return 0x4;                                      // Energy Coat
		case 86: return 0x8;                                      // Explosao Espiritual
		case 87: return 0x10;                                     // Corpo de Aco
		case 85: return 0x20;                                     // Parar Lamina
		case 103: return 0x40;                                    // Aura de Lamina
		case 107: return 0x80;                                    // Berserk
		case 121: case 122: return 0x400;                         // Marionete
		case 110: case 473: return 0x800;                         // Assumptio
		case 164: case 165: case 166: case 167: return 0x1000;    // Calor de Sol, Lua, Estrela
		case 153: return 0x2000;                                  // Kaite
		case 207: return 0x4000;                                  // Bunshin
		case 149: return 0x8000;                                  // Soul Link
		case 97: return 0x10000;                                  // Propriedade morto-vivo
		case 263: return 0x20000;                                 // Contrato
		case 334: return 0x40000;                                 // Hallucination Walk
		default: return 0;
		}
	}
}

void UMythRoSpriteComponent::SetStatusAttachment(FName Name, const TCHAR* SprFile, bool bOn, bool bAboveHead, float SideOffsetUU, float UpOffsetUU)
{
	TWeakObjectPtr<AActor>* Found = StatusAttachments.Find(Name);
	if (bOn && (!Found || !Found->IsValid()))
	{
		if (AActor* A = AMythSprEffect::Attach(GetOwner(), SprFile, bAboveHead, SideOffsetUU, UpOffsetUU))
		{
			StatusAttachments.Add(Name, A);
		}
	}
	else if (!bOn && Found)
	{
		if (Found->IsValid())
		{
			Found->Get()->Destroy();
		}
		StatusAttachments.Remove(Name);
	}
}

void UMythRoSpriteComponent::SetBodyState(int32 Value)
{
	BodyState = Value;
	SetStatusAttachment(TEXT("sleep"), TEXT("status-sleep"), Value == 4);
	SetStatusAttachment(TEXT("stun"), TEXT("status-stun"), Value == 3);
	RecomputeStatusTint();
}

void UMythRoSpriteComponent::SetHealthState(int32 Value)
{
	HealthState = Value;
	SetStatusAttachment(TEXT("curse"), TEXT("status-curse"), (Value & 0x2) != 0);
	RecomputeStatusTint();
}

void UMythRoSpriteComponent::SetEfstVirtue(int32 EfstId, bool bOn)
{
	const int32 Bit = VirtueBitFromEfst(EfstId);
	if (Bit == 0)
	{
		return;
	}
	Virtue = bOn ? (Virtue | Bit) : (Virtue & ~Bit);
	RecomputeStatusTint();
}

void UMythRoSpriteComponent::SetEffectOption(int32 Option, bool bSelf)
{
	// HIDE 0x2, CLOAK 0x4, CHASEWALK 0x4000, INVISIBLE 0x40: os outros jogadores nao veem; voce se ve escurecido
	const bool bConcealed = (Option & (0x2 | 0x4 | 0x4000 | 0x40)) != 0;
	bStatusHidden = bConcealed && !bSelf;
	bStatusDim = bConcealed && bSelf;
	SetVisibility(!bStatusHidden, true);
	SetStatusAttachment(TEXT("falcon"), TEXT("매"), (Option & 0x10) != 0 && !bStatusHidden, true, 0.f, 12.f); // o falcao do Cacador fica sobre a cabeca e vira com o dono
	RecomputeStatusTint();
}

void UMythRoSpriteComponent::RecomputeStatusTint()
{
	FLinearColor C = FLinearColor::White;
	auto Mul = [&C](float R, float G, float B) { C.R *= R; C.G *= G; C.B *= B; };
	// opt1 (BodyState): 1 pedra, 2 gelo, 3 atordoado, 4 sono, 6 pedra chegando
	switch (BodyState)
	{
	case 1: Mul(0.1f, 0.1f, 0.1f); break;
	case 6: Mul(0.3f, 0.3f, 0.3f); break;
	case 2: Mul(0.f, 0.4f, 0.8f); break;
	default: break;
	}
	// opt2 (HealthState): 0x1 veneno, 0x2 maldicao
	if (HealthState & 0x2) { Mul(0.5f, 0.15f, 0.1f); }
	if (HealthState & 0x1) { Mul(0.9f, 0.4f, 0.8f); }
	// opt3 (Virtue), na ordem do updateVirtue do cliente
	if (Virtue & 0x1) { C.B = 0.f; }
	if (Virtue & 0x8) { C.G = FMath::Min(C.G, 0.75f); C.B = FMath::Min(C.B, 0.75f); }
	if (Virtue & 0x20) { Mul(0.25f, 0.25f, 0.25f); }
	if (Virtue & (0x4 | 0x4000)) { Mul(0.5f, 0.5f, 0.85f); }
	if (Virtue & 0x2) { C.G *= 0.75f; C.B *= 0.75f; }
	if (Virtue & 0x1000) { C.G *= 0.4f; C.B *= 0.4f; }
	if (Virtue & 0x8000) { Mul(0.35f, 0.35f, 0.9f); }
	if (Virtue & 0x10000) { C.R *= 0.7f; C.B *= 0.65f; }
	if (Virtue & 0x400) { Mul(1.f, 0.34f, 0.71f); }
	if (Virtue & 0x80) { Mul(1.f, 0.4f, 0.4f); }
	if (bStatusDim) { Mul(0.55f, 0.55f, 0.55f); }
	StatusTint = C;
}

void UMythRoSpriteComponent::SetState(EMythSpriteState NewState, bool bOneShot)
{
	if (State == EMythSpriteState::Dead && NewState != EMythSpriteState::Idle)
	{
		return;
	}
	State = NewState;
	bOneShotPlaying = bOneShot;
	StateTime = 0.f;
	PlayRate = 1.f;
	WalkDistanceRo = 0.f;
	if (NewState != EMythSpriteState::Idle && NewState != EMythSpriteState::Sit)
	{
		HeadTurn = 0; // andar, bater e apanhar endireitam a cabeca
	}
}

int32 UMythRoSpriteComponent::CartTypeFromOption(int32 Option)
{
	if (Option & 0x400) { return 5; } // OPTION_CART5
	if (Option & 0x200) { return 4; }
	if (Option & 0x100) { return 3; }
	if (Option & 0x080) { return 2; }
	if (Option & 0x008) { return 1; } // OPTION_CART1
	return 0;
}

void UMythRoSpriteComponent::SetHeadDir(uint8 NewHeadDir)
{
	HeadTurn = FMath::Min<uint8>(NewHeadDir, 2);
}

void UMythRoSpriteComponent::SetFacingRoDir(uint8 RoDir)
{
	// Rumo em graus a partir do norte (-Y do mundo), horario visto de cima, leste = +X: rAthena 0 norte, 1 noroeste (-45)...
	const float Heading = FMath::DegreesToRadians(-45.f * static_cast<float>(RoDir & 7));
	FacingWorld = FVector2D(FMath::Sin(Heading), -FMath::Cos(Heading));
}

uint8 UMythRoSpriteComponent::GetFacingRoDir() const
{
	const float Heading = FMath::RadiansToDegrees(FMath::Atan2(static_cast<float>(FacingWorld.X), static_cast<float>(-FacingWorld.Y)));
	return static_cast<uint8>((FMath::RoundToInt(-Heading / 45.f) % 8 + 8) % 8);
}

void UMythRoSpriteComponent::PlayAttack(float AttackDuration)
{
	SetState(EMythSpriteState::Attack, true);
	if (AttackDuration > 0.f && Body.IsValid())
	{
		const FRoActAction* Action = Body.Act.Actions.IsValidIndex(GetActionIndex()) ? &Body.Act.Actions[GetActionIndex()] : nullptr;
		if (Action)
		{
			// Como o GameActor do cliente: velocidade = 432 / attackMT (attackMT em ms), e a animacao inteira sempre
			// cabe no ciclo de ataque do servidor
			const float Total = Action->Frames.Num() * Action->IntervalMs / 1000.f;
			PlayRate = FMath::Max(0.432f / AttackDuration, Total / AttackDuration);
		}
	}
}

void UMythRoSpriteComponent::PlayPickup()
{
	if (!IsMonster() && State != EMythSpriteState::Sit && State != EMythSpriteState::Dead)
	{
		SetState(EMythSpriteState::Pickup, true);
	}
}

void UMythRoSpriteComponent::PlayHit()
{
	if (State != EMythSpriteState::Sit) // sentado nao faz a reacao de golpe (o cliente levanta so pelo servidor)
	{
		SetState(EMythSpriteState::Hit, true);
	}
}

void UMythRoSpriteComponent::SetSitting(bool bSit)
{
	if (bSit != (State == EMythSpriteState::Sit))
	{
		SetState(bSit ? EMythSpriteState::Sit : EMythSpriteState::Idle, false);
	}
}
void UMythRoSpriteComponent::Die()     { SetState(EMythSpriteState::Dead, false); }

void UMythRoSpriteComponent::Revive()
{
	State = EMythSpriteState::Idle;
	bOneShotPlaying = false;
	StateTime = 0.f;
}

void UMythRoSpriteComponent::FaceLocation(const FVector& WorldLocation)
{
	if (const AActor* Owner = GetOwner())
	{
		const FVector2D To(WorldLocation.X - Owner->GetActorLocation().X, WorldLocation.Y - Owner->GetActorLocation().Y);
		if (To.SizeSquared() > 1.0)
		{
			FacingWorld = To.GetSafeNormal();
			Direction = ComputeDirection(GetCameraRotation());
		}
	}
}

int32 UMythRoSpriteComponent::GetActionIndex() const
{
	const bool bMob = IsMonster();
	int32 Group = GroupIdle;
	switch (State)
	{
	case EMythSpriteState::Idle:   Group = GroupIdle; break;
	case EMythSpriteState::Walk:   Group = GroupWalk; break;
	case EMythSpriteState::Attack: Group = bMob ? MobGroupAttack : GroupAttack; break;
	case EMythSpriteState::Hit:    Group = bMob ? MobGroupHurt : GroupHurt; break;
	case EMythSpriteState::Dead:   Group = bMob ? MobGroupDead : GroupDead; break;
	case EMythSpriteState::Sit:    Group = bMob ? GroupIdle : GroupSit; break;
	case EMythSpriteState::Pickup: Group = bMob ? GroupIdle : GroupPickup; break;
	case EMythSpriteState::Ready:  Group = bMob ? GroupIdle : GroupReady; break;
	case EMythSpriteState::Frozen: Group = bMob ? GroupIdle : GroupFrozen; break;
	}
	const int32 Index = Group * 8 + static_cast<int32>(Direction);
	// Sprites com poucas acoes (item no chao: 1; varios NPCs: so as 8 de parado) reaproveitam as que existem
	const int32 Num = Body.Act.Actions.Num();
	if (Num > 0 && Index >= Num)
	{
		return Num >= 8 ? static_cast<int32>(Direction) % Num : 0;
	}
	return Index;
}

int32 UMythRoSpriteComponent::GetFrameIndex(const FRoActAction& Action, bool& bOutFinished) const
{
	bOutFinished = false;
	const int32 Count = Action.Frames.Num();
	if (Count == 0)
	{
		return 0;
	}
	if ((State == EMythSpriteState::Idle || State == EMythSpriteState::Sit) && !IsMonster())
	{
		// Jogador parado ou sentado: a acao tem 3 quadros, um por posicao da cabeca (doridori); monstros animam o idle em loop
		return FMath::Min<int32>(HeadTurn, Count - 1);
	}
	if (State == EMythSpriteState::Walk)
	{
		// Passo pela distancia andada, nao pelo tempo: quadro = dist * 1,48 / delay do .act em ticks de 25 ms
		// (GameActor.cpp ~1660). Mais rapido (AGI Up, montaria) = passos mais rapidos, sem patinar
		const float DelayTicks = FMath::Max(Action.IntervalMs / 25.f, 0.0001f);
		return FMath::FloorToInt(WalkDistanceRo * 1.48f / DelayTicks) % Count;
	}
	const int32 Step = FMath::FloorToInt(StateTime * PlayRate * 1000.f / FMath::Max(Action.IntervalMs, 1.f));
	if (bOneShotPlaying || State == EMythSpriteState::Dead)
	{
		bOutFinished = Step >= Count;
		// Jogador morto repousa no quadro 4 da acao de morte (kDeathMotion)
		const int32 Last = (State == EMythSpriteState::Dead && !IsMonster()) ? FMath::Min(Count - 1, 4) : Count - 1;
		return FMath::Min(Step, Last);
	}
	return Step % Count;
}

// ------------------------------------------------------------------------------------------- tick

FRotator UMythRoSpriteComponent::GetCameraRotation() const
{
	if (const UWorld* World = GetWorld())
	{
		if (const APlayerController* PC = World->GetFirstPlayerController())
		{
			if (PC->PlayerCameraManager)
			{
				return PC->PlayerCameraManager->GetCameraRotation();
			}
		}
	}
	return FRotator::ZeroRotator;
}

void UMythRoSpriteComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	const AActor* Owner = GetOwner();
	if (!Owner || !Body.IsValid())
	{
		return;
	}
	StateTime += DeltaTime;
	const FRotator CamRot = GetCameraRotation();
	const FVector Velocity = Owner->GetVelocity();
	if (!bOneShotPlaying && State != EMythSpriteState::Dead && Velocity.Size2D() > WalkSpeedThreshold)
	{
		FacingWorld = FVector2D(Velocity.X, Velocity.Y).GetSafeNormal(); // andando, olha para onde vai
	}
	// Parado tambem: girar a camera mostra outro lado do personagem
	Direction = ComputeDirection(CamRot);

	if (State != EMythSpriteState::Dead)
	{
		if (State == EMythSpriteState::Walk)
		{
			WalkDistanceRo += Velocity.Size2D() * DeltaTime / 10.f;
		}
		// Congelado (opt1 2) ou atordoado (opt1 3): fica na postura de congelado (grupo 7) ate o estado acabar
		const bool bFrozenBody = BodyState == 2 || BodyState == 3;
		if (bFrozenBody)
		{
			if (State != EMythSpriteState::Frozen) { SetState(EMythSpriteState::Frozen, false); }
			UpdateBillboard(CamRot);
			RebuildMesh(CamRot);
			return;
		}
		if (State == EMythSpriteState::Frozen)
		{
			SetState(EMythSpriteState::Idle, false);
		}
		if (bOneShotPlaying)
		{
			bool bFinished = false;
			if (Body.Act.Actions.IsValidIndex(GetActionIndex()))
			{
				GetFrameIndex(Body.Act.Actions[GetActionIndex()], bFinished);
			}
			// Golpe levado dura 200 ms (kHitReactionDurationMs) e nao segura o proximo comando
			bFinished |= State == EMythSpriteState::Hit && StateTime >= 0.2f;
			if (bFinished)
			{
				const bool bFought = !IsMonster() && (State == EMythSpriteState::Attack || State == EMythSpriteState::Hit);
				ReadyTimeLeft = bFought ? ReadyFightSeconds : 0.f;
				SetState(bFought ? EMythSpriteState::Ready : EMythSpriteState::Idle, false);
			}
		}
		else
		{
			const bool bMoving = Velocity.Size2D() > WalkSpeedThreshold;
			if (State == EMythSpriteState::Sit && !bMoving)
			{
				UpdateBillboard(CamRot);
				RebuildMesh(CamRot);
				return; // sentado fica sentado ate o servidor mandar levantar (ou andar)
			}
			StillTime = bMoving ? 0.f : StillTime + DeltaTime;
			if (bMoving) { ReadyTimeLeft = 0.f; }
			ReadyTimeLeft = FMath::Max(0.f, ReadyTimeLeft - DeltaTime);
			// So volta ao Idle depois de IdleDelay parado: quinas do caminho nao reiniciam o ciclo de caminhada.
			EMythSpriteState NewState = bMoving ? EMythSpriteState::Walk : (StillTime >= IdleDelay ? EMythSpriteState::Idle : State);
			if (State == EMythSpriteState::Ready && !bMoving)
			{
				NewState = ReadyTimeLeft > 0.f ? EMythSpriteState::Ready : EMythSpriteState::Idle; // a postura de combate dura alguns segundos
			}
			if (NewState != State)
			{
				SetState(NewState, false);
			}
		}
	}

	UpdateBillboard(CamRot);
	RebuildMesh(CamRot);
}

ERagnarokDirection UMythRoSpriteComponent::ComputeDirection(const FRotator& CamRot) const
{
	// Mantem a direcao atual enquanto a frente do corpo estiver ate 22,5 + histerese graus do centro dela.
	const float FacingAngle = FMath::RadiansToDegrees(FMath::Atan2(static_cast<float>(FacingWorld.Y), static_cast<float>(FacingWorld.X)));
	const float Relative = FRotator::NormalizeAxis(FacingAngle - CamRot.Yaw);
	const float Center = URagnarokDirectionHelper::DirectionToAngle(Direction);
	if (FMath::Abs(FRotator::NormalizeAxis(Relative - Center)) <= 22.5f + DirectionHysteresisDeg)
	{
		return Direction;
	}
	return URagnarokDirectionHelper::CalculateSpriteDirection(FVector(FacingWorld.X, FacingWorld.Y, 0.0) * 100.0, CamRot, Direction);
}

void UMythRoSpriteComponent::UpdateBillboard(const FRotator& CamRot)
{
	// Paralelo a tela (yaw e pitch da camera), como no cliente original, com os pes no ponto de apoio.
	// Para nao ser cortado por paredes/chao, o sprite e puxado AO LONGO DA LINHA DE VISAO da camera:
	// isso nao muda tamanho nem posicao na tela, so a profundidade.
	const FRotationMatrix CamMatrix(CamRot);
	const FVector CamRight = CamMatrix.GetUnitAxis(EAxis::Y);
	const FVector CamUp = CamMatrix.GetUnitAxis(EAxis::Z);
	const FVector CamForward = CamMatrix.GetUnitAxis(EAxis::X);
	SetWorldRotation(FRotationMatrix::MakeFromXZ(CamRight, CamUp).Rotator());
	if (const USceneComponent* Parent = GetAttachParent())
	{
		FVector Feet = Parent->GetComponentTransform().TransformPosition(FeetRelativeLocation);
		// A capsula sobe/desce nos degraus da malha do chao; suavizar a altura tira o "tremido" ao andar.
		const float Dt = GetWorld() ? GetWorld()->GetDeltaSeconds() : 0.f;
		if (!bFeetZInitialized || FeetHeightSmoothing <= 0.f || FMath::Abs(Feet.Z - SmoothedFeetZ) > 100.f)
		{
			SmoothedFeetZ = Feet.Z;
			bFeetZInitialized = true;
		}
		else
		{
			SmoothedFeetZ = FMath::FInterpTo(SmoothedFeetZ, Feet.Z, Dt, FeetHeightSmoothing);
		}
		Feet.Z = SmoothedFeetZ;
		// Puxado para a camera pela linha de visao; a escala compensa a perspectiva (mesmo tamanho na tela).
		// O puxao segue o RAIO camera->pes (nao o eixo da camera): assim o sprite fica exatamente sobre o ponto do
		// chao na tela, alinhado com a sombra redonda e com a area de clique, tambem nas bordas da tela.
		float Scale = 1.f;
		FVector PullDir = CamForward;
		// O quad e paralelo a tela, entao a parte de cima fica ATRAS dos pes (altura x sen da inclinacao): o puxao precisa cobrir isso,
		// senao uma parede logo atras do personagem corta o corpo (o sprite entra nela)
		const float SpriteHeight = FrameLocalBounds.IsValid ? FMath::Clamp(FrameLocalBounds.Max.Z, 60.f, 500.f) : 190.f;
		const float DepthPull = FMath::Max(CameraDepthOffset, SpriteHeight * FMath::Abs(FMath::Sin(FMath::DegreesToRadians(CamRot.Pitch))) + 10.f);
		if (const APlayerController* PC = GetWorld() ? GetWorld()->GetFirstPlayerController() : nullptr)
		{
			if (PC->PlayerCameraManager)
			{
				const FVector ToFeet = Feet - PC->PlayerCameraManager->GetCameraLocation();
				const float Dist = ToFeet.Size();
				Scale = Dist > DepthPull + 1.f ? (Dist - DepthPull) / Dist : 1.f;
				PullDir = Dist > KINDA_SMALL_NUMBER ? ToFeet / Dist : CamForward;
			}
		}
		SetWorldLocation(Feet - PullDir * DepthPull);
		SetWorldScale3D(FVector(Scale));
		UpdateShadowTransform(Feet);
		UpdateTint(Feet, Dt);
		if (BlobMesh)
		{
			// No cliente original o shadow.spr e desenhado direto na tela. Deitado no chao ele achatava de novo pela
			// inclinacao da camera e girava com ela: aqui fica alinhado a camera e a profundidade e esticada por
			// 1/sen(inclinacao), entao na tela a elipse tem exatamente a proporcao do sprite original.
			const float SinPitch = FMath::Max(FMath::Abs(FMath::Sin(FMath::DegreesToRadians(CamRot.Pitch))), 0.25f);
			// Apoia no chao DESENHADO (a malha visual so bloqueia o canal Camera); o chao de colisao do GAT pode ficar
			// alguns cm acima ou abaixo dele, e o blob sumia por baixo da calcada.
			FVector BlobAt = Feet + FVector(0.f, 0.f, 5.f);
			FHitResult GroundHit;
			FCollisionQueryParams BlobParams(TEXT("RoBlobGround"), false, GetOwner());
			if (GetWorld()->LineTraceSingleByChannel(GroundHit, Feet + FVector(0.f, 0.f, 60.f), Feet - FVector(0.f, 0.f, 60.f), ECC_Camera, BlobParams))
			{
				BlobAt.Z = GroundHit.ImpactPoint.Z + 3.f;
			}
			BlobMesh->SetWorldLocationAndRotation(BlobAt, FRotator(0.f, CamRot.Yaw, 0.f));
			BlobMesh->SetWorldScale3D(FVector(1.f / SinPitch, 1.f, 1.f));
		}
	}
}

void UMythRoSpriteComponent::RebuildMesh(const FRotator& CamRot)
{
	const int32 ActionIndex = GetActionIndex();
	if (!Body.Act.Actions.IsValidIndex(ActionIndex))
	{
		return;
	}
	bool bFinished = false;
	const FRoActAction& BodyAction = Body.Act.Actions[ActionIndex];
	const int32 FrameIndex = GetFrameIndex(BodyAction, bFinished);
	const FRoActFrame& BodyFrame = BodyAction.Frames[FrameIndex];

	PlayFrameEvent(BodyFrame, ActionIndex, FrameIndex);

	int32 Section = 0;
	FrameLocalBounds = FBox(ForceInit);
	// Capa (docs/ref/03 4.4): atras do corpo de frente; de costas (noroeste, norte, nordeste) fica POR CIMA do corpo
	const int32 BodyDirIndex = ActionIndex % 8;
	const bool bBackToCamera = BodyDirIndex >= 3 && BodyDirIndex <= 5;
	AddBodyLayer(RobeSource, ActionIndex, FrameIndex, bBackToCamera ? 1.f : -1.f, Section);
	// Carrinho: atras de tudo de frente; de costas fica a frente do personagem. Roda gira ao andar (passo pela distancia)
	if (CartSource.IsValid() && CartSource.Act.Actions.IsValidIndex(BodyDirIndex) && CartSource.Act.Actions[BodyDirIndex].Frames.Num() > 0)
	{
		const FRoActAction& CartAction = CartSource.Act.Actions[BodyDirIndex];
		int32 CartFrame = 0;
		if (State == EMythSpriteState::Walk)
		{
			CartFrame = FMath::FloorToInt(WalkDistanceRo * 1.48f / FMath::Max(CartAction.IntervalMs / 25.f, 0.0001f)) % CartAction.Frames.Num();
		}
		AddLayerQuads(CartSource, CartAction.Frames[CartFrame], FIntPoint::ZeroValue, bBackToCamera ? 9.f : -5.f, Section);
	}
	AddLayerQuads(Body, BodyFrame, FIntPoint::ZeroValue, 0.f, Section);

	AddHeadLayer(Head, BodyFrame, ActionIndex, FrameIndex, 2.f, Section);
	// Acessorios na ordem do cliente: embaixo, meio, topo; depois arma e escudo.
	// De costas (oeste a nordeste) o escudo fica atras do corpo, como no cliente original.
	AddHeadLayer(HatBottom, BodyFrame, ActionIndex, FrameIndex, 3.f, Section);
	AddHeadLayer(HatMid, BodyFrame, ActionIndex, FrameIndex, 4.f, Section);
	AddHeadLayer(HatTop, BodyFrame, ActionIndex, FrameIndex, 5.f, Section);
	const int32 Dir = ActionIndex % 8;
	// De costas para a camera (3..5) a arma tambem vai para tras do tronco (docs/ref/03 4.4)
	const bool bWeaponBehind = Dir > 2 && Dir < 6;
	AddBodyLayer(WeaponSource, ActionIndex, FrameIndex, bWeaponBehind ? -2.f : 6.5f, Section);
	const bool bShieldBehind = Dir > 1 && Dir < 6;
	AddBodyLayer(ShieldSource, ActionIndex, FrameIndex, bShieldBehind ? -3.f : 8.f, Section);

	for (int32 i = Section; i < SectionsCreated; ++i)
	{
		SetMeshSectionVisible(i, false);
		if (ShadowMesh)
		{
			ShadowMesh->SetMeshSectionVisible(i, false);
		}
	}
}

void UMythRoSpriteComponent::AddHeadLayer(const FMythRoSpriteSource& Source, const FRoActFrame& BodyFrame, int32 ActionIndex, int32 FrameIndex, float DepthBase, int32& Section)
{
	if (!Source.IsValid() || !Source.Act.Actions.IsValidIndex(ActionIndex) || Source.Act.Actions[ActionIndex].Frames.Num() == 0)
	{
		return;
	}
	const FRoActAction& Action = Source.Act.Actions[ActionIndex];
	int32 HeadFrame = FrameIndex % Action.Frames.Num();
	if ((State == EMythSpriteState::Idle || State == EMythSpriteState::Sit) && !IsMonster() && Action.Frames.Num() >= 3)
	{
		// Doridori (docs/ref/03 4.3): a acao parada e dividida em 3 blocos iguais, um por posicao da cabeca
		HeadFrame = (Action.Frames.Num() / 3) * HeadTurn;
	}
	const FRoActFrame& Frame = Action.Frames[HeadFrame];
	// Encaixa no corpo pela diferenca entre as ancoras dos dois quadros (cabelo e acessorios).
	FIntPoint Offset = FIntPoint::ZeroValue;
	if (BodyFrame.Anchors.Num() > 0 && Frame.Anchors.Num() > 0)
	{
		Offset = BodyFrame.Anchors[0] - Frame.Anchors[0];
	}
	AddLayerQuads(Source, Frame, Offset, DepthBase, Section);
}

void UMythRoSpriteComponent::AddBodyLayer(const FMythRoSpriteSource& Source, int32 ActionIndex, int32 FrameIndex, float DepthBase, int32& Section)
{
	if (!Source.IsValid() || !Source.Act.Actions.IsValidIndex(ActionIndex) || Source.Act.Actions[ActionIndex].Frames.Num() == 0)
	{
		return;
	}
	const FRoActAction& Action = Source.Act.Actions[ActionIndex];
	const FRoActFrame& Frame = Action.Frames[FrameIndex % Action.Frames.Num()];
	// Parado/andando os .act de arma tem camadas vazias (-1): como no RO, a arma so aparece no golpe
	AddLayerQuads(Source, Frame, FIntPoint::ZeroValue, DepthBase, Section);
}

void UMythRoSpriteComponent::AddLayerQuads(const FMythRoSpriteSource& Source, const FRoActFrame& Frame, const FIntPoint& Offset, float DepthBase, int32& Section)
{
	// No espaco local (ver UpdateBillboard): X = direita da tela, Z = cima da tela, Y = Z x X = -Forward da camera.
	const float S = UnitsPerPixel;
	for (int32 LayerIndex = 0; LayerIndex < Frame.Layers.Num(); ++LayerIndex)
	{
		const FRoActLayer& L = Frame.Layers[LayerIndex];
		const FRoSprImage* Img = Source.Spr.Find(L.SprIndex, L.SprType);
		const TArray<FBox2f>& UVs = L.SprType == 1 ? Source.RgbaUV : Source.IndexedUV;
		if (!Img || !UVs.IsValidIndex(L.SprIndex))
		{
			continue;
		}
		const FBox2f UV = UVs[L.SprIndex];
		const float HalfW = Img->Width * L.Scale.X * 0.5f;
		const float HalfH = Img->Height * L.Scale.Y * 0.5f;
		const float CenterX = L.Offset.X + Offset.X;
		const float CenterY = L.Offset.Y + Offset.Y;
		const float Rad = FMath::DegreesToRadians(static_cast<float>(L.RotationDeg));
		const float C = FMath::Cos(Rad), Sn = FMath::Sin(Rad);
		// Cada camada fica um pouco mais perto da camera que a anterior (evita z-fighting).
		const float Depth = DepthBase + LayerIndex * 0.5f;

		// Cantos em pixels (Y para baixo): sup-esq, sup-dir, inf-dir, inf-esq.
		const FVector2f Corners[4] = { {-HalfW, -HalfH}, {HalfW, -HalfH}, {HalfW, HalfH}, {-HalfW, HalfH} };
		const float U0 = L.bMirror ? UV.Max.X : UV.Min.X;
		const float U1 = L.bMirror ? UV.Min.X : UV.Max.X;
		const FVector2D CornerUV[4] = { {U0, UV.Min.Y}, {U1, UV.Min.Y}, {U1, UV.Max.Y}, {U0, UV.Max.Y} };

		TArray<FVector> Verts;
		TArray<FVector2D> Tex;
		TArray<FColor> Colors;
		TArray<FVector> Normals;
		for (int32 k = 0; k < 4; ++k)
		{
			const float Px = CenterX + Corners[k].X * C - Corners[k].Y * Sn;
			const float Py = CenterY + Corners[k].X * Sn + Corners[k].Y * C;
			Verts.Add(FVector(Px * S, Depth, -Py * S));
			FrameLocalBounds += Verts.Last();
			Tex.Add(CornerUV[k]);
			// Cor da camada (.act) x luz calculada (dia/noite, postes, sombra de predio); o alpha fica o da camada.
			FColor Tinted = (FLinearColor(L.Color) * CurrentTint * StatusTint).ToFColor(false);
			Tinted.A = L.Color.A;
			Colors.Add(Tinted);
			// Normal inclinada para cima e para a camera: o sprite iluminado recebe sol/ceu como o chao
			// e nao escurece quando o sol esta atras dele.
			Normals.Add(FVector(0.f, 0.45f, 0.89f));
		}

		UMaterialInterface* Mat = Source.Material;
		// Dois triangulos nos dois sentidos: o quad aparece mesmo se o winding ficar invertido.
		static const TArray<int32> Tris = { 0, 1, 2, 0, 2, 3, 0, 2, 1, 0, 3, 2 };
		if (Section < SectionsCreated)
		{
			UpdateMeshSection(Section, Verts, Normals, Tex, Colors, TArray<FProcMeshTangent>());
			SetMeshSectionVisible(Section, true);
			if (ShadowMesh)
			{
				ShadowMesh->UpdateMeshSection(Section, Verts, Normals, Tex, Colors, TArray<FProcMeshTangent>());
				ShadowMesh->SetMeshSectionVisible(Section, true);
			}
		}
		else
		{
			CreateMeshSection(Section, Verts, Tris, Normals, Tex, Colors, TArray<FProcMeshTangent>(), false);
			if (ShadowMesh)
			{
				ShadowMesh->CreateMeshSection(Section, Verts, Tris, Normals, Tex, Colors, TArray<FProcMeshTangent>(), false);
			}
			++SectionsCreated;
		}
		if (GetMaterial(Section) != Mat)
		{
			SetMaterial(Section, Mat);
			if (ShadowMesh)
			{
				ShadowMesh->SetMaterial(Section, Mat); // mascarado: a sombra tem o recorte do sprite
			}
		}
		++Section;
	}
}

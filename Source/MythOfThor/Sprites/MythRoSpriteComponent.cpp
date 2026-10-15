#include "Sprites/MythRoSpriteComponent.h"
#include "TimerManager.h"

#include "World/MythDayNightActor.h"
#include "World/MythNightLight.h"
#include "World/MythWater.h"
#include "Sprites/MythSprEffect.h"
#include "Sprites/MythFalcon.h"
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

	/**
	 * Ataque do jogador: o corpo tem tres grupos (5 ATTACK1, 10 ATTACK2, 11 ATTACK3) e o cliente escolhe pela classe e pelo tipo
	 * de arma (Config/weapon_action.json, tabela WeaponAction do roBrowserLegacy). Ex.: Cacador com arco puxa a corda (ATTACK3);
	 * o ATTACK1 e o golpe sem arma.
	 */
	int32 PlayerAttackGroup(int32 JobClass, bool bMale, int32 WeaponType)
	{
		struct FTable { TMap<int32, TMap<int32, int32>> Jobs; TMap<int32, TMap<int32, int32>> Female; TMap<int32, TMap<int32, int32>> Male; };
		static const FTable Table = []
		{
			FTable T;
			FString Text;
			TSharedPtr<FJsonObject> Root;
			if (!FFileHelper::LoadFileToString(Text, *FPaths::Combine(FPaths::ProjectConfigDir(), TEXT("weapon_action.json")))
				|| !FJsonSerializer::Deserialize(TJsonReaderFactory<>::Create(Text), Root) || !Root.IsValid())
			{
				UE_LOG(LogTemp, Warning, TEXT("MythRoSprite: Config/weapon_action.json ausente (tools/gen_weapon_action.py)"));
				return T;
			}
			auto Read = [](const TSharedPtr<FJsonObject>& O, TMap<int32, int32>& Out)
			{
				for (const auto& P : O->Values) { Out.Add(FCString::Atoi(*P.Key), static_cast<int32>(P.Value->AsNumber())); }
			};
			const TSharedPtr<FJsonObject>* Jobs = nullptr;
			if (Root->TryGetObjectField(TEXT("jobs"), Jobs))
			{
				for (const auto& J : (*Jobs)->Values) { Read(J.Value->AsObject(), T.Jobs.Add(FCString::Atoi(*J.Key))); }
			}
			const TSharedPtr<FJsonObject>* BySex = nullptr;
			if (Root->TryGetObjectField(TEXT("jobs_sex"), BySex))
			{
				for (const auto& J : (*BySex)->Values)
				{
					const int32 Id = FCString::Atoi(*J.Key);
					const TSharedPtr<FJsonObject> O = J.Value->AsObject();
					Read(O->GetObjectField(TEXT("f")), T.Female.Add(Id));
					Read(O->GetObjectField(TEXT("m")), T.Male.Add(Id));
				}
			}
			return T;
		}();
		const TMap<int32, int32>* Weapons = Table.Jobs.Find(JobClass);
		if (!Weapons)
		{
			Weapons = (bMale ? Table.Male : Table.Female).Find(JobClass);
		}
		const int32 Attack = Weapons ? Weapons->FindRef(WeaponType) : 0;
		static const int32 Groups[3] = { 5, 10, 11 };
		return Groups[FMath::Clamp(Attack, 0, 2)];
	}

	// Materiais de sprite: tenta o M_RoSprite_Lit do HD-2D (com Rim Light e HitFlash) com fallback para Paper2D.
	const TCHAR* CustomLitMaterialPath = TEXT("/Game/Myth/M_RoSprite_Lit.M_RoSprite_Lit");
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
	// Ordem: -MythAssets=<pasta> na linha de comando, a pasta RagnarokAssets/grf ao lado do jogo (pacote para jogar em outro
	// PC, tools/make_friend_build.ps1), AssetsRoot do DefaultGame.ini e por fim a variavel MYTH_ASSETS
	static const FString Cached = []
	{
		FString Root;
		if (FParse::Value(FCommandLine::Get(), TEXT("MythAssets="), Root) && !Root.IsEmpty())
		{
			return Root;
		}
		const FString Packaged = FPaths::ConvertRelativePathToFull(FPaths::Combine(FPaths::RootDir(), TEXT("RagnarokAssets"), TEXT("grf")));
		if (FPaths::DirectoryExists(FPaths::Combine(Packaged, TEXT("data"))))
		{
			return Packaged;
		}
		if (GConfig && GConfig->GetString(TEXT("Myth"), TEXT("AssetsRoot"), Root, GGameIni) && !Root.IsEmpty())
		{
			return Root;
		}
		const FString Env = FPlatformMisc::GetEnvironmentVariable(TEXT("MYTH_ASSETS"));
		return Env.IsEmpty() ? FString() : FPaths::Combine(Env, TEXT("grf"));
	}();
	return Cached;
}

void UMythRoSpriteComponent::BeginPlay()
{
	Super::BeginPlay();
	FeetRelativeLocation = GetRelativeLocation();
	ReloadSprites();

	// Contorno de Oclusao HD-2D: CustomDepth habilitado com Stencil (1 Jogador, 2 Monstro)
	SetRenderCustomDepth(true);
	SetCustomDepthStencilValue(IsMonster() ? 2 : 1);

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
	if (GetOwner())
	{
		// Ghost Sprite na Agua (Reflexo dos Sprites 2D HD-2D):
		ReflectionMesh = NewObject<UProceduralMeshComponent>(GetOwner(), TEXT("RoSpriteReflection"));
		ReflectionMesh->SetupAttachment(GetAttachParent());
		ReflectionMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		ReflectionMesh->SetCastShadow(false);
		ReflectionMesh->bCastDynamicShadow = false;
		ReflectionMesh->SetVisibility(false);
		ReflectionMesh->RegisterComponent();
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

	// Sombra para o carrinho do mercador (HD-2D):
	CartBlobMesh = NewObject<UProceduralMeshComponent>(GetOwner(), TEXT("RoSpriteCartBlobShadow"));
	CartBlobMesh->SetupAttachment(GetAttachParent());
	CartBlobMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	CartBlobMesh->SetCastShadow(false);
	CartBlobMesh->SetVisibility(false);
	CartBlobMesh->RegisterComponent();
	CartBlobMesh->CreateMeshSection(0, Verts, Tris, Normals, Tex, Colors, TArray<FProcMeshTangent>(), false);
	CartBlobMesh->SetMaterial(0, BlobSource.Material);
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
		// Som do golpe pelo tipo da arma (W_DAGGER 1 ... W_2HSTAFF 23); o proprio personagem recebe o ID do item
		const int32 Type = WeaponTypeOf(Weapon);
		const int32 WeaponType = Type > 0 && Type <= 23 ? Type : (Weapon > 0 ? 2 : 0);
		UMythRoSound::PlayWavAtLocation(this, UMythRoSound::WeaponSwingWav(WeaponType), GetOwner()->GetActorLocation());
	}
}


namespace
{
	/** Config/weapon_types.json (tools/gen_weapon_types.py): ID do item -> tipo de arma / view do escudo. */
	struct FItemVisuals
	{
		TMap<int32, int32> Weapons;
		TMap<int32, int32> Shields;
	};

	const FItemVisuals& ItemVisuals()
	{
		static const FItemVisuals V = []
		{
			FItemVisuals Out;
			FString Text;
			TSharedPtr<FJsonObject> Root;
			if (!FFileHelper::LoadFileToString(Text, *FPaths::Combine(FPaths::ProjectConfigDir(), TEXT("weapon_types.json")))
				|| !FJsonSerializer::Deserialize(TJsonReaderFactory<>::Create(Text), Root) || !Root.IsValid())
			{
				UE_LOG(LogTemp, Warning, TEXT("MythRoSprite: Config/weapon_types.json ausente (tools/gen_weapon_types.py)"));
				return Out;
			}
			auto Read = [&Root](const TCHAR* Field, TMap<int32, int32>& Map)
			{
				const TSharedPtr<FJsonObject>* Obj = nullptr;
				if (Root->TryGetObjectField(Field, Obj))
				{
					for (const auto& P : (*Obj)->Values) { Map.Add(FCString::Atoi(*P.Key), static_cast<int32>(P.Value->AsNumber())); }
				}
			};
			Read(TEXT("weapons"), Out.Weapons);
			Read(TEXT("shields"), Out.Shields);
			return Out;
		}();
		return V;
	}

	/** Duas armas (Mercenario/Algoz): o cliente desenha um sprite so com as duas (WeaponType SHORTSWORD_SHORTSWORD = 25...). */
	int32 DualWeaponType(int32 Right, int32 Left)
	{
		static const TMap<TPair<int32, int32>, int32> Pairs = {
			{ {1, 1}, 25 }, { {2, 2}, 26 }, { {6, 6}, 27 }, { {1, 2}, 28 }, { {1, 6}, 29 }, { {2, 6}, 30 },
		};
		if (const int32* T = Pairs.Find(TPair<int32, int32>(Right, Left))) { return *T; }
		if (const int32* T = Pairs.Find(TPair<int32, int32>(Left, Right))) { return *T; }
		return 0;
	}
}

int32 UMythRoSpriteComponent::WeaponTypeOf(int32 Weapon)
{
	Weapon &= 0xFFFF;
	if (Weapon <= 30)
	{
		return FMath::Max(Weapon, 0);
	}
	const int32* Found = ItemVisuals().Weapons.Find(Weapon);
	if (!Found)
	{
		static TSet<int32> Warned;
		if (!Warned.Contains(Weapon))
		{
			Warned.Add(Weapon);
			UE_LOG(LogTemp, Error, TEXT("MythWeapon: weapon/item %d is not in Config/weapon_types.json (tools/gen_weapon_types.py); without the type the weapon sprite is not drawn. Regenerate the table from the rAthena item_db."), Weapon);
		}
		return 0;
	}
	return *Found;
}

int32 UMythRoSpriteComponent::LeftWeaponTypeOf(int32 ShieldValue)
{
	// Na mao esquerda pode vir um escudo ou uma arma (duas armas); so e arma se o ID for de arma no item_db
	ShieldValue &= 0xFFFF;
	return ShieldValue > 30 ? ItemVisuals().Weapons.FindRef(ShieldValue) : 0;
}

int32 UMythRoSpriteComponent::ShieldViewOf(int32 ShieldValue)
{
	ShieldValue &= 0xFFFF;
	if (ShieldValue <= 30)
	{
		return FMath::Max(ShieldValue, 0);
	}
	const int32* View = ItemVisuals().Shields.Find(ShieldValue);
	return View ? *View : 1; // escudo sem view: guarda, como o cliente
}

void UMythRoSpriteComponent::OnComponentDestroyed(bool bDestroyingHierarchy)
{
	if (Falcon.IsValid())
	{
		Falcon->Destroy();
		Falcon = nullptr;
	}
	if (ShadowMesh)
	{
		ShadowMesh->DestroyComponent();
		ShadowMesh = nullptr;
	}
	if (ReflectionMesh)
	{
		ReflectionMesh->DestroyComponent();
		ReflectionMesh = nullptr;
	}
	if (BlobMesh)
	{
		BlobMesh->DestroyComponent();
		BlobMesh = nullptr;
	}
	if (CartBlobMesh)
	{
		CartBlobMesh->DestroyComponent();
		CartBlobMesh = nullptr;
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
	++LoadVersion;
	const FString Root = GetAssetsRoot();
	if (IsMonster())
	{
		const FString Folder = MonsterSpriteFolder.IsEmpty() ? FString(MonsterDir) : MonsterSpriteFolder;
		const FString Base = FPaths::Combine(Root, TEXT("data"), TEXT("sprite"), Folder, MonsterSpriteName);
		Head = FMythRoSpriteSource();
		if (!LoadSource(Body, Base + TEXT(".spr"), Base + TEXT(".act"), FString()))
		{
			UE_LOG(LogTemp, Warning, TEXT("MythRoSprite: failed to load the monster '%s' (root '%s')."), *MonsterSpriteName, *Root);
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
		UE_LOG(LogTemp, Warning, TEXT("MythRoSprite: failed to load sprites (root '%s'). Body=%d Head=%d. Set [Myth] AssetsRoot in DefaultGame.ini."),
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
			UE_LOG(LogTemp, Verbose, TEXT("MythRoSprite: equipment %s: %s (%d actions)"), bOk ? TEXT("ok") : TEXT("falhou"), *Base, Source.Act.Actions.Num());
		}
		else if (!Base.IsEmpty())
		{
			UE_LOG(LogTemp, Verbose, TEXT("MythRoSprite: equipment without sprite: %s"), *Base);
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
	// O servidor manda o tipo/view ou, para o proprio personagem, o ID do item (Config/weapon_types.json)
	const int32 RightType = WeaponTypeOf(Weapon);
	const int32 LeftType = LeftWeaponTypeOf(Shield);
	const int32 Dual = LeftType > 0 ? DualWeaponType(RightType, LeftType) : 0;
	EffectiveWeaponType = Dual > 0 ? Dual : RightType;
	Load(WeaponSource, WeaponBase(EffectiveWeaponType));
	if (LeftType > 0)
	{
		// Arma na mao esquerda: ja vai no sprite combinado; sem combinacao, desenha a esquerda separada
		if (Dual > 0) { ShieldSource = FMythRoSpriteSource(); } else { Load(ShieldSource, WeaponBase(LeftType)); }
	}
	else if (Shield > 0)
	{
		// Escudos com nome: <classe>_<sexo>_<nome> (가드, 버클러...); os demais: <classe>_<sexo>_<view>_방패
		const int32 ShieldView = ShieldViewOf(Shield);
		const FString* Name = T.Shields.Find(ShieldView);
		const FString Dir = FPaths::Combine(SpriteRoot, ShieldDir, Job);
		FString Base = Name ? FPaths::Combine(Dir, FString::Printf(TEXT("%s_%s_%s"), *Job, *Sex, **Name))
			: FPaths::Combine(Dir, FString::Printf(TEXT("%s_%s_%d_%s"), *Job, *Sex, ShieldView, ShieldDir));
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
		UE_LOG(LogTemp, Warning, TEXT("MythRoSprite: file not found: %s"), *SprPath);
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

	const bool bUseLitMat = bLitByScene || IsMonster();
	const TCHAR* DesiredLit = LitMaterialPath;
	if (bUseLitMat)
	{
		static bool bCheckedCustom = false;
		static bool bHasCustom = false;
		if (!bCheckedCustom)
		{
			bCheckedCustom = true;
			bHasCustom = (LoadObject<UMaterialInterface>(nullptr, CustomLitMaterialPath) != nullptr);
		}
		DesiredLit = bHasCustom ? CustomLitMaterialPath : LitMaterialPath;
	}

	if (UMaterialInterface* Base = LoadObject<UMaterialInterface>(nullptr, bUseLitMat ? DesiredLit : UnlitMaterialPath))
	{
		Source.Material = UMaterialInstanceDynamic::Create(Base, this);
		Source.Material->SetTextureParameterValue(TEXT("SpriteTexture"), Source.Atlas);
		Source.Material->SetScalarParameterValue(TEXT("DissolveProgress"), 0.0f);
		Source.Material->SetVectorParameterValue(TEXT("DissolveGlowColor"), DissolveColor);
	}

	static const TCHAR* ReflectionMaterialPath = TEXT("/Game/Myth/M_RoSprite_Reflection.M_RoSprite_Reflection");
	if (UMaterialInterface* ReflBase = LoadObject<UMaterialInterface>(nullptr, ReflectionMaterialPath))
	{
		Source.ReflectionMaterial = UMaterialInstanceDynamic::Create(ReflBase, this);
		Source.ReflectionMaterial->SetTextureParameterValue(TEXT("SpriteTexture"), Source.Atlas);
		Source.ReflectionMaterial->SetScalarParameterValue(TEXT("ReflectionOpacity"), 0.35f);
		Source.ReflectionMaterial->SetVectorParameterValue(TEXT("WaterTint"), FLinearColor(0.65f, 0.85f, 1.0f, 1.0f));
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
	// Falcao do Cacador: companheiro que voa e segue o dono (Sprites/MythFalcon), nao um adereco preso a cabeca
	const bool bFalcon = (Option & 0x10) != 0 && !bStatusHidden;
	if (bFalcon && !Falcon.IsValid())
	{
		Falcon = AMythFalcon::Spawn(GetOwner(), JobClass);
	}
	else if (!bFalcon && Falcon.IsValid())
	{
		Falcon->Destroy();
		Falcon = nullptr;
	}
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
	if (const UWorld* W = GetWorld(); W && W->GetTimeSeconds() < HitFlashUntil)
	{
		C.R *= 1.8f; C.G *= 1.8f; C.B *= 1.8f; // hit flash: brighter for a few frames
	}
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
void UMythRoSpriteComponent::Die()
{
	SetState(EMythSpriteState::Dead, false);
	if (IsMonster() && !bDissolving)
	{
		StartDeathDissolve(1.0f, FLinearColor(1.0f, 0.45f, 0.1f, 1.0f));
	}
}

void UMythRoSpriteComponent::StartDeathDissolve(float Duration, FLinearColor GlowColor)
{
	bDissolving = true;
	DissolveDuration = FMath::Max(0.1f, Duration);
	DissolveElapsed = 0.0f;
	DissolveColor = GlowColor;

	auto InitDissolve = [this](FMythRoSpriteSource& S)
	{
		if (S.Material)
		{
			S.Material->SetScalarParameterValue(TEXT("DissolveProgress"), 0.0f);
			S.Material->SetVectorParameterValue(TEXT("DissolveGlowColor"), DissolveColor);
		}
	};
	InitDissolve(Body);
	InitDissolve(Head);
	InitDissolve(HatBottom);
	InitDissolve(HatMid);
	InitDissolve(HatTop);
	InitDissolve(WeaponSource);
	InitDissolve(ShieldSource);
	InitDissolve(RobeSource);
	InitDissolve(CartSource);
}

void UMythRoSpriteComponent::Revive()
{
	bDissolving = false;
	DissolveElapsed = 0.0f;
	auto ResetDissolve = [](FMythRoSpriteSource& S)
	{
		if (S.Material)
		{
			S.Material->SetScalarParameterValue(TEXT("DissolveProgress"), 0.0f);
		}
	};
	ResetDissolve(Body);
	ResetDissolve(Head);
	ResetDissolve(HatBottom);
	ResetDissolve(HatMid);
	ResetDissolve(HatTop);
	ResetDissolve(WeaponSource);
	ResetDissolve(ShieldSource);
	ResetDissolve(RobeSource);
	ResetDissolve(CartSource);

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
	case EMythSpriteState::Attack: Group = bMob ? MobGroupAttack : PlayerAttackGroup(JobClass, bMale, EffectiveWeaponType); break;
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
	if (bDissolving)
	{
		DissolveElapsed += DeltaTime;
		const float Prog = FMath::Clamp(DissolveElapsed / DissolveDuration, 0.0f, 1.0f);
		auto UpdateDissolve = [Prog](FMythRoSpriteSource& S)
		{
			if (S.Material)
			{
				S.Material->SetScalarParameterValue(TEXT("DissolveProgress"), Prog);
			}
		};
		UpdateDissolve(Body);
		UpdateDissolve(Head);
		UpdateDissolve(HatBottom);
		UpdateDissolve(HatMid);
		UpdateDissolve(HatTop);
		UpdateDissolve(WeaponSource);
		UpdateDissolve(ShieldSource);
		UpdateDissolve(RobeSource);
		UpdateDissolve(CartSource);
	}
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
		Feet.Z = SmoothedFeetZ + 5.0f; // eleva 5 cm para os pes nunca afundarem no desnivel do piso

		CurrentCamPitch = CamRot.Pitch;
		// Gradient Depth (HD-2D / Shipping Bugs):
		// Os pes permanecem na posicao 3D real no chao (com offset minimo de 2 cm ao longo da visao
		// apenas para evitar z-fighting coplanar com o triangulo do piso).
		// O recuo que impede a cabeca de entrar nas paredes atras do ator e calculado de forma progressiva
		// (gradiente) nos vertices da malha em AddLayerQuads, eliminando a penetracao em paredes
		// sem puxar os pes para a frente de balcoes, degraus ou obstaculos dianteiros.
		const float BaseFeetOffset = FMath::Clamp(CameraDepthOffset, 0.f, 5.f);
		SetWorldLocation(Feet - CamForward * BaseFeetOffset);
		SetWorldScale3D(FVector(1.f));
		UpdateShadowTransform(Feet);
		UpdateTint(Feet, Dt);
		WaterWaveTime += Dt;

		// Ghost Sprite na Agua (Reflexo dos Sprites 2D HD-2D):
		float WaterZ = 0.f;
		if (!Feet.ContainsNaN() && AMythWater::GetWaterPlane(this, WaterZ) && FMath::IsFinite(WaterZ))
		{
			const float DistToWater = Feet.Z - WaterZ;
			// Se o sprite estiver proximo da superficie da agua (de 50 cm abaixo ate 400 cm acima)
			if (DistToWater >= -50.f && DistToWater <= 400.f)
			{
				bNearWater = true;
				CachedWaterZ = WaterZ;
				if (ReflectionMesh)
				{
					FVector ReflFeet = Feet;
					ReflFeet.Z = 2.f * WaterZ - Feet.Z;
					if (!ReflFeet.ContainsNaN() && ReflFeet.SizeSquared() < 1e12)
					{
						ReflectionMesh->SetWorldLocation(ReflFeet - CamForward * BaseFeetOffset);
						ReflectionMesh->SetWorldRotation(GetComponentRotation());
						ReflectionMesh->SetVisibility(true);
					}
				}
			}
			else
			{
				bNearWater = false;
				if (ReflectionMesh)
				{
					ReflectionMesh->SetVisibility(false);
				}
			}
		}
		else
		{
			bNearWater = false;
			if (ReflectionMesh)
			{
				ReflectionMesh->SetVisibility(false);
			}
		}
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
			const bool bIsPeco = (JobClass == 13 || JobClass == 21 || JobClass == 4014 || JobClass == 4022 || JobClass == 4036 || JobClass == 4044 || JobSpriteName.Contains(TEXT("peco")) || JobSpriteName.Contains(TEXT("페코")));
			const float MountScale = bIsPeco ? 1.75f : 1.0f;
			BlobMesh->SetWorldLocationAndRotation(BlobAt, FRotator(0.f, CamRot.Yaw, 0.f));
			BlobMesh->SetWorldScale3D(FVector(MountScale / SinPitch, MountScale, 1.f));
		}
		if (CartBlobMesh)
		{
			if (CartType > 0 && CartSource.IsValid())
			{
				const float SinPitch = FMath::Max(FMath::Abs(FMath::Sin(FMath::DegreesToRadians(CamRot.Pitch))), 0.25f);
				// Sombra projetada do carrinho atras do personagem:
				FVector CartAt = Feet - FVector(FacingWorld.X, FacingWorld.Y, 0.f) * 60.f + FVector(0.f, 0.f, 5.f);
				FHitResult CartGroundHit;
				FCollisionQueryParams CartParams(TEXT("RoCartBlobGround"), false, GetOwner());
				if (GetWorld()->LineTraceSingleByChannel(CartGroundHit, CartAt + FVector(0.f, 0.f, 60.f), CartAt - FVector(0.f, 0.f, 60.f), ECC_Camera, CartParams))
				{
					CartAt.Z = CartGroundHit.ImpactPoint.Z + 3.f;
				}
				CartBlobMesh->SetWorldLocationAndRotation(CartAt, FRotator(0.f, CamRot.Yaw, 0.f));
				CartBlobMesh->SetWorldScale3D(FVector(1.15f / SinPitch, 1.15f, 1.f));
				CartBlobMesh->SetVisibility(true);
			}
			else
			{
				CartBlobMesh->SetVisibility(false);
			}
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
	for (int32 i = Section; i < ReflectionSectionsCreated; ++i)
	{
		if (ReflectionMesh)
		{
			ReflectionMesh->SetMeshSectionVisible(i, false);
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
		// Camadas praticamente invisiveis no .act (alpha zero ou residual) nao geram malha
		if (L.Color.A <= 15)
		{
			continue;
		}
		// Na animacao de morte do RO, camadas com escala ampliada e alpha reduzido
		// sao ondas de choque 2D antigas. No 3D com dissolve procedural e particulas,
		// elas apareciam como o monstro inflando ate 10x de tamanho antes de sumir.
		if (State == EMythSpriteState::Dead && (L.Scale.X > 1.15f || L.Scale.Y > 1.15f) && L.Color.A < 220)
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
			const float LocalZ = -Py * S;
			// Gradient Depth (HD-2D / Shipping Bugs):
			// Cada sprite curva sua profundidade em direcao a camera a medida que sobe.
			// Na base (pes, LocalZ <= 0): inclinacao = 0, os pes ficam na profundidade 3D exata do chao.
			// No topo (cabeca, LocalZ > 0): inclina para frente por LocalZ * sin(|pitch|),
			// igualando a profundidade real de um modelo 3D vertical e impedindo que a cabeca
			// afunde em paredes ou subidas de morro atras do ator, sem projetar os pes a frente de balcoes.
			const float HeightAboveFeet = FMath::Max(0.f, LocalZ);
			const float LeanDepth = HeightAboveFeet * FMath::Abs(FMath::Sin(FMath::DegreesToRadians(CurrentCamPitch)));
			Verts.Add(FVector(Px * S, Depth + LeanDepth, LocalZ));
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

		// Ghost Sprite na Agua (Reflexo dos Sprites 2D HD-2D):
		if (ReflectionMesh && bNearWater && Source.ReflectionMaterial)
		{
			TArray<FVector> ReflVerts;
			ReflVerts.Reserve(4);
			for (int32 k = 0; k < 4; ++k)
			{
				const float VertLocalZ = Verts[k].Z;
				// Distorcao senoidal suave da agua (HD-2D)
				const float WaveX = FMath::Sin(WaterWaveTime * 4.0f + VertLocalZ * 0.08f) * 3.0f;
				// Espelha no eixo Z vertical para baixo do plano dos pes
				ReflVerts.Add(FVector(Verts[k].X + WaveX, Verts[k].Y, -VertLocalZ));
			}

			if (Section < ReflectionSectionsCreated)
			{
				ReflectionMesh->UpdateMeshSection(Section, ReflVerts, Normals, Tex, Colors, TArray<FProcMeshTangent>());
				ReflectionMesh->SetMeshSectionVisible(Section, true);
			}
			else
			{
				ReflectionMesh->CreateMeshSection(Section, ReflVerts, Tris, Normals, Tex, Colors, TArray<FProcMeshTangent>(), false);
				++ReflectionSectionsCreated;
			}
			if (ReflectionMesh->GetMaterial(Section) != Source.ReflectionMaterial)
			{
				ReflectionMesh->SetMaterial(Section, Source.ReflectionMaterial);
			}
		}
		++Section;
	}
}

// ------------------------------------------------------------------------------------------- previa 2D

namespace
{
	/** Desenha as camadas de um quadro do .act numa tela RGBA (Y para baixo), como o cliente: offset, espelho, escala,
	 * rotacao e cor da camada, "por cima" (alpha normal). Amostragem pelo pixel mais proximo. */
	void BlitActFrame(TArray<FColor>& Canvas, int32 W, int32 H, const FIntPoint& Origin, const FRoSpr& Spr, const FRoActFrame& Frame, const FIntPoint& Offset)
	{
		for (const FRoActLayer& L : Frame.Layers)
		{
			const FRoSprImage* Img = Spr.Find(L.SprIndex, L.SprType);
			if (!Img || Img->Width <= 0 || Img->Height <= 0)
			{
				continue;
			}
			const float Sx = FMath::Max(FMath::Abs(L.Scale.X), 0.01f);
			const float Sy = FMath::Max(FMath::Abs(L.Scale.Y), 0.01f);
			const float Cx = Origin.X + L.Offset.X + Offset.X;
			const float Cy = Origin.Y + L.Offset.Y + Offset.Y;
			const float Rad = FMath::DegreesToRadians(static_cast<float>(L.RotationDeg));
			const float C = FMath::Cos(Rad), Sn = FMath::Sin(Rad);
			const float Reach = FMath::Sqrt(FMath::Square(Img->Width * Sx) + FMath::Square(Img->Height * Sy)) * 0.5f + 1.f;
			const int32 X0 = FMath::Max(0, FMath::FloorToInt(Cx - Reach)), X1 = FMath::Min(W - 1, FMath::CeilToInt(Cx + Reach));
			const int32 Y0 = FMath::Max(0, FMath::FloorToInt(Cy - Reach)), Y1 = FMath::Min(H - 1, FMath::CeilToInt(Cy + Reach));
			for (int32 y = Y0; y <= Y1; ++y)
			{
				for (int32 x = X0; x <= X1; ++x)
				{
					// Inverso da rotacao do mundo (AddLayerQuads): volta ao espaco da imagem
					const float Dx = x + 0.5f - Cx, Dy = y + 0.5f - Cy;
					float U = (Dx * C + Dy * Sn) / Sx + Img->Width * 0.5f;
					const float V = (-Dx * Sn + Dy * C) / Sy + Img->Height * 0.5f;
					if (L.bMirror)
					{
						U = Img->Width - U;
					}
					const int32 Iu = FMath::FloorToInt(U), Iv = FMath::FloorToInt(V);
					if (Iu < 0 || Iv < 0 || Iu >= Img->Width || Iv >= Img->Height)
					{
						continue;
					}
					const FColor P = Img->Pixels[Iv * Img->Width + Iu];
					const int32 A = P.A * L.Color.A / 255;
					if (A == 0)
					{
						continue;
					}
					FColor& D = Canvas[y * W + x];
					const int32 R = P.R * L.Color.R / 255, G = P.G * L.Color.G / 255, B = P.B * L.Color.B / 255;
					const int32 OutA = A + D.A * (255 - A) / 255;
					auto Mix = [A, OutA, &D](int32 Src, uint8 Dst) { return static_cast<uint8>((Src * A + Dst * D.A * (255 - A) / 255) / FMath::Max(OutA, 1)); };
					D = FColor(Mix(R, D.R), Mix(G, D.G), Mix(B, D.B), static_cast<uint8>(OutA));
				}
			}
		}
	}
}

UTexture2D* UMythRoSpriteComponent::RenderPortrait(int32 Width, int32 Height, int32 FeetY, uint8 Dir) const
{
	const int32 ActionIndex = GroupIdle * 8 + (Dir & 7);
	if (IsMonster() || Width <= 0 || Height <= 0 || !Body.Act.Actions.IsValidIndex(ActionIndex) || Body.Act.Actions[ActionIndex].Frames.Num() == 0)
	{
		return nullptr;
	}
	const FRoActFrame& BodyFrame = Body.Act.Actions[ActionIndex].Frames[0];
	// Mesma ordem de RebuildMesh (a "profundidade" de cada camada decide quem fica na frente)
	const bool bBack = Dir >= 3 && Dir <= 5;
	struct FPart { float Depth; const FMythRoSpriteSource* Source; bool bHead; };
	TArray<FPart> Parts = {
		{ bBack ? 1.f : -1.f, &RobeSource, false },
		{ 0.f, &Body, false },
		{ 2.f, &Head, true },
		{ 3.f, &HatBottom, true },
		{ 4.f, &HatMid, true },
		{ 5.f, &HatTop, true },
		{ (Dir > 2 && Dir < 6) ? -2.f : 6.5f, &WeaponSource, false },
		{ (Dir > 1 && Dir < 6) ? -3.f : 8.f, &ShieldSource, false },
	};
	Parts.StableSort([](const FPart& A, const FPart& B) { return A.Depth < B.Depth; });

	TArray<FColor> Canvas;
	Canvas.Init(FColor(0, 0, 0, 0), Width * Height);
	const FIntPoint Origin(Width / 2, FeetY);
	for (const FPart& Part : Parts)
	{
		const FMythRoSpriteSource& S = *Part.Source;
		if (!S.IsValid() || !S.Act.Actions.IsValidIndex(ActionIndex) || S.Act.Actions[ActionIndex].Frames.Num() == 0)
		{
			continue;
		}
		const FRoActFrame& Frame = S.Act.Actions[ActionIndex].Frames[0]; // quadro 0 = cabeca reta (doridori na frente)
		FIntPoint Offset = FIntPoint::ZeroValue;
		if (Part.bHead && BodyFrame.Anchors.Num() > 0 && Frame.Anchors.Num() > 0)
		{
			Offset = BodyFrame.Anchors[0] - Frame.Anchors[0];
		}
		BlitActFrame(Canvas, Width, Height, Origin, S.Spr, Frame, Offset);
	}

	UTexture2D* Tex = UTexture2D::CreateTransient(Width, Height, PF_B8G8R8A8);
	if (!Tex)
	{
		return nullptr;
	}
	Tex->Filter = TF_Nearest; // a previa fica em pixel duro, como no print da janela
	Tex->SRGB = true;
	Tex->NeverStream = true;
	Tex->LODGroup = TEXTUREGROUP_Pixels2D;
	FTexture2DMipMap& Mip = Tex->GetPlatformData()->Mips[0];
	void* Data = Mip.BulkData.Lock(LOCK_READ_WRITE);
	FMemory::Memcpy(Data, Canvas.GetData(), Canvas.Num() * sizeof(FColor));
	Mip.BulkData.Unlock();
	Tex->UpdateResource();
	return Tex;
}

void UMythRoSpriteComponent::TriggerHitFlash(float Duration)
{
	UWorld* W = GetWorld();
	if (!W || Duration <= 0.f)
	{
		return;
	}
	HitFlashUntil = W->GetTimeSeconds() + Duration;
	RecomputeStatusTint();

	auto SetHitFlash = [this](float Value)
	{
		for (FMythRoSpriteSource* Src : { &Body, &Head, &HatBottom, &HatMid, &HatTop, &WeaponSource, &ShieldSource, &RobeSource, &CartSource })
		{
			if (Src && Src->Material)
			{
				Src->Material->SetScalarParameterValue(TEXT("HitFlash"), Value);
			}
		}
	};

	SetHitFlash(1.0f);

	W->GetTimerManager().SetTimer(HitFlashTimer, FTimerDelegate::CreateWeakLambda(this, [this, SetHitFlash]()
	{
		RecomputeStatusTint();
		SetHitFlash(0.0f);
	}), Duration, false);
}

void UMythRoSpriteComponent::TriggerHitstop(float Duration)
{
	UWorld* W = GetWorld();
	AActor* Owner = GetOwner();
	if (!W || !Owner || Duration <= 0.f)
	{
		return;
	}
	Owner->CustomTimeDilation = 0.05f; // micro-freeze: the unit that was hit holds its frame for a moment
	W->GetTimerManager().SetTimer(HitstopTimer, FTimerDelegate::CreateWeakLambda(this, [this]()
	{
		if (AActor* O = GetOwner()) { O->CustomTimeDilation = 1.f; }
	}), Duration, false);
}

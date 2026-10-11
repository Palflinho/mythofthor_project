#include "World/MythNetWorld.h"

#include "World/MythMapInfo.h"
#include "Player/MythPlayerController.h"
#include "Player/MythPlayerCharacter.h"
#include "Gameplay/MythStatsComponent.h"
#include "Sprites/MythMonster.h"
#include "Sprites/MythRoSpriteComponent.h"
#include "Sprites/MythSkillEffects.h"
#include "Sprites/MythAura99.h"
#include "Sprites/MythEmotion.h"
#include "World/MythGroundItem.h"
#include "World/MythCellMoverComponent.h"
#include "Sprites/MythRoSound.h"
#include "World/MythWarpPortal.h"
#include "UI/MythRoClientTables.h"
#include "UI/MythHUD.h"
#include "UI/MythRoHUDWidget.h"

#include "AIController.h"
#include "Blueprint/AIBlueprintHelperLibrary.h"
#include "Dom/JsonObject.h"
#include "Engine/GameInstance.h"
#include "EngineUtils.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Misc/CommandLine.h"
#include "Misc/ConfigCacheIni.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "Misc/PackageName.h"
#include "Kismet/GameplayStatics.h"
#include "TimerManager.h"
#include "Serialization/JsonReader.h"
#include "Serialization/JsonSerializer.h"

namespace
{
	const FLinearColor NetSystemColor(0.55f, 0.95f, 0.55f);
	const FLinearColor NetErrorColor(1.f, 0.45f, 0.45f);
	const FLinearColor NetNpcColor(1.f, 0.95f, 0.6f);

	// Tipos de unidade do pacote (clif_bl_type do rAthena)
	constexpr uint8 UnitPC = 0x0;
	constexpr uint8 UnitNpc = 0x6;
	constexpr uint8 UnitNpcMobSprite = 0xC;
	constexpr uint8 UnitPet = 0x7;

	// Tipos de dano do ZC_NOTIFY_ACT (e_damage_type do rAthena)
	constexpr uint8 DmgCritical = 10;
	constexpr uint8 DmgLuckyDodge = 11;

	/** Config/unit_sprites.json (tools/gen_unit_sprite_tables.py): {"npc": {id: nome}, "mob": {id: nome}}. */
	struct FNetSpriteTables
	{
		TMap<int32, FString> Npc;
		TMap<int32, FString> Mob;
	};

	const FNetSpriteTables& NetSpriteTables()
	{
		static const FNetSpriteTables Tables = []()
		{
			FNetSpriteTables T;
			FString Text;
			TSharedPtr<FJsonObject> Root;
			if (FFileHelper::LoadFileToString(Text, *FPaths::Combine(FPaths::ProjectConfigDir(), TEXT("unit_sprites.json")))
				&& FJsonSerializer::Deserialize(TJsonReaderFactory<>::Create(Text), Root) && Root.IsValid())
			{
				auto Fill = [&Root](const TCHAR* Key, TMap<int32, FString>& Out)
				{
					const TSharedPtr<FJsonObject>* Obj = nullptr;
					if (Root->TryGetObjectField(Key, Obj))
					{
						for (const TPair<FString, TSharedPtr<FJsonValue>>& P : (*Obj)->Values)
						{
							Out.Add(FCString::Atoi(*P.Key), P.Value->AsString());
						}
					}
				};
				Fill(TEXT("npc"), T.Npc);
				Fill(TEXT("mob"), T.Mob);
			}
			UE_LOG(LogTemp, Log, TEXT("MythNetWorld: tabelas de sprite com %d NPCs e %d monstros"), T.Npc.Num(), T.Mob.Num());
			return T;
		}();
		return Tables;
	}

	bool NetSpriteExists(const FString& Folder, const FString& Name)
	{
		return !Name.IsEmpty() && FPaths::FileExists(FPaths::Combine(UMythRoSpriteComponent::GetAssetsRoot(), TEXT("data"), TEXT("sprite"), Folder, Name + TEXT(".spr")));
	}

	/** Classe -> nome do corpo (data/sprite/인간족/몸통/<sexo>/<nome>_<sexo>.spr), de Config/job_sprites.json. */
	const TMap<int32, FString>& NetJobSpriteTable()
	{
		static TMap<int32, FString> Table;
		static bool bLoaded = false;
		if (!bLoaded)
		{
			bLoaded = true;
			FString Text;
			TSharedPtr<FJsonObject> Root;
			if (FFileHelper::LoadFileToString(Text, *FPaths::Combine(FPaths::ProjectConfigDir(), TEXT("job_sprites.json")))
				&& FJsonSerializer::Deserialize(TJsonReaderFactory<>::Create(Text), Root) && Root.IsValid())
			{
				for (const auto& Pair : Root->Values)
				{
					Table.Add(FCString::Atoi(*Pair.Key), Pair.Value->AsString());
				}
			}
			UE_LOG(LogTemp, Log, TEXT("MythNetWorld: %d sprites de classe"), Table.Num());
		}
		return Table;
	}

	/**
	 * Corpo da classe para o sexo pedido. Classes de um sexo so (Bardo/Odalisca) usam o arquivo que existe,
	 * trocando bInOutMale. Classe sem sprite (fora do jogo) cai no Aprendiz.
	 */
	FString NetJobSprite(int32 JobClass, bool& bInOutMale)
	{
		const FString* Found = NetJobSpriteTable().Find(JobClass);
		const FString Name = Found ? *Found : FString(TEXT("초보자"));
		auto Exists = [&Name](bool bMale)
		{
			const TCHAR* Sex = bMale ? TEXT("남") : TEXT("여");
			return FPaths::FileExists(FPaths::Combine(UMythRoSpriteComponent::GetAssetsRoot(), TEXT("data"), TEXT("sprite"),
				TEXT("인간족"), TEXT("몸통"), Sex, FString::Printf(TEXT("%s_%s.spr"), *Name, Sex)));
		};
		if (!Exists(bInOutMale) && Exists(!bInOutMale))
		{
			bInOutMale = !bInOutMale;
		}
		return Name;
	}

	FLinearColor NetParseColor(const FString& Hex, const FLinearColor& Fallback)
	{
		FString H = Hex;
		H.RemoveFromStart(TEXT("#"));
		return H.Len() == 6 ? FLinearColor(FColor::FromHex(H)) : Fallback;
	}
}

AMythNetWorld::AMythNetWorld()
{
	PrimaryActorTick.bCanEverTick = false;
	SetRootComponent(CreateDefaultSubobject<USceneComponent>(TEXT("Root")));
}

AMythNetWorld* AMythNetWorld::FindOrSpawn(UWorld* World)
{
	if (!World)
	{
		return nullptr;
	}
	for (TActorIterator<AMythNetWorld> It(World); It; ++It)
	{
		return *It;
	}
	return World->SpawnActor<AMythNetWorld>();
}

URagnarokNetworkSubsystem* AMythNetWorld::GetNet() const
{
	const UGameInstance* GI = GetGameInstance();
	return GI ? GI->GetSubsystem<URagnarokNetworkSubsystem>() : nullptr;
}

UMythRoHUDWidget* AMythNetWorld::GetHud() const
{
	const AMythPlayerController* PC = Cast<AMythPlayerController>(GetWorld()->GetFirstPlayerController());
	return PC ? PC->GetHud() : nullptr;
}

void AMythNetWorld::Chat(const FString& Text, const FLinearColor& Color) const
{
	if (UMythRoHUDWidget* Hud = GetHud())
	{
		Hud->AddChatLine(Text, Color);
	}
	UE_LOG(LogTemp, Log, TEXT("MythNetWorld: %s"), *Text);
}

void AMythNetWorld::BeginPlay()
{
	Super::BeginPlay();
	URagnarokNetworkSubsystem* Net = GetNet();
	if (!Net)
	{
		return;
	}
	Net->OnLoginResult.AddDynamic(this, &AMythNetWorld::HandleLoginResult);
	Net->OnCharacterList.AddDynamic(this, &AMythNetWorld::HandleCharacterList);
	Net->OnMapEnter.AddDynamic(this, &AMythNetWorld::HandleMapEnter);
	Net->OnUnitAppear.AddDynamic(this, &AMythNetWorld::HandleUnitAppear);
	Net->OnUnitWalk.AddDynamic(this, &AMythNetWorld::HandleUnitWalk);
	Net->OnUnitStop.AddDynamic(this, &AMythNetWorld::HandleUnitStop);
	Net->OnUnitDirection.AddDynamic(this, &AMythNetWorld::HandleUnitDirection);
	Net->OnUnitOption.AddDynamic(this, &AMythNetWorld::HandleUnitOption);
	Net->OnUnitRevive.AddDynamic(this, &AMythNetWorld::HandleUnitRevive);
	Net->OnShopDealType.AddDynamic(this, &AMythNetWorld::HandleShopDealType);
	Net->OnShopBuyList.AddDynamic(this, &AMythNetWorld::HandleShopBuyList);
	Net->OnShopSellList.AddDynamic(this, &AMythNetWorld::HandleShopSellList);
	Net->OnShopResult.AddDynamic(this, &AMythNetWorld::HandleShopResult);
	Net->OnStorageOpen.AddDynamic(this, &AMythNetWorld::HandleStorageOpen);
	Net->OnStorageChanged.AddDynamic(this, &AMythNetWorld::HandleStorageChanged);
	Net->OnPartyChanged.AddDynamic(this, &AMythNetWorld::HandlePartyChanged);
	Net->OnPartyInvite.AddDynamic(this, &AMythNetWorld::HandlePartyInvite);
	Net->OnStorageClose.AddDynamic(this, &AMythNetWorld::HandleStorageClose);
	Net->OnSpeech.AddDynamic(this, &AMythNetWorld::HandleSpeech);
	Net->OnEmotion.AddDynamic(this, &AMythNetWorld::HandleEmotion);
	Net->OnUnitVanish.AddDynamic(this, &AMythNetWorld::HandleUnitVanish);
	Net->OnDamage.AddDynamic(this, &AMythNetWorld::HandleDamage);
	Net->OnChatMessage.AddDynamic(this, &AMythNetWorld::HandleChat);
	Net->OnPlayerStatsChanged.AddDynamic(this, &AMythNetWorld::HandleStats);
	Net->OnNetworkDisconnected.AddDynamic(this, &AMythNetWorld::HandleDisconnected);
	Net->OnNpcDialog.AddDynamic(this, &AMythNetWorld::HandleNpcDialog);
	Net->OnNpcMenu.AddDynamic(this, &AMythNetWorld::HandleNpcMenu);
	Net->OnGroundItemAppear.AddDynamic(this, &AMythNetWorld::HandleGroundItemAppear);
	Net->OnGroundItemVanish.AddDynamic(this, &AMythNetWorld::HandleGroundItemVanish);
	Net->OnSkillDamage.AddDynamic(this, &AMythNetWorld::HandleSkillDamage);
	Net->OnSkillEffect.AddDynamic(this, &AMythNetWorld::HandleSkillEffect);
	Net->OnSkillCast.AddDynamic(this, &AMythNetWorld::HandleSkillCast);
	Net->OnSkillFailed.AddDynamic(this, &AMythNetWorld::HandleSkillFailed);
	Net->OnSpecialEffect.AddDynamic(this, &AMythNetWorld::HandleSpecialEffect);
	Net->OnNotifyEffect.AddDynamic(this, &AMythNetWorld::HandleNotifyEffect);
	Net->OnEquipChanged.AddDynamic(this, &AMythNetWorld::HandleEquipChanged);
	Net->OnStatusChange.AddDynamic(this, &AMythNetWorld::HandleStatusChange);

	// Chegou num mapa novo por ordem do servidor (portal/warp): termina a entrada no proximo quadro, com o
	// personagem ja criado pelo GameMode, e so entao avisa o servidor (ACTORINIT)
	if (Net->GetNetworkState() == ERagnarokNetworkState::ConnectedMap && Net->IsAwaitingMapLoad())
	{
		GetWorldTimerManager().SetTimerForNextTick(FTimerDelegate::CreateWeakLambda(this, [this]()
		{
			if (const URagnarokNetworkSubsystem* N = GetNet())
			{
				FString Map = N->GetPendingMapName();
				Map.RemoveFromEnd(TEXT(".gat"));
				bInMap = true;
				RemoveOfflineMonsters();
				FinishMapEnter(Map, N->GetPendingMapCell().X, N->GetPendingMapCell().Y);
			}
		}));
		return;
	}
	if (FParse::Param(FCommandLine::Get(), TEXT("MythOnline")))
	{
		Login();
	}
}

void AMythNetWorld::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	if (URagnarokNetworkSubsystem* Net = GetNet())
	{
		Net->OnLoginResult.RemoveAll(this);
		Net->OnCharacterList.RemoveAll(this);
		Net->OnMapEnter.RemoveAll(this);
		Net->OnUnitAppear.RemoveAll(this);
		Net->OnUnitWalk.RemoveAll(this);
		Net->OnUnitStop.RemoveAll(this);
		Net->OnUnitDirection.RemoveAll(this);
		Net->OnUnitOption.RemoveAll(this);
		Net->OnUnitRevive.RemoveAll(this);
		Net->OnShopDealType.RemoveAll(this);
		Net->OnShopBuyList.RemoveAll(this);
		Net->OnShopSellList.RemoveAll(this);
		Net->OnShopResult.RemoveAll(this);
		Net->OnStorageOpen.RemoveAll(this);
		Net->OnPartyChanged.RemoveAll(this);
		Net->OnPartyInvite.RemoveAll(this);
		Net->OnStorageChanged.RemoveAll(this);
		Net->OnStorageClose.RemoveAll(this);
		Net->OnSpeech.RemoveAll(this);
		Net->OnEmotion.RemoveAll(this);
		Net->OnUnitVanish.RemoveAll(this);
		Net->OnDamage.RemoveAll(this);
		Net->OnChatMessage.RemoveAll(this);
		Net->OnPlayerStatsChanged.RemoveAll(this);
		Net->OnNetworkDisconnected.RemoveAll(this);
		Net->OnNpcDialog.RemoveAll(this);
		Net->OnNpcMenu.RemoveAll(this);
		Net->OnGroundItemAppear.RemoveAll(this);
		Net->OnGroundItemVanish.RemoveAll(this);
		Net->OnSkillDamage.RemoveAll(this);
		Net->OnSkillEffect.RemoveAll(this);
		Net->OnSkillCast.RemoveAll(this);
		Net->OnSkillFailed.RemoveAll(this);
		Net->OnSpecialEffect.RemoveAll(this);
		Net->OnNotifyEffect.RemoveAll(this);
		Net->OnEquipChanged.RemoveAll(this);
		Net->OnStatusChange.RemoveAll(this);
	}
	Super::EndPlay(EndPlayReason);
}

void AMythNetWorld::Login(const FString& User, const FString& Password)
{
	URagnarokNetworkSubsystem* Net = GetNet();
	if (!Net)
	{
		return;
	}
	FString Host = TEXT("127.0.0.1"), CfgUser, CfgPass;
	GConfig->GetString(TEXT("Myth"), TEXT("NetHost"), Host, GGameIni);
	GConfig->GetString(TEXT("Myth"), TEXT("NetUser"), CfgUser, GGameIni);
	GConfig->GetString(TEXT("Myth"), TEXT("NetPassword"), CfgPass, GGameIni);
	const FString U = User.IsEmpty() ? CfgUser : User;
	const FString P = Password.IsEmpty() ? CfgPass : Password;
	if (U.IsEmpty())
	{
		Chat(TEXT("Sem conta configurada ([Myth] NetUser no DefaultGame.ini)."), NetErrorColor);
		return;
	}
	Chat(FString::Printf(TEXT("Conectando a %s como %s..."), *Host, *U), NetSystemColor);
	if (!Net->ConnectAndLogin(Host, U, P))
	{
		Chat(TEXT("Falha ao conectar no servidor de login (6900)."), NetErrorColor);
	}
}

// ------------------------------------------------------------------------------------------------ login

void AMythNetWorld::HandleLoginResult(bool bSuccess, const FString& Message)
{
	Chat(bSuccess ? TEXT("Login aceito.") : FString::Printf(TEXT("Login recusado: %s"), *Message), bSuccess ? NetSystemColor : NetErrorColor);
}

void AMythNetWorld::HandleCharacterList(const TArray<FRagnarokCharacterSlotInfo>& Characters)
{
	if (Characters.IsEmpty())
	{
		Chat(TEXT("Conta sem personagens (crie um no servidor)."), NetErrorColor);
		return;
	}
	// Tela de selecao classica ainda nao existe: entra com o primeiro personagem
	const FRagnarokCharacterSlotInfo& C = Characters[0];
	Chat(FString::Printf(TEXT("Personagem: %s (Nv %d)"), *C.Name, C.BaseLevel), NetSystemColor);
	if (URagnarokNetworkSubsystem* Net = GetNet())
	{
		Net->SelectCharacter(C.Slot);
	}
}

// ------------------------------------------------------------------------------------------------ mapa

FVector AMythNetWorld::CellToGround(int32 X, int32 Y, bool* bOutFound) const
{
	const AMythMapInfo* Info = AMythMapInfo::Find(GetWorld());
	FVector Top = Info ? Info->CellToWorld(FIntPoint(X, Y), 20000.f) : FVector(X * 50.f, Y * 50.f, 20000.f);
	FHitResult Hit;
	// O chao do GAT bloqueia Visibility; a malha visual do mapa nao (ver ue_import_map.py)
	// Ignora as caixas de clique das unidades (so o cenario fixo e chao)
	FCollisionQueryParams CellParams(TEXT("MythNetCell"), false);
	bool bHit = false;
	for (int32 Guard = 0; Guard < 12; ++Guard)
	{
		bHit = GetWorld()->LineTraceSingleByChannel(Hit, Top, Top - FVector(0.f, 0.f, 40000.f), ECC_Visibility, CellParams);
		if (!bHit || !Hit.GetComponent() || Hit.GetComponent()->Mobility == EComponentMobility::Static)
		{
			break;
		}
		CellParams.AddIgnoredActor(Hit.GetActor());
	}
	if (bOutFound)
	{
		*bOutFound = bHit;
	}
	return bHit ? Hit.ImpactPoint : FVector(Top.X, Top.Y, 0.f);
}

void AMythNetWorld::RemoveOfflineMonsters()
{
	// Os monstros de teste colocados no nivel (sem NetID) dao lugar aos do servidor
	for (TActorIterator<AMythMonster> It(GetWorld()); It; ++It)
	{
		if (!It->IsNetUnit())
		{
			It->Destroy();
		}
	}
}

namespace
{
	/** Nivel importado de um mapa do servidor: prt_in -> /Game/Maps/PrtIn/Lvl_PrtIn (mesmo nome do ue_import_map.py). */
	FString NetLevelPath(const FString& Map)
	{
		TArray<FString> Parts;
		Map.ParseIntoArray(Parts, TEXT("_"), true);
		FString Title;
		for (const FString& P : Parts)
		{
			Title += P.Left(1).ToUpper() + P.Mid(1).ToLower();
		}
		return FString::Printf(TEXT("/Game/Maps/%s/Lvl_%s"), *Title, *Title);
	}
}

void AMythNetWorld::HandleMapEnter(const FString& MapName, int32 CellX, int32 CellY, uint8 Direction)
{
	FString Map = MapName;
	Map.RemoveFromEnd(TEXT(".gat"));
	bInMap = true;
	RemoveOfflineMonsters();
	// Troca de mapa (ou warp no mesmo mapa): o servidor reenvia tudo o que estiver em volta depois do ACTORINIT
	for (TPair<int64, TObjectPtr<AMythMonster>>& U : Units)
	{
		if (IsValid(U.Value))
		{
			U.Value->Destroy();
		}
	}
	Units.Reset();
	for (TPair<int64, TObjectPtr<AMythGroundItem>>& I : GroundItems)
	{
		if (IsValid(I.Value))
		{
			I.Value->Destroy();
		}
	}
	GroundItems.Reset();

	const AMythMapInfo* Info = AMythMapInfo::Find(GetWorld());
	if (Info && !Info->MapName.Equals(Map, ESearchCase::IgnoreCase))
	{
		const FString Level = NetLevelPath(Map);
		if (FPackageName::DoesPackageExist(Level))
		{
			// Carrega o mapa novo; o AMythNetWorld de la posiciona o personagem e avisa o servidor (NotifyMapLoaded)
			Chat(FString::Printf(TEXT("Indo para %s..."), *Map), NetSystemColor);
			UGameplayStatics::OpenLevel(this, FName(*Level));
			return;
		}
		Chat(FString::Printf(TEXT("O servidor colocou voce em '%s', mas esse mapa ainda nao foi importado (%s)."), *Map, *Level), NetErrorColor);
	}
	FinishMapEnter(Map, CellX, CellY);
}

void AMythNetWorld::FinishMapEnter(const FString& Map, int32 CellX, int32 CellY)
{
	// Entrar no mapa (inclusive o respawn no ponto de salvamento, que so manda a troca de posicao) tira do estado de morte
	if (const AMythPlayerController* DeadPC = Cast<AMythPlayerController>(GetWorld()->GetFirstPlayerController()))
	{
		if (const AMythPlayerCharacter* Self = Cast<AMythPlayerCharacter>(DeadPC->GetPawn()); Self && Self->Sprite)
		{
			Self->Sprite->Revive();
		}
		if (DeadPC->GetHud())
		{
			DeadPC->GetHud()->HideDeathWindow();
		}
	}
	if (AMythPlayerController* PC = Cast<AMythPlayerController>(GetWorld()->GetFirstPlayerController()))
	{
		PC->TeleportToCell(CellX, CellY);
		PC->StopMovement();
	}
	LastSelfDestination = FVector(TNumericLimits<float>::Max());
	if (URagnarokNetworkSubsystem* Net = GetNet())
	{
		Net->NotifyMapLoaded(); // CZ_NOTIFY_ACTORINIT: agora o servidor manda NPCs, monstros e jogadores daqui
		ApplySelfAppearance();
		HandleStats(Net->GetPlayerStats());
	}
	Chat(FString::Printf(TEXT("Entrou em %s (%d, %d)."), *Map, CellX, CellY), NetSystemColor);
}

// ------------------------------------------------------------------------------------------------ unidades

AActor* AMythNetWorld::FindUnit(int64 ID) const
{
	const TObjectPtr<AMythMonster>* Found = Units.Find(ID);
	return Found && IsValid(*Found) ? Found->Get() : nullptr;
}

AActor* AMythNetWorld::ResolveActor(int64 ID) const
{
	const URagnarokNetworkSubsystem* Net = GetNet();
	if (Net && ID == Net->GetSelfID())
	{
		const APlayerController* PC = GetWorld()->GetFirstPlayerController();
		return PC ? PC->GetPawn() : nullptr;
	}
	return FindUnit(ID);
}

void AMythNetWorld::HandleUnitAppear(const FRagnarokUnitVisualInfo& Info)
{
	const URagnarokNetworkSubsystem* Net = GetNet();
	if (!Net || Info.GID == 0 || Info.GID == Net->GetSelfID() || Info.ObjectType == 0x2 /* item: OnGroundItemAppear */)
	{
		return;
	}
	bool bFound = false;
	const FVector Ground = CellToGround(Info.CellX, Info.CellY, &bFound);
	if (AActor* Existing = FindUnit(Info.GID))
	{
		// Unidade ja em campo: o rAthena reenvia o pacote a cada passo (0x09fd = andando). So corrige a posicao
		// se divergiu mais de 2 celulas; o destino chega logo depois pelo OnUnitMove.
		AMythMonster* Mob = Cast<AMythMonster>(Existing);
		if (FVector::Dist2D(Existing->GetActorLocation(), Ground) > 100.f && !(Mob && Mob->Mover && Mob->Mover->IsMoving()))
		{
			Existing->SetActorLocation(Ground + FVector(0.f, 0.f, 45.f), false, nullptr, ETeleportType::TeleportPhysics);
			if (Mob && Mob->Mover) { Mob->Mover->PlaceOnGround(); }
		}
		return;
	}

	// Sprite: monstros/pets pelo AegisName, NPCs pelo JT_*; jogadores pelo corpo da classe
	const FNetSpriteTables& Tables = NetSpriteTables();
	FString Folder = TEXT("몬스터"); // 몬스터
	FString SpriteName;
	const bool bPlayer = Info.ObjectType == UnitPC;
	if (Info.ObjectType == UnitNpc)
	{
		Folder = TEXT("npc");
		if (const FString* N = Tables.Npc.Find(Info.JobClass)) { SpriteName = *N; }
		if (!NetSpriteExists(Folder, SpriteName) && Tables.Mob.Contains(Info.JobClass))
		{
			Folder = TEXT("몬스터");
			SpriteName = Tables.Mob[Info.JobClass];
		}
	}
	else if (!bPlayer)
	{
		if (const FString* N = Tables.Mob.Find(Info.JobClass)) { SpriteName = *N; }
	}
	const bool bWarp = Info.ObjectType == UnitNpc && (Info.JobClass == 45 || Info.JobClass == 139);
	if (!bPlayer && !bWarp && !NetSpriteExists(Folder, SpriteName))
	{
		// NPC sem sprite neste cliente (efeito, bandeira de guilda...): nao desenha nada. O portal tambem nao tem
		// sprite, mas precisa existir (aneis, cursor e clique de portal)
		if (Info.ObjectType == UnitNpc)
		{
			return;
		}
		UE_LOG(LogTemp, Warning, TEXT("MythNetWorld: sem sprite para classe %d (%s) tipo %d; usando poring"), Info.JobClass, *SpriteName, Info.ObjectType);
		Folder = TEXT("몬스터");
		SpriteName = TEXT("poring");
	}

	FActorSpawnParameters Params;
	Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
	const FTransform Xf(FRotator::ZeroRotator, Ground + FVector(0.f, 0.f, 45.f));
	AMythMonster* Unit = GetWorld()->SpawnActorDeferred<AMythMonster>(AMythMonster::StaticClass(), Xf, nullptr, nullptr, ESpawnActorCollisionHandlingMethod::AlwaysSpawn);
	if (!Unit)
	{
		return;
	}
	Unit->NetID = Info.GID;
	Unit->NetObjectType = Info.ObjectType;
	Unit->MonsterName = Info.Name;
	Unit->MaxHP = FMath::Max(Info.MaxHP, 1);
	Unit->AIControllerClass = AAIController::StaticClass();
	Unit->AutoPossessAI = EAutoPossessAI::Spawned;
	bool bAuraWanted = false;
	if (UMythRoSpriteComponent* S = Unit->Sprite)
	{
		if (Info.bDirectionKnown)
		{
			S->SetFacingRoDir(Info.Direction); // parado olha para onde o servidor manda; andando, a velocidade decide
		}
		if (bPlayer)
		{
			S->MonsterSpriteName.Empty();
			S->bMale = Info.Sex != 0;
			S->JobSpriteName = NetJobSprite(Info.JobClass, S->bMale);
			S->JobClass = Info.JobClass;
			S->HeadTop = Info.HeadTop;
			S->HeadMid = Info.HeadMid;
			S->HeadBottom = Info.HeadBottom;
			S->Weapon = Info.Weapon & 0xFFFF;
			S->Shield = Info.Shield & 0xFFFF;
			S->Robe = Info.Robe;
			S->CartType = UMythRoSpriteComponent::CartTypeFromOption(Info.Option);
			bAuraWanted = Info.BaseLevel >= 99;
			S->HairStyle = FMath::Max(Info.Head, 1);
			S->HairColor = Info.HeadPalette;
			S->SetHeadDir(Info.HeadDir);
		}
		else if (!bWarp) // o portal nao tem sprite (os aneis sao do AMythWarpPortal)
		{
			S->MonsterSpriteName = SpriteName;
			S->MonsterSpriteFolder = Folder;
		}
	}
	Unit->FinishSpawning(Xf);
	if (bAuraWanted)
	{
		AMythAura99::Ensure(Unit, true); // outros jogadores nivel 99 tambem brilham (docs/ref/13 2.1)
	}
	if (UCharacterMovementComponent* Move = Unit->GetCharacterMovement())
	{
		// Velocidade do RO: ms por celula (150 = padrao) -> cm/s com celula de 50 cm
		Move->MaxWalkSpeed = 50.f * 1000.f / FMath::Max(Info.Speed, 20);
	}
	Unit->NetSpeed = FMath::Max(Info.Speed, 20);
	Units.Add(Info.GID, Unit);
	if (Unit->Mover)
	{
		Unit->Mover->PlaceOnGround(); // sem fisica: assenta no chao do .gat ao nascer (antes ficava 45 uu no ar)
	}

	// Portais (JT_WARPNPC 45): no RO sao aneis azuis desenhados pelo executavel, sem sprite. O 139 (portal
	// escondido) nao mostra nada.
	if (bWarp)
	{
		// Celula de portal: cursor de portal e clique = andar ate ela (o cliente nao "fala" com o warp)
		Unit->Tags.AddUnique(TEXT("MythWarp"));
		if (Unit->Sprite)
		{
			Unit->Sprite->SetVisibility(false, true);
			Unit->Sprite->bBlobShadow = false;
			Unit->Sprite->bCastSpriteShadow = false;
		}
		if (Info.JobClass == 45)
		{
			FActorSpawnParameters P;
			P.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
			P.Owner = Unit;
			if (AMythWarpPortal* Portal = GetWorld()->SpawnActor<AMythWarpPortal>(Ground, FRotator::ZeroRotator, P))
			{
				Portal->AttachToActor(Unit, FAttachmentTransformRules::KeepWorldTransform);
			}
		}
	}
}

void AMythNetWorld::HandleUnitWalk(int64 ID, int32 FromX, int32 FromY, int32 TargetX, int32 TargetY, int32 StartTick)
{
	// Rota do servidor (ZC_NOTIFY_PLAYERMOVE para o proprio jogador, ZC_NOTIFY_MOVE para os outros): anda pelas
	// celulas como o cliente original, partindo de onde o ator esta (sem estalo; salta so com mais de 16 de erro)
	const URagnarokNetworkSubsystem* Net = GetNet();
	if (Net && ID == Net->GetSelfID())
	{
		APlayerController* PC = GetWorld()->GetFirstPlayerController();
		if (AMythPlayerCharacter* Char = PC ? Cast<AMythPlayerCharacter>(PC->GetPawn()) : nullptr; Char && Char->Mover)
		{
			Char->Mover->ServerWalk(FIntPoint(FromX, FromY), FIntPoint(TargetX, TargetY), 150);
		}
		return;
	}
	AMythMonster* Unit = Cast<AMythMonster>(FindUnit(ID));
	if (!Unit || Unit->IsDead())
	{
		return;
	}
	if (Unit->Mover)
	{
		Unit->Mover->ServerWalk(FIntPoint(FromX, FromY), FIntPoint(TargetX, TargetY), Unit->NetSpeed);
	}
}

void AMythNetWorld::HandleUnitStop(int64 ID, int32 X, int32 Y)
{
	// ZC_STOPMOVE: a regra de ignorar paradas atrasadas (sem "passo para tras") fica no ServerStop
	const AActor* Actor = ResolveActor(ID);
	if (UMythCellMoverComponent* Mover = Actor ? Actor->FindComponentByClass<UMythCellMoverComponent>() : nullptr)
	{
		Mover->ServerStop(FIntPoint(X, Y));
	}
}

void AMythNetWorld::HandleUnitDirection(int64 ID, uint8 HeadTurnValue, uint8 BodyDirValue)
{
	// ZC_CHANGE_DIRECTION: outro jogador virou o corpo e/ou a cabeca (doridori, Shift+clique)
	const AActor* Actor = ResolveActor(ID);
	if (UMythRoSpriteComponent* S = Actor ? Actor->FindComponentByClass<UMythRoSpriteComponent>() : nullptr)
	{
		S->SetFacingRoDir(BodyDirValue);
		S->SetHeadDir(HeadTurnValue);
	}
}

void AMythNetWorld::HandleUnitOption(int64 ID, int32 Option)
{
	// ZC_STATE_CHANGE: carrinho (e depois montaria, falcao...) liga e desliga
	const AActor* Actor = ResolveActor(ID);
	UMythRoSpriteComponent* S = Actor ? Actor->FindComponentByClass<UMythRoSpriteComponent>() : nullptr;
	const int32 Cart = UMythRoSpriteComponent::CartTypeFromOption(Option);
	if (S && S->MonsterSpriteName.IsEmpty() && S->CartType != Cart)
	{
		S->CartType = Cart;
		S->ReloadSprites();
	}
}

void AMythNetWorld::HandleEmotion(int64 UnitID, int32 EmotionType)
{
	// ZC_EMOTION: a acao <tipo> do emotion.act sobre a cabeca (2,5 s)
	AMythEmotion::Show(ResolveActor(UnitID), EmotionType);
}

void AMythNetWorld::HandleSpeech(int64 UnitID, const FString& Text)
{
	// Fala publica: balao sobre a cabeca de quem falou (4,5 s)
	if (AActor* Speaker = ResolveActor(UnitID))
	{
		if (AMythHUD* Hud = GetGameHud())
		{
			Hud->AddSpeechBubble(Speaker, Text);
		}
	}
}

void AMythNetWorld::HandleStorageOpen(const TArray<FRagnarokInventoryItem>&)
{
	if (AMythPlayerController* PC = Cast<AMythPlayerController>(GetWorld()->GetFirstPlayerController()); PC && PC->GetHud())
	{
		PC->GetHud()->OpenStorage();
	}
}

void AMythNetWorld::HandleStorageChanged(const TArray<FRagnarokInventoryItem>&)
{
	if (AMythPlayerController* PC = Cast<AMythPlayerController>(GetWorld()->GetFirstPlayerController()); PC && PC->GetHud())
	{
		PC->GetHud()->RefreshStorage();
	}
}

void AMythNetWorld::HandlePartyChanged()
{
	if (AMythPlayerController* PC = Cast<AMythPlayerController>(GetWorld()->GetFirstPlayerController()); PC && PC->GetHud())
	{
		PC->GetHud()->RefreshParty();
	}
}

void AMythNetWorld::HandlePartyInvite(int32 PartyId, const FString& PartyName)
{
	if (AMythPlayerController* PC = Cast<AMythPlayerController>(GetWorld()->GetFirstPlayerController()); PC && PC->GetHud())
	{
		PC->GetHud()->ShowPartyInvite(PartyId, PartyName);
	}
}

void AMythNetWorld::HandleStorageClose()
{
	if (AMythPlayerController* PC = Cast<AMythPlayerController>(GetWorld()->GetFirstPlayerController()); PC && PC->GetHud())
	{
		PC->GetHud()->CloseStorage(false);
	}
}

void AMythNetWorld::HandleShopDealType(int64 NpcID)
{
	if (AMythPlayerController* PC = Cast<AMythPlayerController>(GetWorld()->GetFirstPlayerController()); PC && PC->GetHud())
	{
		PC->GetHud()->ShowShopDealType(NpcID);
	}
}

void AMythNetWorld::HandleShopBuyList(const TArray<FRagnarokShopItem>& Items)
{
	if (AMythPlayerController* PC = Cast<AMythPlayerController>(GetWorld()->GetFirstPlayerController()); PC && PC->GetHud())
	{
		PC->GetHud()->ShowShopItems(false, Items);
	}
}

void AMythNetWorld::HandleShopSellList(const TArray<FRagnarokShopItem>& Items)
{
	if (AMythPlayerController* PC = Cast<AMythPlayerController>(GetWorld()->GetFirstPlayerController()); PC && PC->GetHud())
	{
		PC->GetHud()->ShowShopItems(true, Items);
	}
}

void AMythNetWorld::HandleShopResult(bool bSell, int32 Result)
{
	if (AMythPlayerController* PC = Cast<AMythPlayerController>(GetWorld()->GetFirstPlayerController()); PC && PC->GetHud())
	{
		PC->GetHud()->ShopResult(bSell, Result);
	}
}

void AMythNetWorld::HandleUnitRevive(int64 ID)
{
	// ZC_RESURRECTION: de pe de novo; para o proprio jogador, fecha a janela de morte
	const URagnarokNetworkSubsystem* Net = GetNet();
	const AActor* Actor = ResolveActor(ID);
	if (UMythRoSpriteComponent* S = Actor ? Actor->FindComponentByClass<UMythRoSpriteComponent>() : nullptr)
	{
		S->Revive();
	}
	if (Net && ID == Net->GetSelfID())
	{
		if (AMythPlayerController* PC = Cast<AMythPlayerController>(GetWorld()->GetFirstPlayerController()); PC && PC->GetHud())
		{
			PC->GetHud()->HideDeathWindow();
		}
	}
}

void AMythNetWorld::HandleUnitVanish(int64 ID, uint8 Reason)
{
	// O proprio jogador morreu (type 1): deita no chao e abre a janela de morte. O servidor manda o vanish tambem a ele
	if (const URagnarokNetworkSubsystem* Net = GetNet(); Net && ID == Net->GetSelfID())
	{
		if (Reason == 1)
		{
			const AActor* Self = ResolveActor(ID);
			if (UMythRoSpriteComponent* S = Self ? Self->FindComponentByClass<UMythRoSpriteComponent>() : nullptr)
			{
				S->Die();
			}
			if (AMythPlayerController* PC = Cast<AMythPlayerController>(GetWorld()->GetFirstPlayerController()); PC && PC->GetHud())
			{
				PC->GetHud()->ShowDeathWindow();
			}
		}
		return;
	}
	TObjectPtr<AMythMonster> Unit;
	if (!Units.RemoveAndCopyValue(ID, Unit) || !IsValid(Unit))
	{
		return;
	}
	// Visuais presos na unidade (aneis do portal) saem junto
	TArray<AActor*> Attached;
	Unit->GetAttachedActors(Attached);
	for (AActor* A : Attached)
	{
		A->Destroy();
	}
	// 1 = morreu (toca a animacao de morte); 0 saiu da visao, 2 logout, 3 teleporte
	if (Reason == 1)
	{
		Unit->Kill();
	}
	else
	{
		Unit->Destroy();
	}
}

void AMythNetWorld::HandleDamage(int64 SourceID, int64 TargetID, int32 Damage, uint8 DamageType, int32 DelayMs)
{
	const URagnarokNetworkSubsystem* Net = GetNet();
	const int64 Self = Net ? Net->GetSelfID() : 0;
	AActor* Source = ResolveActor(SourceID);
	AActor* Target = ResolveActor(TargetID);
	UE_LOG(LogTemp, Log, TEXT("MythNetWorld: dano %d (tipo %d) de %lld em %lld"), Damage, DamageType, SourceID, TargetID);

	// ZC_NOTIFY_ACT que nao e golpe (e_damage_type do rAthena): 1 pegou item, 2 sentou, 3 levantou
	if (DamageType == 1 || DamageType == 2 || DamageType == 3)
	{
		if (UMythRoSpriteComponent* S = Source ? Source->FindComponentByClass<UMythRoSpriteComponent>() : nullptr; S && DamageType != 1)
		{
			S->SetSitting(DamageType == 2);
		}
		return;
	}

	// Quem ataca vira para o alvo e toca o golpe no ritmo do servidor (attackMT do ZC_NOTIFY_ACT)
	if (AMythPlayerCharacter* Char = Cast<AMythPlayerCharacter>(Source))
	{
		if (Char->Sprite && Target)
		{
			Char->Sprite->FaceLocation(Target->GetActorLocation());
			Char->Sprite->PlayAttack(DelayMs > 0 ? DelayMs / 1000.f : 0.f);
		}
	}
	else if (AMythMonster* Mob = Cast<AMythMonster>(Source))
	{
		Mob->PlayAttackFeedback();
	}
	if (!Target)
	{
		return;
	}

	// Arco (view 11 da arma): a flecha sai ao fim do golpe e leva ~190 ms; o numero de dano e a reacao do alvo so
	// aparecem quando ela chega (docs/ref/03; roBrowser soma 8 quadros de 24 ms ao attackMT para o arco)
	const UMythRoSpriteComponent* SrcSprite = Source ? Source->FindComponentByClass<UMythRoSpriteComponent>() : nullptr;
	if (SrcSprite && SrcSprite->MonsterSpriteName.IsEmpty() && SrcSprite->Weapon == 11)
	{
		const float Attack = FMath::Max(DelayMs, 1) / 1000.f;
		TWeakObjectPtr<AActor> WSource(Source);
		TWeakObjectPtr<AActor> WTarget(Target);
		FTimerHandle LaunchHandle;
		GetWorldTimerManager().SetTimer(LaunchHandle, FTimerDelegate::CreateWeakLambda(this, [this, WSource, WTarget]()
		{
			if (WSource.IsValid() && WTarget.IsValid())
			{
				FMythSkillEffects::PlayEffectId(this, 2003, WTarget.Get(), WTarget->GetActorLocation(), WSource.Get()); // ef_arrow_projectile
			}
		}), Attack + 0.05f, false);
		FTimerHandle ImpactHandle;
		GetWorldTimerManager().SetTimer(ImpactHandle, FTimerDelegate::CreateWeakLambda(this, [this, TargetID, WSource, WTarget, Damage, DamageType]()
		{
			ShowImpact(TargetID, WSource.Get(), WTarget.Get(), Damage, DamageType);
		}), Attack + 0.192f, false);
		return;
	}
	ShowImpact(TargetID, Source, Target, Damage, DamageType);
}

void AMythNetWorld::ShowImpact(int64 TargetID, AActor* Source, AActor* Target, int32 Damage, uint8 DamageType)
{
	if (!Target)
	{
		return;
	}
	const URagnarokNetworkSubsystem* Net = GetNet();
	const int64 Self = Net ? Net->GetSelfID() : 0;
	const FVector At = Target->GetActorLocation() + FVector(0.f, 0.f, 50.f);
	if (Damage <= 0 || DamageType == DmgLuckyDodge)
	{
		UMythCombatText::ShowMiss(this, At);
		return;
	}
	ECombatTextType Type = TargetID == Self ? ECombatTextType::DamageTaken : (DamageType == DmgCritical ? ECombatTextType::Critical : ECombatTextType::DamageDealt);
	UMythCombatText::ShowDamage(this, At, Damage, Type);

	// Sons de impacto (docs/ref/07 2.3): a arma de quem bateu (jogador) e, se o alvo e um jogador, a roupa dele
	const UMythRoSpriteComponent* SourceSprite = Source ? Source->FindComponentByClass<UMythRoSpriteComponent>() : nullptr;
	const UMythRoSpriteComponent* TargetSprite = Target->FindComponentByClass<UMythRoSpriteComponent>();
	if (SourceSprite && SourceSprite->MonsterSpriteName.IsEmpty())
	{
		const int32 W = SourceSprite->Weapon;
		UMythRoSound::PlayWavAtLocation(this, UMythRoSound::WeaponHitWav(W > 0 && W <= 23 ? W : (W > 0 ? 2 : 0)), Target->GetActorLocation());
	}
	if (TargetSprite && TargetSprite->MonsterSpriteName.IsEmpty())
	{
		UMythRoSound::PlayWavAtLocation(this, UMythRoSound::JobHitWav(TargetSprite->JobClass, TargetSprite->bMale), Target->GetActorLocation());
	}
	if (AMythMonster* Mob = Cast<AMythMonster>(Target))
	{
		Mob->NetHit();
	}
	else if (AMythPlayerCharacter* Char = Cast<AMythPlayerCharacter>(Target))
	{
		if (Char->Sprite)
		{
			Char->Sprite->PlayHit();
		}
	}
}

// ------------------------------------------------------------------------------------------------ chat, status, NPC

void AMythNetWorld::HandleChat(const FString& Sender, const FString& Text, const FString& ColorHex)
{
	if (ColorHex == TEXT("#FFFF00") && !Sender.IsEmpty())
	{
		if (AMythPlayerController* WhisperPC = Cast<AMythPlayerController>(GetWorld()->GetFirstPlayerController()); WhisperPC && WhisperPC->GetHud())
		{
			WhisperPC->GetHud()->SetLastWhisperFrom(Sender); // para o /r
		}
	}
	const FString Line = Sender.IsEmpty() ? Text : FString::Printf(TEXT("%s : %s"), *Sender, *Text);
	Chat(Line, NetParseColor(ColorHex, FLinearColor::White));
}

void AMythNetWorld::HandleStats(const FRagnarokPlayerStats& S)
{
	const APlayerController* PC = GetWorld()->GetFirstPlayerController();
	const AMythPlayerCharacter* Char = PC ? Cast<AMythPlayerCharacter>(PC->GetPawn()) : nullptr;
	if (!Char || !Char->Stats)
	{
		return;
	}
	const URagnarokNetworkSubsystem* Net = GetNet();
	Char->Stats->SetIdentity(Net && !Net->GetSelfName().IsEmpty() ? Net->GetSelfName() : Char->Stats->GetCharName(), S.BaseLevel);
	AMythAura99::Ensure(const_cast<AMythPlayerCharacter*>(Char), S.BaseLevel >= 99);
	if (S.MaxHP > 0)
	{
		Char->Stats->SetVitals(static_cast<int32>(S.HP), static_cast<int32>(S.MaxHP), static_cast<int32>(S.SP), static_cast<int32>(FMath::Max<int64>(S.MaxSP, 1)));
	}
	if (S.NextBaseExp > 0)
	{
		Char->Stats->SetExp(S.BaseExp, S.NextBaseExp);
	}
}

void AMythNetWorld::HandleDisconnected(const FString& Reason)
{
	bInMap = false;
	Chat(FString::Printf(TEXT("Desconectado do servidor: %s"), *Reason), NetErrorColor);
}

void AMythNetWorld::HandleNpcDialog(int64 NpcID, const FString& Text)
{
	UMythRoHUDWidget* Hud = GetHud();
	if (!Hud)
	{
		return;
	}
	// O subsistema manda as falas (mes) e os marcadores de controle do script (next; e close;)
	if (Text == TEXT("[WAIT]") || Text == TEXT("[NEXT]"))
	{
		Hud->NpcWait(NpcID);
	}
	else if (Text == TEXT("[CLOSE]"))
	{
		Hud->NpcCloseButton(NpcID);
	}
	else
	{
		Hud->NpcSay(NpcID, Text);
	}
}

void AMythNetWorld::HandleNpcMenu(int64 NpcID, const FString& Title, const TArray<FString>& Options)
{
	if (UMythRoHUDWidget* Hud = GetHud())
	{
		Hud->NpcMenu(NpcID, Options);
	}
}

// ------------------------------------------------------------------------------------------------ itens no chao

void AMythNetWorld::HandleGroundItemAppear(int64 ID, int32 ItemID, int32 Amount, int32 CellX, int32 CellY)
{
	if (AMythGroundItem* Existing = GroundItems.FindRef(ID))
	{
		if (IsValid(Existing))
		{
			return;
		}
	}
	const FVector Ground = CellToGround(CellX, CellY);
	const FTransform Xf(FRotator::ZeroRotator, Ground);
	AMythGroundItem* Item = GetWorld()->SpawnActorDeferred<AMythGroundItem>(AMythGroundItem::StaticClass(), Xf, nullptr, nullptr, ESpawnActorCollisionHandlingMethod::AlwaysSpawn);
	if (!Item)
	{
		return;
	}
	// Item novo perto de quem acabou de matar algo: toca o pulinho de queda (o ZC_ITEM_ENTRY e item ja parado)
	Item->Setup(ID, ItemID, Amount, bInMap);
	Item->FinishSpawning(Xf);
	GroundItems.Add(ID, Item);
	UE_LOG(LogTemp, Log, TEXT("MythNetWorld: item %d x%d no chao em (%d, %d)"), ItemID, Amount, CellX, CellY);
}

void AMythNetWorld::HandleGroundItemVanish(int64 ID)
{
	TObjectPtr<AMythGroundItem> Item;
	if (GroundItems.RemoveAndCopyValue(ID, Item) && IsValid(Item))
	{
		UE_LOG(LogTemp, Log, TEXT("MythNetWorld: item %lld saiu do chao"), ID);
		Item->Destroy();
	}
}

// ------------------------------------------------------------------------------------------------ skills

AMythHUD* AMythNetWorld::GetGameHud() const
{
	const APlayerController* PC = GetWorld()->GetFirstPlayerController();
	return PC ? Cast<AMythHUD>(PC->GetHUD()) : nullptr;
}

void AMythNetWorld::HandleSkillDamage(int64 SourceID, int64 TargetID, int32 SkillID, int32 Damage, int32 Hits, uint8 DamageType)
{
	AActor* Source = ResolveActor(SourceID);
	if (AMythHUD* Hud = GetGameHud(); Hud && Source)
	{
		Hud->AddSkillShout(Source, FMythRoClientTables::SkillName(SkillID));
	}
	// Efeitos como o cliente (SkillEffect): no conjurador uma vez por uso (Impacto Explosivo acerta varios monstros
	// mas explode uma vez), depois no alvo e o de impacto, sem repetir o mesmo efeito
	const int32 CastFx = FMythSkillEffects::EffectIdFor(SkillID, EMythSkillEffectStage::Cast);
	const int32 MainFx = FMythSkillEffects::EffectIdFor(SkillID, EMythSkillEffectStage::Effect);
	const int32 HitFx = FMythSkillEffects::EffectIdFor(SkillID, EMythSkillEffectStage::Hit);
	bool bAnyFx = false;
	if (Source && CastFx > 0)
	{
		const double Now = GetWorld()->GetTimeSeconds();
		const TPair<int64, int32> Key(SourceID, SkillID);
		if (Now - LastCasterFx.FindRef(Key) > 0.3)
		{
			LastCasterFx.Add(Key, Now);
			bAnyFx |= FMythSkillEffects::Play(this, SkillID, EMythSkillEffectStage::Cast, Source, Source->GetActorLocation(), false, Source);
		}
		else
		{
			bAnyFx = true;
		}
	}
	if (AActor* Target = ResolveActor(TargetID))
	{
		if (MainFx > 0 && MainFx != CastFx)
		{
			bAnyFx |= FMythSkillEffects::Play(this, SkillID, EMythSkillEffectStage::Effect, Target, Target->GetActorLocation(), false, Source);
		}
		if (HitFx > 0 && HitFx != CastFx && HitFx != MainFx)
		{
			bAnyFx |= FMythSkillEffects::Play(this, SkillID, EMythSkillEffectStage::Hit, Target, Target->GetActorLocation(), false, Source);
		}
		if (!bAnyFx)
		{
			FMythSkillEffects::Play(this, SkillID, EMythSkillEffectStage::Hit, Target, Target->GetActorLocation(), true, Source); // clarao do elemento
		}
	}
	// Mesmo tratamento do ataque normal: golpe de quem usou, numeros classicos e reacao do alvo.
	// Golpes multiplos (ex.: Rajada de Flechas) aparecem como um numero por golpe, como no RO.
	if (Hits <= 1 || Damage <= 0)
	{
		HandleDamage(SourceID, TargetID, Damage, DamageType, 0);
		return;
	}
	const int32 PerHit = Damage / Hits;
	for (int32 i = 0; i < Hits; ++i)
	{
		FTimerHandle Handle;
		const int32 Amount = (i == Hits - 1) ? Damage - PerHit * (Hits - 1) : PerHit;
		GetWorldTimerManager().SetTimer(Handle, FTimerDelegate::CreateWeakLambda(this, [this, SourceID, TargetID, Amount, DamageType]()
		{
			HandleDamage(SourceID, TargetID, Amount, DamageType, 0);
		}), 0.01f + 0.15f * i, false);
	}
}

void AMythNetWorld::HandleSkillEffect(int64 SourceID, int64 TargetID, int32 SkillID, int32 Value)
{
	AActor* Source = ResolveActor(SourceID);
	AActor* Target = ResolveActor(TargetID);
	if (AMythHUD* Hud = GetGameHud(); Hud && Source)
	{
		Hud->AddSkillShout(Source, FMythRoClientTables::SkillName(SkillID));
	}
	// Efeito no conjurador (Curar: a coluna de luz em quem cura) e no alvo (Provocar, Angelus, Bencao...)
	const int32 CastFx = FMythSkillEffects::EffectIdFor(SkillID, EMythSkillEffectStage::Cast);
	if (Source && CastFx > 0)
	{
		const double Now = GetWorld()->GetTimeSeconds();
		const TPair<int64, int32> Key(SourceID, SkillID);
		if (Now - LastCasterFx.FindRef(Key) > 0.3) // o dano da mesma skill chega junto e nao repete
		{
			LastCasterFx.Add(Key, Now);
			FMythSkillEffects::Play(this, SkillID, EMythSkillEffectStage::Cast, Source, Source->GetActorLocation(), false, Source);
		}
	}
	if (AActor* At = Target ? Target : Source)
	{
		const int32 MainFx = FMythSkillEffects::EffectIdFor(SkillID, EMythSkillEffectStage::Effect);
		if (MainFx != CastFx || At != Source)
		{
			FMythSkillEffects::Play(this, SkillID, EMythSkillEffectStage::Effect, At, At->GetActorLocation(), CastFx == 0, Source);
		}
	}
	// Curas (AL_HEAL 28, curar no aprendiz NV_FIRSTAID 142): numero verde de cura sobre o alvo
	if ((SkillID == 28 || SkillID == 142) && Target && Value > 0)
	{
		UMythCombatText::ShowDamage(this, Target->GetActorLocation() + FVector(0.f, 0.f, 50.f), Value, ECombatTextType::Heal);
	}
	if (AMythPlayerCharacter* Char = Cast<AMythPlayerCharacter>(Source); Char && Char->Sprite)
	{
		Char->Sprite->PlayAttack(); // gesto de conjurar
	}
}

void AMythNetWorld::HandleSkillCast(int64 SourceID, int64 TargetID, int32 SkillID, int32 CastMs, int32 CellX, int32 CellY)
{
	AActor* Source = ResolveActor(SourceID);
	if (AMythHUD* Hud = GetGameHud(); Hud && Source && CastMs > 0)
	{
		Hud->AddCastBar(Source, CastMs / 1000.f);
	}
	if (AActor* Target = ResolveActor(TargetID); Target && Source)
	{
		if (AMythPlayerCharacter* Char = Cast<AMythPlayerCharacter>(Source); Char && Char->Sprite)
		{
			Char->Sprite->FaceLocation(Target->GetActorLocation());
		}
	}
}

void AMythNetWorld::ApplySelfAppearance()
{
	const URagnarokNetworkSubsystem* Net = GetNet();
	const APlayerController* PC = GetWorld()->GetFirstPlayerController();
	const AMythPlayerCharacter* Char = PC ? Cast<AMythPlayerCharacter>(PC->GetPawn()) : nullptr;
	if (!Net || !Char || !Char->Sprite)
	{
		return;
	}
	UMythRoSpriteComponent* S = Char->Sprite;
	const FRagnarokCharacterSlotInfo& Slot = Net->GetSelectedSlot();
	S->bMale = Net->GetSelfSex() != 0;
	S->JobSpriteName = NetJobSprite(Net->GetPlayerStats().JobClass, S->bMale);
	S->HairStyle = FMath::Max(Slot.HairStyle, 1);
	S->HairColor = Slot.HairColor;
	S->JobClass = Net->GetPlayerStats().JobClass;
	S->HeadTop = Slot.HeadTop;
	S->HeadMid = Slot.HeadMid;
	S->HeadBottom = Slot.HeadBottom;
	S->Weapon = Slot.Weapon;
	S->Shield = Slot.Shield;
	S->Robe = Slot.Robe;
	S->ReloadSprites();
}

void AMythNetWorld::HandleStatusChange(int64 UnitID, int32 EfstID, bool bActive, int32 TotalMs, int32 RemainMs)
{
	// Icones de estado so do proprio personagem, na coluna da direita (como no cliente original)
	const URagnarokNetworkSubsystem* Net = GetNet();
	if (Net && UnitID == Net->GetSelfID())
	{
		if (AMythHUD* Hud = GetGameHud())
		{
			Hud->SetStatusIcon(EfstID, bActive, TotalMs, RemainMs);
		}
	}
}

void AMythNetWorld::HandleEquipChanged(int64 UnitID, uint8 Look, int32 Value)
{
	// ZC_SPRITE_CHANGE: Look e o enum _look do rAthena (map.hpp): 0 classe, 1 cabelo, 2 arma, 3 acessorio de baixo,
	// 4 de cima, 5 do meio, 6 cor do cabelo, 8 escudo
	AActor* Unit = ResolveActor(UnitID);
	UMythRoSpriteComponent* S = Unit ? Unit->FindComponentByClass<UMythRoSpriteComponent>() : nullptr;
	if (!S || !S->MonsterSpriteName.IsEmpty())
	{
		return;
	}
	switch (Look)
	{
	case 0:
		if (const URagnarokNetworkSubsystem* Net = GetNet(); Net && UnitID == Net->GetSelfID())
		{
			S->bMale = Net->GetSelfSex() != 0; // volta ao sexo da conta antes de checar classes de um sexo so
		}
		S->JobSpriteName = NetJobSprite(Value, S->bMale);
		S->JobClass = Value;
		break;
	case 1: S->HairStyle = FMath::Max(Value, 1); break;
	case 2: S->Weapon = Value; break;
	case 3: S->HeadBottom = Value; break;
	case 4: S->HeadTop = Value; break;
	case 5: S->HeadMid = Value; break;
	case 6: S->HairColor = Value; break;
	case 8: S->Shield = Value; break;
	case 12: S->Robe = Value; break; // LOOK_ROBE
	default: return; // cor da roupa e sapatos: ainda nao desenhados
	}
	S->ReloadSprites();
}

void AMythNetWorld::HandleNotifyEffect(int64 UnitID, int32 Type)
{
	// ZC_NOTIFY_EFFECT: 0 level de base, 1 de classe, 2/3 refino falhou/deu certo, 7/8 super aprendiz, 9 taekwon.
	// Como o cliente: o anjo (angel.str + levelup.wav, efeito 371) e o joblvup.str (158) da tabela de efeitos
	AActor* Unit = ResolveActor(UnitID);
	if (!Unit)
	{
		return;
	}
	int32 EffectID = 0;
	switch (Type)
	{
	case 0: case 7: case 9: EffectID = 371; break;
	case 1: case 8: EffectID = 158; break;
	case 2: EffectID = 155; break;
	case 3: EffectID = 154; break;
	default: break;
	}
	if (EffectID > 0)
	{
		FMythSkillEffects::PlayEffectId(this, EffectID, Unit, Unit->GetActorLocation());
	}
	if (Type == 1 || Type == 8)
	{
		UMythRoSound::PlayWavAtLocation(this, TEXT("levelup.wav"), Unit->GetActorLocation());
	}
}

void AMythNetWorld::HandleSpecialEffect(int64 UnitID, int32 EffectID)
{
	if (AActor* Unit = ResolveActor(UnitID))
	{
		FMythSkillEffects::PlayEffectId(this, EffectID, Unit, Unit->GetActorLocation());
	}
}

void AMythNetWorld::HandleSkillFailed(int32 SkillID, int32 Cause)
{
	// Motivos do rAthena (e_useskill_fail): os mais comuns com a mensagem do cliente
	FString Reason;
	switch (Cause)
	{
	case 1: Reason = TEXT("SP insuficiente."); break;
	case 2: Reason = TEXT("HP insuficiente."); break;
	case 3: Reason = TEXT("Faltam itens para usar a habilidade."); break;
	case 4: Reason = TEXT("Ainda nao e possivel usar a habilidade."); break;
	case 9: Reason = TEXT("Peso acima de 90%."); break;
	default: Reason = TEXT("Falha ao usar a habilidade."); break;
	}
	const FString Name = FMythRoClientTables::SkillName(SkillID);
	Chat(Name.IsEmpty() ? Reason : FString::Printf(TEXT("%s: %s"), *Name, *Reason), NetErrorColor);
}

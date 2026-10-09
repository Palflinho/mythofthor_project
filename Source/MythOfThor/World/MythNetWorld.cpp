#include "World/MythNetWorld.h"

#include "World/MythMapInfo.h"
#include "Player/MythPlayerController.h"
#include "Player/MythPlayerCharacter.h"
#include "Gameplay/MythStatsComponent.h"
#include "Sprites/MythMonster.h"
#include "Sprites/MythRoSpriteComponent.h"
#include "Sprites/MythSkillEffects.h"
#include "World/MythGroundItem.h"
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
	Net->OnUnitMove.AddDynamic(this, &AMythNetWorld::HandleUnitMove);
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
		Net->OnUnitMove.RemoveAll(this);
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
	const bool bHit = GetWorld()->LineTraceSingleByChannel(Hit, Top, Top - FVector(0.f, 0.f, 40000.f), ECC_Visibility, FCollisionQueryParams(TEXT("MythNetCell"), false));
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
		if (FVector::Dist2D(Existing->GetActorLocation(), Ground) > 100.f)
		{
			Existing->SetActorLocation(Ground + FVector(0.f, 0.f, 45.f), false, nullptr, ETeleportType::TeleportPhysics);
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
	if (!bPlayer && !NetSpriteExists(Folder, SpriteName))
	{
		// NPC sem sprite neste cliente (warp, efeito, bandeira de guilda...): nao desenha nada
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
	if (UMythRoSpriteComponent* S = Unit->Sprite)
	{
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
			S->HairStyle = FMath::Max(Info.Head, 1);
			S->HairColor = Info.HeadPalette;
		}
		else
		{
			S->MonsterSpriteName = SpriteName;
			S->MonsterSpriteFolder = Folder;
		}
	}
	Unit->FinishSpawning(Xf);
	if (UCharacterMovementComponent* Move = Unit->GetCharacterMovement())
	{
		// Velocidade do RO: ms por celula (150 = padrao) -> cm/s com celula de 50 cm
		Move->MaxWalkSpeed = 50.f * 1000.f / FMath::Max(Info.Speed, 20);
	}
	Units.Add(Info.GID, Unit);
}

void AMythNetWorld::HandleUnitMove(int64 ID, int32 TargetX, int32 TargetY, int32 StartTick)
{
	const URagnarokNetworkSubsystem* Net = GetNet();
	if (Net && ID == Net->GetSelfID())
	{
		// O servidor tambem move o proprio jogador (ex.: ao atacar, anda ate o alvo). Segue a rota dele;
		// se ja estiver indo para la (clique no chao), nao refaz o caminho.
		APlayerController* PC = GetWorld()->GetFirstPlayerController();
		const FVector Dest = CellToGround(TargetX, TargetY);
		if (PC && PC->GetPawn() && (FVector::Dist2D(LastSelfDestination, Dest) > 60.f))
		{
			LastSelfDestination = Dest;
			UAIBlueprintHelperLibrary::SimpleMoveToLocation(PC, Dest);
		}
		return;
	}
	AMythMonster* Unit = Cast<AMythMonster>(FindUnit(ID));
	if (!Unit || Unit->IsDead())
	{
		return;
	}
	if (AController* C = Unit->GetController())
	{
		UAIBlueprintHelperLibrary::SimpleMoveToLocation(C, CellToGround(TargetX, TargetY));
	}
}

void AMythNetWorld::HandleUnitVanish(int64 ID, uint8 Reason)
{
	TObjectPtr<AMythMonster> Unit;
	if (!Units.RemoveAndCopyValue(ID, Unit) || !IsValid(Unit))
	{
		return;
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

	// Quem ataca vira para o alvo e toca o golpe
	if (AMythPlayerCharacter* Char = Cast<AMythPlayerCharacter>(Source))
	{
		if (Char->Sprite && Target)
		{
			Char->Sprite->FaceLocation(Target->GetActorLocation());
			Char->Sprite->PlayAttack();
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

	const FVector At = Target->GetActorLocation() + FVector(0.f, 0.f, 50.f);
	if (Damage <= 0 || DamageType == DmgLuckyDodge)
	{
		UMythCombatText::ShowMiss(this, At);
		return;
	}
	ECombatTextType Type = TargetID == Self ? ECombatTextType::DamageTaken : (DamageType == DmgCritical ? ECombatTextType::Critical : ECombatTextType::DamageDealt);
	UMythCombatText::ShowDamage(this, At, Damage, Type);
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
}

void AMythNetWorld::HandleGroundItemVanish(int64 ID)
{
	TObjectPtr<AMythGroundItem> Item;
	if (GroundItems.RemoveAndCopyValue(ID, Item) && IsValid(Item))
	{
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
	// Efeito visual: o da skill no alvo e o de impacto (ou o clarao do elemento), com a luz do Lumen
	if (AActor* Target = ResolveActor(TargetID))
	{
		const bool bEffect = FMythSkillEffects::Play(this, SkillID, EMythSkillEffectStage::Effect, Target, Target->GetActorLocation(), false);
		FMythSkillEffects::Play(this, SkillID, EMythSkillEffectStage::Hit, Target, Target->GetActorLocation(), !bEffect);
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
	// Efeito visual no alvo (Provocar, Angelus, Bencao...) ou no proprio usuario
	if (AActor* At = Target ? Target : Source)
	{
		FMythSkillEffects::Play(this, SkillID, EMythSkillEffectStage::Effect, At, At->GetActorLocation());
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
	default: return; // cor da roupa, sapatos, capa: ainda nao desenhados
	}
	S->ReloadSprites();
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

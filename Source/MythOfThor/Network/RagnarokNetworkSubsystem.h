#pragma once

#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "Tickable.h"
#include "Sockets.h"
#include "SocketSubsystem.h"
#include "RagnarokNetworkSubsystem.generated.h"

/**
 * Informações visuais e de atributos de um slot de personagem no Char-Server.
 */
USTRUCT(BlueprintType)
struct FRagnarokCharacterSlotInfo
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly, Category = "Ragnarok|Character")
	int32 Slot = 0;

	int32 Zeny = 0;

	UPROPERTY(BlueprintReadOnly, Category = "Ragnarok|Character")
	int64 GID = 0;

	UPROPERTY(BlueprintReadOnly, Category = "Ragnarok|Character")
	FString Name;

	UPROPERTY(BlueprintReadOnly, Category = "Ragnarok|Character")
	int32 JobClass = 0;

	UPROPERTY(BlueprintReadOnly, Category = "Ragnarok|Character")
	int32 BaseLevel = 1;

	UPROPERTY(BlueprintReadOnly, Category = "Ragnarok|Character")
	int32 JobLevel = 1;

	UPROPERTY(BlueprintReadOnly, Category = "Ragnarok|Character")
	int64 HP = 0;

	UPROPERTY(BlueprintReadOnly, Category = "Ragnarok|Character")
	int64 MaxHP = 0;

	UPROPERTY(BlueprintReadOnly, Category = "Ragnarok|Character")
	int64 SP = 0;

	UPROPERTY(BlueprintReadOnly, Category = "Ragnarok|Character")
	int64 MaxSP = 0;

	UPROPERTY(BlueprintReadOnly, Category = "Ragnarok|Character")
	int32 HairStyle = 0;

	UPROPERTY(BlueprintReadOnly, Category = "Ragnarok|Character")
	int32 HairColor = 0;

	UPROPERTY(BlueprintReadOnly, Category = "Ragnarok|Character")
	int32 ClothesColor = 0;

	UPROPERTY(BlueprintReadOnly, Category = "Ragnarok|Character")
	int32 HeadTop = 0;

	UPROPERTY(BlueprintReadOnly, Category = "Ragnarok|Character")
	int32 HeadMid = 0;

	UPROPERTY(BlueprintReadOnly, Category = "Ragnarok|Character")
	int32 HeadBottom = 0;

	UPROPERTY(BlueprintReadOnly, Category = "Ragnarok|Character")
	int32 Weapon = 0;

	UPROPERTY(BlueprintReadOnly, Category = "Ragnarok|Character")
	int32 Shield = 0;

	UPROPERTY(BlueprintReadOnly, Category = "Ragnarok|Character")
	int32 Robe = 0;

	UPROPERTY(BlueprintReadOnly, Category = "Ragnarok|Character")
	uint8 Sex = 0;
};

/**
 * Informações visuais completas de uma unidade em campo (Jogador, NPC ou Monstro).
 */
USTRUCT(BlueprintType)
struct FRagnarokUnitVisualInfo
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly, Category = "Ragnarok|Unit")
	int64 GID = 0;

	UPROPERTY(BlueprintReadOnly, Category = "Ragnarok|Unit")
	uint8 ObjectType = 0; // 0=PC, 1=NPC, 2=Item, 5=Monster

	UPROPERTY(BlueprintReadOnly, Category = "Ragnarok|Unit")
	int32 JobClass = 0;

	UPROPERTY(BlueprintReadOnly, Category = "Ragnarok|Unit")
	int32 CellX = 0;

	UPROPERTY(BlueprintReadOnly, Category = "Ragnarok|Unit")
	int32 CellY = 0;

	UPROPERTY(BlueprintReadOnly, Category = "Ragnarok|Unit")
	uint8 Direction = 0;

	/** Direcao da cabeca (0 frente, 1 CW, 2 CCW) e se Direction veio no pacote (o de andando nao traz). */
	UPROPERTY(BlueprintReadOnly, Category = "Ragnarok|Unit")
	uint8 HeadDir = 0;

	UPROPERTY(BlueprintReadOnly, Category = "Ragnarok|Unit")
	bool bDirectionKnown = false;

	/** effectState do pacote (OPTION_* do rAthena). */
	UPROPERTY(BlueprintReadOnly, Category = "Ragnarok|Unit")
	int32 Option = 0;

	/** opt1 (pedra, gelo, atordoar, sono) e opt2 (veneno, maldicao, silencio...) do pacote. */
	UPROPERTY(BlueprintReadOnly, Category = "Ragnarok|Unit")
	int32 BodyState = 0;

	UPROPERTY(BlueprintReadOnly, Category = "Ragnarok|Unit")
	int32 HealthState = 0;

	/** clevel do pacote (nivel de base da unidade; 0 se o pacote nao traz). */
	UPROPERTY(BlueprintReadOnly, Category = "Ragnarok|Unit")
	int32 BaseLevel = 0;

	UPROPERTY(BlueprintReadOnly, Category = "Ragnarok|Unit")
	int32 Speed = 150;

	UPROPERTY(BlueprintReadOnly, Category = "Ragnarok|Unit")
	int32 Head = 0;

	UPROPERTY(BlueprintReadOnly, Category = "Ragnarok|Unit")
	int32 HeadTop = 0;

	UPROPERTY(BlueprintReadOnly, Category = "Ragnarok|Unit")
	int32 HeadMid = 0;

	UPROPERTY(BlueprintReadOnly, Category = "Ragnarok|Unit")
	int32 HeadBottom = 0;

	UPROPERTY(BlueprintReadOnly, Category = "Ragnarok|Unit")
	int32 Weapon = 0;

	UPROPERTY(BlueprintReadOnly, Category = "Ragnarok|Unit")
	int32 Shield = 0;

	UPROPERTY(BlueprintReadOnly, Category = "Ragnarok|Unit")
	int32 Robe = 0;

	UPROPERTY(BlueprintReadOnly, Category = "Ragnarok|Unit")
	int32 HeadPalette = 0;

	UPROPERTY(BlueprintReadOnly, Category = "Ragnarok|Unit")
	int32 BodyPalette = 0;

	/** 0 = feminino, 1 = masculino (byte "sex" antes de PosDir/MoveData). */
	UPROPERTY(BlueprintReadOnly, Category = "Ragnarok|Unit")
	uint8 Sex = 1;

	UPROPERTY(BlueprintReadOnly, Category = "Ragnarok|Unit")
	int32 HP = 0;

	UPROPERTY(BlueprintReadOnly, Category = "Ragnarok|Unit")
	int32 MaxHP = 0;

	UPROPERTY(BlueprintReadOnly, Category = "Ragnarok|Unit")
	FString Name;
};

/**
 * Status completos do jogador sincronizados do rAthena.
 */
USTRUCT(BlueprintType)
struct FRagnarokPlayerStats
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly, Category = "Ragnarok|Status") int32 Str = 1;
	UPROPERTY(BlueprintReadOnly, Category = "Ragnarok|Status") int32 Agi = 1;
	UPROPERTY(BlueprintReadOnly, Category = "Ragnarok|Status") int32 Vit = 1;
	UPROPERTY(BlueprintReadOnly, Category = "Ragnarok|Status") int32 Int = 1;
	UPROPERTY(BlueprintReadOnly, Category = "Ragnarok|Status") int32 Dex = 1;
	UPROPERTY(BlueprintReadOnly, Category = "Ragnarok|Status") int32 Luk = 1;
	/** Bonus (+) de cada atributo vindo do ZC_COUPLESTATUS 0x0141 (equipamento, buffs); ordem Str, Agi, Vit, Int, Dex, Luk. */
	int32 StatBonus[6] = { 0, 0, 0, 0, 0, 0 };

	// Custos de ponto para aumentar cada atributo (pelo ZC_STATUS need_str ou SP_U*)
	UPROPERTY(BlueprintReadOnly, Category = "Ragnarok|Status") int32 StrNeed = 2;
	UPROPERTY(BlueprintReadOnly, Category = "Ragnarok|Status") int32 AgiNeed = 2;
	UPROPERTY(BlueprintReadOnly, Category = "Ragnarok|Status") int32 VitNeed = 2;
	UPROPERTY(BlueprintReadOnly, Category = "Ragnarok|Status") int32 IntNeed = 2;
	UPROPERTY(BlueprintReadOnly, Category = "Ragnarok|Status") int32 DexNeed = 2;
	UPROPERTY(BlueprintReadOnly, Category = "Ragnarok|Status") int32 LukNeed = 2;

	UPROPERTY(BlueprintReadOnly, Category = "Ragnarok|Status") int32 StatusPoints = 0;
	UPROPERTY(BlueprintReadOnly, Category = "Ragnarok|Status") int32 SkillPoints = 0;

	UPROPERTY(BlueprintReadOnly, Category = "Ragnarok|Status") int32 Atk = 0;
	UPROPERTY(BlueprintReadOnly, Category = "Ragnarok|Status") int32 Atk2 = 0;
	UPROPERTY(BlueprintReadOnly, Category = "Ragnarok|Status") int32 MatkMin = 0;
	UPROPERTY(BlueprintReadOnly, Category = "Ragnarok|Status") int32 MatkMax = 0;
	UPROPERTY(BlueprintReadOnly, Category = "Ragnarok|Status") int32 Def = 0;
	UPROPERTY(BlueprintReadOnly, Category = "Ragnarok|Status") int32 Def2 = 0;
	UPROPERTY(BlueprintReadOnly, Category = "Ragnarok|Status") int32 Mdef = 0;
	UPROPERTY(BlueprintReadOnly, Category = "Ragnarok|Status") int32 Mdef2 = 0;
	UPROPERTY(BlueprintReadOnly, Category = "Ragnarok|Status") int32 Hit = 0;
	UPROPERTY(BlueprintReadOnly, Category = "Ragnarok|Status") int32 Flee = 0;
	UPROPERTY(BlueprintReadOnly, Category = "Ragnarok|Status") int32 Flee2 = 0;
	UPROPERTY(BlueprintReadOnly, Category = "Ragnarok|Status") int32 Critical = 0;
	UPROPERTY(BlueprintReadOnly, Category = "Ragnarok|Status") int32 Aspd = 0;

	UPROPERTY(BlueprintReadOnly, Category = "Ragnarok|Status") int32 BaseLevel = 1;
	UPROPERTY(BlueprintReadOnly, Category = "Ragnarok|Status") int32 JobLevel = 1;
	UPROPERTY(BlueprintReadOnly, Category = "Ragnarok|Status") int32 JobClass = 0;
	UPROPERTY(BlueprintReadOnly, Category = "Ragnarok|Status") FString JobName = TEXT("Aprendiz");

	UPROPERTY(BlueprintReadOnly, Category = "Ragnarok|Status") int64 BaseExp = 0;
	UPROPERTY(BlueprintReadOnly, Category = "Ragnarok|Status") int64 NextBaseExp = 0;
	UPROPERTY(BlueprintReadOnly, Category = "Ragnarok|Status") int64 JobExp = 0;
	UPROPERTY(BlueprintReadOnly, Category = "Ragnarok|Status") int64 NextJobExp = 0;

	UPROPERTY(BlueprintReadOnly, Category = "Ragnarok|Status") int32 Weight = 0;
	UPROPERTY(BlueprintReadOnly, Category = "Ragnarok|Status") int32 MaxWeight = 0;
	UPROPERTY(BlueprintReadOnly, Category = "Ragnarok|Status") int64 Zeny = 0;

	UPROPERTY(BlueprintReadOnly, Category = "Ragnarok|Status") int64 HP = 0;
	UPROPERTY(BlueprintReadOnly, Category = "Ragnarok|Status") int64 MaxHP = 0;
	UPROPERTY(BlueprintReadOnly, Category = "Ragnarok|Status") int64 SP = 0;
	UPROPERTY(BlueprintReadOnly, Category = "Ragnarok|Status") int64 MaxSP = 0;
};

/**
 * Item de inventário ou equipamento.
 */
USTRUCT(BlueprintType)
struct FRagnarokInventoryItem
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly, Category = "Ragnarok|Inventory") int32 Index = 0;
	UPROPERTY(BlueprintReadOnly, Category = "Ragnarok|Inventory") int32 ItemID = 0;
	UPROPERTY(BlueprintReadOnly, Category = "Ragnarok|Inventory") int32 Type = 0;
	UPROPERTY(BlueprintReadOnly, Category = "Ragnarok|Inventory") int32 Amount = 0;
	UPROPERTY(BlueprintReadOnly, Category = "Ragnarok|Inventory") int32 EquipLocation = 0;
	UPROPERTY(BlueprintReadOnly, Category = "Ragnarok|Inventory") int32 WearState = 0;
	UPROPERTY(BlueprintReadOnly, Category = "Ragnarok|Inventory") int32 Location = 0;
	UPROPERTY(BlueprintReadOnly, Category = "Ragnarok|Inventory") int32 Refine = 0;
	UPROPERTY(BlueprintReadOnly, Category = "Ragnarok|Inventory") bool bIdentified = true;
	UPROPERTY(BlueprintReadOnly, Category = "Ragnarok|Inventory") bool bIsEquipped = false;
	UPROPERTY(BlueprintReadOnly, Category = "Ragnarok|Inventory") bool bFavorite = false;
	UPROPERTY(BlueprintReadOnly, Category = "Ragnarok|Inventory") int32 Slot1 = 0;
	UPROPERTY(BlueprintReadOnly, Category = "Ragnarok|Inventory") int32 Slot2 = 0;
	UPROPERTY(BlueprintReadOnly, Category = "Ragnarok|Inventory") int32 Slot3 = 0;
	UPROPERTY(BlueprintReadOnly, Category = "Ragnarok|Inventory") int32 Slot4 = 0;
};

/** Item de loja de NPC: compra (ItemID, Price, DiscountPrice) ou venda (Index do inventario, Price, DiscountPrice = overcharge). */
USTRUCT(BlueprintType)
struct FRagnarokShopItem
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly, Category = "Ragnarok|Shop") int32 ItemID = 0;
	UPROPERTY(BlueprintReadOnly, Category = "Ragnarok|Shop") int32 Index = 0;
	UPROPERTY(BlueprintReadOnly, Category = "Ragnarok|Shop") int32 Price = 0;
	UPROPERTY(BlueprintReadOnly, Category = "Ragnarok|Shop") int32 DiscountPrice = 0;
	UPROPERTY(BlueprintReadOnly, Category = "Ragnarok|Shop") int32 ItemType = 0;
};

/**
 * Informação de habilidade na lista de skills do jogador.
 */
USTRUCT(BlueprintType)
struct FRagnarokSkillInfo
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly, Category = "Ragnarok|Skill") int32 SkillID = 0;
	UPROPERTY(BlueprintReadOnly, Category = "Ragnarok|Skill") int32 Level = 0;
	UPROPERTY(BlueprintReadOnly, Category = "Ragnarok|Skill") int32 SpCost = 0;
	UPROPERTY(BlueprintReadOnly, Category = "Ragnarok|Skill") int32 Range = 0;
	UPROPERTY(BlueprintReadOnly, Category = "Ragnarok|Skill") bool bUpgradable = false;
	/** Tipo de alvo (e_skill_inf do rAthena): 0 passiva, 0x01 inimigo, 0x02 chao, 0x04 em si, 0x10 suporte, 0x20 armadilha. */
	UPROPERTY(BlueprintReadOnly, Category = "Ragnarok|Skill") int32 Inf = 0;
	UPROPERTY(BlueprintReadOnly, Category = "Ragnarok|Skill") FString SkillName;
};

/**
 * Atalho da barra (F1..F9), guardado no servidor (ZC_SHORTCUT_KEY_LIST / CZ_SHORTCUT_KEY_CHANGE2).
 */
USTRUCT(BlueprintType)
struct FRagnarokHotkey
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly, Category = "Ragnarok|Hotkey") bool bIsSkill = false;
	/** ID do item ou da skill (0 = vazio). */
	UPROPERTY(BlueprintReadOnly, Category = "Ragnarok|Hotkey") int32 ID = 0;
	/** Quantidade do item ou nivel da skill. */
	UPROPERTY(BlueprintReadOnly, Category = "Ragnarok|Hotkey") int32 Count = 0;
};

// ============================================================================
// Delegates do Contrato Cliente Fiel (docs/PLANO_CLIENTE_FIEL.md - Seção 3)
// ============================================================================
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnNetworkConnected, const FString&, ServerName);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnNetworkDisconnected, const FString&, Reason);

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnLoginResult, bool, bSuccess, const FString&, Message);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnCharacterList, const TArray<FRagnarokCharacterSlotInfo>&, CharacterList);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnCharCreateResult, bool, bSuccess, const FString&, Message);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_FourParams(FOnMapEnter, const FString&, MapName, int32, CellX, int32, CellY, uint8, Direction);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnUnitAppear, const FRagnarokUnitVisualInfo&, UnitInfo);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_FourParams(FOnUnitMove, int64, GID, int32, TargetX, int32, TargetY, int32, StartTick);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnUnitVanish, int64, GID, uint8, VanishReason);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_FiveParams(FOnDamage, int64, SourceID, int64, TargetID, int32, Damage, uint8, DamageType, int32, DelayMs);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnStatusChanged, int32, ParamType, int32, Value);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnPlayerStatsChanged, const FRagnarokPlayerStats&, Stats);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_ThreeParams(FOnEquipChanged, int64, GID, uint8, Slot, int32, VisualID);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_ThreeParams(FOnChatMessage, const FString&, Sender, const FString&, Text, const FString&, ColorHex);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnInventory, const TArray<FRagnarokInventoryItem>&, Items);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnSkillList, const TArray<FRagnarokSkillInfo>&, Skills);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_FiveParams(FOnGroundItemAppear, int64, GID, int32, ItemID, int32, Amount, int32, CellX, int32, CellY);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnGroundItemVanish, int64, GID);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnNpcDialog, int64, NpcID, const FString&, Text);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_ThreeParams(FOnNpcMenu, int64, NpcID, const FString&, Title, const TArray<FString>&, Options);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnWeather, const FString&, WeatherType, bool, bEnabled);

// Skills (Claude): resultado e conjuracao vindos do servidor
DECLARE_DYNAMIC_MULTICAST_DELEGATE_SixParams(FOnSkillDamage, int64, SourceID, int64, TargetID, int32, SkillID, int32, Damage, int32, Hits, uint8, DamageType);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_FourParams(FOnSkillEffect, int64, SourceID, int64, TargetID, int32, SkillID, int32, Value);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_SixParams(FOnSkillCast, int64, SourceID, int64, TargetID, int32, SkillID, int32, CastMs, int32, CellX, int32, CellY);
/** ZC_NOTIFY_GROUNDSKILL 0x0117: skill de area usada em uma celula (com ou sem alvos). */
DECLARE_DYNAMIC_MULTICAST_DELEGATE_FiveParams(FOnSkillGround, int64, SourceID, int32, SkillID, int32, Level, int32, CellX, int32, CellY);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnSkillFailed, int32, SkillID, int32, Cause);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnSpecialEffect, int64, UnitID, int32, EffectID);

// Buffs / Estados (Tarefa A1)
DECLARE_DYNAMIC_MULTICAST_DELEGATE_FiveParams(FOnStatusChange, int64, UnitID, int32, EfstID, bool, bActive, int32, TotalMs, int32, RemainMs);

// Skills de chão (Tarefa A2)
DECLARE_DYNAMIC_MULTICAST_DELEGATE_FiveParams(FOnSkillUnitAppear, int64, UnitID, int64, OwnerID, int32, UnitType, int32, X, int32, Y);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnSkillUnitDisappear, int64, UnitID);

// Efeitos gerais do servidor (Tarefa A3)
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnNotifyEffect, int64, UnitID, int32, Type);

// Equipar / Desequipar (Tarefa A4)
DECLARE_DYNAMIC_MULTICAST_DELEGATE_ThreeParams(FOnEquipResult, int32, Index, int32, WearLocation, bool, bSuccess);

// Ataque fora de alcance (Tarefa A5)
DECLARE_DYNAMIC_MULTICAST_DELEGATE_FourParams(FOnAttackOutOfRange, int64, TargetID, int32, TargetX, int32, TargetY, int32, Range);

// Posição de partida e parada (Tarefa A6)
DECLARE_DYNAMIC_MULTICAST_DELEGATE_SixParams(FOnUnitWalk, int64, UnitID, int32, FromX, int32, FromY, int32, ToX, int32, ToY, int32, StartTick);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_ThreeParams(FOnUnitStop, int64, UnitID, int32, X, int32, Y);
/** ZC_CHANGE_DIRECTION (0x009c): corpo e cabeca de uma unidade viraram (dir do rAthena: 0 norte ... 7 nordeste; cabeca 0 frente, 1 CW, 2 CCW). */
DECLARE_DYNAMIC_MULTICAST_DELEGATE_ThreeParams(FOnUnitDirection, int64, UnitID, uint8, HeadDir, uint8, BodyDir);
/** ZC_STATE_CHANGE (0x0229 / 0x0119): opcoes de uma unidade (OPTION_CART1..5 0x08/0x80/0x100/0x200/0x400, riding 0x20, falcon...). */
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnUnitOption, int64, UnitID, int32, Option);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_ThreeParams(FOnUnitBodyState, int64, UnitID, int32, BodyState, int32, HealthState);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnSpiritBalls, int64, UnitID, int32, Count);
/** ZC_RESURRECTION (0x0148): a unidade voltou a vida. */
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnUnitRevive, int64, UnitID);

/** Carta da caixa de entrada do correio (0x0ac2). */
struct FRoMail
{
	int64 Id = 0;
	uint8 Tab = 0;
	bool bRead = false;
	uint8 Kind = 0;
	FString Sender;
	FString Title;
};

/** Conquista (0x0a23, 0x0a24): progresso por objetivo, concluida e premio recebido; o servidor decide tudo. */
struct FRoAchievement
{
	int32 Id = 0;
	bool bCompleted = false;
	int32 Objective[10] = {};
	int32 CompletedAt = 0;
	bool bRewardTaken = false;
};

/** Objetivo de uma quest (cacar monstros): contagem atual e total vindos do servidor. */
struct FRoQuestObjective
{
	int32 MobId = 0;
	int32 Count = 0;
	int32 Total = 0;
	FString Name;
};

/** Quest do registro (0x09f8, 0x09f9): estado e objetivos; o servidor decide tudo. */
struct FRoQuest
{
	int32 Id = 0;
	bool bActive = true;
	TArray<FRoQuestObjective> Objectives;
};

/** Sala de chat anunciada sobre a cabeca de quem a abriu (0x00d7). */
struct FRoChatRoom
{
	int32 Id = 0;
	int32 OwnerId = 0;
	int32 Limit = 0;
	int32 Users = 0;
	int32 Type = 1; // 0 privada, 1 publica, 2 arena, 3 zona PK
	FString Title;
};

/** Amigo da lista (0x0201): o estado online vem do 0x0206. */
struct FRoFriend
{
	int32 AccountId = 0;
	int32 CharId = 0;
	FString Name;
	bool bOnline = false;
};

/** Item (ou zeny, quando ItemId = 0) de um dos lados da troca entre jogadores. */
struct FRoTradeItem
{
	int32 Index = 0;
	int32 ItemId = 0;
	int32 Amount = 0;
};

/** Membro do grupo como o servidor informa (0x0ae5 lista, 0x0ae4 entrada, 0x080e PV, 0x0107 posicao, 0x0ab2 morte). */
struct FRoPartyMember
{
	int32 AccountId = 0;
	FString Name;
	FString Map;
	bool bLeader = false;
	bool bOffline = false;
	bool bDead = false;
	int32 ClassId = 0;
	int32 Level = 0;
	int32 Hp = 0;
	int32 MaxHp = 0;
	int32 X = 0;
	int32 Y = 0;
};

/** Fala publica (0x008d de outros, 0x008e propria) para o balao sobre a cabeca, e emocao (0x00c0). */
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnSpeech, int64, UnitID, const FString&, Text);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnEmotion, int64, UnitID, int32, EmotionType);
/** ZC_HP_INFO 0x0977: PV do monstro que voce atingiu (barra sob os pes). */
DECLARE_DYNAMIC_MULTICAST_DELEGATE_ThreeParams(FOnMonsterHp, int64, UnitID, int32, Hp, int32, MaxHp);

/** Armazem da Kafra (clif.cpp): lista completa (0x0b08 inicio, 0x0b09/0x0b39 invType 2, 0x0b0b fim), 0x0b44 item entrou, 0x00f6 saiu, 0x00f2 contagem, 0x00f8 fechou. */
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnStorageOpen, const TArray<FRagnarokInventoryItem>&, Items);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnStorageChanged, const TArray<FRagnarokInventoryItem>&, Items);
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnStorageClose);
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnCartChanged);
/**
 * Listas de escolha que o servidor abre depois de uma skill ou item (EMythSelectList): destinos do Portal, itens para
 * identificar, equipamentos para a carta, flechas, itens para fabricar, magias do Autoconjurar e armas para refinar.
 * Values: indice na mochila, ID de item ou de skill, conforme o tipo; Names: so os mapas do Portal.
 */
DECLARE_DYNAMIC_MULTICAST_DELEGATE_ThreeParams(FOnSelectList, uint8, ListType, const TArray<int32>&, Values, const TArray<FString>&, Names);

/** Mascote (ZC_PROPERTY_PET 0x01a2 e ZC_CHANGESTATE_PET 0x01a4). Fome 0..100, lealdade 0..1000. */
struct FRoPetInfo
{
	FString Name;
	bool bRenamed = false;
	int32 Level = 0;
	int32 Hunger = 0;
	int32 Intimacy = 0;
	int32 Accessory = 0;
	int32 ClassId = 0;
};

/** Mascote: 0 comecou a captura (escolher o monstro), 1 dados mudaram, 2 voltou para o ovo. */
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnPet, uint8, Event);

/** Cla (guilda): dados do ZC_GUILD_INFO 0x0b7b. */
struct FRoGuildInfo
{
	int32 Level = 0;
	int32 Online = 0;
	int32 MaxMembers = 0;
	int32 AvgLevel = 0;
	int64 Exp = 0;
	int64 MaxExp = 0;
	int32 MasterCharId = 0;
	FString Name;
	FString Master;
	FString Land;
};

/** Membro do cla (ZC_MEMBERMGR_INFO 0x0b7d, 58 bytes cada). */
struct FRoGuildMember
{
	int32 AccountId = 0;
	int32 CharId = 0;
	int32 ClassId = 0;
	int32 Level = 0;
	int32 Exp = 0;
	int32 Position = 0;
	int32 LastLogin = 0;
	bool bOnline = false;
	FString Name;
};

/** Cla: 0 dados mudaram, 1 convite recebido (GetGuildInviteId/Name), 2 voce saiu, foi expulso ou o cla acabou. */
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnGuild, uint8, Event);

/** Loja de jogador (Vending): 0 abrir a loja (ZC_OPENSTORE), 1 lista de outra loja chegou, 2 a minha loja mudou, 3 a minha loja fechou. */
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnVending, uint8, Event);

/** Item de uma loja de jogador (a minha ou a que estou olhando). */
struct FRoVendItem
{
	int32 Index = 0;    // na loja de outro: indice no carrinho dele; na minha: indice no meu carrinho
	int32 ItemID = 0;
	int32 Amount = 0;
	int32 Price = 0;
	int32 Type = 0;
	int32 Refine = 0;
	bool bIdentified = true;
	int32 Slots[4] = { 0, 0, 0, 0 };
};

/** Tipos de lista de escolha (FOnSelectList). */
enum class EMythSelectList : uint8
{
	Warp,       // ZC_WARPLIST 0x0abe -> CZ_SELECT_WARPPOINT 0x011b
	Identify,   // ZC_ITEMIDENTIFY_LIST 0x0177 -> CZ_REQ_ITEMIDENTIFY 0x0178
	Card,       // ZC_ITEMCOMPOSITION_LIST 0x017b -> CZ_REQ_ITEMCOMPOSITION 0x017c
	Arrow,      // ZC_MAKINGARROW_LIST 0x01ad -> CZ_REQ_MAKINGARROW 0x01ae
	Produce,    // ZC_MAKABLEITEMLIST 0x018d -> CZ_REQMAKINGITEM 0x018e
	Autospell,  // ZC_AUTOSPELLLIST 0x0afb -> CZ_SELECTAUTOSPELL 0x01ce
	Refine,     // ZC_NOTIFY_WEAPONITEMLIST 0x0221 -> CZ_REQ_WEAPONREFINE 0x0222
	PetEgg,     // ZC_PETEGG_LIST 0x01a6 -> CZ_SELECT_PETEGG 0x01a7 (Incubadora)
	Repair      // ZC_REPAIRITEMLIST 0x0b65 -> CZ_REQ_ITEMREPAIR2 0x0b66 (Reparar arma; Values = ID do item)
};

/** Lojas de NPC (clif.cpp): 0x00c4 escolher comprar/vender, 0x0b77 lista de compra, 0x00c7 lista de venda, 0x00ca/0x00cb resultado. */
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnShopDealType, int64, NpcID);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnShopBuyList, const TArray<FRagnarokShopItem>&, Items);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnShopSellList, const TArray<FRagnarokShopItem>&, Items);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnShopResult, bool, bSell, int32, Result);

/** Grupo (clif.cpp party): a lista mudou (membros, PV, posicao, lider) ou o convite chegou / o resultado do convite. */
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnPartyChanged);
/** Troca (clif.cpp trade): pedido de outro jogador e mudancas na janela (itens, travas, fim). */
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnTradeRequest, const FString&, FromName);
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnTradeChanged);
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnFriendsChanged);
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnQuestsChanged);
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnBankChanged);
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnAchievementsChanged);
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnEquipSwitchChanged);
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnMailChanged);
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnChatRoomChanged);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_ThreeParams(FOnFriendRequest, int32, AccountId, int32, CharId, const FString&, Name);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnPartyInvite, int32, PartyId, const FString&, PartyName);

// Delegates de compatibilidade anterior
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnPlayerMoveConfirmed, int32, TargetX, int32, TargetY);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_FourParams(FOnDamageReceived, int64, TargetID, int32, Damage, bool, bIsCritical, bool, bIsMiss);

/**
 * Estados do ciclo de conexão cliente rAthena.
 */
UENUM(BlueprintType)
enum class ERagnarokNetworkState : uint8
{
	Disconnected,
	ConnectingLogin,
	ConnectedLogin,
	ConnectingChar,
	ConnectedChar,
	ConnectingMap,
	ConnectedMap
};

/**
 * Subsistema de Rede nativo em C++ para comunicação TCP não bloqueante
 * entre o cliente Unreal Engine 5 e os servidores do rAthena (Login, Char e Map servers).
 * Implementa FTickableGameObject para processamento contínuo de pacotes por frame.
 */
UCLASS()
class MYTHOFTHOR_API URagnarokNetworkSubsystem : public UGameInstanceSubsystem, public FTickableGameObject
{
	GENERATED_BODY()

public:
	virtual void Initialize(FSubsystemCollectionBase& Collection) override;
	virtual void Deinitialize() override;

	// FTickableGameObject interface
	virtual void Tick(float DeltaTime) override;
	virtual bool IsTickable() const override { return !IsTemplate(); }
	virtual TStatId GetStatId() const override { RETURN_QUICK_DECLARE_CYCLE_STAT(URagnarokNetworkSubsystem, STATGROUP_Tickables); }

	// Portas e Constantes Canônicas do Servidor
	static constexpr int32 LOGIN_SERVER_PORT = 6900;
	static constexpr int32 CHAR_SERVER_PORT  = 6121;
	static constexpr int32 MAP_SERVER_PORT   = 5121;
	static constexpr uint32 CLIENT_PACKETVER = 20250402;

	/**
	 * Conecta ao Login-Server do rAthena e autentica a conta (CA_LOGIN 0x0064).
	 */
	UFUNCTION(BlueprintCallable, Category = "Ragnarok|Network")
	bool ConnectAndLogin(const FString& ServerHost, const FString& Username, const FString& Password);
	/** Login da sessao (so na memoria): usado pelo menu Esc "Selecao de personagem" para entrar de novo sem pedir a senha. */
	const FString& GetSessionUser() const { return SessionUser; }
	const FString& GetSessionPass() const { return SessionPass; }

	/**
	 * Seleciona um personagem na lista do Char-Server para entrar no mundo (CH_SELECT_CHAR 0x0066).
	 */
	UFUNCTION(BlueprintCallable, Category = "Ragnarok|Network")
	bool SelectCharacter(int32 SlotIndex);

	/**
	 * Envia requisição de movimento para o Map-Server (CZ_REQUEST_MOVE 0x035F).
	 * Suporta coordenadas do mouse (Point & Click) ou geradas pelo WASD.
	 */
	UFUNCTION(BlueprintCallable, Category = "Ragnarok|Network")
	bool SendMoveRequest(int32 TargetX, int32 TargetY);

	/** Vira o personagem sem andar (Shift+clique, sentado): CZ_CHANGE_DIRECTION. Dir do rAthena: 0 norte ... 7 nordeste. */
	bool SendChangeDirection(uint8 HeadDir, uint8 BodyDir);

	/** CZ_RESTART (0x00b2): 0 = voltar ao ponto de salvamento (reviver), 1 = tela de selecao de personagem. */
	bool SendRestart(uint8 Type);
	/** CZ_REQ_CARTOFF (0x012a): tira falcao, Peco Peco, dragao ou Madogear (o carrinho so se nao houver os outros). */
	bool SendRemoveOption();
	/** Tira todos os equipamentos dos encaixes de Location (mascara EQP_*): CZ_REQ_TAKEOFF_EQUIP_ALL 0x0bf5. */
	bool SendTakeOffAll(uint32 Location = 0xFFFFFFFF);
	/** "Mostrar equipamento" (outros jogadores podem abrir sua janela de equipamento): CZ_CONFIG tipo 0. */
	bool SendShowEquipConfig(bool bShow);
	/** Estado de "mostrar equipamento" que o servidor guardou (ZC_CONFIG_NOTIFY / ZC_CONFIG). */
	bool IsShowEquip() const { return bShowEquip; }
	/** Ultimo estado (OPTION_*) do proprio jogador, vindo do servidor. */
	int32 GetSelfOption() const { return SelfOption; }
	/** Posicao dentro da celula (subX, subY, 0..15) do item no chao, do ZC_ITEM_FALL_ENTRY; falso se o servidor nao mandou. */
	bool GetGroundItemSub(int64 GID, FIntPoint& Out) const { if (const FIntPoint* P = GroundItemSubs.Find(GID)) { Out = *P; return true; } return false; }

	/** Armazem: mover item da mochila (bToStorage) ou do armazem para a mochila; fechar. IDs da tabela embaralhada (0x0364, 0x0365) e 0x0193. */
	bool SendStorageMove(bool bToStorage, int32 Index, int32 Amount);
	bool SendStorageClose();
	/** Resposta a uma lista de escolha (EMythSelectList). Value: o valor escolhido (Values da lista); Name: mapa do Portal.
	 * bCancel: fecha sem escolher (Portal manda "cancel", Identificar manda -1; as outras so fecham). */
	bool SendSelectListChoice(EMythSelectList Type, int32 Value, const FString& Name, bool bCancel);
	/** Loja de jogador (RagnarokNetworkVending.cpp). Abrir: titulo e itens do carrinho {indice, quantidade, preco}; bOpen falso cancela. */
	bool SendOpenVending(const FString& Title, const TArray<FIntVector>& Items, bool bOpen);
	bool SendVendingListReq(int32 OwnerAID);
	/** Comprar da loja aberta: {indice na loja, quantidade}. */
	bool SendVendingBuy(const TArray<FIntPoint>& Items);
	bool SendCloseVending();
	UPROPERTY(BlueprintAssignable, Category = "Ragnarok|Vending") FOnVending OnVending;
	const TMap<int32, FString>& GetVendingBoards() const { return VendingBoards; }
	int32 GetVendMaxItems() const { return VendMaxItems; }
	const TArray<FRoVendItem>& GetVendBuyItems() const { return VendBuyItems; }
	int32 GetVendBuyOwner() const { return VendBuyAID; }
	const TArray<FRoVendItem>& GetMyVendItems() const { return MyVendItems; }
	bool IsVending() const { return bVending; }
	/** Mascote (RagnarokNetworkPet.cpp). Menu: 0 informacoes, 1 alimentar, 2 brincar, 3 voltar para o ovo, 4 desequipar acessorio. */
	bool SendPetMenu(int32 Command);
	bool SendPetRename(const FString& Name);
	/** Captura: o monstro escolhido depois do ZC_START_CAPTURE (0 desiste). */
	bool SendCatchPet(int32 TargetId);
	/** CZ_CONFIG 0x02d8: 2 = autoalimentar o mascote. */
	bool SendConfig(int32 Type, bool bOn);
	UPROPERTY(BlueprintAssignable, Category = "Ragnarok|Pet") FOnPet OnPet;
	bool HasPet() const { return PetGID != 0; }
	int64 GetPetId() const { return PetGID; }
	const FRoPetInfo& GetPetInfo() const { return PetInfo; }
	bool IsCapturing() const { return bCapturing; }
	bool IsPetAutoFeed() const { return bPetAutoFeed; }
	/** Cla (RagnarokNetworkGuild.cpp). CZ_REQ_GUILD_MENU: 0 informacoes, 1 membros, 2 cargos, 3 habilidades, 4 expulsos. */
	bool SendGuildMenuRequest(int32 Type);
	bool SendCreateGuild(const FString& Name);
	bool SendGuildInvite(const FString& CharName);
	bool SendGuildInviteReply(int32 InviteGuildId, bool bAccept);
	bool SendLeaveGuild(const FString& Reason);
	bool SendGuildExpel(int32 AccountId, int32 CharId, const FString& Reason);
	bool SendGuildNotice(const FString& Subject, const FString& Notice);
	bool SendBreakGuild(const FString& Name);
	UPROPERTY(BlueprintAssignable, Category = "Ragnarok|Guild") FOnGuild OnGuild;
	bool IsInGuild() const { return GuildId != 0; }
	bool IsGuildMaster() const { return bGuildMaster; }
	int32 GetGuildId() const { return GuildId; }
	const FString& GetGuildName() const { return GuildName; }
	const FRoGuildInfo& GetGuildInfo() const { return GuildInfo; }
	const TArray<FRoGuildMember>& GetGuildMembers() const { return GuildMembers; }
	FString GetGuildPositionName(int32 Position) const { const FString* P = GuildPositions.Find(Position); return P ? *P : FString(); }
	const FString& GetGuildNoticeSubject() const { return GuildNoticeSubject; }
	const FString& GetGuildNoticeText() const { return GuildNoticeText; }
	int32 GetGuildInviteId() const { return GuildInviteId; }
	const FString& GetGuildInviteName() const { return GuildInviteName; }
	/** Usar uma carta: pede a lista de equipamentos que a aceitam (CZ_REQ_ITEMCOMPOSITION_LIST 0x017a). */
	bool SendCardListRequest(int32 CardIndex);
	UPROPERTY(BlueprintAssignable, Category = "Ragnarok|Skill") FOnSelectList OnSelectList;
	/** Carrinho do Mercador: mochila -> carrinho (CZ_MOVE_ITEM_FROM_BODY_TO_CART 0x0126) ou carrinho -> mochila (0x0127). */
	bool SendCartMove(bool bToCart, int32 Index, int32 Amount);
	const TArray<FRagnarokInventoryItem>& GetCartItems() const { return CartItems; }
	int32 GetCartCount() const { return CartCount; }
	int32 GetCartMax() const { return CartMax; }
	int32 GetCartWeight() const { return CartWeight; }
	int32 GetCartMaxWeight() const { return CartMaxWeight; }
	/** Com carrinho (OPTION_CART*): o servidor mandou ZC_NOTIFY_CARTITEM_COUNTINFO e ainda nao tirou (ZC_CARTOFF). */
	bool HasCart() const { return bHasCart; }

	/** CZ_ITEM_THROW / DropItem (0x0363 neste PACKETVER: tabela embaralhada de clif_shuffle.hpp, bloco PACKETVER > 20180307): joga o item do indice no chao. */
	bool SendDropItem(int32 Index, int32 Amount);

	/** Loja de NPC: escolher comprar (0) ou vender (1) / comprar / vender / fechar. IDs e layouts conferidos no clif.cpp do rAthena. */
	bool SendShopDealType(int64 NpcID, uint8 Type);
	bool SendShopBuy(const TArray<TPair<int32, int32>>& ItemIdAmount);
	bool SendShopSell(const TArray<TPair<int32, int32>>& IndexAmount);
	bool SendShopClose();

	/** Grupo: criar (0x01e8), convidar pelo nome (0x02c4), responder ao convite (0x02c7), sair (0x0100), expulsar (0x0103), passar a lideranca (0x07da). */
	bool SendCreateParty(const FString& Name);
	bool SendPartyInvite(const FString& CharName);
	bool SendPartyInviteReply(int32 PartyId, bool bAccept);
	bool SendLeaveParty();
	bool SendPartyExpel(int32 AccountId, const FString& CharName);
	bool SendPartyChangeLeader(int32 AccountId);

	/** Troca: pedir (0x00e4), responder (0x00e6), por item ou zeny (0x00e8), travar (0x00eb), confirmar (0x00ef), cancelar (0x00ed). */
	bool SendTradeRequest(int32 AccountId);
	bool SendTradeReply(bool bAccept);
	bool SendTradeAddItem(int32 InventoryIndex, int32 Amount);
	bool SendTradeLock();
	bool SendTradeCommit();
	bool SendTradeCancel();

	/** Amigos: adicionar pelo nome (0x0202), remover (0x0203), responder ao pedido (0x0208). */
	bool SendFriendAdd(const FString& Name);
	bool SendFriendDelete(int32 AccountId, int32 CharId);
	bool SendFriendReply(int32 AccountId, int32 CharId, bool bAccept);

	/** CZ_ACTIVE_QUEST 0x02b6: mostra ou esconde uma quest no rastreador. */
	bool SendQuestActive(int32 QuestId, bool bActive);

	/** Banco: 0x09b6 abrir, 0x09ab consultar, 0x09b8 fechar, 0x09a7 depositar, 0x09a9 sacar. */
	bool SendBankCommand(uint16 PacketId, int32 Money);

	/** CZ_REQ_ACH_REWARD 0x0a25: pede o premio de uma conquista concluida. */
	bool SendAchievementReward(int32 AchievementId);

	/** Troca rapida de equipamento: 0x0a97 incluir, 0x0a99 remover, 0x0a9c trocar tudo. */
	bool SendEquipSwitchAdd(int32 ItemIndex, int32 Position);
	bool SendEquipSwitchRemove(int32 ItemIndex);
	bool SendEquipSwitchAll();

	/** Aba de favoritos da mochila: CZ_INVENTORY_TAB 0x0907 (o servidor guarda e devolve em 0x0908). */
	bool SendItemFavorite(int32 ItemIndex, bool bCurrentlyFavorite);

	/** Correio (RODEX): pedidos 0x0ac0 abrir, 0x09ea ler, 0x09f5 excluir, 0x09f1 zeny, 0x09f3 itens. */
	bool SendMailRequest(uint16 PacketId, int64 MailId, uint8 Tab);
	bool SendMailBeginWrite(const FString& Receiver);
	bool SendMailWrite(const FString& Receiver, const FString& Title, const FString& Body);

	/** Sala de chat: criar (0x00d5), entrar (0x00d9), sair (0x00e3), expulsar (0x00e2). */
	bool SendCreateChatRoom(const FString& Title, int32 Limit, bool bPublic, const FString& Password);
	bool SendEnterChatRoom(int32 ChatId, const FString& Password);
	bool SendLeaveChatRoom();
	bool SendKickFromChatRoom(const FString& Name);

	/**
	 * Envia requisição de ação/ataque básico para o Map-Server (CZ_REQUEST_ACT 0x088E / 0x0360).
	 */
	UFUNCTION(BlueprintCallable, Category = "Ragnarok|Network")
	bool SendActionRequest(int64 TargetID, uint8 ActionType = 0);

	/**
	 * Envia requisição de conjuração de habilidade para o Map-Server (CZ_USE_SKILL 0x0438 / 0x0113).
	 */
	UFUNCTION(BlueprintCallable, Category = "Ragnarok|Network")
	bool SendUseSkillRequest(int32 SkillID, int32 SkillLevel, int64 TargetID);

	/**
	 * Cria um novo personagem no Char-Server (CH_MAKE_CHAR 0x0a39).
	 */
	UFUNCTION(BlueprintCallable, Category = "Ragnarok|Network")
	bool CreateCharacter(const FString& Name, uint8 Slot, int32 HairStyle, int32 HairColor, int32 JobClass = 0);

	/**
	 * Deleta um personagem no Char-Server (CH_DELETE_CHAR 0x0068).
	 */
	UFUNCTION(BlueprintCallable, Category = "Ragnarok|Network")
	bool DeleteCharacter(int32 CharID, const FString& Key = TEXT(""));

	/**
	 * Envia mensagem global/chat ou comando @ no Map-Server (CZ_GLOBAL_MESSAGE 0x00f3).
	 * Formata canonicamente como "<Nome> : <Mensagem>\0".
	 */
	UFUNCTION(BlueprintCallable, Category = "Ragnarok|Network")
	bool SendGlobalMessage(const FString& Message);
	/** Cochicho (CZ_WHISPER 0x0096), grupo (CZ_REQUEST_CHAT_PARTY 0x0108) e emocao (CZ_REQ_EMOTION 0x00bf), do clif.cpp. */
	bool SendWhisper(const FString& TargetName, const FString& Message);
	bool SendPartyMessage(const FString& Message);
	bool SendEmotion(int32 EmotionType);

	/**
	 * Distribui pontos de atributo (STR, AGI, VIT, INT, DEX, LUK) (CZ_STATUS_CHANGE 0x00bb).
	 */
	UFUNCTION(BlueprintCallable, Category = "Ragnarok|Network")
	bool SendStatusUp(int32 StatusType, int32 Amount = 1);

	/**
	 * Usa um item consumível do inventário (CZ_USE_ITEM 0x0439).
	 */
	UFUNCTION(BlueprintCallable, Category = "Ragnarok|Network")
	bool UseItem(int32 ItemIndex);

	/**
	 * Equipa um item no personagem (CZ_REQ_WEAR_EQUIP 0x0998).
	 */
	UFUNCTION(BlueprintCallable, Category = "Ragnarok|Network")
	bool EquipItem(int32 ItemIndex, int32 WearLocation = 0);

	/**
	 * Desequipa um item do personagem (CZ_REQ_TAKEOFF_EQUIP 0x00ab).
	 */
	UFUNCTION(BlueprintCallable, Category = "Ragnarok|Network")
	bool UnequipItem(int32 ItemIndex);

	/**
	 * Pega um item que está no chão pelo seu GID/AID (CZ_ITEM_PICKUP 0x0362).
	 */
	UFUNCTION(BlueprintCallable, Category = "Ragnarok|Network")
	bool PickUpItem(int64 ItemGID);

	/**
	 * Usa habilidade em alvo ou chão (CZ_USE_SKILL 0x0438).
	 */
	UFUNCTION(BlueprintCallable, Category = "Ragnarok|Network")
	bool UseSkill(int32 SkillID, int32 SkillLevel, int64 TargetID = 0, int32 TargetX = 0, int32 TargetY = 0);

	/**
	 * Distribui ponto de habilidade (CZ_UPGRADE_SKILLLEVEL 0x0112).
	 */
	UFUNCTION(BlueprintCallable, Category = "Ragnarok|Network")
	bool SendSkillUp(int32 SkillID);

	/**
	 * Converte ID de classe do rAthena para nome em português.
	 */
	UFUNCTION(BlueprintPure, Category = "Ragnarok|Network")
	static FString GetJobNameFromClass(int32 JobClass);

	/**
	 * Inicia diálogo ao clicar em um NPC (CZ_CONTACTNPC 0x0090).
	 */
	UFUNCTION(BlueprintCallable, Category = "Ragnarok|Network")
	bool ContactNpc(int64 NpcID);

	/**
	 * Avança para a próxima página de diálogo de um NPC (CZ_REQ_NEXT_SCRIPT 0x00b9).
	 */
	UFUNCTION(BlueprintCallable, Category = "Ragnarok|Network")
	bool NextNpcDialog(int64 NpcID);

	/**
	 * Seleciona uma opção de menu do NPC (CZ_CHOOSE_MENU 0x00b8).
	 */
	UFUNCTION(BlueprintCallable, Category = "Ragnarok|Network")
	bool SelectNpcMenu(int64 NpcID, int32 MenuIndex);
	/** Resposta ao input do script: numero (CZ_INPUT_EDITDLG 0x0143) ou texto (CZ_INPUT_EDITDLGSTR 0x01d5). */
	bool SendNpcInputNumber(int64 NpcID, int32 Value);
	bool SendNpcInputText(int64 NpcID, const FString& Text);

	/**
	 * Fecha a janela de diálogo com o NPC (CZ_CLOSE_DIALOG 0x0146).
	 */
	UFUNCTION(BlueprintCallable, Category = "Ragnarok|Network")
	bool CloseNpcDialog(int64 NpcID);

	/**
	 * Acesso síncrono aos status, inventário e habilidades em cache.
	 */
	UFUNCTION(BlueprintPure, Category = "Ragnarok|Network")
	const FRagnarokPlayerStats& GetPlayerStats() const { return CurrentStats; }

	UFUNCTION(BlueprintPure, Category = "Ragnarok|Network")
	const TArray<FRagnarokInventoryItem>& GetInventoryItems() const { return InventoryItems; }
	const TArray<FRagnarokInventoryItem>& GetStorageItems() const { return StorageItems; }
	int32 GetStorageCount() const { return StorageCount; }
	int32 GetStorageMax() const { return StorageMax; }

	bool IsInParty() const { return !PartyName.IsEmpty(); }
	bool IsTradeOpen() const { return bTradeOpen; }
	const TArray<FRoFriend>& GetFriends() const { return Friends; }
	const TArray<FRoQuest>& GetQuests() const { return Quests; }
	int64 GetBankBalance() const { return BankBalance; }
	const TArray<FRoAchievement>& GetAchievements() const { return Achievements; }
	/** Titulo (aba Titulo do Equipamento): CZ_REQ_CHANGE_TITLE 0x0a2e (0 tira); o servidor confirma com ZC_ACK_CHANGE_TITLE 0x0a2f. */
	bool SendChangeTitle(int32 TitleId);
	int32 GetCurrentTitle() const { return CurrentTitle; }
	int32 GetAchievementPoints() const { return AchievementPoints; }
	int32 GetAchievementRank() const { return AchievementRank; }
	/** Itens do segundo conjunto: X = indice (como no pacote), Y = posicao de equipar. */
	const TArray<FIntPoint>& GetEquipSwitch() const { return EquipSwitch; }
	const TArray<FRoMail>& GetMails() const { return Mails; }
	int64 GetOpenMailId() const { return OpenMailId; }
	const FString& GetOpenMailText() const { return OpenMailText; }
	int64 GetOpenMailZeny() const { return OpenMailZeny; }
	int32 GetOpenMailItems() const { return OpenMailItems; }
	bool HasUnreadMail() const { if (bMailUnreadFlag) { return true; } for (const FRoMail& M : Mails) { if (!M.bRead) { return true; } } return false; }
	void ClearOpenMail() { OpenMailId = 0; OpenMailText.Reset(); OpenMailZeny = 0; OpenMailItems = 0; }
	bool IsInChatRoom() const { return bInChatRoom; }
	const FString& GetChatRoomTitle() const { return CurrentChatRoomTitle; }
	const TArray<FString>& GetChatRoomMembers() const { return ChatRoomMembers; }
	const TMap<int32, FRoChatRoom>& GetChatRooms() const { return ChatRooms; }
	const FString& GetTradePartner() const { return TradePartner; }
	const TArray<FRoTradeItem>& GetTradeMine() const { return TradeMine; }
	const TArray<FRoTradeItem>& GetTradeTheirs() const { return TradeTheirs; }
	bool IsTradeLockedMine() const { return bTradeLockMine; }
	bool IsTradeLockedTheirs() const { return bTradeLockTheirs; }
	const FString& GetPartyName() const { return PartyName; }
	const TArray<FRoPartyMember>& GetPartyMembers() const { return PartyMembers; }

	UFUNCTION(BlueprintPure, Category = "Ragnarok|Network")
	const TArray<FRagnarokSkillInfo>& GetSkillList() const { return SkillList; }

	/** ID do proprio jogador nos pacotes do map-server (AID da conta), para separar "eu" das outras unidades. */
	int64 GetSelfID() const { return SavedAccountID; }
	/** Sexo da conta (AC_ACCEPT_LOGIN): 0 = feminino, 1 = masculino. */
	uint8 GetSelfSex() const { return SavedSex; }
	/** Personagem escolhido na selecao (cabelo, classe inicial...). */
	const FRagnarokCharacterSlotInfo& GetSelectedSlot() const { return SelectedSlotInfo; }

	/** Atalhos F1.. da barra (aba 0), como o servidor guardou. */
	const TArray<FRagnarokHotkey>& GetHotkeys() const { return Hotkeys; }

	/** Grava um atalho no servidor (CZ_SHORTCUT_KEY_CHANGE2 0x0b21); ID 0 limpa. */
	UFUNCTION(BlueprintCallable, Category = "Ragnarok|Network")
	bool SetHotkey(int32 Index, bool bIsSkill, int32 ID, int32 Count);

	/** Mapa/celula de destino da ultima entrada ou troca de mapa (ZC_ACCEPT_ENTER / ZC_NPCACK_MAPMOVE). */
	const FString& GetPendingMapName() const { return PendingMapName; }
	FIntPoint GetPendingMapCell() const { return PendingMapCell; }
	/** true entre a ordem de entrar no mapa e o NotifyMapLoaded(). */
	bool IsAwaitingMapLoad() const { return bAwaitingMapLoad; }

	/** O cliente terminou de carregar o mapa: envia CZ_NOTIFY_ACTORINIT (0x007d) e o servidor manda as unidades. */
	UFUNCTION(BlueprintCallable, Category = "Ragnarok|Network")
	void NotifyMapLoaded();

	/** Nome do personagem selecionado. */
	const FString& GetSelfName() const { return SelectedCharName; }

	/**
	 * Desconecta todos os sockets ativos de forma limpa.
	 */
	UFUNCTION(BlueprintCallable, Category = "Ragnarok|Network")
	void Disconnect();

	/**
	 * Retorna o estado atual da máquina de conexão.
	 */
	UFUNCTION(BlueprintPure, Category = "Ragnarok|Network")
	ERagnarokNetworkState GetNetworkState() const { return CurrentState; }

	// ========================================================================
	// Eventos Dispatchers (Contrato Seção 3)
	// ========================================================================
	UPROPERTY(BlueprintAssignable, Category = "Ragnarok|Network|Events")
	FOnNetworkConnected OnNetworkConnected;

	UPROPERTY(BlueprintAssignable, Category = "Ragnarok|Network|Events")
	FOnNetworkDisconnected OnNetworkDisconnected;

	UPROPERTY(BlueprintAssignable, Category = "Ragnarok|Network|Events")
	FOnLoginResult OnLoginResult;

	UPROPERTY(BlueprintAssignable, Category = "Ragnarok|Network|Events")
	FOnCharacterList OnCharacterList;
	FOnCharCreateResult OnCharCreateResult;

	UPROPERTY(BlueprintAssignable, Category = "Ragnarok|Network|Events")
	FOnMapEnter OnMapEnter;

	UPROPERTY(BlueprintAssignable, Category = "Ragnarok|Network|Events")
	FOnUnitAppear OnUnitAppear;

	UPROPERTY(BlueprintAssignable, Category = "Ragnarok|Network|Events")
	FOnUnitMove OnUnitMove;

	UPROPERTY(BlueprintAssignable, Category = "Ragnarok|Movement")
	FOnUnitWalk OnUnitWalk;

	UPROPERTY(BlueprintAssignable, Category = "Ragnarok|Movement")
	FOnUnitStop OnUnitStop;

	UPROPERTY(BlueprintAssignable, Category = "Ragnarok|Movement")
	FOnUnitDirection OnUnitDirection;

	UPROPERTY(BlueprintAssignable, Category = "Ragnarok|Movement")
	FOnUnitOption OnUnitOption;
	FOnUnitBodyState OnUnitBodyState;
	FOnSpiritBalls OnSpiritBalls;

	UPROPERTY(BlueprintAssignable, Category = "Ragnarok|Movement")
	FOnUnitRevive OnUnitRevive;

	UPROPERTY(BlueprintAssignable, Category = "Ragnarok|Shop") FOnShopDealType OnShopDealType;
	UPROPERTY(BlueprintAssignable, Category = "Ragnarok|Shop") FOnShopBuyList OnShopBuyList;
	UPROPERTY(BlueprintAssignable, Category = "Ragnarok|Shop") FOnShopSellList OnShopSellList;
	UPROPERTY(BlueprintAssignable, Category = "Ragnarok|Shop") FOnShopResult OnShopResult;
	UPROPERTY(BlueprintAssignable, Category = "Ragnarok|Chat") FOnSpeech OnSpeech;
	UPROPERTY(BlueprintAssignable, Category = "Ragnarok|Chat") FOnEmotion OnEmotion;
	UPROPERTY(BlueprintAssignable, Category = "Ragnarok|Combat") FOnMonsterHp OnMonsterHp;
	UPROPERTY(BlueprintAssignable, Category = "Ragnarok|Storage") FOnStorageOpen OnStorageOpen;
	UPROPERTY(BlueprintAssignable, Category = "Ragnarok|Storage") FOnStorageChanged OnStorageChanged;
	UPROPERTY(BlueprintAssignable, Category = "Ragnarok|Storage") FOnStorageClose OnStorageClose;
	UPROPERTY(BlueprintAssignable, Category = "Ragnarok|Cart") FOnCartChanged OnCartChanged;
	UPROPERTY(BlueprintAssignable, Category = "Ragnarok|Party") FOnPartyChanged OnPartyChanged;
	UPROPERTY(BlueprintAssignable, Category = "Ragnarok|Party") FOnPartyInvite OnPartyInvite;
	UPROPERTY(BlueprintAssignable, Category = "Ragnarok|Trade") FOnTradeRequest OnTradeRequest;
	UPROPERTY(BlueprintAssignable, Category = "Ragnarok|Trade") FOnTradeChanged OnTradeChanged;
	UPROPERTY(BlueprintAssignable, Category = "Ragnarok|Friends") FOnFriendsChanged OnFriendsChanged;
	UPROPERTY(BlueprintAssignable, Category = "Ragnarok|Quests") FOnQuestsChanged OnQuestsChanged;
	UPROPERTY(BlueprintAssignable, Category = "Ragnarok|Bank") FOnBankChanged OnBankChanged;
	UPROPERTY(BlueprintAssignable, Category = "Ragnarok|Achievements") FOnAchievementsChanged OnAchievementsChanged;
	FOnEquipSwitchChanged OnEquipSwitchChanged;
	FOnMailChanged OnMailChanged;
	UPROPERTY(BlueprintAssignable, Category = "Ragnarok|ChatRoom") FOnChatRoomChanged OnChatRoomChanged;
	UPROPERTY(BlueprintAssignable, Category = "Ragnarok|Friends") FOnFriendRequest OnFriendRequest;

	UPROPERTY(BlueprintAssignable, Category = "Ragnarok|Network|Events")
	FOnUnitVanish OnUnitVanish;

	UPROPERTY(BlueprintAssignable, Category = "Ragnarok|Network|Events")
	FOnDamage OnDamage;

	UPROPERTY(BlueprintAssignable, Category = "Ragnarok|Network|Events")
	FOnStatusChanged OnStatusChanged;

	UPROPERTY(BlueprintAssignable, Category = "Ragnarok|Network|Events")
	FOnPlayerStatsChanged OnPlayerStatsChanged;

	UPROPERTY(BlueprintAssignable, Category = "Ragnarok|Network|Events")
	FOnEquipChanged OnEquipChanged;

	UPROPERTY(BlueprintAssignable, Category = "Ragnarok|Network|Events")
	FOnChatMessage OnChatMessage;

	UPROPERTY(BlueprintAssignable, Category = "Ragnarok|Network|Events")
	FOnInventory OnInventory;

	UPROPERTY(BlueprintAssignable, Category = "Ragnarok|Network|Events")
	FOnSkillList OnSkillList;

	UPROPERTY(BlueprintAssignable, Category = "Ragnarok|Network|Events")
	FOnGroundItemAppear OnGroundItemAppear;

	UPROPERTY(BlueprintAssignable, Category = "Ragnarok|Network|Events")
	FOnGroundItemVanish OnGroundItemVanish;

	UPROPERTY(BlueprintAssignable, Category = "Ragnarok|Network|Events")
	FOnNpcDialog OnNpcDialog;

	UPROPERTY(BlueprintAssignable, Category = "Ragnarok|Network|Events")
	FOnNpcMenu OnNpcMenu;

	UPROPERTY(BlueprintAssignable, Category = "Ragnarok|Network|Events")
	FOnWeather OnWeather;

	/** Dano de skill (ZC_NOTIFY_SKILL 0x01de). */
	UPROPERTY(BlueprintAssignable, Category = "Ragnarok|Network|Events")
	FOnSkillDamage OnSkillDamage;

	/** Skill sem dano: cura, buff (ZC_USE_SKILL 0x09cb). Value = cura/nivel. */
	UPROPERTY(BlueprintAssignable, Category = "Ragnarok|Network|Events")
	FOnSkillEffect OnSkillEffect;

	/** Inicio da conjuracao (ZC_USESKILL_ACK 0x0b1a): barra de conjuracao por CastMs. */
	UPROPERTY(BlueprintAssignable, Category = "Ragnarok|Network|Events")
	FOnSkillCast OnSkillCast;
	FOnSkillGround OnSkillGround;

	/** Falha ao usar a skill (ZC_ACK_TOUSESKILL 0x0110): sem SP, fora de alcance... */
	UPROPERTY(BlueprintAssignable, Category = "Ragnarok|Network|Events")
	FOnSkillFailed OnSkillFailed;

	/** ZC_NOTIFY_EFFECT2 (0x01f3) que nao e clima: efeito da EffectTable sobre a unidade (doc/effect_list.txt do rAthena). */
	UPROPERTY(BlueprintAssignable, Category = "Ragnarok|Skills")
	FOnSpecialEffect OnSpecialEffect;

	/** Buffs/estados: ativacao com duracao e desativacao (ZC_MSG_STATE_CHANGE3 0x0983, ZC_MSG_STATE_CHANGE 0x0196, ZC_EFST_SET_ENTER 0x0984). */
	UPROPERTY(BlueprintAssignable, Category = "Ragnarok|Status")
	FOnStatusChange OnStatusChange;

	/** Unidade de skill no chão que surge (ZC_SKILL_ENTRY5 0x09ca: Firewall, Pneuma, etc.). */
	UPROPERTY(BlueprintAssignable, Category = "Ragnarok|Skills")
	FOnSkillUnitAppear OnSkillUnitAppear;

	/** Unidade de skill no chão removida (ZC_SKILL_DISAPPEAR 0x0120). */
	UPROPERTY(BlueprintAssignable, Category = "Ragnarok|Skills")
	FOnSkillUnitDisappear OnSkillUnitDisappear;

	/** Efeitos gerais da unidade: subir de nível, refino (ZC_NOTIFY_EFFECT 0x019b). */
	UPROPERTY(BlueprintAssignable, Category = "Ragnarok|Effects")
	FOnNotifyEffect OnNotifyEffect;

	/** Resultado de equipar ou desequipar item (ZC_REQ_WEAR_EQUIP_ACK 0x0999, ZC_REQ_TAKEOFF_EQUIP_ACK 0x099a). */
	UPROPERTY(BlueprintAssignable, Category = "Ragnarok|Inventory")
	FOnEquipResult OnEquipResult;

	/** Notificacao de alvo fora de alcance de ataque (ZC_ATTACK_FAILURE_FOR_DISTANCE 0x0139). */
	UPROPERTY(BlueprintAssignable, Category = "Ragnarok|Combat")
	FOnAttackOutOfRange OnAttackOutOfRange;

	// Compatibilidade
	UPROPERTY(BlueprintAssignable, Category = "Ragnarok|Network|Events")
	FOnPlayerMoveConfirmed OnPlayerMoveConfirmed;

	UPROPERTY(BlueprintAssignable, Category = "Ragnarok|Network|Events")
	FOnDamageReceived OnDamageReceived;

private:
	FSocket* CurrentSocket;
	TArray<uint8> ReceiveBuffer;
	ERagnarokNetworkState CurrentState;

	/** Ultimo "ping" de manutencao (o rAthena derruba a sessao com 60 s sem receber nada: stall_time) */
	double LastKeepAliveTime = 0.0;
	void SendKeepAlive();

	FString SavedHost;
	FString SessionUser;
	FString SessionPass;
	uint32 SavedAccountID;
	int32 SelfOption = 0;
	bool bShowEquip = false;
	/** Alcance de ataque do proprio personagem em celulas (ZC_ATTACK_RANGE): 1 corpo a corpo, 9+ arco. */
	int32 SelfAttackRange = 1;
	TMap<int64, FIntPoint> GroundItemSubs;
	uint32 SavedLoginID1;
	uint32 SavedLoginID2;
	uint8 SavedSex;
	int32 SavedCharPort;

	uint32 PendingCharID;
	FString PendingMapName;
	FString SelectedCharName;
	FRagnarokCharacterSlotInfo SelectedSlotInfo;
	TArray<FRagnarokCharacterSlotInfo> CachedCharacterSlots;

	FRagnarokPlayerStats CurrentStats;
	TArray<FRagnarokInventoryItem> InventoryItems;
	TArray<FRagnarokInventoryItem> StorageItems;
	int32 StorageCount = 0;
	int32 StorageMax = 300;
	TArray<FRagnarokInventoryItem> CartItems;
	int32 CartCount = 0;
	int32 CartMax = 100;
	int32 CartWeight = 0;
	int32 CartMaxWeight = 8000;
	bool bHasCart = false;
	void HandleCartPacket(const uint8* PacketData, int32 PacketSize, uint16 PacketID);
	/** Listas de escolha e as respostas delas (RagnarokNetworkSelectLists.cpp). */
	void HandleSelectListPacket(const uint8* PacketData, int32 PacketSize, uint16 PacketID);
	int32 PendingCardIndex = -1;
	TArray<TArray<uint8>> RepairEntries; // entradas de 24 bytes do ZC_REPAIRITEMLIST, devolvidas como vieram
	void HandlePetPacket(const uint8* PacketData, int32 PacketSize, uint16 PacketID);
	int64 PetGID = 0;
	FRoPetInfo PetInfo;
	bool bCapturing = false;
	bool bPetAutoFeed = false;
	void HandleGuildPacket(const uint8* PacketData, int32 PacketSize, uint16 PacketID);
	void ClearGuild();
	int32 GuildId = 0;
	bool bGuildMaster = false;
	FString GuildName;
	FRoGuildInfo GuildInfo;
	TArray<FRoGuildMember> GuildMembers;
	TMap<int32, FString> GuildPositions;
	FString GuildNoticeSubject;
	FString GuildNoticeText;
	int32 GuildInviteId = 0;
	FString GuildInviteName;
	void HandleVendingPacket(const uint8* PacketData, int32 PacketSize, uint16 PacketID);
	TMap<int32, FString> VendingBoards; // dono (AID) -> titulo da loja sobre a cabeca
	int32 VendMaxItems = 0;
	int32 VendBuyAID = 0;
	int32 VendBuyId = 0;
	TArray<FRoVendItem> VendBuyItems;
	TArray<FRoVendItem> MyVendItems;
	bool bVending = false;
	int32 SelectSkillId = 0; // skill do Portal (vai de volta no CZ_SELECT_WARPPOINT)

	FString PartyName;
	TArray<FRoFriend> Friends;
	TArray<FRoQuest> Quests;
	int64 BankBalance = 0;
	TArray<FRoAchievement> Achievements;
	int32 CurrentTitle = 0;
	TArray<FIntPoint> EquipSwitch;
	TArray<FRoMail> Mails;
	int64 OpenMailId = 0;
	FString OpenMailText;
	int64 OpenMailZeny = 0;
	int32 OpenMailItems = 0;
	bool bMailWriting = false;
	bool bMailUnreadFlag = false;
	int32 AchievementPoints = 0;
	int32 AchievementRank = 0;
	bool bBankOpen = false;
	TMap<int32, FRoChatRoom> ChatRooms;
	bool bInChatRoom = false;
	int32 CurrentChatRoomId = 0;
	FString CurrentChatRoomTitle;
	FString ChatRoomOwnerName;
	FString PendingChatRoomTitle;
	TArray<FString> ChatRoomMembers;
	bool bTradeOpen = false;
	bool bTradeLockMine = false;
	bool bTradeLockTheirs = false;
	FString TradePartner;
	int32 TradeRequesterId = 0;
	TArray<FRoTradeItem> TradeMine;
	TArray<FRoTradeItem> TradeTheirs;
	TMap<int32, int32> TradePendingAmount; // indice -> quantidade pedida, confirmada pelo 0x00ea
	TArray<FRoPartyMember> PartyMembers;
	TArray<FRagnarokSkillInfo> SkillList;

	bool ConnectToPort(const FString& Host, int32 Port, ERagnarokNetworkState NewState);
	void SendData(const uint8* Data, int32 Size);
	void ProcessIncomingData();

	// Handlers especializados por pacote
	void HandleLoginAccept(const uint8* PacketData, int32 PacketSize);
	void HandleLoginRefuse(const uint8* PacketData, int32 PacketSize);
	void HandleNotifyBan(const uint8* PacketData, int32 PacketSize);
	/** Motivo da proxima desconexao (vazio = encerrada pelo cliente) */
	FString DisconnectReason;
	void HandleCharacterList(const uint8* PacketData, int32 PacketSize);
	void HandleCharMakeResult(const uint8* PacketData, int32 PacketSize, uint16 PacketID);
	void HandleZoneServerNotify(const uint8* PacketData, int32 PacketSize);
	void HandleMapAcceptEnter(const uint8* PacketData, int32 PacketSize);
	void HandleUnitAppear(const uint8* PacketData, int32 PacketSize, uint16 PacketID);
	void HandlePlayerMove(const uint8* PacketData, int32 PacketSize);
	void FlushPendingSend();
	void HandleMapMove(const uint8* PacketData, int32 PacketSize);
	void HandleItemEntry(const uint8* PacketData, int32 PacketSize);
	void HandleItemPickupAck(const uint8* PacketData, int32 PacketSize);
	void RemoveInventoryAmount(int32 Index, int32 Count);
	FIntPoint PendingMapCell = FIntPoint::ZeroValue;
	bool bAwaitingMapLoad = false;
	/** Bytes aguardando o socket conectar (connect nao-bloqueante) ou sobra de envio parcial. */
	TArray<uint8> PendingSend;
	/** Incrementado a cada troca de socket; o laco de framing para se um handler reconectar. */
	uint32 ReceiveGeneration = 0;
	/** -MythNetLog: registra cada pacote enviado/recebido (depuracao). */
	bool bLogPackets = false;
	/** ID -> tamanho (-1 = dinamico), de Config/packet_lengths_<PACKETVER>.json. */
	TMap<uint16, int16> PacketLengths;
	TArray<FRagnarokHotkey> Hotkeys;
	void HandleHotkeys(const uint8* PacketData, int32 PacketSize);
	void HandleSkillResult(const uint8* PacketData, int32 PacketSize, uint16 PacketID);
	uint16 LastGoodPacketID = 0;
	int32 LastGoodPacketLen = 0;
	void HandleUnitVanish(const uint8* PacketData, int32 PacketSize);
	void HandleActionNotify(const uint8* PacketData, int32 PacketSize);
	void HandleParamChange(const uint8* PacketData, int32 PacketSize, uint16 PacketID);
	void HandleSpriteChange(const uint8* PacketData, int32 PacketSize);
	void HandleChatMessage(const uint8* PacketData, int32 PacketSize, uint16 PacketID);
	void HandleStatus(const uint8* PacketData, int32 PacketSize, uint16 PacketID);
	void HandleItemFall(const uint8* PacketData, int32 PacketSize);
	void HandleItemDisappear(const uint8* PacketData, int32 PacketSize);
	void HandleNpcDialog(const uint8* PacketData, int32 PacketSize);
	void HandleNpcWait(const uint8* PacketData, int32 PacketSize);
	void HandleNpcClose(const uint8* PacketData, int32 PacketSize);
	void HandleNpcMenu(const uint8* PacketData, int32 PacketSize);
	void HandleSkillList(const uint8* PacketData, int32 PacketSize);
	void HandleInventoryList(const uint8* PacketData, int32 PacketSize, bool bIsEquip);
	void HandleEffect(const uint8* PacketData, int32 PacketSize);
	void HandleOptionChange(const uint8* PacketData, int32 PacketSize, uint16 PacketID);
	void HandleResurrection(const uint8* PacketData, int32 PacketSize);
	void HandleChatExtras(const uint8* PacketData, int32 PacketSize, uint16 PacketID);
	void HandleStoragePacket(const uint8* PacketData, int32 PacketSize, uint16 PacketID);
	void HandleShopPacket(const uint8* PacketData, int32 PacketSize, uint16 PacketID);
	void HandlePartyPacket(const uint8* PacketData, int32 PacketSize, uint16 PacketID);
	void HandleTradePacket(const uint8* PacketData, int32 PacketSize, uint16 PacketID);
	void HandleFriendsPacket(const uint8* PacketData, int32 PacketSize, uint16 PacketID);
	void HandleQuestPacket(const uint8* PacketData, int32 PacketSize, uint16 PacketID);
	void HandleBankPacket(const uint8* PacketData, int32 PacketSize, uint16 PacketID);
	void HandleAchievementPacket(const uint8* PacketData, int32 PacketSize, uint16 PacketID);
	void HandleEquipSwitchPacket(const uint8* PacketData, int32 PacketSize, uint16 PacketID);
	void HandleItemFavoritePacket(const uint8* PacketData, int32 PacketSize);
	void HandleMailPacket(const uint8* PacketData, int32 PacketSize, uint16 PacketID);
	void HandleChatRoomPacket(const uint8* PacketData, int32 PacketSize, uint16 PacketID);
	void HandleSkillUnitAppear(const uint8* PacketData, int32 PacketSize);
	void HandleSkillUnitDisappear(const uint8* PacketData, int32 PacketSize);
	void HandleNotifyEffect(const uint8* PacketData, int32 PacketSize);
	void HandleEquipAck(const uint8* PacketData, int32 PacketSize, uint16 PacketID);
	void HandleAttackOutOfRange(const uint8* PacketData, int32 PacketSize);
	void HandleNotifyMove(const uint8* PacketData, int32 PacketSize);
	void HandleStopMove(const uint8* PacketData, int32 PacketSize);
	void HandleChangeDirection(const uint8* PacketData, int32 PacketSize);
	void HandleStateChange(const uint8* PacketData, int32 PacketSize, uint16 PacketID);
};

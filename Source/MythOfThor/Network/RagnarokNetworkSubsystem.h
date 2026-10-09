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
	UPROPERTY(BlueprintReadOnly, Category = "Ragnarok|Inventory") int32 Refine = 0;
	UPROPERTY(BlueprintReadOnly, Category = "Ragnarok|Inventory") bool bIdentified = true;
	UPROPERTY(BlueprintReadOnly, Category = "Ragnarok|Inventory") bool bIsEquipped = false;
	UPROPERTY(BlueprintReadOnly, Category = "Ragnarok|Inventory") int32 Slot1 = 0;
	UPROPERTY(BlueprintReadOnly, Category = "Ragnarok|Inventory") int32 Slot2 = 0;
	UPROPERTY(BlueprintReadOnly, Category = "Ragnarok|Inventory") int32 Slot3 = 0;
	UPROPERTY(BlueprintReadOnly, Category = "Ragnarok|Inventory") int32 Slot4 = 0;
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
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnSkillFailed, int32, SkillID, int32, Cause);

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
	static constexpr uint32 CLIENT_PACKETVER = 20211103;

	/**
	 * Conecta ao Login-Server do rAthena e autentica a conta (CA_LOGIN 0x0064).
	 */
	UFUNCTION(BlueprintCallable, Category = "Ragnarok|Network")
	bool ConnectAndLogin(const FString& ServerHost, const FString& Username, const FString& Password);

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

	UFUNCTION(BlueprintPure, Category = "Ragnarok|Network")
	const TArray<FRagnarokSkillInfo>& GetSkillList() const { return SkillList; }

	/** ID do proprio jogador nos pacotes do map-server (AID da conta), para separar "eu" das outras unidades. */
	int64 GetSelfID() const { return SavedAccountID; }

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

	UPROPERTY(BlueprintAssignable, Category = "Ragnarok|Network|Events")
	FOnMapEnter OnMapEnter;

	UPROPERTY(BlueprintAssignable, Category = "Ragnarok|Network|Events")
	FOnUnitAppear OnUnitAppear;

	UPROPERTY(BlueprintAssignable, Category = "Ragnarok|Network|Events")
	FOnUnitMove OnUnitMove;

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

	/** Falha ao usar a skill (ZC_ACK_TOUSESKILL 0x0110): sem SP, fora de alcance... */
	UPROPERTY(BlueprintAssignable, Category = "Ragnarok|Network|Events")
	FOnSkillFailed OnSkillFailed;

	// Compatibilidade
	UPROPERTY(BlueprintAssignable, Category = "Ragnarok|Network|Events")
	FOnPlayerMoveConfirmed OnPlayerMoveConfirmed;

	UPROPERTY(BlueprintAssignable, Category = "Ragnarok|Network|Events")
	FOnDamageReceived OnDamageReceived;

private:
	FSocket* CurrentSocket;
	TArray<uint8> ReceiveBuffer;
	ERagnarokNetworkState CurrentState;

	FString SavedHost;
	uint32 SavedAccountID;
	uint32 SavedLoginID1;
	uint32 SavedLoginID2;
	uint8 SavedSex;
	int32 SavedCharPort;

	uint32 PendingCharID;
	FString PendingMapName;
	FString SelectedCharName;
	TArray<FRagnarokCharacterSlotInfo> CachedCharacterSlots;

	FRagnarokPlayerStats CurrentStats;
	TArray<FRagnarokInventoryItem> InventoryItems;
	TArray<FRagnarokSkillInfo> SkillList;

	bool ConnectToPort(const FString& Host, int32 Port, ERagnarokNetworkState NewState);
	void SendData(const uint8* Data, int32 Size);
	void ProcessIncomingData();

	// Handlers especializados por pacote
	void HandleLoginAccept(const uint8* PacketData, int32 PacketSize);
	void HandleLoginRefuse(const uint8* PacketData, int32 PacketSize);
	void HandleCharacterList(const uint8* PacketData, int32 PacketSize);
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
};

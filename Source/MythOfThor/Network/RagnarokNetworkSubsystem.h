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
DECLARE_DYNAMIC_MULTICAST_DELEGATE_ThreeParams(FOnEquipChanged, int64, GID, uint8, Slot, int32, VisualID);

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
	FOnEquipChanged OnEquipChanged;

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
	void HandleUnitVanish(const uint8* PacketData, int32 PacketSize);
	void HandleActionNotify(const uint8* PacketData, int32 PacketSize);
	void HandleParamChange(const uint8* PacketData, int32 PacketSize, bool bIsLong);
	void HandleSpriteChange(const uint8* PacketData, int32 PacketSize);
};

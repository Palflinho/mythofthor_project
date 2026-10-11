#include "RagnarokNetworkSubsystem.h"
#include "RagnarokPacketDatabase.h"
#include "Common/TcpSocketBuilder.h"
#include "Dom/JsonObject.h"
#include "Misc/FileHelper.h"
#include "Misc/CommandLine.h"
#include "Misc/Paths.h"
#include "Serialization/JsonReader.h"
#include "Serialization/JsonSerializer.h"
#include "IPAddress.h"

void URagnarokNetworkSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);
	CurrentSocket = nullptr;
	CurrentState = ERagnarokNetworkState::Disconnected;
	SavedAccountID = 0;
	SavedLoginID1 = 0;
	SavedLoginID2 = 0;
	SavedSex = 0;
	SavedCharPort = CHAR_SERVER_PORT;
	PendingCharID = 0;

	// Tamanhos de pacote gerados do proprio rAthena (tools/gen_packet_lengths.py). A tabela fixa do
	// RagnarokPacketDatabase.h fica so como reserva para IDs que nao estiverem no arquivo.
	FString Text;
	TSharedPtr<FJsonObject> Root;
	const FString Path = FPaths::Combine(FPaths::ProjectConfigDir(), FString::Printf(TEXT("packet_lengths_%u.json"), CLIENT_PACKETVER));
	if (FFileHelper::LoadFileToString(Text, *Path) && FJsonSerializer::Deserialize(TJsonReaderFactory<>::Create(Text), Root) && Root.IsValid())
	{
		for (const TPair<FString, TSharedPtr<FJsonValue>>& P : Root->Values)
		{
			PacketLengths.Add(static_cast<uint16>(FCString::Strtoi(*P.Key, nullptr, 16)), static_cast<int16>(P.Value->AsNumber()));
		}
	}
	// Grupo (PACKETVER >= 20171207): ZC_ADD_MEMBER_TO_GROUP 0x0ae4 e ZC_GROUP_LIST 0x0ae5 (packets_struct.hpp) nao estao no JSON
	PacketLengths.FindOrAdd(0x0ae4) = 89;
	PacketLengths.FindOrAdd(0x0ae5) = -1;
	UE_LOG(LogTemp, Log, TEXT("[RagnarokNetwork] %d tamanhos de pacote carregados de %s"), PacketLengths.Num(), *Path);
	bLogPackets = FParse::Param(FCommandLine::Get(), TEXT("MythNetLog"));
}

void URagnarokNetworkSubsystem::Deinitialize()
{
	Disconnect();
	Super::Deinitialize();
}

void URagnarokNetworkSubsystem::Tick(float DeltaTime)
{
	if (CurrentSocket && CurrentState != ERagnarokNetworkState::Disconnected)
	{
		FlushPendingSend();
		ProcessIncomingData();
		// Como o cliente original: um pacote de manutencao a cada 12 s no char-server e no map-server
		if (CurrentSocket && FPlatformTime::Seconds() - LastKeepAliveTime >= 12.0)
		{
			SendKeepAlive();
		}
		// Servidor fechou a conexao (timeout, kick): avisa em vez de seguir "enviando" para um socket morto
		if (CurrentSocket && CurrentSocket->GetConnectionState() == SCS_ConnectionError)
		{
			DisconnectReason = TEXT("A conexão com o servidor caiu.");
			Disconnect();
		}
	}
}

void URagnarokNetworkSubsystem::SendKeepAlive()
{
	LastKeepAliveTime = FPlatformTime::Seconds();
	#pragma pack(push, 1)
	struct FKeepAlive
	{
		uint16 PacketID;
		uint32 Value;
	};
	#pragma pack(pop)
	FKeepAlive Packet;
	if (CurrentState == ERagnarokNetworkState::ConnectedMap)
	{
		// CZ_REQUEST_TIME (clif_parse_TickSend, 0x0360 na tabela embaralhada de clif_shuffle.hpp, PACKETVER > 20180307). Atencao: 0x0363 e o DropItem
		Packet.PacketID = 0x0360;
		Packet.Value = FPlatformTime::Cycles();
	}
	else if (CurrentState == ERagnarokNetworkState::ConnectedChar)
	{
		// CH_PING (0x0187): AID
		Packet.PacketID = 0x0187;
		Packet.Value = SavedAccountID;
	}
	else
	{
		return;
	}
	SendData(reinterpret_cast<const uint8*>(&Packet), sizeof(Packet));
}

bool URagnarokNetworkSubsystem::ConnectToPort(const FString& Host, int32 Port, ERagnarokNetworkState NewState)
{
	if (CurrentSocket)
	{
		CurrentSocket->Close();
		ISocketSubsystem::Get(PLATFORM_SOCKETSUBSYSTEM)->DestroySocket(CurrentSocket);
		CurrentSocket = nullptr;
	}

	PendingSend.Empty();
	ReceiveBuffer.Empty();
	++ReceiveGeneration;

	ISocketSubsystem* const SocketSubsystem = ISocketSubsystem::Get(PLATFORM_SOCKETSUBSYSTEM);
	if (!SocketSubsystem)
	{
		OnNetworkDisconnected.Broadcast(TEXT("Falha ao obter subsistema de sockets."));
		CurrentState = ERagnarokNetworkState::Disconnected;
		return false;
	}

	TSharedRef<FInternetAddr> Addr = SocketSubsystem->CreateInternetAddr();
	bool bIsValid = false;
	Addr->SetIp(*Host, bIsValid);
	Addr->SetPort(Port);

	if (!bIsValid)
	{
		TSharedPtr<FInternetAddr> ResolvedAddr = SocketSubsystem->GetAddressFromString(Host);
		if (ResolvedAddr.IsValid())
		{
			Addr = ResolvedAddr.ToSharedRef();
			Addr->SetPort(Port);
		}
		else
		{
			OnNetworkDisconnected.Broadcast(FString::Printf(TEXT("Endereço IP inválido: %s"), *Host));
			CurrentState = ERagnarokNetworkState::Disconnected;
			return false;
		}
	}

	CurrentSocket = FTcpSocketBuilder(TEXT("MythOfThor_ClientSocket"))
		.AsNonBlocking()
		.AsReusable()
		.WithReceiveBufferSize(64 * 1024)
		.WithSendBufferSize(64 * 1024)
		.Build();

	if (!CurrentSocket)
	{
		OnNetworkDisconnected.Broadcast(TEXT("Falha ao instanciar socket TCP."));
		CurrentState = ERagnarokNetworkState::Disconnected;
		return false;
	}

	CurrentSocket->Connect(*Addr);
	CurrentState = NewState;
	OnNetworkConnected.Broadcast(FString::Printf(TEXT("Conectado a %s:%d"), *Host, Port));
	return true;
}

bool URagnarokNetworkSubsystem::ConnectAndLogin(const FString& ServerHost, const FString& Username, const FString& Password)
{
	SavedHost = ServerHost;

	if (!ConnectToPort(ServerHost, LOGIN_SERVER_PORT, ERagnarokNetworkState::ConnectingLogin))
	{
		return false;
	}

	// Montagem do pacote CA_LOGIN (0x0064) - 55 Bytes
	#pragma pack(push, 1)
	struct FCA_LoginPacket
	{
		uint16 PacketID = RagnarokPackets::HEADER_CA_LOGIN;
		uint32 Version = CLIENT_PACKETVER;
		char Username[24] = { 0 };
		char Password[24] = { 0 };
		uint8 ClientType = 0;
	};
	#pragma pack(pop)

	FCA_LoginPacket Packet;
	FCStringAnsi::Strncpy(Packet.Username, TCHAR_TO_ANSI(*Username), sizeof(Packet.Username));
	FCStringAnsi::Strncpy(Packet.Password, TCHAR_TO_ANSI(*Password), sizeof(Packet.Password));

	SendData(reinterpret_cast<const uint8*>(&Packet), sizeof(Packet));
	CurrentState = ERagnarokNetworkState::ConnectedLogin;
	return true;
}

bool URagnarokNetworkSubsystem::SelectCharacter(int32 SlotIndex)
{
	if (!CurrentSocket || CurrentState != ERagnarokNetworkState::ConnectedChar)
	{
		return false;
	}

	for (const FRagnarokCharacterSlotInfo& Slot : CachedCharacterSlots)
	{
		if (Slot.Slot == SlotIndex)
		{
			SelectedCharName = Slot.Name;
			SelectedSlotInfo = Slot;
			CurrentStats.JobClass = Slot.JobClass;
			CurrentStats.JobName = GetJobNameFromClass(Slot.JobClass);
			CurrentStats.BaseLevel = Slot.BaseLevel;
			CurrentStats.JobLevel = Slot.JobLevel;
			CurrentStats.HP = Slot.HP;
			CurrentStats.MaxHP = Slot.MaxHP;
			CurrentStats.SP = Slot.SP;
			CurrentStats.MaxSP = Slot.MaxSP;
			break;
		}
	}
	if (SelectedCharName.IsEmpty() && CachedCharacterSlots.IsValidIndex(SlotIndex))
	{
		const FRagnarokCharacterSlotInfo& Slot = CachedCharacterSlots[SlotIndex];
		SelectedCharName = Slot.Name;
		SelectedSlotInfo = Slot;
		CurrentStats.JobClass = Slot.JobClass;
		CurrentStats.JobName = GetJobNameFromClass(Slot.JobClass);
		CurrentStats.BaseLevel = Slot.BaseLevel;
		CurrentStats.JobLevel = Slot.JobLevel;
		CurrentStats.HP = Slot.HP;
		CurrentStats.MaxHP = Slot.MaxHP;
		CurrentStats.SP = Slot.SP;
		CurrentStats.MaxSP = Slot.MaxSP;
	}

	// Pacote CH_SELECT_CHAR (0x0066) - 3 Bytes
	#pragma pack(push, 1)
	struct FCH_SelectCharPacket
	{
		uint16 PacketID = RagnarokPackets::HEADER_CH_SELECT_CHAR;
		uint8 Slot = 0;
	};
	#pragma pack(pop)

	FCH_SelectCharPacket Packet;
	Packet.Slot = static_cast<uint8>(SlotIndex);
	SendData(reinterpret_cast<const uint8*>(&Packet), sizeof(Packet));
	return true;
}

bool URagnarokNetworkSubsystem::SendMoveRequest(int32 TargetX, int32 TargetY)
{
	if (!CurrentSocket || CurrentState != ERagnarokNetworkState::ConnectedMap)
	{
		return false;
	}

	// Montagem do pacote CZ_REQUEST_MOVE (0x035F) - 5 Bytes
	#pragma pack(push, 1)
	struct FCZ_MovePacket
	{
		uint16 PacketID = RagnarokPackets::HEADER_CZ_REQUEST_MOVE;
		uint8 Coords[3] = { 0 };
	};
	#pragma pack(pop)

	FCZ_MovePacket Packet;
	Packet.Coords[0] = static_cast<uint8>((TargetX >> 2) & 0xFF);
	Packet.Coords[1] = static_cast<uint8>(((TargetX << 6) & 0xC0) | ((TargetY >> 4) & 0x3F));
	Packet.Coords[2] = static_cast<uint8>((TargetY << 4) & 0xF0);

	SendData(reinterpret_cast<const uint8*>(&Packet), sizeof(Packet));
	return true;
}

bool URagnarokNetworkSubsystem::SendRestart(uint8 Type)
{
	// CZ_RESTART (clif_parse_Restart, 0x00b2, 3 bytes): tipo no byte 2
	if (!CurrentSocket || CurrentState != ERagnarokNetworkState::ConnectedMap)
	{
		return false;
	}
	const uint8 Packet[3] = { 0xb2, 0x00, Type };
	SendData(Packet, sizeof(Packet));
	return true;
}

bool URagnarokNetworkSubsystem::SendChangeDirection(uint8 HeadDir, uint8 BodyDir)
{
	if (!CurrentSocket || CurrentState != ERagnarokNetworkState::ConnectedMap)
	{
		return false;
	}
	// CZ_CHANGE_DIRECTION (clif_parse_ChangeDir 0x0897, 5 bytes): headdir no byte 2, dir no byte 4 (DIR_NORTH 0 ... NORTHEAST 7)
	uint8 Packet[5] = { 0x97, 0x08, HeadDir, 0, BodyDir };
	SendData(Packet, sizeof(Packet));
	return true;
}

bool URagnarokNetworkSubsystem::SendActionRequest(int64 TargetID, uint8 ActionType)
{
	if (!CurrentSocket || CurrentState != ERagnarokNetworkState::ConnectedMap)
	{
		return false;
	}

	// Pacote CZ_REQUEST_ACT (0x088E / 0x0360) - 7 Bytes
	#pragma pack(push, 1)
	struct FCZ_ActionPacket
	{
		uint16 PacketID = RagnarokPackets::HEADER_CZ_REQUEST_ACT;
		uint32 TargetID = 0;
		uint8 ActionType = 0; // 0 = Ataque normal
	};
	#pragma pack(pop)

	FCZ_ActionPacket Packet;
	Packet.TargetID = static_cast<uint32>(TargetID);
	Packet.ActionType = ActionType;

	SendData(reinterpret_cast<const uint8*>(&Packet), sizeof(Packet));
	return true;
}

bool URagnarokNetworkSubsystem::SendUseSkillRequest(int32 SkillID, int32 SkillLevel, int64 TargetID)
{
	if (!CurrentSocket || CurrentState != ERagnarokNetworkState::ConnectedMap)
	{
		return false;
	}

	// Pacote CZ_USE_SKILL (0x0438 / 0x0113) - 10 Bytes
	#pragma pack(push, 1)
	struct FCZ_UseSkillPacket
	{
		uint16 PacketID = RagnarokPackets::HEADER_CZ_USE_SKILL; // 0x089b CZ_USE_SKILL (lv 2, id 4, alvo 6)
		uint16 SkillLevel = 1;
		uint16 SkillID = 0;
		uint32 TargetID = 0;
	};
	#pragma pack(pop)

	FCZ_UseSkillPacket Packet;
	Packet.SkillLevel = static_cast<uint16>(SkillLevel);
	Packet.SkillID = static_cast<uint16>(SkillID);
	Packet.TargetID = static_cast<uint32>(TargetID);

	SendData(reinterpret_cast<const uint8*>(&Packet), sizeof(Packet));
	return true;
}

bool URagnarokNetworkSubsystem::CreateCharacter(const FString& Name, uint8 Slot, int32 HairStyle, int32 HairColor, int32 JobClass)
{
	if (!CurrentSocket || CurrentState != ERagnarokNetworkState::ConnectedChar)
	{
		return false;
	}

	#pragma pack(push, 1)
	struct FCH_MakeCharPacket
	{
		uint16 PacketID = RagnarokPackets::HEADER_CH_MAKE_CHAR; // 0x0a39
		char Name[24] = { 0 };
		uint8 Slot = 0;
		uint16 HairColor = 0;
		uint16 HairStyle = 0;
		uint32 Job = 0;
		uint8 Sex = 0;
	};
	#pragma pack(pop)

	FCH_MakeCharPacket Packet;
	FTCHARToUTF8 NameUtf8(*Name);
	FMemory::Memcpy(Packet.Name, NameUtf8.Get(), FMath::Min((int32)sizeof(Packet.Name) - 1, NameUtf8.Length()));
	Packet.Slot = Slot;
	Packet.HairColor = static_cast<uint16>(HairColor);
	Packet.HairStyle = static_cast<uint16>(HairStyle);
	Packet.Job = static_cast<uint32>(JobClass);
	Packet.Sex = SavedSex;

	SendData(reinterpret_cast<const uint8*>(&Packet), sizeof(Packet));
	return true;
}

bool URagnarokNetworkSubsystem::DeleteCharacter(int32 CharID, const FString& Key)
{
	if (!CurrentSocket || CurrentState != ERagnarokNetworkState::ConnectedChar)
	{
		return false;
	}

	#pragma pack(push, 1)
	struct FCH_DeleteCharPacket
	{
		uint16 PacketID = RagnarokPackets::HEADER_CH_DELETE_CHAR; // 0x0068
		uint32 CID = 0;
		char Key[40] = { 0 };
	};
	#pragma pack(pop)

	FCH_DeleteCharPacket Packet;
	Packet.CID = static_cast<uint32>(CharID);
	FTCHARToUTF8 KeyUtf8(*Key);
	if (KeyUtf8.Length() > 0)
	{
		FMemory::Memcpy(Packet.Key, KeyUtf8.Get(), FMath::Min((int32)sizeof(Packet.Key) - 1, KeyUtf8.Length()));
	}

	SendData(reinterpret_cast<const uint8*>(&Packet), sizeof(Packet));
	return true;
}

bool URagnarokNetworkSubsystem::SendGlobalMessage(const FString& Message)
{
	if (!CurrentSocket || CurrentState != ERagnarokNetworkState::ConnectedMap)
	{
		return false;
	}

	FString Speaker = SelectedCharName.IsEmpty() ? TEXT("ThorTester") : SelectedCharName;
	FString FormattedText = FString::Printf(TEXT("%s : %s"), *Speaker, *Message);

	FTCHARToUTF8 Converter(*FormattedText);
	int32 TextLen = Converter.Length() + 1;
	uint16 PacketLen = static_cast<uint16>(4 + TextLen);

	TArray<uint8> Pkt;
	Pkt.SetNumUninitialized(PacketLen);
	*reinterpret_cast<uint16*>(Pkt.GetData()) = RagnarokPackets::HEADER_CZ_GLOBAL_MESSAGE; // 0x00f3
	*reinterpret_cast<uint16*>(Pkt.GetData() + 2) = PacketLen;
	FMemory::Memcpy(Pkt.GetData() + 4, Converter.Get(), TextLen);

	SendData(Pkt.GetData(), Pkt.Num());
	return true;
}

bool URagnarokNetworkSubsystem::SendStatusUp(int32 StatusType, int32 Amount)
{
	if (!CurrentSocket || CurrentState != ERagnarokNetworkState::ConnectedMap)
	{
		return false;
	}

	#pragma pack(push, 1)
	struct FCZ_StatusChangePacket
	{
		uint16 PacketID = RagnarokPackets::HEADER_CZ_STATUS_CHANGE; // 0x00bb
		uint16 StatusType = 0;
		uint8 Amount = 1;
	};
	#pragma pack(pop)

	FCZ_StatusChangePacket Packet;
	Packet.StatusType = static_cast<uint16>(StatusType);
	Packet.Amount = static_cast<uint8>(FMath::Clamp(Amount, 1, 255));

	SendData(reinterpret_cast<const uint8*>(&Packet), sizeof(Packet));
	return true;
}

bool URagnarokNetworkSubsystem::UseItem(int32 ItemIndex)
{
	if (!CurrentSocket || CurrentState != ERagnarokNetworkState::ConnectedMap)
	{
		return false;
	}

	#pragma pack(push, 1)
	struct FCZ_UseItemPacket
	{
		uint16 PacketID = RagnarokPackets::HEADER_CZ_USE_ITEM; // 0x0439
		uint16 Index = 0;
		uint32 TargetAID = 0;
	};
	#pragma pack(pop)

	FCZ_UseItemPacket Packet;
	Packet.Index = static_cast<uint16>(ItemIndex);
	Packet.TargetAID = SavedAccountID;

	SendData(reinterpret_cast<const uint8*>(&Packet), sizeof(Packet));
	return true;
}

bool URagnarokNetworkSubsystem::EquipItem(int32 ItemIndex, int32 WearLocation)
{
	if (!CurrentSocket || CurrentState != ERagnarokNetworkState::ConnectedMap)
	{
		return false;
	}

	if (WearLocation == 0)
	{
		for (const FRagnarokInventoryItem& Item : InventoryItems)
		{
			if (Item.Index == ItemIndex)
			{
				WearLocation = (Item.Location != 0) ? Item.Location : Item.EquipLocation;
				break;
			}
		}
	}

	if (WearLocation == 0)
	{
		UE_LOG(LogTemp, Warning, TEXT("[RagnarokNetwork] EquipItem: could not determine WearLocation for ItemIndex %d"), ItemIndex);
		return false;
	}

	#pragma pack(push, 1)
	struct FCZ_WearEquipPacket
	{
		uint16 PacketID = RagnarokPackets::HEADER_CZ_REQ_WEAR_EQUIP; // 0x0998
		uint16 Index = 0;
		uint32 WearLocation = 0;
	};
	#pragma pack(pop)

	FCZ_WearEquipPacket Packet;
	Packet.Index = static_cast<uint16>(ItemIndex);
	Packet.WearLocation = static_cast<uint32>(WearLocation);

	SendData(reinterpret_cast<const uint8*>(&Packet), sizeof(Packet));
	return true;
}

bool URagnarokNetworkSubsystem::UnequipItem(int32 ItemIndex)
{
	if (!CurrentSocket || CurrentState != ERagnarokNetworkState::ConnectedMap)
	{
		return false;
	}

	#pragma pack(push, 1)
	struct FCZ_TakeOffEquipPacket
	{
		uint16 PacketID = RagnarokPackets::HEADER_CZ_REQ_TAKEOFF_EQUIP; // 0x00ab
		uint16 Index = 0;
	};
	#pragma pack(pop)

	FCZ_TakeOffEquipPacket Packet;
	Packet.Index = static_cast<uint16>(ItemIndex);

	SendData(reinterpret_cast<const uint8*>(&Packet), sizeof(Packet));
	return true;
}

bool URagnarokNetworkSubsystem::PickUpItem(int64 ItemGID)
{
	if (!CurrentSocket || CurrentState != ERagnarokNetworkState::ConnectedMap)
	{
		return false;
	}

	#pragma pack(push, 1)
	struct FCZ_PickUpPacket
	{
		uint16 PacketID = RagnarokPackets::HEADER_CZ_ITEM_PICKUP; // 0x0933
		uint32 ItemAID = 0;
	};
	#pragma pack(pop)

	FCZ_PickUpPacket Packet;
	Packet.ItemAID = static_cast<uint32>(ItemGID);

	SendData(reinterpret_cast<const uint8*>(&Packet), sizeof(Packet));
	return true;
}

bool URagnarokNetworkSubsystem::UseSkill(int32 SkillID, int32 SkillLevel, int64 TargetID, int32 TargetX, int32 TargetY)
{
	if (!CurrentSocket || CurrentState != ERagnarokNetworkState::ConnectedMap)
	{
		return false;
	}

	if (TargetID > 0)
	{
		return SendUseSkillRequest(SkillID, SkillLevel, TargetID);
	}
	else
	{
		#pragma pack(push, 1)
		struct FCZ_UseSkillToPosPacket
		{
			uint16 PacketID = 0x0af4; // CZ_USE_SKILL_TOGROUND (clif_parse_UseSkillToPos 0x0af4, len: 11 no PACKETVER 20250402)
			uint16 SkillLevel = 1;
			uint16 SkillID = 0;
			uint16 X = 0;
			uint16 Y = 0;
			uint8 MoreInfo = 0;
		};
		#pragma pack(pop)

		FCZ_UseSkillToPosPacket Packet;
		Packet.SkillLevel = static_cast<uint16>(SkillLevel);
		Packet.SkillID = static_cast<uint16>(SkillID);
		Packet.X = static_cast<uint16>(TargetX);
		Packet.Y = static_cast<uint16>(TargetY);
		Packet.MoreInfo = 0;
		SendData(reinterpret_cast<const uint8*>(&Packet), sizeof(Packet));
		return true;
	}
}

bool URagnarokNetworkSubsystem::SendSkillUp(int32 SkillID)
{
	if (!CurrentSocket || CurrentState != ERagnarokNetworkState::ConnectedMap)
	{
		return false;
	}

	#pragma pack(push, 1)
	struct FCZ_SkillUpPacket
	{
		uint16 PacketID = RagnarokPackets::HEADER_CZ_UPGRADE_SKILLLEVEL; // 0x0112
		uint16 SkillID = 0;
	};
	#pragma pack(pop)

	FCZ_SkillUpPacket Packet;
	Packet.SkillID = static_cast<uint16>(SkillID);

	SendData(reinterpret_cast<const uint8*>(&Packet), sizeof(Packet));
	return true;
}

FString URagnarokNetworkSubsystem::GetJobNameFromClass(int32 JobClass)
{
	// IDs do enum e_job (rathena/src/common/mmo.hpp); nomes do bRO
	static const TCHAR* Base[] = {
		TEXT("Aprendiz"), TEXT("Espadachim"), TEXT("Mago"), TEXT("Arqueiro"), TEXT("Noviço"), TEXT("Mercador"), TEXT("Gatuno"),
		TEXT("Cavaleiro"), TEXT("Sacerdote"), TEXT("Bruxo"), TEXT("Ferreiro"), TEXT("Caçador"), TEXT("Mercenário"),
		TEXT("Cavaleiro"), TEXT("Templário"), TEXT("Monge"), TEXT("Sábio"), TEXT("Arruaceiro"), TEXT("Alquimista"),
		TEXT("Bardo"), TEXT("Odalisca"), TEXT("Templário"), TEXT("Casamento"), TEXT("Super Aprendiz"), TEXT("Justiceiro"),
		TEXT("Ninja"), TEXT("Natal") };
	// 4001..4022: transclasses (a 1a classe transcendental leva "T.")
	static const TCHAR* Trans[] = {
		TEXT("Aprendiz T."), TEXT("Espadachim T."), TEXT("Mago T."), TEXT("Arqueiro T."), TEXT("Noviço T."), TEXT("Mercador T."),
		TEXT("Gatuno T."), TEXT("Lorde"), TEXT("Sumo Sacerdote"), TEXT("Arquimago"), TEXT("Mestre-Ferreiro"),
		TEXT("Atirador de Elite"), TEXT("Algoz"), TEXT("Lorde"), TEXT("Paladino"), TEXT("Mestre"), TEXT("Professor"),
		TEXT("Desordeiro"), TEXT("Criador"), TEXT("Menestrel"), TEXT("Cigana"), TEXT("Paladino") };
	if (JobClass >= 0 && JobClass < UE_ARRAY_COUNT(Base))
	{
		return Base[JobClass];
	}
	if (JobClass >= 4001 && JobClass < 4001 + UE_ARRAY_COUNT(Trans))
	{
		return Trans[JobClass - 4001];
	}
	if (JobClass >= 4023 && JobClass <= 4044) // bebes: 4023 = Aprendiz ... na mesma ordem das classes basicas
	{
		return FString::Printf(TEXT("%s Bebê"), Base[JobClass - 4023]);
	}
	switch (JobClass)
	{
		case 4045: return TEXT("Super Aprendiz Bebê");
		case 4046: return TEXT("Taekwon");
		case 4047: case 4048: return TEXT("Mestre Taekwon");
		case 4049: return TEXT("Espiritualista");
		default: return FString::Printf(TEXT("Classe %d"), JobClass);
	}
}

bool URagnarokNetworkSubsystem::ContactNpc(int64 NpcID)
{
	if (!CurrentSocket || CurrentState != ERagnarokNetworkState::ConnectedMap)
	{
		return false;
	}

	#pragma pack(push, 1)
	struct FCZ_ContactNpcPacket
	{
		uint16 PacketID = RagnarokPackets::HEADER_CZ_CONTACTNPC; // 0x0090
		uint32 AID = 0;
		uint8 Type = 1;
	};
	#pragma pack(pop)

	FCZ_ContactNpcPacket Packet;
	Packet.AID = static_cast<uint32>(NpcID);
	Packet.Type = 1;

	SendData(reinterpret_cast<const uint8*>(&Packet), sizeof(Packet));
	return true;
}

bool URagnarokNetworkSubsystem::NextNpcDialog(int64 NpcID)
{
	if (!CurrentSocket || CurrentState != ERagnarokNetworkState::ConnectedMap)
	{
		return false;
	}

	#pragma pack(push, 1)
	struct FCZ_NextScriptPacket
	{
		uint16 PacketID = RagnarokPackets::HEADER_CZ_REQ_NEXT_SCRIPT; // 0x00b9
		uint32 NpcID = 0;
	};
	#pragma pack(pop)

	FCZ_NextScriptPacket Packet;
	Packet.NpcID = static_cast<uint32>(NpcID);

	SendData(reinterpret_cast<const uint8*>(&Packet), sizeof(Packet));
	return true;
}

bool URagnarokNetworkSubsystem::SelectNpcMenu(int64 NpcID, int32 MenuIndex)
{
	if (!CurrentSocket || CurrentState != ERagnarokNetworkState::ConnectedMap)
	{
		return false;
	}

	#pragma pack(push, 1)
	struct FCZ_ChooseMenuPacket
	{
		uint16 PacketID = RagnarokPackets::HEADER_CZ_CHOOSE_MENU; // 0x00b8
		uint32 NpcID = 0;
		uint8 Select = 0;
	};
	#pragma pack(pop)

	FCZ_ChooseMenuPacket Packet;
	Packet.NpcID = static_cast<uint32>(NpcID);
	Packet.Select = static_cast<uint8>(MenuIndex);

	SendData(reinterpret_cast<const uint8*>(&Packet), sizeof(Packet));
	return true;
}

bool URagnarokNetworkSubsystem::CloseNpcDialog(int64 NpcID)
{
	if (!CurrentSocket || CurrentState != ERagnarokNetworkState::ConnectedMap)
	{
		return false;
	}

	#pragma pack(push, 1)
	struct FCZ_CloseDialogPacket
	{
		uint16 PacketID = RagnarokPackets::HEADER_CZ_CLOSE_DIALOG; // 0x0146
		uint32 NpcID = 0;
	};
	#pragma pack(pop)

	FCZ_CloseDialogPacket Packet;
	Packet.NpcID = static_cast<uint32>(NpcID);

	SendData(reinterpret_cast<const uint8*>(&Packet), sizeof(Packet));
	return true;
}

void URagnarokNetworkSubsystem::SendData(const uint8* Data, int32 Size)
{
	if (!CurrentSocket || !Data || Size <= 0)
	{
		return;
	}
	if (bLogPackets && Size >= 2)
	{
		UE_LOG(LogTemp, Log, TEXT("[RagnarokNetwork] -> 0x%04x (%d bytes) conexao %d"), *reinterpret_cast<const uint16*>(Data), Size, (int32)CurrentSocket->GetConnectionState());
	}

	// O connect e nao-bloqueante: ate o TCP fechar o handshake (ou com fila pendente) os bytes vao para a fila,
	// que o Tick esvazia. Sem isso o CA_LOGIN, enviado logo apos o Connect, se perdia.
	if (CurrentSocket->GetConnectionState() != SCS_Connected || PendingSend.Num() > 0)
	{
		PendingSend.Append(Data, Size);
		return;
	}
	int32 BytesSent = 0;
	if (!CurrentSocket->Send(Data, Size, BytesSent))
	{
		BytesSent = 0;
	}
	if (BytesSent < Size)
	{
		PendingSend.Append(Data + BytesSent, Size - BytesSent);
	}
}

void URagnarokNetworkSubsystem::FlushPendingSend()
{
	if (!CurrentSocket || PendingSend.IsEmpty())
	{
		return;
	}
	const ESocketConnectionState State = CurrentSocket->GetConnectionState();
	if (State == SCS_ConnectionError)
	{
		UE_LOG(LogTemp, Warning, TEXT("[RagnarokNetwork] Falha de conexao (estado %d)"), (int32)CurrentState);
		PendingSend.Empty();
		Disconnect();
		return;
	}
	if (State != SCS_Connected)
	{
		return;
	}
	int32 BytesSent = 0;
	if (CurrentSocket->Send(PendingSend.GetData(), PendingSend.Num(), BytesSent) && BytesSent > 0)
	{
		PendingSend.RemoveAt(0, BytesSent);
	}
}

void URagnarokNetworkSubsystem::Disconnect()
{
	if (CurrentSocket)
	{
		CurrentSocket->Close();
		ISocketSubsystem::Get(PLATFORM_SOCKETSUBSYSTEM)->DestroySocket(CurrentSocket);
		CurrentSocket = nullptr;
		CurrentState = ERagnarokNetworkState::Disconnected;
		ReceiveBuffer.Empty();
		const FString Reason = DisconnectReason.IsEmpty() ? FString(TEXT("Conexão encerrada pelo cliente.")) : DisconnectReason;
		DisconnectReason.Reset();
		OnNetworkDisconnected.Broadcast(Reason);
	}
}

void URagnarokNetworkSubsystem::ProcessIncomingData()
{
	if (!CurrentSocket)
	{
		return;
	}

	uint32 PendingDataSize = 0;
	while (CurrentSocket->HasPendingData(PendingDataSize) && PendingDataSize > 0)
	{
		int32 OldSize = ReceiveBuffer.Num();
		ReceiveBuffer.AddUninitialized(PendingDataSize);
		int32 BytesRead = 0;
		if (CurrentSocket->Recv(ReceiveBuffer.GetData() + OldSize, PendingDataSize, BytesRead, ESocketReceiveFlags::None) && BytesRead > 0)
		{
			ReceiveBuffer.SetNum(OldSize + BytesRead);
		}
		else
		{
			ReceiveBuffer.SetNum(OldSize);
			break;
		}
	}

	// Máquina de framing de pacotes via tabela exata do rAthena
	while (ReceiveBuffer.Num() >= 2)
	{
		// No Char-Server, o primeiro dado enviado pelo rAthena é um eco de 4 bytes contendo o AccountID
		if (CurrentState == ERagnarokNetworkState::ConnectedChar && ReceiveBuffer.Num() >= 4)
		{
			uint32 PossibleAID = *reinterpret_cast<const uint32*>(ReceiveBuffer.GetData());
			if (PossibleAID == SavedAccountID)
			{
				ReceiveBuffer.RemoveAt(0, 4);
				continue;
			}
		}

		uint16 PacketID = *reinterpret_cast<const uint16*>(ReceiveBuffer.GetData());
		const int16* FromTable = PacketLengths.Find(PacketID);
		int16 ExpectedLen = FromTable ? *FromTable : RagnarokPackets::GetPacketLength(PacketID);

		if (ExpectedLen == 0)
		{
			// Pacote não reconhecido ou dessincronizado no fluxo TCP
			if (LastGoodPacketID != 0)
			{
				UE_LOG(LogTemp, Warning, TEXT("[RagnarokNetwork] Pacote desconhecido 0x%04x no estado %d (ultimo valido: 0x%04x com %d bytes). Descartando 1 byte."), PacketID, (int32)CurrentState, LastGoodPacketID, LastGoodPacketLen);
				LastGoodPacketID = 0;
			}
			ReceiveBuffer.RemoveAt(0, 1);
			continue;
		}

		int32 ActualLen = ExpectedLen;
		if (ExpectedLen == -1) // Comprimento dinâmico
		{
			if (ReceiveBuffer.Num() < 4)
			{
				break; // Aguarda os 4 bytes para saber o comprimento total
			}
			ActualLen = *reinterpret_cast<const uint16*>(ReceiveBuffer.GetData() + 2);
			if (ActualLen < 4 || ActualLen > 65535)
			{
				UE_LOG(LogTemp, Error, TEXT("[RagnarokNetwork] Tamanho de pacote dinâmico inválido (%d) para 0x%04x"), ActualLen, PacketID);
				ReceiveBuffer.Empty();
				break;
			}
		}

		if (ReceiveBuffer.Num() < ActualLen)
		{
			// Pacote incompleto no buffer TCP: aguarda próxima iteração de rede
			break;
		}

		const uint8* PacketData = ReceiveBuffer.GetData();
		const uint32 GenerationBefore = ReceiveGeneration;
		if (bLogPackets)
		{
			UE_LOG(LogTemp, Log, TEXT("[RagnarokNetwork] <- 0x%04x (%d bytes) estado %d"), PacketID, ActualLen, (int32)CurrentState);
		}

		switch (PacketID)
		{
			case RagnarokPackets::HEADER_AC_ACCEPT_LOGIN:
				HandleLoginAccept(PacketData, ActualLen);
				break;
			case RagnarokPackets::HEADER_AC_REFUSE_LOGIN:
			case RagnarokPackets::HEADER_SC_NOTIFY_BAN:
				HandleNotifyBan(PacketData, ActualLen);
				break;
			case RagnarokPackets::HEADER_HC_ACCEPT_ENTER:
				HandleCharacterList(PacketData, ActualLen);
				break;
			case RagnarokPackets::HEADER_HC_NOTIFY_ZONESVR:
				HandleZoneServerNotify(PacketData, ActualLen);
				break;
			case RagnarokPackets::HEADER_ZC_ACCEPT_ENTER:
				HandleMapAcceptEnter(PacketData, ActualLen);
				break;
			case 0x0091: // ZC_NPCACK_MAPMOVE
				HandleMapMove(PacketData, ActualLen);
				break;
			case RagnarokPackets::HEADER_ZC_IDLE_UNIT:
			case RagnarokPackets::HEADER_ZC_SPAWN_UNIT:
			case RagnarokPackets::HEADER_ZC_UNIT_WALKING:
				HandleUnitAppear(PacketData, ActualLen, PacketID);
				break;
			case RagnarokPackets::HEADER_ZC_NOTIFY_PLAYERMOVE:
				HandlePlayerMove(PacketData, ActualLen);
				break;
			case RagnarokPackets::HEADER_ZC_NOTIFY_MOVE:
				HandleNotifyMove(PacketData, ActualLen);
				break;
			case RagnarokPackets::HEADER_ZC_STOPMOVE:
				HandleStopMove(PacketData, ActualLen);
				break;
			case 0x009c: // ZC_CHANGE_DIRECTION
				HandleChangeDirection(PacketData, ActualLen);
				break;
			case 0x00c4: // ZC_SELECT_DEALTYPE
			case 0x0b77: // ZC_PC_PURCHASE_ITEMLIST (PACKETVER >= 20210203)
			case 0x00c7: // ZC_PC_SELL_ITEMLIST
			case 0x00ca: // ZC_PC_PURCHASE_RESULT
			case 0x00cb: // ZC_PC_SELL_RESULT
				HandleShopPacket(PacketData, ActualLen, PacketID);
				break;
			case 0x0b08: // ZC_INVENTORY_START
			case 0x0b0b: // ZC_INVENTORY_END
			case 0x0b44: // ZC_ADD_ITEM_TO_STORE (PACKETVER >= 20200916)
			case 0x00f6: // ZC_DELETE_ITEM_FROM_STORE
			case 0x00f2: // ZC_NOTIFY_STOREITEM_COUNTINFO
			case 0x00f8: // ZC_CLOSE_STORE
				HandleStoragePacket(PacketData, ActualLen, PacketID);
				break;
			case 0x00fa: // ZC_ACK_MAKE_GROUP
			case 0x0ae4: // ZC_ADD_MEMBER_TO_GROUP (PACKETVER >= 20171207)
			case 0x0ae5: // ZC_GROUP_LIST
			case 0x02c6: // ZC_PARTY_JOIN_REQ
			case 0x02c5: // ZC_PARTY_JOIN_REQ_ACK
			case 0x0105: // ZC_DELETE_MEMBER_FROM_GROUP
			case 0x080e: // ZC_NOTIFY_HP_TO_GROUPM
			case 0x0107: // ZC_NOTIFY_POSITION_TO_GROUPM
			case 0x0ab2: // ZC_GROUP_ISALIVE
				HandlePartyPacket(PacketData, ActualLen, PacketID);
				break;
			case 0x008d: // ZC_NOTIFY_CHAT (fala publica de outro jogador)
			case 0x0109: // ZC_NOTIFY_CHAT_PARTY
			case 0x09df: // ZC_ACK_WHISPER (resultado do cochicho)
			case 0x00c0: // ZC_EMOTION
				HandleChatExtras(PacketData, ActualLen, PacketID);
				break;
			case 0x0977: // ZC_HP_INFO <id>.L <pv>.L <pv max>.L
				if (ActualLen >= 14)
				{
					OnMonsterHp.Broadcast(*reinterpret_cast<const uint32*>(PacketData + 2), *reinterpret_cast<const int32*>(PacketData + 6), *reinterpret_cast<const int32*>(PacketData + 10));
				}
				break;
			case 0x0148: // ZC_RESURRECTION
				HandleResurrection(PacketData, ActualLen);
				break;
			case 0x0229: // ZC_STATE_CHANGE (effectState de 4 bytes)
			case 0x0119: // ZC_STATE_CHANGE (versao antiga, 2 bytes)
				HandleOptionChange(PacketData, ActualLen, PacketID);
				break;
			case RagnarokPackets::HEADER_ZC_NOTIFY_VANISH:
				HandleUnitVanish(PacketData, ActualLen);
				break;
			case RagnarokPackets::HEADER_ZC_NOTIFY_ACT:
				HandleActionNotify(PacketData, ActualLen);
				break;
			case RagnarokPackets::HEADER_ZC_PAR_CHANGE:
			case RagnarokPackets::HEADER_ZC_LONGPAR_CHANGE:
			case RagnarokPackets::HEADER_ZC_LONGLONGPAR_CHANGE:
			case RagnarokPackets::HEADER_ZC_COUPLESTATUS:
				HandleParamChange(PacketData, ActualLen, PacketID);
				break;
			case RagnarokPackets::HEADER_ZC_SPRITE_CHANGE:
				HandleSpriteChange(PacketData, ActualLen);
				break;
			case RagnarokPackets::HEADER_ZC_NOTIFY_CHAT:
			case RagnarokPackets::HEADER_ZC_DISP_MESSAGE:
			case RagnarokPackets::HEADER_ZC_DISP_MESSAGE2:
			case RagnarokPackets::HEADER_ZC_WHISPER:
			case RagnarokPackets::HEADER_ZC_WHISPER2:
				HandleChatMessage(PacketData, ActualLen, PacketID);
				break;
			case RagnarokPackets::HEADER_ZC_STATUS:
			case RagnarokPackets::HEADER_ZC_STATUS_VALUES:
			case RagnarokPackets::HEADER_ZC_STATUS_CHANGE_ACK:
				HandleStatus(PacketData, ActualLen, PacketID);
				break;
			case RagnarokPackets::HEADER_ZC_ITEM_FALL_ENTRY:
			case RagnarokPackets::HEADER_ZC_ITEM_FALL_ENTRY2:
				HandleItemFall(PacketData, ActualLen);
				break;
			case 0x0b20: // ZC_SHORTCUT_KEY_LIST
				HandleHotkeys(PacketData, ActualLen);
				break;
			case 0x0117: // ZC_NOTIFY_GROUNDSKILL
			case 0x01de: // ZC_NOTIFY_SKILL
			case 0x09cb: // ZC_USE_SKILL
			case 0x0b1a: // ZC_USESKILL_ACK
			case 0x0110: // ZC_ACK_TOUSESKILL
				HandleSkillResult(PacketData, ActualLen, PacketID);
				break;
			case 0x0b41: // ZC_ITEM_PICKUP_ACK (20211103)
				HandleItemPickupAck(PacketData, ActualLen);
				break;
			case 0x00af: // ZC_ITEM_THROW_ACK: index +2, count +4
				RemoveInventoryAmount(*reinterpret_cast<const uint16*>(PacketData + 2), *reinterpret_cast<const uint16*>(PacketData + 4));
				break;
			case 0x07fa: // ZC_DELETE_ITEM_FROM_BODY: deleteType +2, index +4, count +6
				RemoveInventoryAmount(*reinterpret_cast<const uint16*>(PacketData + 4), *reinterpret_cast<const int16*>(PacketData + 6));
				break;
			case 0x009d: // ZC_ITEM_ENTRY: item que ja estava no chao ao entrar na visao
				HandleItemEntry(PacketData, ActualLen);
				break;
			case RagnarokPackets::HEADER_ZC_ITEM_DISAPPEAR:
				HandleItemDisappear(PacketData, ActualLen);
				break;
			case RagnarokPackets::HEADER_ZC_SAY_DIALOG:
			case 0x0972:
				HandleNpcDialog(PacketData, ActualLen);
				break;
			case RagnarokPackets::HEADER_ZC_WAIT_DIALOG:
			case 0x0973:
				HandleNpcWait(PacketData, ActualLen);
				break;
			case RagnarokPackets::HEADER_ZC_CLOSE_DIALOG:
				HandleNpcClose(PacketData, ActualLen);
				break;
			case RagnarokPackets::HEADER_ZC_MENU_LIST:
				HandleNpcMenu(PacketData, ActualLen);
				break;
			case RagnarokPackets::HEADER_ZC_SKILL_INFO_LIST:
			case RagnarokPackets::HEADER_ZC_SKILLINFO_LIST_RE:
			case RagnarokPackets::HEADER_ZC_ADD_SKILL:
			case RagnarokPackets::HEADER_ZC_SKILLINFO_UPDATE2:
				HandleSkillList(PacketData, ActualLen);
				break;
			case RagnarokPackets::HEADER_ZC_NORMAL_ITEMLIST:
			case RagnarokPackets::HEADER_ZC_NORMAL_ITEMLIST_OLD:
				HandleInventoryList(PacketData, ActualLen, false);
				break;
			case RagnarokPackets::HEADER_ZC_EQUIPMENT_ITEMLIST:
			case RagnarokPackets::HEADER_ZC_EQUIPMENT_ITEMLIST_OLD:
				HandleInventoryList(PacketData, ActualLen, true);
				break;
			case RagnarokPackets::HEADER_ZC_NOTIFY_EFFECT2:
				HandleEffect(PacketData, ActualLen);
				break;
			case RagnarokPackets::HEADER_ZC_MSG_STATE_CHANGE3:
			case RagnarokPackets::HEADER_ZC_MSG_STATE_CHANGE:
			case RagnarokPackets::HEADER_ZC_EFST_SET_ENTER:
				HandleStateChange(PacketData, ActualLen, PacketID);
				break;
			case RagnarokPackets::HEADER_ZC_SKILL_ENTRY5:
				HandleSkillUnitAppear(PacketData, ActualLen);
				break;
			case RagnarokPackets::HEADER_ZC_SKILL_DISAPPEAR:
				HandleSkillUnitDisappear(PacketData, ActualLen);
				break;
			case RagnarokPackets::HEADER_ZC_NOTIFY_EFFECT:
				HandleNotifyEffect(PacketData, ActualLen);
				break;
			case RagnarokPackets::HEADER_ZC_REQ_WEAR_EQUIP_ACK:
			case RagnarokPackets::HEADER_ZC_REQ_TAKEOFF_EQUIP_ACK:
				HandleEquipAck(PacketData, ActualLen, PacketID);
				break;
			case RagnarokPackets::HEADER_ZC_ATTACK_FAILURE_FOR_DISTANCE:
				HandleAttackOutOfRange(PacketData, ActualLen);
				break;
			case RagnarokPackets::HEADER_ZC_SKILLINFO_UPDATE:
				HandleSkillList(PacketData, ActualLen);
				break;
			default:
				// Pacote conhecido mas sem tratamento visual imediato (ex.: slots, ticks, ping)
				break;
		}

		LastGoodPacketID = PacketID;
		LastGoodPacketLen = ActualLen;
		if (ReceiveGeneration != GenerationBefore || ReceiveBuffer.Num() < ActualLen || !CurrentSocket)
		{
			// O handler trocou de servidor ou desconectou (Disconnect/ConnectToPort limpam o buffer): o resto e
			// do socket antigo. Sem esta checagem o RemoveAt abaixo estourava (crash de 10/10 01:35)
			break;
		}
		ReceiveBuffer.RemoveAt(0, ActualLen);
	}
}

void URagnarokNetworkSubsystem::HandleLoginAccept(const uint8* PacketData, int32 PacketSize)
{
	SavedLoginID1 = *reinterpret_cast<const uint32*>(PacketData + 4);
	SavedAccountID = *reinterpret_cast<const uint32*>(PacketData + 8);
	SavedLoginID2 = *reinterpret_cast<const uint32*>(PacketData + 12);
	SavedSex = PacketData[46];

	// Sub-estrutura do primeiro Char-Server inicia no offset 64
	if (PacketSize >= 64 + 160)
	{
		uint16 CharPort = *reinterpret_cast<const uint16*>(PacketData + 68);
		SavedCharPort = (CharPort != 0) ? CharPort : CHAR_SERVER_PORT;
	}

	OnLoginResult.Broadcast(true, TEXT("Login aceito com sucesso pelo rAthena."));

	// Transição automática para o Char-Server
	if (ConnectToPort(SavedHost, SavedCharPort, ERagnarokNetworkState::ConnectingChar))
	{
		// Envia CH_ENTER (0x0065) - 17 Bytes
		#pragma pack(push, 1)
		struct FCH_EnterPacket
		{
			uint16 PacketID = RagnarokPackets::HEADER_CH_ENTER;
			uint32 AccountID;
			uint32 LoginID1;
			uint32 LoginID2;
			uint16 Pad = 0;
			uint8 Sex;
		};
		#pragma pack(pop)

		FCH_EnterPacket EnterPacket;
		EnterPacket.AccountID = SavedAccountID;
		EnterPacket.LoginID1 = SavedLoginID1;
		EnterPacket.LoginID2 = SavedLoginID2;
		EnterPacket.Sex = SavedSex;

		SendData(reinterpret_cast<const uint8*>(&EnterPacket), sizeof(EnterPacket));
		CurrentState = ERagnarokNetworkState::ConnectedChar;
	}
}

void URagnarokNetworkSubsystem::HandleNotifyBan(const uint8* PacketData, int32 PacketSize)
{
	// SC_NOTIFY_BAN (0x0081, 3 bytes: id + codigo), clif_authfail_fd do rAthena. Pode chegar em qualquer servidor,
	// inclusive jogando (conta aberta em outro lugar): nao e "login recusado" (coord 006)
	const uint8 Code = PacketSize >= 3 ? PacketData[2] : 0;
	switch (Code)
	{
	case 1: DisconnectReason = TEXT("Servidor fechado para manutenção."); break;
	case 2: DisconnectReason = TEXT("Alguém entrou com esta conta em outro lugar."); break;
	case 3: DisconnectReason = TEXT("Desconectado por atraso na conexão."); break;
	case 4: DisconnectReason = TEXT("O servidor está cheio."); break;
	case 8: DisconnectReason = TEXT("O servidor ainda reconhece a conexão anterior. Tente de novo em instantes."); break;
	case 15: DisconnectReason = TEXT("Você foi desconectado por um GM."); break;
	default: DisconnectReason = FString::Printf(TEXT("Desconectado pelo servidor (código %d)."), Code); break;
	}
	if (CurrentState != ERagnarokNetworkState::ConnectedMap && CurrentState != ERagnarokNetworkState::ConnectedChar)
	{
		OnLoginResult.Broadcast(false, DisconnectReason);
	}
	Disconnect();
}

void URagnarokNetworkSubsystem::HandleLoginRefuse(const uint8* PacketData, int32 PacketSize)
{
	OnLoginResult.Broadcast(false, TEXT("Login recusado pelo servidor."));
	Disconnect();
}

void URagnarokNetworkSubsystem::HandleCharacterList(const uint8* PacketData, int32 PacketSize)
{
	// PACKET_HC_ACCEPT_ENTER: Cabeçalho de 27 bytes seguido por N structs CHARACTER_INFO (175 bytes cada)
	static constexpr int32 HEADER_SIZE = 27;
	static constexpr int32 CHAR_INFO_SIZE = 175;

	TArray<FRagnarokCharacterSlotInfo> Slots;
	int32 AvailableBytes = PacketSize - HEADER_SIZE;
	int32 NumChars = AvailableBytes / CHAR_INFO_SIZE;

	for (int32 i = 0; i < NumChars; ++i)
	{
		const uint8* Ptr = PacketData + HEADER_SIZE + (i * CHAR_INFO_SIZE);
		FRagnarokCharacterSlotInfo Info;
		Info.GID = *reinterpret_cast<const uint32*>(Ptr + 0);
		Info.JobLevel = *reinterpret_cast<const int32*>(Ptr + 24);
		Info.HP = *reinterpret_cast<const int64*>(Ptr + 50);
		Info.MaxHP = *reinterpret_cast<const int64*>(Ptr + 58);
		Info.SP = *reinterpret_cast<const int64*>(Ptr + 66);
		Info.MaxSP = *reinterpret_cast<const int64*>(Ptr + 74);
		Info.JobClass = *reinterpret_cast<const int16*>(Ptr + 84);
		Info.HairStyle = *reinterpret_cast<const int16*>(Ptr + 86);
		Info.Weapon = *reinterpret_cast<const int16*>(Ptr + 90);
		Info.BaseLevel = *reinterpret_cast<const int16*>(Ptr + 92);
		Info.HeadTop = *reinterpret_cast<const int16*>(Ptr + 96);
		Info.Shield = *reinterpret_cast<const int16*>(Ptr + 98);
		Info.HeadMid = *reinterpret_cast<const int16*>(Ptr + 100);
		Info.HeadBottom = *reinterpret_cast<const int16*>(Ptr + 102);
		Info.ClothesColor = *reinterpret_cast<const int16*>(Ptr + 106);

		char NameBuf[25] = { 0 };
		FMemory::Memcpy(NameBuf, Ptr + 108, 24);
		Info.Name = UTF8_TO_TCHAR(NameBuf);

		Info.Slot = Ptr[138]; // CharNum
		Info.HairColor = Ptr[139];
		Info.Robe = *reinterpret_cast<const int32*>(Ptr + 162);
		Info.Sex = Ptr[174];

		Slots.Add(Info);
	}

	CachedCharacterSlots = Slots;
	OnCharacterList.Broadcast(Slots);
}

void URagnarokNetworkSubsystem::HandleZoneServerNotify(const uint8* PacketData, int32 PacketSize)
{
	PendingCharID = *reinterpret_cast<const uint32*>(PacketData + 2);

	char MapBuf[17] = { 0 };
	FMemory::Memcpy(MapBuf, PacketData + 6, 16);
	PendingMapName = UTF8_TO_TCHAR(MapBuf);

	uint16 MapPort = *reinterpret_cast<const uint16*>(PacketData + 26);
	int32 TargetMapPort = (MapPort != 0) ? MapPort : MAP_SERVER_PORT;

	// Transição para o Map-Server
	if (ConnectToPort(SavedHost, TargetMapPort, ERagnarokNetworkState::ConnectingMap))
	{
		// Pacote CZ_ENTER (0x0888) - 19 Bytes
		#pragma pack(push, 1)
		struct FCZ_EnterPacket
		{
			uint16 PacketID = RagnarokPackets::HEADER_CZ_ENTER;
			uint32 AccountID;
			uint32 CharID;
			uint32 LoginID1;
			uint32 ClientTick;
			uint8 Sex;
		};
		#pragma pack(pop)

		FCZ_EnterPacket EnterMap;
		EnterMap.AccountID = SavedAccountID;
		EnterMap.CharID = PendingCharID;
		EnterMap.LoginID1 = SavedLoginID1;
		EnterMap.ClientTick = static_cast<uint32>(FPlatformTime::Cycles());
		EnterMap.Sex = SavedSex;

		SendData(reinterpret_cast<const uint8*>(&EnterMap), sizeof(EnterMap));
		// Continua "ConnectingMap" ate o ZC_ACCEPT_ENTER: o rAthena exige que o primeiro pacote da conexao seja so
		// o de entrada (cliques no chao durante a conexao iam juntos, 19+5+5 bytes, e o servidor derrubava).
	}
}

void URagnarokNetworkSubsystem::HandleMapAcceptEnter(const uint8* PacketData, int32 PacketSize)
{
	CurrentState = ERagnarokNetworkState::ConnectedMap; // so agora o cliente pode mandar pedidos de jogo
	int32 CellX = 0, CellY = 0;
	uint8 Direction = 0;
	RagnarokPackets::UnpackCellCoord(PacketData + 6, CellX, CellY, Direction);

	PendingMapCell = FIntPoint(CellX, CellY);
	bAwaitingMapLoad = true;
	OnMapEnter.Broadcast(PendingMapName, CellX, CellY, Direction);
	// Quem carrega o mapa (AMythNetWorld) chama NotifyMapLoaded() -> CZ_NOTIFY_ACTORINIT (0x007d), como o cliente
	// original faz ao terminar o carregamento; sem ouvinte, avisa ja para nao travar o personagem no servidor.
	if (!OnMapEnter.IsBound())
	{
		NotifyMapLoaded();
	}
}

void URagnarokNetworkSubsystem::NotifyMapLoaded()
{
	if (!bAwaitingMapLoad || !CurrentSocket)
	{
		return;
	}
	bAwaitingMapLoad = false;
	uint16 LoadAck = RagnarokPackets::HEADER_CZ_NOTIFY_ACTORINIT;
	SendData(reinterpret_cast<const uint8*>(&LoadAck), sizeof(LoadAck));
}

void URagnarokNetworkSubsystem::HandleMapMove(const uint8* PacketData, int32 PacketSize)
{
	// ZC_NPCACK_MAPMOVE (0x0091): mapName[16] (+2), xPos (+18), yPos (+20) - warp/portal no mesmo map-server
	char MapBuf[17] = { 0 };
	FMemory::Memcpy(MapBuf, PacketData + 2, 16);
	PendingMapName = UTF8_TO_TCHAR(MapBuf);
	const int32 CellX = *reinterpret_cast<const uint16*>(PacketData + 18);
	const int32 CellY = *reinterpret_cast<const uint16*>(PacketData + 20);
	PendingMapCell = FIntPoint(CellX, CellY);
	bAwaitingMapLoad = true;
	OnMapEnter.Broadcast(PendingMapName, CellX, CellY, 0);
	if (!OnMapEnter.IsBound())
	{
		NotifyMapLoaded();
	}
}

void URagnarokNetworkSubsystem::HandleUnitAppear(const uint8* PacketData, int32 PacketSize, uint16 PacketID)
{
	// Offsets de rathena/src/map/packets_struct.hpp (packet_idle_unit / packet_spawn_unit / packet_unit_walking)
	// para PACKETVER 20211103. AID (+5) e o ID usado nos demais pacotes (movimento, saida, ataque); o GID (+9)
	// e o char_id e vale 0 para monstros e NPCs. O walking tem moveStartTime (4 bytes) depois do accessory,
	// entao tudo a partir dali fica 4 bytes adiante.
	const bool bWalking = PacketID == RagnarokPackets::HEADER_ZC_UNIT_WALKING;
	FIntPoint WalkTo(-1, -1);
	FIntPoint WalkFrom(-1, -1);
	uint32 WalkTick = 0;
	const int32 Shift = bWalking ? 4 : 0;
	auto U16 = [PacketData](int32 Offset) { return static_cast<int32>(*reinterpret_cast<const uint16*>(PacketData + Offset)); };
	auto I16 = [PacketData](int32 Offset) { return static_cast<int32>(*reinterpret_cast<const int16*>(PacketData + Offset)); };
	auto I32 = [PacketData](int32 Offset) { return *reinterpret_cast<const int32*>(PacketData + Offset); };

	FRagnarokUnitVisualInfo Unit;
	Unit.ObjectType = PacketData[4];
	Unit.GID = *reinterpret_cast<const uint32*>(PacketData + 5);
	Unit.Speed = I16(13);
	Unit.JobClass = I16(23);
	Unit.Head = U16(25);
	Unit.Weapon = I32(27);
	Unit.Shield = I32(31);
	Unit.HeadBottom = U16(35);           // accessory
	Unit.HeadTop = U16(37 + Shift);      // accessory2
	Unit.HeadMid = U16(39 + Shift);      // accessory3
	Unit.HeadPalette = I16(41 + Shift);
	Unit.BodyPalette = I16(43 + Shift);
	Unit.HeadDir = static_cast<uint8>(FMath::Clamp(U16(45 + Shift), 0, 2));
	Unit.Option = I32(19); // effectState, antes do job (igual nos tres pacotes)
	Unit.BodyState = U16(15);   // opt1 e opt2 vem logo apos a velocidade
	Unit.HealthState = U16(17);
	Unit.Robe = U16(47 + Shift);
	Unit.Sex = PacketData[bWalking ? 66 : 62]; // logo antes de MoveData (walking) / PosDir

	// PosDir em 63 (idle/spawn); MoveData em 67 (walking). Depois: xSize, ySize, [state], clevel, font, maxHP, HP
	if (!bWalking)
	{
		RagnarokPackets::UnpackCellCoord(PacketData + 63, Unit.CellX, Unit.CellY, Unit.Direction);
		Unit.bDirectionKnown = true;
		if (PacketSize >= 71) { Unit.BaseLevel = I16(69); } // xSize 66, ySize 67, state 68, clevel 69
		if (PacketSize >= 81)
		{
			Unit.MaxHP = I32(PacketID == RagnarokPackets::HEADER_ZC_IDLE_UNIT ? 73 : 72);
			Unit.HP = I32(PacketID == RagnarokPackets::HEADER_ZC_IDLE_UNIT ? 77 : 76);
		}
	}
	else
	{
		int32 FromX = 0, FromY = 0, ToX = 0, ToY = 0;
		RagnarokPackets::UnpackMovePath(PacketData + 67, FromX, FromY, ToX, ToY);
		Unit.CellX = FromX;
		Unit.CellY = FromY;
		if (PacketSize >= 78) { Unit.BaseLevel = I16(76); } // MoveData 67..72, xSize 73, ySize 74, state 75, clevel 76
		if (PacketSize >= 87) // MoveData 67-72, xSize, ySize, clevel, font, maxHP 79, HP 83
		{
			Unit.MaxHP = I32(79);
			Unit.HP = I32(83);
		}
		WalkTo = FIntPoint(ToX, ToY); // anunciado depois do OnUnitAppear (o ator precisa existir)
		WalkFrom = FIntPoint(FromX, FromY);
		WalkTick = (PacketSize >= 41) ? *reinterpret_cast<const uint32*>(PacketData + 37) : static_cast<uint32>(FPlatformTime::Cycles());
	}

	char NameBuf[25] = { 0 };
	int32 NameOffset = PacketSize - 24;
	if (NameOffset >= 0 && NameOffset < PacketSize)
	{
		FMemory::Memcpy(NameBuf, PacketData + NameOffset, 24);
		Unit.Name = UTF8_TO_TCHAR(NameBuf);
	}

	OnUnitAppear.Broadcast(Unit);
	if (WalkTo.X >= 0)
	{
		OnUnitWalk.Broadcast(Unit.GID, WalkFrom.X, WalkFrom.Y, WalkTo.X, WalkTo.Y, static_cast<int32>(WalkTick));
		OnUnitMove.Broadcast(Unit.GID, WalkTo.X, WalkTo.Y, static_cast<int32>(WalkTick));
	}
}

void URagnarokNetworkSubsystem::HandlePlayerMove(const uint8* PacketData, int32 PacketSize)
{
	int32 FromX = 0, FromY = 0, ToX = 0, ToY = 0;
	RagnarokPackets::UnpackMovePath(PacketData + 6, FromX, FromY, ToX, ToY);
	uint32 StartTick = (PacketSize >= 6) ? *reinterpret_cast<const uint32*>(PacketData + 2) : static_cast<uint32>(FPlatformTime::Cycles());

	OnUnitWalk.Broadcast(SavedAccountID, FromX, FromY, ToX, ToY, static_cast<int32>(StartTick));
	OnUnitMove.Broadcast(SavedAccountID, ToX, ToY, static_cast<int32>(StartTick)); // no map-server o proprio jogador e o AID
	OnPlayerMoveConfirmed.Broadcast(ToX, ToY);
}

void URagnarokNetworkSubsystem::HandleUnitVanish(const uint8* PacketData, int32 PacketSize)
{
	uint32 GID = *reinterpret_cast<const uint32*>(PacketData + 2);
	uint8 Reason = PacketData[6];
	OnUnitVanish.Broadcast(GID, Reason);
}

void URagnarokNetworkSubsystem::HandleActionNotify(const uint8* PacketData, int32 PacketSize)
{
	int32 SrcID = *reinterpret_cast<const int32*>(PacketData + 2);
	int32 TargetID = *reinterpret_cast<const int32*>(PacketData + 6);
	// srcSpeed (+14) = attackMT de quem bate, o ritmo da animacao do golpe; dmgSpeed (+18) e o do alvo
	int32 DelayMs = *reinterpret_cast<const int32*>(PacketData + 14);
	int32 Damage = *reinterpret_cast<const int32*>(PacketData + 22);
	uint8 ActionType = PacketData[29]; // PACKET_ZC_NOTIFY_ACT 0x08c8: damage(22), isSPDamage(26), div(27), type(29), damage2(30)

	OnDamage.Broadcast(SrcID, TargetID, Damage, ActionType, DelayMs);
	OnDamageReceived.Broadcast(TargetID, Damage, ActionType == 1, Damage == 0);
}

void URagnarokNetworkSubsystem::HandleParamChange(const uint8* PacketData, int32 PacketSize, uint16 PacketID)
{
	uint16 ParamType = 0;
	int64 Value = 0;

	if (PacketID == RagnarokPackets::HEADER_ZC_PAR_CHANGE && PacketSize >= 8) // 0x00b0
	{
		ParamType = *reinterpret_cast<const uint16*>(PacketData + 2);
		Value = *reinterpret_cast<const int32*>(PacketData + 4);
	}
	else if (PacketID == RagnarokPackets::HEADER_ZC_LONGPAR_CHANGE && PacketSize >= 8) // 0x00b1
	{
		ParamType = *reinterpret_cast<const uint16*>(PacketData + 2);
		Value = *reinterpret_cast<const int32*>(PacketData + 4);
	}
	else if (PacketID == RagnarokPackets::HEADER_ZC_LONGLONGPAR_CHANGE && PacketSize >= 12) // 0x0acb
	{
		ParamType = *reinterpret_cast<const uint16*>(PacketData + 2);
		Value = *reinterpret_cast<const int64*>(PacketData + 4);
	}
	else if (PacketID == RagnarokPackets::HEADER_ZC_COUPLESTATUS && PacketSize >= 14) // 0x0141
	{
		uint32 StatusType = *reinterpret_cast<const uint32*>(PacketData + 2);
		int32 DefaultStatus = *reinterpret_cast<const int32*>(PacketData + 6);
		ParamType = static_cast<uint16>(StatusType);
		Value = DefaultStatus;
	}
	else
	{
		return;
	}

	// Atualiza cache de status
	switch (ParamType)
	{
		case 1:  CurrentStats.BaseExp = Value; break;
		case 2:  CurrentStats.JobExp = Value; break;
		case 5:  CurrentStats.HP = Value; break;
		case 6:  CurrentStats.MaxHP = Value; break;
		case 7:  CurrentStats.SP = Value; break;
		case 8:  CurrentStats.MaxSP = Value; break;
		case 9:  CurrentStats.StatusPoints = static_cast<int32>(Value); break;
		case 11: CurrentStats.BaseLevel = static_cast<int32>(Value); break;
		case 12: CurrentStats.SkillPoints = static_cast<int32>(Value); break;
		case 13: CurrentStats.Str = static_cast<int32>(Value); break;
		case 14: CurrentStats.Agi = static_cast<int32>(Value); break;
		case 15: CurrentStats.Vit = static_cast<int32>(Value); break;
		case 16: CurrentStats.Int = static_cast<int32>(Value); break;
		case 17: CurrentStats.Dex = static_cast<int32>(Value); break;
		case 18: CurrentStats.Luk = static_cast<int32>(Value); break;
		case 19:
			CurrentStats.JobClass = static_cast<int32>(Value);
			CurrentStats.JobName = GetJobNameFromClass(CurrentStats.JobClass);
			break;
		case 20: CurrentStats.Zeny = Value; break;
		case 22: CurrentStats.NextBaseExp = Value; break;
		case 23: CurrentStats.NextJobExp = Value; break;
		case 24: CurrentStats.Weight = static_cast<int32>(Value); break;
		case 25: CurrentStats.MaxWeight = static_cast<int32>(Value); break;
		case 41: CurrentStats.Atk = static_cast<int32>(Value); break;
		case 42: CurrentStats.Atk2 = static_cast<int32>(Value); break;
		case 43: CurrentStats.MatkMax = static_cast<int32>(Value); break;
		case 44: CurrentStats.MatkMin = static_cast<int32>(Value); break;
		case 45: CurrentStats.Def = static_cast<int32>(Value); break;
		case 46: CurrentStats.Def2 = static_cast<int32>(Value); break;
		case 47: CurrentStats.Mdef = static_cast<int32>(Value); break;
		case 48: CurrentStats.Mdef2 = static_cast<int32>(Value); break;
		case 49: CurrentStats.Hit = static_cast<int32>(Value); break;
		case 50: CurrentStats.Flee = static_cast<int32>(Value); break;
		case 51: CurrentStats.Flee2 = static_cast<int32>(Value); break;
		case 52: CurrentStats.Critical = static_cast<int32>(Value); break;
		case 53: CurrentStats.Aspd = static_cast<int32>(Value); break;
		case 55: CurrentStats.JobLevel = static_cast<int32>(Value); break;
		default: break;
	}

	OnStatusChanged.Broadcast(ParamType, static_cast<int32>(Value));
	OnPlayerStatsChanged.Broadcast(CurrentStats);
}

void URagnarokNetworkSubsystem::HandleSpriteChange(const uint8* PacketData, int32 PacketSize)
{
	uint32 GID = *reinterpret_cast<const uint32*>(PacketData + 2);
	uint8 Slot = PacketData[6];
	int32 VisualID = *reinterpret_cast<const int32*>(PacketData + 7);
	if (Slot == 0 && GID == SavedAccountID) // LOOK_BASE do proprio personagem: mudou de classe
	{
		CurrentStats.JobClass = VisualID;
		CurrentStats.JobName = GetJobNameFromClass(VisualID);
		OnPlayerStatsChanged.Broadcast(CurrentStats);
	}
	OnEquipChanged.Broadcast(GID, Slot, VisualID);
}

void URagnarokNetworkSubsystem::HandleChatMessage(const uint8* PacketData, int32 PacketSize, uint16 PacketID)
{
	FString Sender = TEXT("System");
	FString Text = TEXT("");
	FString ColorHex = TEXT("#FFFFFF");

	if (PacketID == RagnarokPackets::HEADER_ZC_NOTIFY_CHAT) // 0x008e
	{
		if (PacketSize > 4) // clif.cpp: WFIFOW(fd,0)=0x8e; WFIFOW(fd,2)=len; texto em WFIFOCP(fd,4)
		{
			int32 StrLen = PacketSize - 4;
			TArray<uint8> CleanBuf;
			CleanBuf.Append(PacketData + 4, StrLen);
			CleanBuf.Add(0);
			FString FullMsg = UTF8_TO_TCHAR(reinterpret_cast<const char*>(CleanBuf.GetData()));

			int32 ColonIdx = INDEX_NONE;
			if (FullMsg.FindChar(TEXT(':'), ColonIdx) && ColonIdx > 0)
			{
				Sender = FullMsg.Left(ColonIdx).TrimStartAndEnd();
				Text = FullMsg.Mid(ColonIdx + 1).TrimStartAndEnd();
			}
			else
			{
				Text = FullMsg;
			}
		}
	}
	else if (PacketID == RagnarokPackets::HEADER_ZC_DISP_MESSAGE) // 0x02c1
	{
		if (PacketSize > 12)
		{
			uint32 ColorVal = *reinterpret_cast<const uint32*>(PacketData + 8);
			uint8 R = (ColorVal >> 16) & 0xFF;
			uint8 G = (ColorVal >> 8) & 0xFF;
			uint8 B = ColorVal & 0xFF;
			ColorHex = FString::Printf(TEXT("#%02X%02X%02X"), R, G, B);

			int32 StrLen = PacketSize - 12;
			TArray<uint8> CleanBuf;
			CleanBuf.Append(PacketData + 12, StrLen);
			CleanBuf.Add(0);
			Text = UTF8_TO_TCHAR(reinterpret_cast<const char*>(CleanBuf.GetData()));
		}
	}
	else if (PacketID == RagnarokPackets::HEADER_ZC_DISP_MESSAGE2) // 0x017f
	{
		if (PacketSize > 4)
		{
			int32 StrLen = PacketSize - 4;
			TArray<uint8> CleanBuf;
			CleanBuf.Append(PacketData + 4, StrLen);
			CleanBuf.Add(0);
			Text = UTF8_TO_TCHAR(reinterpret_cast<const char*>(CleanBuf.GetData()));
			ColorHex = TEXT("#FFFF00");
		}
	}
	else if (PacketID == RagnarokPackets::HEADER_ZC_WHISPER2) // 0x09de
	{
		if (PacketSize > 33)
		{
			char NameBuf[25] = { 0 };
			FMemory::Memcpy(NameBuf, PacketData + 8, 24);
			Sender = UTF8_TO_TCHAR(NameBuf);

			int32 StrLen = PacketSize - 33;
			TArray<uint8> CleanBuf;
			CleanBuf.Append(PacketData + 33, StrLen);
			CleanBuf.Add(0);
			Text = UTF8_TO_TCHAR(reinterpret_cast<const char*>(CleanBuf.GetData()));
			ColorHex = TEXT("#FFFF00");
		}
	}
	else if (PacketID == RagnarokPackets::HEADER_ZC_WHISPER) // 0x008a
	{
		if (PacketSize > 28)
		{
			char NameBuf[25] = { 0 };
			FMemory::Memcpy(NameBuf, PacketData + 4, 24);
			Sender = UTF8_TO_TCHAR(NameBuf);

			int32 StrLen = PacketSize - 28;
			TArray<uint8> CleanBuf;
			CleanBuf.Append(PacketData + 28, StrLen);
			CleanBuf.Add(0);
			Text = UTF8_TO_TCHAR(reinterpret_cast<const char*>(CleanBuf.GetData()));
			ColorHex = TEXT("#FFFF00");
		}
	}

	if (PacketID == RagnarokPackets::HEADER_ZC_NOTIFY_CHAT && !Text.IsEmpty() && Sender.Equals(SelectedCharName, ESearchCase::IgnoreCase)) // so a propria fala "Nome : texto"; avisos do servidor tambem chegam por aqui (ex.: "rAthena Version Git Hash : ...")
	{
		OnSpeech.Broadcast(static_cast<int64>(SavedAccountID), Text); // propria fala: balao sobre o proprio personagem
	}
	OnChatMessage.Broadcast(Sender, Text, ColorHex);
}

void URagnarokNetworkSubsystem::HandleStatus(const uint8* PacketData, int32 PacketSize, uint16 PacketID)
{
	if (PacketID == RagnarokPackets::HEADER_ZC_STATUS) // 0x00bd - 44 bytes
	{
		#pragma pack(push, 1)
		struct FPACKET_ZC_STATUS
		{
			int16 packetType;
			uint16 point;
			uint8 str;
			uint8 standardStr;
			uint8 agi;
			uint8 standardAgi;
			uint8 vit;
			uint8 standardVit;
			uint8 int_;
			uint8 standardInt;
			uint8 dex;
			uint8 standardDex;
			uint8 luk;
			uint8 standardLuk;
			int16 attPower;
			int16 refiningPower;
			int16 max_mattPower;
			int16 min_mattPower;
			int16 itemdefPower;
			int16 plusdefPower;
			int16 mdefPower;
			int16 plusmdefPower;
			int16 hitSuccessValue;
			int16 avoidSuccessValue;
			int16 plusAvoidSuccessValue;
			int16 criticalSuccessValue;
			int16 ASPD;
			int16 plusASPD;
		};
		#pragma pack(pop)

		if (PacketSize >= sizeof(FPACKET_ZC_STATUS))
		{
			const FPACKET_ZC_STATUS* Pkt = reinterpret_cast<const FPACKET_ZC_STATUS*>(PacketData);
			CurrentStats.StatusPoints = Pkt->point;
			CurrentStats.Str = Pkt->str;
			CurrentStats.StrNeed = Pkt->standardStr;
			CurrentStats.Agi = Pkt->agi;
			CurrentStats.AgiNeed = Pkt->standardAgi;
			CurrentStats.Vit = Pkt->vit;
			CurrentStats.VitNeed = Pkt->standardVit;
			CurrentStats.Int = Pkt->int_;
			CurrentStats.IntNeed = Pkt->standardInt;
			CurrentStats.Dex = Pkt->dex;
			CurrentStats.DexNeed = Pkt->standardDex;
			CurrentStats.Luk = Pkt->luk;
			CurrentStats.LukNeed = Pkt->standardLuk;
			CurrentStats.Atk = Pkt->attPower;
			CurrentStats.Atk2 = Pkt->refiningPower;
			CurrentStats.MatkMin = Pkt->min_mattPower;
			CurrentStats.MatkMax = Pkt->max_mattPower;
			CurrentStats.Def = Pkt->itemdefPower;
			CurrentStats.Def2 = Pkt->plusdefPower;
			CurrentStats.Mdef = Pkt->mdefPower;
			CurrentStats.Mdef2 = Pkt->plusmdefPower;
			CurrentStats.Hit = Pkt->hitSuccessValue;
			CurrentStats.Flee = Pkt->avoidSuccessValue;
			CurrentStats.Flee2 = Pkt->plusAvoidSuccessValue;
			CurrentStats.Critical = Pkt->criticalSuccessValue;
			CurrentStats.Aspd = Pkt->ASPD;

			OnPlayerStatsChanged.Broadcast(CurrentStats);
		}
	}
	else if (PacketID == RagnarokPackets::HEADER_ZC_STATUS_VALUES) // 0x00be - 5 bytes
	{
		uint16 StatusID = *reinterpret_cast<const uint16*>(PacketData + 2);
		uint8 Value = PacketData[4];
		switch (StatusID)
		{
		case 32: CurrentStats.StrNeed = Value; break; // SP_USTR
		case 33: CurrentStats.AgiNeed = Value; break; // SP_UAGI
		case 34: CurrentStats.VitNeed = Value; break; // SP_UVIT
		case 35: CurrentStats.IntNeed = Value; break; // SP_UINT
		case 36: CurrentStats.DexNeed = Value; break; // SP_UDEX
		case 37: CurrentStats.LukNeed = Value; break; // SP_ULUK
		default: break;
		}
		OnStatusChanged.Broadcast(StatusID, Value);
		OnPlayerStatsChanged.Broadcast(CurrentStats);
	}
	else if (PacketID == RagnarokPackets::HEADER_ZC_STATUS_CHANGE_ACK) // 0x00bc - 6 bytes
	{
		uint16 SP = *reinterpret_cast<const uint16*>(PacketData + 2);
		uint8 bOk = PacketData[4];
		uint8 Val = PacketData[5];
		if (bOk)
		{
			switch (SP)
			{
			case 13: CurrentStats.Str = Val; break; // SP_STR
			case 14: CurrentStats.Agi = Val; break; // SP_AGI
			case 15: CurrentStats.Vit = Val; break; // SP_VIT
			case 16: CurrentStats.Int = Val; break; // SP_INT
			case 17: CurrentStats.Dex = Val; break; // SP_DEX
			case 18: CurrentStats.Luk = Val; break; // SP_LUK
			default: break;
			}
			OnStatusChanged.Broadcast(SP, Val);
			OnPlayerStatsChanged.Broadcast(CurrentStats);
		}
	}
}

void URagnarokNetworkSubsystem::HandleItemFall(const uint8* PacketData, int32 PacketSize)
{
	if (PacketSize < 21)
	{
		return;
	}

	uint32 GID = *reinterpret_cast<const uint32*>(PacketData + 2);
	uint32 ItemID = *reinterpret_cast<const uint32*>(PacketData + 6);
	int16 CellX = *reinterpret_cast<const int16*>(PacketData + 13);
	int16 CellY = *reinterpret_cast<const int16*>(PacketData + 15);
	int16 Count = *reinterpret_cast<const int16*>(PacketData + 19);

	OnGroundItemAppear.Broadcast(GID, ItemID, FMath::Max<int32>(1, Count), CellX, CellY);
}

void URagnarokNetworkSubsystem::HandleItemPickupAck(const uint8* PacketData, int32 PacketSize)
{
	// PACKET_ZC_ITEM_PICKUP_ACK (packets_struct.hpp, 0x0b41): Index +2, count +4, nameid (u32) +6, IsIdentified +10,
	// slot[4] +12, location (u32) +28, type +32, result +33 (0 = ok)
	if (PacketSize < 34 || PacketData[33] != 0)
	{
		return;
	}
	const int32 Index = *reinterpret_cast<const uint16*>(PacketData + 2);
	const int32 Count = *reinterpret_cast<const uint16*>(PacketData + 4);
	FRagnarokInventoryItem* Existing = InventoryItems.FindByPredicate([Index](const FRagnarokInventoryItem& I) { return I.Index == Index; });
	if (Existing)
	{
		Existing->Amount += Count;
	}
	else
	{
		FRagnarokInventoryItem Item;
		Item.Index = Index;
		Item.Amount = Count;
		Item.ItemID = static_cast<int32>(*reinterpret_cast<const uint32*>(PacketData + 6));
		Item.bIdentified = PacketData[10] != 0;
		Item.EquipLocation = static_cast<int32>(*reinterpret_cast<const uint32*>(PacketData + 28));
		Item.Location = Item.EquipLocation;
		Item.WearState = 0;
		Item.bIsEquipped = false;
		Item.Type = PacketData[32];
		InventoryItems.Add(Item);
	}
	OnInventory.Broadcast(InventoryItems);
}

void URagnarokNetworkSubsystem::RemoveInventoryAmount(int32 Index, int32 Count)
{
	const int32 Pos = InventoryItems.IndexOfByPredicate([Index](const FRagnarokInventoryItem& I) { return I.Index == Index; });
	if (Pos == INDEX_NONE)
	{
		return;
	}
	InventoryItems[Pos].Amount -= Count;
	if (InventoryItems[Pos].Amount <= 0)
	{
		InventoryItems.RemoveAt(Pos);
	}
	OnInventory.Broadcast(InventoryItems);
}

void URagnarokNetworkSubsystem::HandleItemEntry(const uint8* PacketData, int32 PacketSize)
{
	// PACKET_ZC_ITEM_ENTRY (packets_struct.hpp): AID +2, itemId (u32) +6, identify +10, x +11, y +13, amount +15
	if (PacketSize < 17)
	{
		return;
	}
	const uint32 GID = *reinterpret_cast<const uint32*>(PacketData + 2);
	const uint32 ItemID = *reinterpret_cast<const uint32*>(PacketData + 6);
	const int32 CellX = *reinterpret_cast<const uint16*>(PacketData + 11);
	const int32 CellY = *reinterpret_cast<const uint16*>(PacketData + 13);
	const int32 Count = *reinterpret_cast<const uint16*>(PacketData + 15);
	OnGroundItemAppear.Broadcast(GID, ItemID, FMath::Max(1, Count), CellX, CellY);
}

void URagnarokNetworkSubsystem::HandleItemDisappear(const uint8* PacketData, int32 PacketSize)
{
	if (PacketSize >= 6)
	{
		uint32 GID = *reinterpret_cast<const uint32*>(PacketData + 2);
		OnGroundItemVanish.Broadcast(GID);
	}
}

void URagnarokNetworkSubsystem::HandleNpcDialog(const uint8* PacketData, int32 PacketSize)
{
	if (PacketSize <= 8)
	{
		return;
	}

	uint32 NpcID = *reinterpret_cast<const uint32*>(PacketData + 4);
	int32 MsgLen = PacketSize - 8;
	TArray<uint8> CleanBuf;
	CleanBuf.Append(PacketData + 8, MsgLen);
	CleanBuf.Add(0);

	FString DialogText = UTF8_TO_TCHAR(reinterpret_cast<const char*>(CleanBuf.GetData()));
	OnNpcDialog.Broadcast(NpcID, DialogText);
}

void URagnarokNetworkSubsystem::HandleNpcWait(const uint8* PacketData, int32 PacketSize)
{
	if (PacketSize >= 6)
	{
		uint32 NpcID = *reinterpret_cast<const uint32*>(PacketData + 2);
		OnNpcDialog.Broadcast(NpcID, TEXT("[WAIT]"));
	}
}

void URagnarokNetworkSubsystem::HandleNpcClose(const uint8* PacketData, int32 PacketSize)
{
	if (PacketSize >= 6)
	{
		uint32 NpcID = *reinterpret_cast<const uint32*>(PacketData + 2);
		OnNpcDialog.Broadcast(NpcID, TEXT("[CLOSE]"));
	}
}

void URagnarokNetworkSubsystem::HandleNpcMenu(const uint8* PacketData, int32 PacketSize)
{
	if (PacketSize <= 8)
	{
		return;
	}

	uint32 NpcID = *reinterpret_cast<const uint32*>(PacketData + 4);
	int32 MenuLen = PacketSize - 8;
	TArray<uint8> CleanBuf;
	CleanBuf.Append(PacketData + 8, MenuLen);
	CleanBuf.Add(0);

	FString RawMenu = UTF8_TO_TCHAR(reinterpret_cast<const char*>(CleanBuf.GetData()));
	TArray<FString> Parts;
	RawMenu.ParseIntoArray(Parts, TEXT(":"), false);

	// select("A:B:C") do script: todos os itens sao opcoes (nao ha titulo). Itens vazios ficam na lista para manter
	// o indice: o CZ_CHOOSE_MENU envia a posicao original (1..N; 255 = cancelar), como no clif_parse_NpcSelectMenu.
	const FString Title;
	TArray<FString> Options = Parts;

	OnNpcMenu.Broadcast(NpcID, Title, Options);
}

void URagnarokNetworkSubsystem::HandleSkillList(const uint8* PacketData, int32 PacketSize)
{
	uint16 PacketID = *reinterpret_cast<const uint16*>(PacketData);

	if (PacketID == RagnarokPackets::HEADER_ZC_SKILLINFO_LIST_RE) // 0x0b32 (PACKETVER >= 20190807)
	{
		static constexpr int32 HEADER_SIZE = 4;
		static constexpr int32 SKILL_DATA_SIZE = 15;

		int32 Available = PacketSize - HEADER_SIZE;
		int32 Count = Available / SKILL_DATA_SIZE;
		SkillList.Empty();

		for (int32 i = 0; i < Count; ++i)
		{
			const uint8* Ptr = PacketData + HEADER_SIZE + (i * SKILL_DATA_SIZE);
			FRagnarokSkillInfo Info;
			Info.SkillID = *reinterpret_cast<const uint16*>(Ptr + 0);
			Info.Inf = *reinterpret_cast<const int32*>(Ptr + 2);
			Info.Level = *reinterpret_cast<const uint16*>(Ptr + 6);
			Info.SpCost = *reinterpret_cast<const uint16*>(Ptr + 8);
			Info.Range = *reinterpret_cast<const uint16*>(Ptr + 10);
			Info.bUpgradable = (Ptr[12] != 0);

			SkillList.Add(Info);
		}

		OnSkillList.Broadcast(SkillList);
	}
	else if (PacketID == RagnarokPackets::HEADER_ZC_ADD_SKILL) // 0x0b31 (17 bytes)
	{
		if (PacketSize >= 17)
		{
			const uint8* Ptr = PacketData + 2;
			FRagnarokSkillInfo Info;
			Info.SkillID = *reinterpret_cast<const uint16*>(Ptr + 0);
			Info.Inf = *reinterpret_cast<const int32*>(Ptr + 2);
			Info.Level = *reinterpret_cast<const uint16*>(Ptr + 6);
			Info.SpCost = *reinterpret_cast<const uint16*>(Ptr + 8);
			Info.Range = *reinterpret_cast<const uint16*>(Ptr + 10);
			Info.bUpgradable = (Ptr[12] != 0);

			int32 ExistingIdx = SkillList.IndexOfByPredicate([&](const FRagnarokSkillInfo& Item) { return Item.SkillID == Info.SkillID; });
			if (ExistingIdx != INDEX_NONE)
			{
				SkillList[ExistingIdx] = Info;
			}
			else
			{
				SkillList.Add(Info);
			}

			OnSkillList.Broadcast(SkillList);
		}
	}
	else if (PacketID == RagnarokPackets::HEADER_ZC_SKILLINFO_UPDATE2) // 0x0b33 (17 bytes)
	{
		if (PacketSize >= 17)
		{
			const uint8* Ptr = PacketData + 2;
			uint16 SkillID = *reinterpret_cast<const uint16*>(Ptr + 0);
			int32 ExistingIdx = SkillList.IndexOfByPredicate([&](const FRagnarokSkillInfo& Item) { return Item.SkillID == SkillID; });
			if (ExistingIdx != INDEX_NONE)
			{
				SkillList[ExistingIdx].Level = *reinterpret_cast<const uint16*>(Ptr + 6);
				SkillList[ExistingIdx].SpCost = *reinterpret_cast<const uint16*>(Ptr + 8);
				SkillList[ExistingIdx].Range = *reinterpret_cast<const uint16*>(Ptr + 10);
				SkillList[ExistingIdx].bUpgradable = (Ptr[12] != 0);
				OnSkillList.Broadcast(SkillList);
			}
		}
	}
	else if (PacketID == RagnarokPackets::HEADER_ZC_SKILLINFO_UPDATE) // 0x010e (11 bytes)
	{
		if (PacketSize >= 11)
		{
			uint16 SkillID = *reinterpret_cast<const uint16*>(PacketData + 2);
			int32 ExistingIdx = SkillList.IndexOfByPredicate([&](const FRagnarokSkillInfo& Item) { return Item.SkillID == SkillID; });
			if (ExistingIdx != INDEX_NONE)
			{
				SkillList[ExistingIdx].Level = *reinterpret_cast<const uint16*>(PacketData + 4);
				SkillList[ExistingIdx].SpCost = *reinterpret_cast<const uint16*>(PacketData + 6);
				SkillList[ExistingIdx].Range = *reinterpret_cast<const uint16*>(PacketData + 8);
				SkillList[ExistingIdx].bUpgradable = (PacketData[10] != 0);
				OnSkillList.Broadcast(SkillList);
			}
		}
	}
	else if (PacketID == RagnarokPackets::HEADER_ZC_SKILL_INFO_LIST) // 0x010f (legado)
	{
		static constexpr int32 HEADER_SIZE = 4;
		static constexpr int32 SKILL_DATA_SIZE = 37;

		int32 Available = PacketSize - HEADER_SIZE;
		int32 Count = Available / SKILL_DATA_SIZE;
		SkillList.Empty();

		for (int32 i = 0; i < Count; ++i)
		{
			const uint8* Ptr = PacketData + HEADER_SIZE + (i * SKILL_DATA_SIZE);
			FRagnarokSkillInfo Info;
			Info.SkillID = *reinterpret_cast<const uint16*>(Ptr + 0);
			Info.Inf = *reinterpret_cast<const int32*>(Ptr + 2);
			Info.Level = *reinterpret_cast<const uint16*>(Ptr + 6);
			Info.SpCost = *reinterpret_cast<const uint16*>(Ptr + 8);
			Info.Range = *reinterpret_cast<const uint16*>(Ptr + 10);

			char NameBuf[25] = { 0 };
			FMemory::Memcpy(NameBuf, Ptr + 12, 24);
			Info.SkillName = UTF8_TO_TCHAR(NameBuf);
			Info.bUpgradable = (Ptr[36] != 0);

			SkillList.Add(Info);
		}

		OnSkillList.Broadcast(SkillList);
	}
}

void URagnarokNetworkSubsystem::HandleInventoryList(const uint8* PacketData, int32 PacketSize, bool bIsEquip)
{
	// IDs 0x0b09 / 0x0b39 (PACKETVER >= 20181002) trazem o tipo da lista no byte 4 (e_inventory_type: 0 mochila, 1 carrinho,
	// 2 armazem, 3 armazem do cla). So a mochila mexe em InventoryItems; antes o armazem e o carrinho sobrescreviam a mochila.
	const uint16 ListId = *reinterpret_cast<const uint16*>(PacketData);
	const uint8 InvType = (ListId == 0x0b09 || ListId == 0x0b39) ? PacketData[4] : 0;
	if (InvType == 1 || InvType == 3)
	{
		return; // carrinho e armazem do cla: ainda sem janela
	}
	TArray<FRagnarokInventoryItem>& Target = InvType == 2 ? StorageItems : InventoryItems;
	if (bIsEquip)
	{
		static constexpr int32 HEADER_SIZE = 5;
		static constexpr int32 EQUIP_INFO_SIZE = 68;

		int32 Available = PacketSize - HEADER_SIZE;
		int32 Count = Available / EQUIP_INFO_SIZE;

		if (InvType == 0) { InventoryItems.RemoveAll([](const FRagnarokInventoryItem& Item) { return Item.bIsEquipped; }); }

		for (int32 i = 0; i < Count; ++i)
		{
			const uint8* Ptr = PacketData + HEADER_SIZE + (i * EQUIP_INFO_SIZE);
			FRagnarokInventoryItem Item;
			Item.Index = *reinterpret_cast<const int16*>(Ptr + 0);
			Item.ItemID = *reinterpret_cast<const uint32*>(Ptr + 2);
			Item.Type = Ptr[6];
			uint32 Location = *reinterpret_cast<const uint32*>(Ptr + 7);
			uint32 WearState = *reinterpret_cast<const uint32*>(Ptr + 11);
			Item.Location = Location;
			Item.WearState = WearState;
			Item.bIsEquipped = (WearState != 0);
			Item.EquipLocation = (WearState != 0) ? WearState : Location;
			Item.Amount = 1;

			Item.Slot1 = *reinterpret_cast<const uint32*>(Ptr + 15);
			Item.Slot2 = *reinterpret_cast<const uint32*>(Ptr + 19);
			Item.Slot3 = *reinterpret_cast<const uint32*>(Ptr + 23);
			Item.Slot4 = *reinterpret_cast<const uint32*>(Ptr + 27);

			Item.Refine = Ptr[65];
			Item.bIdentified = (Ptr[67] & 1) != 0;

			Target.Add(Item);
		}
	}
	else
	{
		static constexpr int32 HEADER_SIZE = 5;
		static constexpr int32 NORMAL_INFO_SIZE = 34;

		int32 Available = PacketSize - HEADER_SIZE;
		int32 Count = Available / NORMAL_INFO_SIZE;

		if (InvType == 0) { InventoryItems.RemoveAll([](const FRagnarokInventoryItem& Item) { return !Item.bIsEquipped; }); }

		for (int32 i = 0; i < Count; ++i)
		{
			const uint8* Ptr = PacketData + HEADER_SIZE + (i * NORMAL_INFO_SIZE);
			FRagnarokInventoryItem Item;
			Item.Index = *reinterpret_cast<const int16*>(Ptr + 0);
			Item.ItemID = *reinterpret_cast<const uint32*>(Ptr + 2);
			Item.Type = Ptr[6];
			Item.Amount = *reinterpret_cast<const int16*>(Ptr + 7);
			Item.EquipLocation = 0;
			Item.bIsEquipped = false;

			Item.Slot1 = *reinterpret_cast<const uint32*>(Ptr + 13);
			Item.Slot2 = *reinterpret_cast<const uint32*>(Ptr + 17);
			Item.Slot3 = *reinterpret_cast<const uint32*>(Ptr + 21);
			Item.Slot4 = *reinterpret_cast<const uint32*>(Ptr + 25);

			Item.Refine = 0;
			Item.bIdentified = (Ptr[33] & 1) != 0;

			Target.Add(Item);
		}
	}

	if (InvType == 0)
	{
		OnInventory.Broadcast(InventoryItems);
	}
}

void URagnarokNetworkSubsystem::HandleEffect(const uint8* PacketData, int32 PacketSize)
{
	if (PacketSize < 10)
	{
		return;
	}

	int32 EffectID = *reinterpret_cast<const int32*>(PacketData + 6);
	FString WeatherType;

	switch (EffectID)
	{
		case RagnarokPackets::WeatherEffects::EF_RAIN:
			WeatherType = TEXT("rain");
			break;
		case RagnarokPackets::WeatherEffects::EF_SNOW:
			WeatherType = TEXT("snow");
			break;
		case RagnarokPackets::WeatherEffects::EF_SAKURA:
			WeatherType = TEXT("sakura");
			break;
		case RagnarokPackets::WeatherEffects::EF_CLOUDS:
		case RagnarokPackets::WeatherEffects::EF_CLOUDS2:
			WeatherType = TEXT("clouds");
			break;
		case RagnarokPackets::WeatherEffects::EF_FIREWORKS:
			WeatherType = TEXT("fireworks");
			break;
		case RagnarokPackets::WeatherEffects::EF_LEAVES:
			WeatherType = TEXT("leaves");
			break;
		case RagnarokPackets::WeatherEffects::EF_FOG:
			WeatherType = TEXT("fog");
			break;
		default:
			break;
	}

	if (!WeatherType.IsEmpty())
	{
		OnWeather.Broadcast(WeatherType, true);
		return;
	}
	OnSpecialEffect.Broadcast(static_cast<int64>(*reinterpret_cast<const uint32*>(PacketData + 2)), EffectID);
}

// ------------------------------------------------------------------------------------------------ atalhos e skills
// Offsets de rathena/src/map/packets_struct.hpp para PACKETVER 20211103 (RE): ver docs/PLANO_CLIENTE_FIEL.md.

void URagnarokNetworkSubsystem::HandleHotkeys(const uint8* PacketData, int32 PacketSize)
{
	// PACKET_ZC_SHORTCUT_KEY_LIST (0x0b20): rotate +2, tab +3 (int16), 38 x hotkey_data(isSkill 1, id 4, count 2) a partir de +5
	if (PacketSize < 5 || *reinterpret_cast<const int16*>(PacketData + 3) != 0)
	{
		return; // so a aba 0 (F1..F9 da barra principal)
	}
	Hotkeys.Reset();
	for (int32 Off = 5; Off + 7 <= PacketSize; Off += 7)
	{
		FRagnarokHotkey H;
		H.bIsSkill = PacketData[Off] != 0;
		H.ID = static_cast<int32>(*reinterpret_cast<const uint32*>(PacketData + Off + 1));
		H.Count = *reinterpret_cast<const int16*>(PacketData + Off + 5);
		Hotkeys.Add(H);
	}
}

bool URagnarokNetworkSubsystem::SetHotkey(int32 Index, bool bIsSkill, int32 ID, int32 Count)
{
	if (!CurrentSocket || CurrentState != ERagnarokNetworkState::ConnectedMap || Index < 0 || Index >= 38)
	{
		return false;
	}
	// PACKET_CZ_SHORTCUT_KEY_CHANGE2 (0x0b21): tab +2, index +4, hotkey_data +6 (isSkill, id, count) = 13 bytes
	uint8 Pkt[13] = { 0 };
	*reinterpret_cast<uint16*>(Pkt) = 0x0b21;
	*reinterpret_cast<uint16*>(Pkt + 2) = 0;
	*reinterpret_cast<uint16*>(Pkt + 4) = static_cast<uint16>(Index);
	Pkt[6] = bIsSkill ? 1 : 0;
	*reinterpret_cast<uint32*>(Pkt + 7) = static_cast<uint32>(ID);
	*reinterpret_cast<int16*>(Pkt + 11) = static_cast<int16>(Count);
	SendData(Pkt, sizeof(Pkt));
	if (!Hotkeys.IsValidIndex(Index))
	{
		Hotkeys.SetNum(38);
	}
	Hotkeys[Index].bIsSkill = bIsSkill;
	Hotkeys[Index].ID = ID;
	Hotkeys[Index].Count = Count;
	return true;
}

void URagnarokNetworkSubsystem::HandleSkillResult(const uint8* PacketData, int32 PacketSize, uint16 PacketID)
{
	auto U16 = [PacketData](int32 O) { return static_cast<int32>(*reinterpret_cast<const uint16*>(PacketData + O)); };
	auto I32 = [PacketData](int32 O) { return *reinterpret_cast<const int32*>(PacketData + O); };
	auto U32 = [PacketData](int32 O) { return static_cast<int64>(*reinterpret_cast<const uint32*>(PacketData + O)); };
	switch (PacketID)
	{
	case 0x0117: // ZC_NOTIFY_GROUNDSKILL: SKID +2, AID +4, level +8, x +10, y +12
		if (PacketSize >= 18)
		{
			OnSkillGround.Broadcast(U32(4), U16(2), U16(8), U16(10), U16(12));
		}
		break;
	case 0x01de: // ZC_NOTIFY_SKILL: SKID +2, AID +4, targetID +8, damage +24, level +28, count +30, action +32
		if (PacketSize >= 33)
		{
			OnSkillDamage.Broadcast(U32(4), U32(8), U16(2), I32(24), FMath::Max(1, U16(30)), PacketData[32]);
		}
		break;
	case 0x09cb: // ZC_USE_SKILL: SKID +2, level/valor +4, targetAID +8, srcAID +12, result +16
		if (PacketSize >= 17 && PacketData[16] != 0)
		{
			OnSkillEffect.Broadcast(U32(12), U32(8), U16(2), I32(4));
		}
		break;
	case 0x0b1a: // ZC_USESKILL_ACK: srcId +2, dstId +6, x +10, y +12, skillId +14, element +16, delayTime +20
		if (PacketSize >= 24)
		{
			OnSkillCast.Broadcast(U32(2), U32(6), U16(14), I32(20), U16(10), U16(12));
		}
		break;
	case 0x0110: // ZC_ACK_TOUSESKILL: skillId +2, btype +4, itemId +8, flag +12, cause +13
		if (PacketSize >= 14 && PacketData[12] == 0)
		{
			OnSkillFailed.Broadcast(U16(2), PacketData[13]);
		}
		break;
	default:
		break;
	}
}

void URagnarokNetworkSubsystem::HandleStateChange(const uint8* PacketData, int32 PacketSize, uint16 PacketID)
{
	int64 UnitID = 0;
	int32 EfstID = 0;
	bool bActive = false;
	int32 TotalMs = 0;
	int32 RemainMs = 0;

	if (PacketID == RagnarokPackets::HEADER_ZC_MSG_STATE_CHANGE3) // 0x0983 (29 bytes)
	{
		if (PacketSize < 29)
		{
			return;
		}
		EfstID = *reinterpret_cast<const uint16*>(PacketData + 2);
		UnitID = static_cast<int64>(*reinterpret_cast<const uint32*>(PacketData + 4));
		uint8 Flag = PacketData[8];
		bActive = (Flag != 0);
		TotalMs = static_cast<int32>(*reinterpret_cast<const uint32*>(PacketData + 9));
		RemainMs = static_cast<int32>(*reinterpret_cast<const uint32*>(PacketData + 13));
	}
	else if (PacketID == RagnarokPackets::HEADER_ZC_MSG_STATE_CHANGE) // 0x0196 (9 bytes)
	{
		if (PacketSize < 9)
		{
			return;
		}
		EfstID = *reinterpret_cast<const uint16*>(PacketData + 2);
		UnitID = static_cast<int64>(*reinterpret_cast<const uint32*>(PacketData + 4));
		uint8 Flag = PacketData[8];
		bActive = (Flag != 0);
		TotalMs = 0;
		RemainMs = 0;
	}
	else if (PacketID == RagnarokPackets::HEADER_ZC_EFST_SET_ENTER) // 0x0984 (28 bytes)
	{
		if (PacketSize < 28)
		{
			return;
		}
		UnitID = static_cast<int64>(*reinterpret_cast<const uint32*>(PacketData + 2));
		EfstID = *reinterpret_cast<const uint16*>(PacketData + 6);
		TotalMs = static_cast<int32>(*reinterpret_cast<const uint32*>(PacketData + 8));
		RemainMs = static_cast<int32>(*reinterpret_cast<const uint32*>(PacketData + 12));
		bActive = true;
	}
	else
	{
		return;
	}

	if (bLogPackets)
	{
		UE_LOG(LogTemp, Log, TEXT("[RagnarokNetwork] [OnStatusChange] UnitID=%lld EfstID=%d Active=%d TotalMs=%d RemainMs=%d"),
			UnitID, EfstID, bActive ? 1 : 0, TotalMs, RemainMs);
	}

	OnStatusChange.Broadcast(UnitID, EfstID, bActive, TotalMs, RemainMs);
}

void URagnarokNetworkSubsystem::HandleSkillUnitAppear(const uint8* PacketData, int32 PacketSize)
{
	// 0x09ca ZC_SKILL_ENTRY5 (23 bytes): [id 0x09ca][len 23][unit_id 4][src_id 8][x 12][y 14][unit_type 16][range 20][visible 21][skill_lv 22]
	if (PacketSize < 23)
	{
		return;
	}

	int64 UnitID = static_cast<int64>(*reinterpret_cast<const uint32*>(PacketData + 4));
	int64 OwnerID = static_cast<int64>(*reinterpret_cast<const uint32*>(PacketData + 8));
	int32 CellX = static_cast<int32>(*reinterpret_cast<const int16*>(PacketData + 12));
	int32 CellY = static_cast<int32>(*reinterpret_cast<const int16*>(PacketData + 14));
	int32 UnitType = static_cast<int32>(*reinterpret_cast<const uint32*>(PacketData + 16));

	if (bLogPackets)
	{
		UE_LOG(LogTemp, Log, TEXT("[RagnarokNetwork] [OnSkillUnitAppear] UnitID=%lld OwnerID=%lld UnitType=%d Cell=(%d,%d)"),
			UnitID, OwnerID, UnitType, CellX, CellY);
	}

	OnSkillUnitAppear.Broadcast(UnitID, OwnerID, UnitType, CellX, CellY);
}

void URagnarokNetworkSubsystem::HandleSkillUnitDisappear(const uint8* PacketData, int32 PacketSize)
{
	// 0x0120 ZC_SKILL_DISAPPEAR (6 bytes): [id 0x0120][unit_id 2]
	if (PacketSize < 6)
	{
		return;
	}

	int64 UnitID = static_cast<int64>(*reinterpret_cast<const uint32*>(PacketData + 2));

	if (bLogPackets)
	{
		UE_LOG(LogTemp, Log, TEXT("[RagnarokNetwork] [OnSkillUnitDisappear] UnitID=%lld"), UnitID);
	}

	OnSkillUnitDisappear.Broadcast(UnitID);
}

void URagnarokNetworkSubsystem::HandleNotifyEffect(const uint8* PacketData, int32 PacketSize)
{
	// 0x019b ZC_NOTIFY_EFFECT (10 bytes): [id 0x019b][aid 2][effect_id 6]
	if (PacketSize < 10)
	{
		return;
	}

	int64 UnitID = static_cast<int64>(*reinterpret_cast<const uint32*>(PacketData + 2));
	int32 Type = *reinterpret_cast<const int32*>(PacketData + 6);

	if (bLogPackets)
	{
		UE_LOG(LogTemp, Log, TEXT("[RagnarokNetwork] [OnNotifyEffect] UnitID=%lld Type=%d"), UnitID, Type);
	}

	OnNotifyEffect.Broadcast(UnitID, Type);
}

void URagnarokNetworkSubsystem::HandleEquipAck(const uint8* PacketData, int32 PacketSize, uint16 PacketID)
{
	if (PacketID == RagnarokPackets::HEADER_ZC_REQ_WEAR_EQUIP_ACK) // 0x0999 (11 bytes)
	{
		if (PacketSize < 11)
		{
			return;
		}

		#pragma pack(push, 1)
		struct FZC_WearEquipAckPacket
		{
			int16 PacketType;
			uint16 Index;
			uint32 WearLocation;
			uint16 ItemSpriteNumber;
			uint8 Result;
		};
		#pragma pack(pop)

		const FZC_WearEquipAckPacket* Ack = reinterpret_cast<const FZC_WearEquipAckPacket*>(PacketData);
		// rAthena clif_equipitemack: ITEM_EQUIP_ACK_OK = 0 em PACKETVER >= 20121107
		const bool bSuccess = (Ack->Result == 0);

		if (bLogPackets)
		{
			UE_LOG(LogTemp, Log, TEXT("[RagnarokNetwork] [OnEquipResult] (Wear) Index=%d WearLocation=%u Sprite=%u Result=%d Success=%s"),
				Ack->Index, Ack->WearLocation, Ack->ItemSpriteNumber, Ack->Result, bSuccess ? TEXT("true") : TEXT("false"));
		}

		if (bSuccess)
		{
			FRagnarokInventoryItem* Item = InventoryItems.FindByPredicate([Ack](const FRagnarokInventoryItem& I)
			{
				return I.Index == Ack->Index;
			});

			if (Item)
			{
				Item->bIsEquipped = true;
				Item->WearState = Ack->WearLocation;
				Item->EquipLocation = Ack->WearLocation;
			}

			OnEquipResult.Broadcast(Ack->Index, Ack->WearLocation, true);
			OnInventory.Broadcast(InventoryItems);
		}
		else
		{
			OnEquipResult.Broadcast(Ack->Index, Ack->WearLocation, false);
		}
	}
	else if (PacketID == RagnarokPackets::HEADER_ZC_REQ_TAKEOFF_EQUIP_ACK) // 0x099a (9 bytes)
	{
		if (PacketSize < 9)
		{
			return;
		}

		#pragma pack(push, 1)
		struct FZC_TakeOffEquipAckPacket
		{
			uint16 PacketType;
			uint16 Index;
			uint32 WearLocation;
			uint8 Flag;
		};
		#pragma pack(pop)

		const FZC_TakeOffEquipAckPacket* Ack = reinterpret_cast<const FZC_TakeOffEquipAckPacket*>(PacketData);
		// rAthena clif_unequipitemack: success = !success em PACKETVER >= 20110824, logo Flag == 0 significa sucesso
		const bool bSuccess = (Ack->Flag == 0);

		if (bLogPackets)
		{
			UE_LOG(LogTemp, Log, TEXT("[RagnarokNetwork] [OnEquipResult] (TakeOff) Index=%d WearLocation=%u Flag=%d Success=%s"),
				Ack->Index, Ack->WearLocation, Ack->Flag, bSuccess ? TEXT("true") : TEXT("false"));
		}

		if (bSuccess)
		{
			FRagnarokInventoryItem* Item = InventoryItems.FindByPredicate([Ack](const FRagnarokInventoryItem& I)
			{
				return I.Index == Ack->Index;
			});

			if (Item)
			{
				Item->bIsEquipped = false;
				Item->WearState = 0;
				Item->EquipLocation = (Item->Location != 0) ? Item->Location : Ack->WearLocation;
			}

			OnEquipResult.Broadcast(Ack->Index, 0, true);
			OnInventory.Broadcast(InventoryItems);
		}
		else
		{
			OnEquipResult.Broadcast(Ack->Index, Ack->WearLocation, false);
		}
	}
}

void URagnarokNetworkSubsystem::HandleAttackOutOfRange(const uint8* PacketData, int32 PacketSize)
{
	// 0x0139 ZC_ATTACK_FAILURE_FOR_DISTANCE (16 bytes)
	// targetAID: +2 (u32), targetXPos: +6 (i16), targetYPos: +8 (i16), xPos: +10 (i16), yPos: +12 (i16), currentAttRange: +14 (i16)
	if (PacketSize < 16)
	{
		return;
	}

	#pragma pack(push, 1)
	struct FZC_AttackFailureForDistancePacket
	{
		int16 PacketType;
		uint32 TargetAID;
		int16 TargetX;
		int16 TargetY;
		int16 X;
		int16 Y;
		int16 CurrentAttRange;
	};
	#pragma pack(pop)

	const FZC_AttackFailureForDistancePacket* Pkt = reinterpret_cast<const FZC_AttackFailureForDistancePacket*>(PacketData);

	if (bLogPackets)
	{
		UE_LOG(LogTemp, Log, TEXT("[RagnarokNetwork] [OnAttackOutOfRange] TargetAID=%u TargetPos=(%d,%d) Pos=(%d,%d) Range=%d"),
			Pkt->TargetAID, Pkt->TargetX, Pkt->TargetY, Pkt->X, Pkt->Y, Pkt->CurrentAttRange);
	}

	OnAttackOutOfRange.Broadcast(Pkt->TargetAID, Pkt->TargetX, Pkt->TargetY, Pkt->CurrentAttRange);
}

void URagnarokNetworkSubsystem::HandleNotifyMove(const uint8* PacketData, int32 PacketSize)
{
	if (PacketSize < 16)
	{
		return;
	}

	uint32 GID = *reinterpret_cast<const uint32*>(PacketData + 2);
	int32 FromX = 0, FromY = 0, ToX = 0, ToY = 0;
	RagnarokPackets::UnpackMovePath(PacketData + 6, FromX, FromY, ToX, ToY);
	uint32 StartTick = *reinterpret_cast<const uint32*>(PacketData + 12);

	if (bLogPackets)
	{
		UE_LOG(LogTemp, Log, TEXT("[RagnarokNetwork] [OnUnitWalk] GID=%u From=(%d,%d) To=(%d,%d) Tick=%u"),
			GID, FromX, FromY, ToX, ToY, StartTick);
	}

	OnUnitWalk.Broadcast(GID, FromX, FromY, ToX, ToY, static_cast<int32>(StartTick));
	OnUnitMove.Broadcast(GID, ToX, ToY, static_cast<int32>(StartTick));
}

void URagnarokNetworkSubsystem::HandleStopMove(const uint8* PacketData, int32 PacketSize)
{
	if (PacketSize < 10)
	{
		return;
	}

	uint32 AID = *reinterpret_cast<const uint32*>(PacketData + 2);
	uint16 xPos = *reinterpret_cast<const uint16*>(PacketData + 6);
	uint16 yPos = *reinterpret_cast<const uint16*>(PacketData + 8);

	if (bLogPackets)
	{
		UE_LOG(LogTemp, Log, TEXT("[RagnarokNetwork] [OnUnitStop] AID=%u Pos=(%u,%u)"),
			AID, xPos, yPos);
	}

	OnUnitStop.Broadcast(AID, static_cast<int32>(xPos), static_cast<int32>(yPos));
}


void URagnarokNetworkSubsystem::HandleChangeDirection(const uint8* PacketData, int32 PacketSize)
{
	// 009c <id>.L <head dir>.W <dir>.B (PACKET_ZC_CHANGE_DIRECTION, 9 bytes); o servidor nao reenvia ao proprio autor
	if (PacketSize < 9)
	{
		return;
	}
	const uint32 AID = *reinterpret_cast<const uint32*>(PacketData + 2);
	const uint16 HeadDir = *reinterpret_cast<const uint16*>(PacketData + 6);
	const uint8 Dir = PacketData[8];
	if (bLogPackets)
	{
		UE_LOG(LogTemp, Log, TEXT("[RagnarokNetwork] [OnUnitDirection] AID=%u head=%u dir=%u"), AID, HeadDir, Dir);
	}
	OnUnitDirection.Broadcast(AID, static_cast<uint8>(FMath::Min<uint16>(HeadDir, 2)), Dir & 7);
}

void URagnarokNetworkSubsystem::HandleOptionChange(const uint8* PacketData, int32 PacketSize, uint16 PacketID)
{
	// 0229 <id>.L <body state>.W <health state>.W <effect state>.L <pk mode>.B (15 bytes)
	// 0119 <id>.L <body state>.W <health state>.W <effect state>.W <pk mode>.B (13 bytes)
	const bool bWide = PacketID == 0x0229;
	if (PacketSize < (bWide ? 15 : 13))
	{
		return;
	}
	const uint32 AID = *reinterpret_cast<const uint32*>(PacketData + 2);
	const int32 Option = bWide ? *reinterpret_cast<const int32*>(PacketData + 10) : static_cast<int32>(*reinterpret_cast<const uint16*>(PacketData + 10));
	if (bLogPackets)
	{
		UE_LOG(LogTemp, Log, TEXT("[RagnarokNetwork] [OnUnitOption] AID=%u option=0x%X"), AID, Option);
	}
	OnUnitOption.Broadcast(AID, Option);
	OnUnitBodyState.Broadcast(AID, static_cast<int32>(*reinterpret_cast<const uint16*>(PacketData + 6)), static_cast<int32>(*reinterpret_cast<const uint16*>(PacketData + 8)));
}

void URagnarokNetworkSubsystem::HandleResurrection(const uint8* PacketData, int32 PacketSize)
{
	// 0148 <id>.L <type>.W (PACKET_ZC_RESURRECTION, 8 bytes)
	if (PacketSize < 8)
	{
		return;
	}
	OnUnitRevive.Broadcast(*reinterpret_cast<const uint32*>(PacketData + 2));
}

bool URagnarokNetworkSubsystem::SendShopDealType(int64 NpcID, uint8 Type)
{
	// CZ_ACK_SELECT_DEALTYPE 0x00c5 <npc id>.L <type>.B (7 bytes): 0 comprar, 1 vender
	if (!CurrentSocket || CurrentState != ERagnarokNetworkState::ConnectedMap)
	{
		return false;
	}
	uint8 Packet[7] = { 0xc5, 0x00 };
	*reinterpret_cast<uint32*>(Packet + 2) = static_cast<uint32>(NpcID);
	Packet[6] = Type;
	SendData(Packet, sizeof(Packet));
	return true;
}

bool URagnarokNetworkSubsystem::SendShopBuy(const TArray<TPair<int32, int32>>& ItemIdAmount)
{
	// CZ_PC_PURCHASE_ITEMLIST 0x00c8 <len>.W { <amount>.W <item id>.L }* (PACKETVER >= 20181121: id de 4 bytes)
	if (!CurrentSocket || CurrentState != ERagnarokNetworkState::ConnectedMap || ItemIdAmount.Num() == 0)
	{
		return false;
	}
	TArray<uint8> Packet;
	Packet.SetNumZeroed(4 + 6 * ItemIdAmount.Num());
	Packet[0] = 0xc8;
	*reinterpret_cast<uint16*>(Packet.GetData() + 2) = static_cast<uint16>(Packet.Num());
	for (int32 i = 0; i < ItemIdAmount.Num(); ++i)
	{
		*reinterpret_cast<uint16*>(Packet.GetData() + 4 + 6 * i) = static_cast<uint16>(ItemIdAmount[i].Value);
		*reinterpret_cast<uint32*>(Packet.GetData() + 6 + 6 * i) = static_cast<uint32>(ItemIdAmount[i].Key);
	}
	SendData(Packet.GetData(), Packet.Num());
	return true;
}

bool URagnarokNetworkSubsystem::SendShopSell(const TArray<TPair<int32, int32>>& IndexAmount)
{
	// CZ_PC_SELL_ITEMLIST 0x00c9 <len>.W { <index>.W <amount>.W }*
	if (!CurrentSocket || CurrentState != ERagnarokNetworkState::ConnectedMap || IndexAmount.Num() == 0)
	{
		return false;
	}
	TArray<uint8> Packet;
	Packet.SetNumZeroed(4 + 4 * IndexAmount.Num());
	Packet[0] = 0xc9;
	*reinterpret_cast<uint16*>(Packet.GetData() + 2) = static_cast<uint16>(Packet.Num());
	for (int32 i = 0; i < IndexAmount.Num(); ++i)
	{
		*reinterpret_cast<uint16*>(Packet.GetData() + 4 + 4 * i) = static_cast<uint16>(IndexAmount[i].Key);
		*reinterpret_cast<uint16*>(Packet.GetData() + 6 + 4 * i) = static_cast<uint16>(IndexAmount[i].Value);
	}
	SendData(Packet.GetData(), Packet.Num());
	return true;
}

bool URagnarokNetworkSubsystem::SendShopClose()
{
	// CZ_NPC_SHOP_CLOSED 0x09d4 (2 bytes): libera o NPC (o servidor zera npc_shopid)
	if (!CurrentSocket || CurrentState != ERagnarokNetworkState::ConnectedMap)
	{
		return false;
	}
	const uint8 Packet[2] = { 0xd4, 0x09 };
	SendData(Packet, sizeof(Packet));
	return true;
}

// ---- Grupo (clif_party_*, packets_struct.hpp) ----

namespace
{
	FString PartyFixed(const uint8* Data, int32 Len)
	{
		char Buf[64] = {};
		FMemory::Memcpy(Buf, Data, FMath::Min(Len, 63));
		return UTF8_TO_TCHAR(Buf);
	}
}

bool URagnarokNetworkSubsystem::SendCreateParty(const FString& Name)
{
	// CZ_MAKE_GROUP2 0x01e8 <nome>.24B <pegar item>.B <dividir item>.B (28 bytes)
	if (!CurrentSocket || CurrentState != ERagnarokNetworkState::ConnectedMap || Name.IsEmpty())
	{
		return false;
	}
	uint8 Packet[28] = { 0xe8, 0x01 };
	FTCHARToUTF8 Utf8(*Name);
	FMemory::Memcpy(Packet + 2, Utf8.Get(), FMath::Min<int32>(Utf8.Length(), 23));
	Packet[26] = 1; // cada um fica com o que pega
	Packet[27] = 1; // dividir igualmente (o servidor decide o que vale)
	SendData(Packet, sizeof(Packet));
	return true;
}

bool URagnarokNetworkSubsystem::SendPartyInvite(const FString& CharName)
{
	// CZ_PARTY_JOIN_REQ 0x02c4 <nome>.24B (26 bytes)
	if (!CurrentSocket || CurrentState != ERagnarokNetworkState::ConnectedMap || CharName.IsEmpty())
	{
		return false;
	}
	uint8 Packet[26] = { 0xc4, 0x02 };
	FTCHARToUTF8 Utf8(*CharName);
	FMemory::Memcpy(Packet + 2, Utf8.Get(), FMath::Min<int32>(Utf8.Length(), 23));
	SendData(Packet, sizeof(Packet));
	return true;
}

bool URagnarokNetworkSubsystem::SendPartyInviteReply(int32 PartyId, bool bAccept)
{
	// CZ_PARTY_JOIN_REQ_ACK 0x02c7 <party id>.L <aceitar>.B (7 bytes)
	if (!CurrentSocket || CurrentState != ERagnarokNetworkState::ConnectedMap)
	{
		return false;
	}
	uint8 Packet[7] = { 0xc7, 0x02 };
	*reinterpret_cast<uint32*>(Packet + 2) = static_cast<uint32>(PartyId);
	Packet[6] = bAccept ? 1 : 0;
	SendData(Packet, sizeof(Packet));
	return true;
}

bool URagnarokNetworkSubsystem::SendLeaveParty()
{
	// CZ_REQ_LEAVE_GROUP 0x0100 (2 bytes)
	if (!CurrentSocket || CurrentState != ERagnarokNetworkState::ConnectedMap)
	{
		return false;
	}
	const uint8 Packet[2] = { 0x00, 0x01 };
	SendData(Packet, sizeof(Packet));
	return true;
}

bool URagnarokNetworkSubsystem::SendPartyExpel(int32 AccountId, const FString& CharName)
{
	// CZ_REQ_EXPEL_GROUP_MEMBER 0x0103 <aid>.L <nome>.24B (30 bytes)
	if (!CurrentSocket || CurrentState != ERagnarokNetworkState::ConnectedMap)
	{
		return false;
	}
	uint8 Packet[30] = { 0x03, 0x01 };
	*reinterpret_cast<uint32*>(Packet + 2) = static_cast<uint32>(AccountId);
	FTCHARToUTF8 Utf8(*CharName);
	FMemory::Memcpy(Packet + 6, Utf8.Get(), FMath::Min<int32>(Utf8.Length(), 23));
	SendData(Packet, sizeof(Packet));
	return true;
}

bool URagnarokNetworkSubsystem::SendPartyChangeLeader(int32 AccountId)
{
	// CZ_PARTY_CHANGE_LEADER 0x07da <aid>.L (6 bytes)
	if (!CurrentSocket || CurrentState != ERagnarokNetworkState::ConnectedMap)
	{
		return false;
	}
	uint8 Packet[6] = { 0xda, 0x07 };
	*reinterpret_cast<uint32*>(Packet + 2) = static_cast<uint32>(AccountId);
	SendData(Packet, sizeof(Packet));
	return true;
}

void URagnarokNetworkSubsystem::HandlePartyPacket(const uint8* PacketData, int32 PacketSize, uint16 PacketID)
{
	auto Say = [this](const FString& Text) { OnChatMessage.Broadcast(TEXT(""), Text, TEXT("#FFFF00")); };
	auto FindMember = [this](int32 Aid) -> FRoPartyMember*
	{
		return PartyMembers.FindByPredicate([Aid](const FRoPartyMember& M) { return M.AccountId == Aid; });
	};
	switch (PacketID)
	{
	case 0x00fa: // <resultado>.B
	{
		const uint8 R = PacketSize >= 3 ? PacketData[2] : 0;
		switch (R)
		{
		case 0: Say(TEXT("Grupo criado.")); break;
		case 1: Say(TEXT("Já existe um grupo com esse nome.")); break;
		case 2: Say(TEXT("Você já está em um grupo.")); break;
		case 3: Say(TEXT("Não é possível criar um grupo neste mapa.")); break;
		default: break;
		}
		break;
	}
	case 0x0ae4: // <aid>.L <gid>.L <lider>.L <classe>.W <nivel>.W <x>.W <y>.W <offline>.B <grupo>.24B <nome>.24B <mapa>.16B <pegar>.B <dividir>.B
		if (PacketSize >= 89)
		{
			const int32 Aid = *reinterpret_cast<const int32*>(PacketData + 2);
			FRoPartyMember* M = FindMember(Aid);
			if (!M)
			{
				M = &PartyMembers.AddDefaulted_GetRef();
				M->AccountId = Aid;
			}
			M->bLeader = *reinterpret_cast<const uint32*>(PacketData + 10) == 0; // 0 = lider
			M->ClassId = *reinterpret_cast<const uint16*>(PacketData + 14);
			M->Level = *reinterpret_cast<const uint16*>(PacketData + 16);
			M->X = *reinterpret_cast<const int16*>(PacketData + 18);
			M->Y = *reinterpret_cast<const int16*>(PacketData + 20);
			M->bOffline = PacketData[22] != 0;
			PartyName = PartyFixed(PacketData + 23, 24);
			M->Name = PartyFixed(PacketData + 47, 24);
			M->Map = PartyFixed(PacketData + 71, 16);
			OnPartyChanged.Broadcast();
		}
		break;
	case 0x0ae5: // <len>.W <grupo>.24B { <aid>.L <gid>.L <nome>.24B <mapa>.16B <lider>.B <offline>.B <classe>.W <nivel>.W }* (54 bytes)
	{
		TArray<FRoPartyMember> Old = MoveTemp(PartyMembers);
		PartyMembers.Reset();
		PartyName = PartyFixed(PacketData + 4, 24);
		for (int32 Off = 28; Off + 54 <= PacketSize; Off += 54)
		{
			FRoPartyMember M;
			M.AccountId = *reinterpret_cast<const int32*>(PacketData + Off);
			M.Name = PartyFixed(PacketData + Off + 8, 24);
			M.Map = PartyFixed(PacketData + Off + 32, 16);
			M.bLeader = PacketData[Off + 48] == 0;
			M.bOffline = PacketData[Off + 49] != 0;
			M.ClassId = *reinterpret_cast<const uint16*>(PacketData + Off + 50);
			M.Level = *reinterpret_cast<const uint16*>(PacketData + Off + 52);
			if (const FRoPartyMember* Prev = Old.FindByPredicate([&M](const FRoPartyMember& O) { return O.AccountId == M.AccountId; }))
			{
				M.Hp = Prev->Hp;
				M.MaxHp = Prev->MaxHp;
				M.X = Prev->X;
				M.Y = Prev->Y;
				M.bDead = Prev->bDead;
			}
			PartyMembers.Add(M);
		}
		OnPartyChanged.Broadcast();
		break;
	}
	case 0x02c6: // <id do grupo>.L <nome do grupo>.24B
		if (PacketSize >= 30)
		{
			OnPartyInvite.Broadcast(*reinterpret_cast<const int32*>(PacketData + 2), PartyFixed(PacketData + 6, 24));
		}
		break;
	case 0x02c5: // <nome>.24B <resultado>.L
		if (PacketSize >= 30)
		{
			const FString Who = PartyFixed(PacketData + 2, 24);
			FString Text;
			switch (*reinterpret_cast<const int32*>(PacketData + 26))
			{
			case 0: Text = FString::Printf(TEXT("%s já está em um grupo."), *Who); break;
			case 1: Text = FString::Printf(TEXT("%s recusou o convite."), *Who); break;
			case 2: Text = FString::Printf(TEXT("%s aceitou o convite."), *Who); break;
			case 3: Text = TEXT("O grupo está cheio."); break;
			case 4: Text = FString::Printf(TEXT("Outro personagem da conta de %s já está no grupo."), *Who); break;
			case 5: Text = FString::Printf(TEXT("%s não aceita convites de grupo."), *Who); break;
			case 7: Text = TEXT("Esse jogador não está conectado."); break;
			case 8: Text = FString::Printf(TEXT("%s está em um mapa onde não se pode entrar em grupo."), *Who); break;
			case 9: Text = TEXT("Não é possível entrar em grupo neste mapa."); break;
			case 10: Text = TEXT("Não é possível convidar ou expulsar em uma masmorra de memória."); break;
			case 11: Text = FString::Printf(TEXT("%s tem um nível que não pode entrar no grupo."), *Who); break;
			default: break;
			}
			if (!Text.IsEmpty())
			{
				Say(Text);
			}
		}
		break;
	case 0x0105: // <aid>.L <nome>.24B <resultado>.B (0 saiu, 1 expulso)
		if (PacketSize >= 31)
		{
			const int32 Aid = *reinterpret_cast<const int32*>(PacketData + 2);
			const FString Who = PartyFixed(PacketData + 6, 24);
			const bool bExpelled = PacketData[30] == 1;
			if (static_cast<uint32>(Aid) == SavedAccountID)
			{
				PartyMembers.Reset();
				PartyName.Reset();
				Say(bExpelled ? TEXT("Você foi expulso do grupo.") : TEXT("Você saiu do grupo."));
			}
			else
			{
				PartyMembers.RemoveAll([Aid](const FRoPartyMember& M) { return M.AccountId == Aid; });
				Say(bExpelled ? FString::Printf(TEXT("%s foi expulso do grupo."), *Who) : FString::Printf(TEXT("%s saiu do grupo."), *Who));
			}
			OnPartyChanged.Broadcast();
		}
		break;
	case 0x080e: // <aid>.L <pv>.L <pv max>.L
		if (PacketSize >= 14)
		{
			if (FRoPartyMember* M = FindMember(*reinterpret_cast<const int32*>(PacketData + 2)))
			{
				M->Hp = *reinterpret_cast<const int32*>(PacketData + 6);
				M->MaxHp = *reinterpret_cast<const int32*>(PacketData + 10);
				OnPartyChanged.Broadcast();
			}
		}
		break;
	case 0x0107: // <aid>.L <x>.W <y>.W
		if (PacketSize >= 10)
		{
			if (FRoPartyMember* M = FindMember(*reinterpret_cast<const int32*>(PacketData + 2)))
			{
				M->X = *reinterpret_cast<const int16*>(PacketData + 6);
				M->Y = *reinterpret_cast<const int16*>(PacketData + 8);
			}
		}
		break;
	case 0x0ab2: // <aid>.L <morto>.B
		if (PacketSize >= 7)
		{
			if (FRoPartyMember* M = FindMember(*reinterpret_cast<const int32*>(PacketData + 2)))
			{
				M->bDead = PacketData[6] != 0;
				OnPartyChanged.Broadcast();
			}
		}
		break;
	default:
		break;
	}
}


void URagnarokNetworkSubsystem::HandleShopPacket(const uint8* PacketData, int32 PacketSize, uint16 PacketID)
{
	switch (PacketID)
	{
	case 0x00c4: // <npc id>.L
		if (PacketSize >= 6)
		{
			OnShopDealType.Broadcast(*reinterpret_cast<const uint32*>(PacketData + 2));
		}
		break;
	case 0x0b77: // <len>.W { <item id>.L <price>.L <discount price>.L <type>.B <view sprite>.W <location>.L }* (19 bytes cada)
	{
		TArray<FRagnarokShopItem> Items;
		for (int32 Off = 4; Off + 19 <= PacketSize; Off += 19)
		{
			FRagnarokShopItem I;
			I.ItemID = *reinterpret_cast<const int32*>(PacketData + Off);
			I.Price = *reinterpret_cast<const int32*>(PacketData + Off + 4);
			I.DiscountPrice = *reinterpret_cast<const int32*>(PacketData + Off + 8);
			I.ItemType = PacketData[Off + 12];
			Items.Add(I);
		}
		OnShopBuyList.Broadcast(Items);
		break;
	}
	case 0x00c7: // <len>.W { <index>.W <price>.L <overcharge price>.L }* (10 bytes cada)
	{
		TArray<FRagnarokShopItem> Items;
		for (int32 Off = 4; Off + 10 <= PacketSize; Off += 10)
		{
			FRagnarokShopItem I;
			I.Index = *reinterpret_cast<const uint16*>(PacketData + Off);
			I.Price = *reinterpret_cast<const int32*>(PacketData + Off + 2);
			I.DiscountPrice = *reinterpret_cast<const int32*>(PacketData + Off + 6);
			for (const FRagnarokInventoryItem& Inv : InventoryItems)
			{
				if (Inv.Index == I.Index)
				{
					I.ItemID = Inv.ItemID;
					I.ItemType = Inv.Type;
					break;
				}
			}
			Items.Add(I);
		}
		OnShopSellList.Broadcast(Items);
		break;
	}
	case 0x00ca: // <result>.B (compra)
		OnShopResult.Broadcast(false, PacketSize >= 3 ? PacketData[2] : 11);
		break;
	case 0x00cb: // <result>.B (venda)
		OnShopResult.Broadcast(true, PacketSize >= 3 ? PacketData[2] : 1);
		break;
	default:
		break;
	}
}

bool URagnarokNetworkSubsystem::SendDropItem(int32 Index, int32 Amount)
{
	// clif_parse_DropItem 0x0363 (6 bytes, clif_shuffle.hpp): indice no byte 2, quantidade no byte 4
	if (!CurrentSocket || CurrentState != ERagnarokNetworkState::ConnectedMap || Amount <= 0)
	{
		return false;
	}
	uint8 Packet[6] = { 0x63, 0x03 };
	*reinterpret_cast<uint16*>(Packet + 2) = static_cast<uint16>(Index);
	*reinterpret_cast<uint16*>(Packet + 4) = static_cast<uint16>(Amount);
	SendData(Packet, sizeof(Packet));
	return true;
}

bool URagnarokNetworkSubsystem::SendStorageMove(bool bToStorage, int32 Index, int32 Amount)
{
	// clif_parse_MoveToKafra 0x0364 / clif_parse_MoveFromKafra 0x0365 (8 bytes, clif_shuffle.hpp): indice W no byte 2, quantidade L no 4
	if (!CurrentSocket || CurrentState != ERagnarokNetworkState::ConnectedMap || Amount <= 0)
	{
		return false;
	}
	uint8 Packet[8] = { static_cast<uint8>(bToStorage ? 0x64 : 0x65), 0x03 };
	*reinterpret_cast<uint16*>(Packet + 2) = static_cast<uint16>(Index);
	*reinterpret_cast<uint32*>(Packet + 4) = static_cast<uint32>(Amount);
	SendData(Packet, sizeof(Packet));
	return true;
}

bool URagnarokNetworkSubsystem::SendStorageClose()
{
	// clif_parse_CloseKafra 0x0193 (2 bytes)
	if (!CurrentSocket || CurrentState != ERagnarokNetworkState::ConnectedMap)
	{
		return false;
	}
	const uint8 Packet[2] = { 0x93, 0x01 };
	SendData(Packet, sizeof(Packet));
	return true;
}

void URagnarokNetworkSubsystem::HandleStoragePacket(const uint8* PacketData, int32 PacketSize, uint16 PacketID)
{
	switch (PacketID)
	{
	case 0x0b08: // <len>.W <invType>.B <name>: lista nova do armazem (invType 2) comeca
		if (PacketSize >= 5 && PacketData[4] == 2)
		{
			StorageItems.Reset();
		}
		break;
	case 0x0b0b: // <invType>.B <flag>.B: fim da lista
		if (PacketSize >= 3 && PacketData[2] == 2)
		{
			OnStorageOpen.Broadcast(StorageItems);
		}
		break;
	case 0x0b44: // <index>.W <amount>.L <item id>.L <type>.B <identified>.B <damaged>.B <slots 16> <options 25> <refine>.B <grade>.B
	{
		if (PacketSize < 58)
		{
			break;
		}
		const int32 Index = *reinterpret_cast<const int16*>(PacketData + 2);
		const int32 Amount = *reinterpret_cast<const int32*>(PacketData + 4);
		FRagnarokInventoryItem* Found = StorageItems.FindByPredicate([Index](const FRagnarokInventoryItem& I) { return I.Index == Index; });
		if (Found)
		{
			Found->Amount += Amount;
		}
		else
		{
			FRagnarokInventoryItem Item;
			Item.Index = Index;
			Item.Amount = Amount;
			Item.ItemID = *reinterpret_cast<const uint32*>(PacketData + 8);
			Item.Type = PacketData[12];
			Item.bIdentified = PacketData[13] != 0;
			Item.Refine = PacketData[56];
			StorageItems.Add(Item);
		}
		OnStorageChanged.Broadcast(StorageItems);
		break;
	}
	case 0x00f6: // <index>.W <amount>.L
	{
		if (PacketSize < 8)
		{
			break;
		}
		const int32 Index = *reinterpret_cast<const int16*>(PacketData + 2);
		const int32 Amount = *reinterpret_cast<const int32*>(PacketData + 4);
		for (int32 i = 0; i < StorageItems.Num(); ++i)
		{
			if (StorageItems[i].Index == Index)
			{
				StorageItems[i].Amount -= Amount;
				if (StorageItems[i].Amount <= 0)
				{
					StorageItems.RemoveAt(i);
				}
				break;
			}
		}
		OnStorageChanged.Broadcast(StorageItems);
		break;
	}
	case 0x00f2: // <atual>.W <maximo>.W
		if (PacketSize >= 6)
		{
			StorageCount = *reinterpret_cast<const uint16*>(PacketData + 2);
			StorageMax = *reinterpret_cast<const uint16*>(PacketData + 4);
		}
		break;
	case 0x00f8: // armazem fechado
		OnStorageClose.Broadcast();
		break;
	default:
		break;
	}
}

bool URagnarokNetworkSubsystem::SendWhisper(const FString& TargetName, const FString& Message)
{
	// CZ_WHISPER 0x0096 <len>.W <nick>.24B <mensagem>.?B (clif_process_message com whisper: o texto vai com o terminador)
	if (!CurrentSocket || CurrentState != ERagnarokNetworkState::ConnectedMap || TargetName.IsEmpty() || Message.IsEmpty())
	{
		return false;
	}
	FTCHARToUTF8 Name(*TargetName);
	FTCHARToUTF8 Text(*Message);
	const int32 TextLen = Text.Length() + 1;
	TArray<uint8> Pkt;
	Pkt.SetNumZeroed(4 + 24 + TextLen);
	*reinterpret_cast<uint16*>(Pkt.GetData()) = 0x0096;
	*reinterpret_cast<uint16*>(Pkt.GetData() + 2) = static_cast<uint16>(Pkt.Num());
	FMemory::Memcpy(Pkt.GetData() + 4, Name.Get(), FMath::Min(Name.Length(), 23));
	FMemory::Memcpy(Pkt.GetData() + 28, Text.Get(), TextLen);
	SendData(Pkt.GetData(), Pkt.Num());
	return true;
}

bool URagnarokNetworkSubsystem::SendPartyMessage(const FString& Message)
{
	// CZ_REQUEST_CHAT_PARTY 0x0108 <len>.W "<nome> : <mensagem>" 00
	if (!CurrentSocket || CurrentState != ERagnarokNetworkState::ConnectedMap || Message.IsEmpty())
	{
		return false;
	}
	const FString Speaker = SelectedCharName.IsEmpty() ? TEXT("ThorTester") : SelectedCharName;
	FTCHARToUTF8 Text(*FString::Printf(TEXT("%s : %s"), *Speaker, *Message));
	const int32 TextLen = Text.Length() + 1;
	TArray<uint8> Pkt;
	Pkt.SetNumZeroed(4 + TextLen);
	*reinterpret_cast<uint16*>(Pkt.GetData()) = 0x0108;
	*reinterpret_cast<uint16*>(Pkt.GetData() + 2) = static_cast<uint16>(Pkt.Num());
	FMemory::Memcpy(Pkt.GetData() + 4, Text.Get(), TextLen);
	SendData(Pkt.GetData(), Pkt.Num());
	return true;
}

bool URagnarokNetworkSubsystem::SendEmotion(int32 EmotionType)
{
	// CZ_REQ_EMOTION 0x00bf <tipo>.B (3 bytes)
	if (!CurrentSocket || CurrentState != ERagnarokNetworkState::ConnectedMap)
	{
		return false;
	}
	const uint8 Packet[3] = { 0xbf, 0x00, static_cast<uint8>(EmotionType) };
	SendData(Packet, sizeof(Packet));
	return true;
}

void URagnarokNetworkSubsystem::HandleChatExtras(const uint8* PacketData, int32 PacketSize, uint16 PacketID)
{
	auto ReadText = [&](int32 Offset)
	{
		if (PacketSize <= Offset)
		{
			return FString();
		}
		TArray<uint8> Buf;
		Buf.Append(PacketData + Offset, PacketSize - Offset);
		Buf.Add(0);
		return FString(UTF8_TO_TCHAR(reinterpret_cast<const char*>(Buf.GetData())));
	};
	switch (PacketID)
	{
	case 0x008d: // <len>.W <gid>.L "<nome> : <mensagem>"
	{
		if (PacketSize < 9)
		{
			break;
		}
		const int64 Gid = *reinterpret_cast<const uint32*>(PacketData + 4);
		const FString Full = ReadText(8);
		FString Sender, Text = Full;
		int32 Colon = INDEX_NONE;
		if (Full.FindChar(TEXT(':'), Colon) && Colon > 0)
		{
			Sender = Full.Left(Colon).TrimStartAndEnd();
			Text = Full.Mid(Colon + 1).TrimStartAndEnd();
		}
		OnChatMessage.Broadcast(Sender, Text, TEXT("#FFFFFF"));
		OnSpeech.Broadcast(Gid, Text);
		break;
	}
	case 0x0109: // <len>.W <aid>.L "<nome> : <mensagem>" (canal do grupo, ciano)
	{
		FString Full = ReadText(8);
		FString Sender, Text = Full;
		int32 Colon = INDEX_NONE;
		if (Full.FindChar(TEXT(':'), Colon) && Colon > 0)
		{
			Sender = FString::Printf(TEXT("[Grupo] %s"), *Full.Left(Colon).TrimStartAndEnd());
			Text = Full.Mid(Colon + 1).TrimStartAndEnd();
		}
		OnChatMessage.Broadcast(Sender, Text, TEXT("#33CCFF"));
		break;
	}
	case 0x09df: // <resultado>.B <cid>.L (e_ack_whisper: 0 ok, 1 nao esta conectado, 2 ignorado, 3 todos ignorados)
	{
		const uint8 Result = PacketSize >= 3 ? PacketData[2] : 0;
		if (Result == 1) { OnChatMessage.Broadcast(TEXT(""), TEXT("Esse jogador não está conectado."), TEXT("#FF3333")); }
		else if (Result == 2) { OnChatMessage.Broadcast(TEXT(""), TEXT("Esse jogador está ignorando você."), TEXT("#FF3333")); }
		else if (Result == 3) { OnChatMessage.Broadcast(TEXT(""), TEXT("Esse jogador está ignorando todos os cochichos."), TEXT("#FF3333")); }
		break;
	}
	case 0x00c0: // <gid>.L <tipo>.B
		if (PacketSize >= 7)
		{
			OnEmotion.Broadcast(*reinterpret_cast<const uint32*>(PacketData + 2), PacketData[6]);
		}
		break;
	default:
		break;
	}
}

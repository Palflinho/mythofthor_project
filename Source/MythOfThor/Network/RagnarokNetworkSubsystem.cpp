#include "RagnarokNetworkSubsystem.h"
#include "RagnarokPacketDatabase.h"
#include "Common/TcpSocketBuilder.h"
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
		ProcessIncomingData();
	}
}

bool URagnarokNetworkSubsystem::ConnectToPort(const FString& Host, int32 Port, ERagnarokNetworkState NewState)
{
	if (CurrentSocket)
	{
		CurrentSocket->Close();
		ISocketSubsystem::Get(PLATFORM_SOCKETSUBSYSTEM)->DestroySocket(CurrentSocket);
		CurrentSocket = nullptr;
	}

	ReceiveBuffer.Empty();

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
		uint16 PacketID = 0x0438;
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

void URagnarokNetworkSubsystem::SendData(const uint8* Data, int32 Size)
{
	if (!CurrentSocket || !Data || Size <= 0)
	{
		return;
	}

	int32 BytesSent = 0;
	CurrentSocket->Send(Data, Size, BytesSent);
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
		OnNetworkDisconnected.Broadcast(TEXT("Conexão encerrada pelo cliente."));
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
		int16 ExpectedLen = RagnarokPackets::GetPacketLength(PacketID);

		if (ExpectedLen == 0)
		{
			// Pacote não reconhecido ou dessincronizado no fluxo TCP
			UE_LOG(LogTemp, Warning, TEXT("[RagnarokNetwork] Pacote desconhecido 0x%04x recebido no estado %d. Descartando 1 byte."), PacketID, (int32)CurrentState);
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

		switch (PacketID)
		{
			case RagnarokPackets::HEADER_AC_ACCEPT_LOGIN:
				HandleLoginAccept(PacketData, ActualLen);
				break;
			case RagnarokPackets::HEADER_AC_REFUSE_LOGIN:
			case RagnarokPackets::HEADER_SC_NOTIFY_BAN:
				HandleLoginRefuse(PacketData, ActualLen);
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
			case RagnarokPackets::HEADER_ZC_IDLE_UNIT:
			case RagnarokPackets::HEADER_ZC_SPAWN_UNIT:
			case RagnarokPackets::HEADER_ZC_UNIT_WALKING:
				HandleUnitAppear(PacketData, ActualLen, PacketID);
				break;
			case RagnarokPackets::HEADER_ZC_NOTIFY_PLAYERMOVE:
				HandlePlayerMove(PacketData, ActualLen);
				break;
			case RagnarokPackets::HEADER_ZC_NOTIFY_VANISH:
				HandleUnitVanish(PacketData, ActualLen);
				break;
			case RagnarokPackets::HEADER_ZC_NOTIFY_ACT:
				HandleActionNotify(PacketData, ActualLen);
				break;
			case RagnarokPackets::HEADER_ZC_PAR_CHANGE:
				HandleParamChange(PacketData, ActualLen, false);
				break;
			case RagnarokPackets::HEADER_ZC_LONGPAR_CHANGE:
				HandleParamChange(PacketData, ActualLen, true);
				break;
			case RagnarokPackets::HEADER_ZC_SPRITE_CHANGE:
				HandleSpriteChange(PacketData, ActualLen);
				break;
			default:
				// Pacote conhecido mas sem tratamento visual imediato (ex.: slots, ticks, ping)
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
		CurrentState = ERagnarokNetworkState::ConnectedMap;
	}
}

void URagnarokNetworkSubsystem::HandleMapAcceptEnter(const uint8* PacketData, int32 PacketSize)
{
	int32 CellX = 0, CellY = 0;
	uint8 Direction = 0;
	RagnarokPackets::UnpackCellCoord(PacketData + 6, CellX, CellY, Direction);

	OnMapEnter.Broadcast(PendingMapName, CellX, CellY, Direction);

	// Notifica fim do carregamento do mapa para o servidor liberar o ator (CZ_NOTIFY_ACTORINIT 0x007D)
	uint16 LoadAck = RagnarokPackets::HEADER_CZ_NOTIFY_ACTORINIT;
	SendData(reinterpret_cast<const uint8*>(&LoadAck), sizeof(LoadAck));
}

void URagnarokNetworkSubsystem::HandleUnitAppear(const uint8* PacketData, int32 PacketSize, uint16 PacketID)
{
	FRagnarokUnitVisualInfo Unit;
	Unit.ObjectType = PacketData[4];
	Unit.GID = *reinterpret_cast<const uint32*>(PacketData + 9);
	Unit.Speed = *reinterpret_cast<const int16*>(PacketData + 13);
	Unit.JobClass = *reinterpret_cast<const int16*>(PacketData + 23);
	Unit.Head = *reinterpret_cast<const uint16*>(PacketData + 25);
	Unit.Weapon = *reinterpret_cast<const uint32*>(PacketData + 27);
	Unit.Shield = *reinterpret_cast<const uint32*>(PacketData + 31);
	Unit.HeadTop = *reinterpret_cast<const uint16*>(PacketData + 35);
	Unit.HeadMid = *reinterpret_cast<const uint16*>(PacketData + 37);
	Unit.HeadBottom = *reinterpret_cast<const uint16*>(PacketData + 39);
	Unit.HeadPalette = *reinterpret_cast<const int16*>(PacketData + 41);
	Unit.BodyPalette = *reinterpret_cast<const int16*>(PacketData + 43);
	Unit.Robe = *reinterpret_cast<const uint16*>(PacketData + 47);

	// Coordenadas
	if (PacketID == RagnarokPackets::HEADER_ZC_IDLE_UNIT || PacketID == RagnarokPackets::HEADER_ZC_SPAWN_UNIT)
	{
		RagnarokPackets::UnpackCellCoord(PacketData + 60, Unit.CellX, Unit.CellY, Unit.Direction);
	}
	else if (PacketID == RagnarokPackets::HEADER_ZC_UNIT_WALKING)
	{
		int32 FromX = 0, FromY = 0, ToX = 0, ToY = 0;
		RagnarokPackets::UnpackMovePath(PacketData + 60, FromX, FromY, ToX, ToY);
		Unit.CellX = FromX;
		Unit.CellY = FromY;
		OnUnitMove.Broadcast(Unit.GID, ToX, ToY, static_cast<int32>(FPlatformTime::Cycles()));
	}

	char NameBuf[25] = { 0 };
	int32 NameOffset = PacketSize - 24;
	if (NameOffset >= 0 && NameOffset < PacketSize)
	{
		FMemory::Memcpy(NameBuf, PacketData + NameOffset, 24);
		Unit.Name = UTF8_TO_TCHAR(NameBuf);
	}

	OnUnitAppear.Broadcast(Unit);
}

void URagnarokNetworkSubsystem::HandlePlayerMove(const uint8* PacketData, int32 PacketSize)
{
	int32 FromX = 0, FromY = 0, ToX = 0, ToY = 0;
	RagnarokPackets::UnpackMovePath(PacketData + 6, FromX, FromY, ToX, ToY);

	OnUnitMove.Broadcast(PendingCharID, ToX, ToY, static_cast<int32>(FPlatformTime::Cycles()));
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
	int32 DelayMs = *reinterpret_cast<const int32*>(PacketData + 18);
	int32 Damage = *reinterpret_cast<const int32*>(PacketData + 22);
	uint8 ActionType = PacketData[31];

	OnDamage.Broadcast(SrcID, TargetID, Damage, ActionType, DelayMs);
	OnDamageReceived.Broadcast(TargetID, Damage, ActionType == 1, Damage == 0);
}

void URagnarokNetworkSubsystem::HandleParamChange(const uint8* PacketData, int32 PacketSize, bool bIsLong)
{
	uint16 ParamType = *reinterpret_cast<const uint16*>(PacketData + 2);
	int32 Value = *reinterpret_cast<const int32*>(PacketData + 4);
	OnStatusChanged.Broadcast(ParamType, Value);
}

void URagnarokNetworkSubsystem::HandleSpriteChange(const uint8* PacketData, int32 PacketSize)
{
	uint32 GID = *reinterpret_cast<const uint32*>(PacketData + 2);
	uint8 Slot = PacketData[6];
	int32 VisualID = *reinterpret_cast<const int32*>(PacketData + 7);
	OnEquipChanged.Broadcast(GID, Slot, VisualID);
}

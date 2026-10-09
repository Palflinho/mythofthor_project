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

	for (const FRagnarokCharacterSlotInfo& Slot : CachedCharacterSlots)
	{
		if (Slot.Slot == SlotIndex)
		{
			SelectedCharName = Slot.Name;
			break;
		}
	}
	if (SelectedCharName.IsEmpty() && CachedCharacterSlots.IsValidIndex(SlotIndex))
	{
		SelectedCharName = CachedCharacterSlots[SlotIndex].Name;
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
		uint16 PacketID = RagnarokPackets::HEADER_CZ_ITEM_PICKUP; // 0x0362
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
			uint16 PacketID = 0x0438;
			uint16 SkillLevel = 1;
			uint16 SkillID = 0;
			uint16 X = 0;
			uint16 Y = 0;
		};
		#pragma pack(pop)

		FCZ_UseSkillToPosPacket Packet;
		Packet.SkillLevel = static_cast<uint16>(SkillLevel);
		Packet.SkillID = static_cast<uint16>(SkillID);
		Packet.X = static_cast<uint16>(TargetX);
		Packet.Y = static_cast<uint16>(TargetY);
		SendData(reinterpret_cast<const uint8*>(&Packet), sizeof(Packet));
		return true;
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
	OnEquipChanged.Broadcast(GID, Slot, VisualID);
}

void URagnarokNetworkSubsystem::HandleChatMessage(const uint8* PacketData, int32 PacketSize, uint16 PacketID)
{
	FString Sender = TEXT("System");
	FString Text = TEXT("");
	FString ColorHex = TEXT("#FFFFFF");

	if (PacketID == RagnarokPackets::HEADER_ZC_NOTIFY_CHAT) // 0x008e
	{
		if (PacketSize > 8)
		{
			int32 StrLen = PacketSize - 8;
			TArray<uint8> CleanBuf;
			CleanBuf.Append(PacketData + 8, StrLen);
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
			CurrentStats.Agi = Pkt->agi;
			CurrentStats.Vit = Pkt->vit;
			CurrentStats.Int = Pkt->int_;
			CurrentStats.Dex = Pkt->dex;
			CurrentStats.Luk = Pkt->luk;
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
		OnStatusChanged.Broadcast(StatusID, Value);
	}
	else if (PacketID == RagnarokPackets::HEADER_ZC_STATUS_CHANGE_ACK) // 0x00bc - 6 bytes
	{
		uint16 SP = *reinterpret_cast<const uint16*>(PacketData + 2);
		uint8 bOk = PacketData[4];
		uint8 Val = PacketData[5];
		if (bOk)
		{
			OnStatusChanged.Broadcast(SP, Val);
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

	FString Title = TEXT("");
	TArray<FString> Options;
	if (Parts.Num() > 1)
	{
		Title = Parts[0];
		for (int32 i = 1; i < Parts.Num(); ++i)
		{
			if (!Parts[i].IsEmpty())
			{
				Options.Add(Parts[i]);
			}
		}
	}
	else if (Parts.Num() == 1)
	{
		Options.Add(Parts[0]);
	}

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
	if (bIsEquip)
	{
		static constexpr int32 HEADER_SIZE = 5;
		static constexpr int32 EQUIP_INFO_SIZE = 68;

		int32 Available = PacketSize - HEADER_SIZE;
		int32 Count = Available / EQUIP_INFO_SIZE;

		InventoryItems.RemoveAll([](const FRagnarokInventoryItem& Item) { return Item.bIsEquipped; });

		for (int32 i = 0; i < Count; ++i)
		{
			const uint8* Ptr = PacketData + HEADER_SIZE + (i * EQUIP_INFO_SIZE);
			FRagnarokInventoryItem Item;
			Item.Index = *reinterpret_cast<const int16*>(Ptr + 0);
			Item.ItemID = *reinterpret_cast<const uint32*>(Ptr + 2);
			Item.Type = Ptr[6];
			Item.EquipLocation = *reinterpret_cast<const uint32*>(Ptr + 7);
			Item.bIsEquipped = (*reinterpret_cast<const uint32*>(Ptr + 11) != 0);
			Item.Amount = 1;

			Item.Slot1 = *reinterpret_cast<const uint32*>(Ptr + 15);
			Item.Slot2 = *reinterpret_cast<const uint32*>(Ptr + 19);
			Item.Slot3 = *reinterpret_cast<const uint32*>(Ptr + 23);
			Item.Slot4 = *reinterpret_cast<const uint32*>(Ptr + 27);

			Item.Refine = Ptr[65];
			Item.bIdentified = (Ptr[67] & 1) != 0;

			InventoryItems.Add(Item);
		}
	}
	else
	{
		static constexpr int32 HEADER_SIZE = 5;
		static constexpr int32 NORMAL_INFO_SIZE = 34;

		int32 Available = PacketSize - HEADER_SIZE;
		int32 Count = Available / NORMAL_INFO_SIZE;

		InventoryItems.RemoveAll([](const FRagnarokInventoryItem& Item) { return !Item.bIsEquipped; });

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

			InventoryItems.Add(Item);
		}
	}

	OnInventory.Broadcast(InventoryItems);
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
	}
}

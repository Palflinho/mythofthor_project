#include "RagnarokNetworkSubsystem.h"
#include "Common/TcpSocketBuilder.h"
#include "IPAddress.h"

void URagnarokNetworkSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);
	CurrentSocket = nullptr;
}

void URagnarokNetworkSubsystem::Deinitialize()
{
	Disconnect();
	Super::Deinitialize();
}

bool URagnarokNetworkSubsystem::ConnectToPort(const FString& Host, int32 Port)
{
	Disconnect();

	ISocketSubsystem* const SocketSubsystem = ISocketSubsystem::Get(PLATFORM_SOCKETSUBSYSTEM);
	if (!SocketSubsystem)
	{
		OnNetworkDisconnected.Broadcast(TEXT("Falha ao inicializar subsistema de socket da Unreal."));
		return false;
	}

	TSharedRef<FInternetAddr> Addr = SocketSubsystem->CreateInternetAddr();
	bool bIsValid = false;
	Addr->SetIp(*Host, bIsValid);
	Addr->SetPort(Port);

	if (!bIsValid)
	{
		// Tenta resolução por DNS/Hostname caso não seja um IP bruto
		ESocketErrors ResolveError = ESocketErrors::SE_NO_ERROR;
		TSharedPtr<FInternetAddr> ResolvedAddr = SocketSubsystem->GetAddressFromString(Host);
		if (ResolvedAddr.IsValid())
		{
			Addr = ResolvedAddr.ToSharedRef();
			Addr->SetPort(Port);
		}
		else
		{
			OnNetworkDisconnected.Broadcast(FString::Printf(TEXT("Endereço de IP inválido: %s"), *Host));
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
		OnNetworkDisconnected.Broadcast(TEXT("Não foi possível criar o socket TCP."));
		return false;
	}

	CurrentSocket->Connect(*Addr);
	OnNetworkConnected.Broadcast(FString::Printf(TEXT("Conectado a %s:%d"), *Host, Port));
	return true;
}

bool URagnarokNetworkSubsystem::ConnectAndLogin(const FString& ServerHost, const FString& Username, const FString& Password)
{
	if (!ConnectToPort(ServerHost, LOGIN_SERVER_PORT))
	{
		return false;
	}

	// Montagem do pacote CA_LOGIN (0x0064) - 55 Bytes
	// Estrutura: PacketID (2B), Version (4B), Username (24B), Password (24B), ClientType (1B)
	#pragma pack(push, 1)
	struct FCA_LoginPacket
	{
		uint16 PacketID = 0x0064;
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
	return true;
}

bool URagnarokNetworkSubsystem::SendMoveRequest(int32 TargetX, int32 TargetY)
{
	if (!CurrentSocket)
	{
		return false;
	}

	// Montagem do pacote CZ_REQUEST_MOVE (0x035F) - 5 Bytes
	// Estrutura: PacketID (2B), PackedCoords (3B) [10 bits X, 10 bits Y]
	#pragma pack(push, 1)
	struct FCZ_MovePacket
	{
		uint16 PacketID = 0x035F;
		uint8 Coords[3] = { 0 };
	};
	#pragma pack(pop)

	FCZ_MovePacket Packet;
	const uint32 PackedVal = ((TargetX & 0x3FF) << 14) | ((TargetY & 0x3FF) << 4);

	Packet.Coords[0] = static_cast<uint8>((PackedVal >> 16) & 0xFF);
	Packet.Coords[1] = static_cast<uint8>((PackedVal >> 8) & 0xFF);
	Packet.Coords[2] = static_cast<uint8>(PackedVal & 0xFF);

	SendData(reinterpret_cast<const uint8*>(&Packet), sizeof(Packet));
	OnPlayerMoveConfirmed.Broadcast(TargetX, TargetY);
	return true;
}

bool URagnarokNetworkSubsystem::SendActionRequest(int64 TargetID, uint8 ActionType)
{
	if (!CurrentSocket)
	{
		return false;
	}

	// Montagem do pacote CZ_REQUEST_ACT (0x0360) - 7 Bytes
	// Estrutura: PacketID (2B), TargetID (4B), ActionType (1B)
	#pragma pack(push, 1)
	struct FCZ_ActionPacket
	{
		uint16 PacketID = 0x0360;
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
	if (!CurrentSocket)
	{
		return false;
	}

	// Montagem do pacote CZ_USE_SKILL (0x0113) - 10 Bytes
	// Estrutura: PacketID (2B), SkillLevel (2B), SkillID (2B), TargetID (4B)
	#pragma pack(push, 1)
	struct FCZ_UseSkillPacket
	{
		uint16 PacketID = 0x0113;
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
		OnNetworkDisconnected.Broadcast(TEXT("Conexão encerrada pelo cliente."));
	}
}

void URagnarokNetworkSubsystem::ProcessIncomingData()
{
	// Processamento assíncrono de buffers TCP
}

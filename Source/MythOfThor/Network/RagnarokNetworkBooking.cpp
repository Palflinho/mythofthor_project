// Procura de grupo (party booking) - rAthena clif.cpp / party.cpp, PACKETVER 20250402 (tabela final conferida com o pre-processador):
//   CZ_PARTY_BOOKING_REQ_REGISTER 0x08a5 <nivel>.W <mapa>.W { <classe>.W }*6, CZ_PARTY_BOOKING_REQ_SEARCH 0x0804
//   <nivel>.W <mapa>.W <classe>.W <ultimo indice>.L <quantos>.W, CZ_PARTY_BOOKING_REQ_DELETE 0x0806.
//   ZC_PARTY_BOOKING_ACK_REGISTER 0x0803 <resultado>.W (0 ok, 1 falhou, 2 ja anunciado), ZC_PARTY_BOOKING_ACK_SEARCH 0x0805
//   <len>.W <tem mais>.B { <indice>.L <nome>.24B <expira>.L <nivel>.W <mapa>.W { <classe>.W }*6 }* (48 bytes cada),
//   ZC_PARTY_BOOKING_ACK_DELETE 0x0807 <resultado>.W (0 ok, 1 expirou, 2 falhou, 3 nao havia).

#include "Network/RagnarokNetworkSubsystem.h"

namespace
{
	template<typename T> T BkRead(const uint8* Data, int32 Offset) { T V; FMemory::Memcpy(&V, Data + Offset, sizeof(T)); return V; }
}

void URagnarokNetworkSubsystem::HandleBookingPacket(const uint8* PacketData, int32 PacketSize, uint16 PacketID)
{
	switch (PacketID)
	{
	case 0x0803: // register ack
		if (PacketSize >= 4)
		{
			BookingLastResult = BkRead<uint16>(PacketData, 2);
			BookingRevision++;
		}
		break;
	case 0x0807: // delete ack
		if (PacketSize >= 4)
		{
			BookingLastResult = 10 + BkRead<uint16>(PacketData, 2);
			BookingRevision++;
		}
		break;
	case 0x0805: // search results
		if (PacketSize >= 5)
		{
			BookingAds.Reset();
			bBookingMore = PacketData[4] != 0;
			for (int32 Off = 5; Off + 48 <= PacketSize; Off += 48)
			{
				FRoPartyBookingAd Ad;
				Ad.Index = BkRead<int32>(PacketData, Off);
				TArray<uint8> Name;
				Name.Append(PacketData + Off + 4, 24);
				Name.Add(0);
				Ad.Name = FString(UTF8_TO_TCHAR(reinterpret_cast<const char*>(Name.GetData())));
				Ad.Level = BkRead<uint16>(PacketData, Off + 32);
				Ad.MapId = BkRead<uint16>(PacketData, Off + 34);
				for (int32 J = 0; J < 6; ++J)
				{
					const int32 Job = BkRead<uint16>(PacketData, Off + 36 + J * 2);
					if (Job != 0 && Job != 0xFFFF) { Ad.Jobs.Add(Job); }
				}
				BookingAds.Add(Ad);
			}
			BookingLastResult = 20;
			BookingRevision++;
		}
		break;
	default:
		break;
	}
}

bool URagnarokNetworkSubsystem::SendPartyBookingRegister(int32 Level, int32 MapId, const TArray<int32>& Jobs)
{
	if (!CurrentSocket || CurrentState != ERagnarokNetworkState::ConnectedMap)
	{
		return false;
	}
	uint8 Packet[18] = { 0xa5, 0x08 };
	FMemory::Memset(Packet + 6, 0xFF, 12); // classe nao usada = 0xFF (o servidor guarda -1)
	const uint16 L = static_cast<uint16>(Level), M = static_cast<uint16>(MapId);
	FMemory::Memcpy(Packet + 2, &L, 2);
	FMemory::Memcpy(Packet + 4, &M, 2);
	for (int32 J = 0; J < 6 && J < Jobs.Num(); ++J)
	{
		const uint16 Job = static_cast<uint16>(Jobs[J]);
		FMemory::Memcpy(Packet + 6 + J * 2, &Job, 2);
	}
	SendData(Packet, sizeof(Packet));
	return true;
}

bool URagnarokNetworkSubsystem::SendPartyBookingSearch(int32 Level, int32 MapId, int32 Job, int32 LastIndex, int32 Count)
{
	if (!CurrentSocket || CurrentState != ERagnarokNetworkState::ConnectedMap)
	{
		return false;
	}
	uint8 Packet[14] = { 0x04, 0x08 };
	const uint16 L = static_cast<uint16>(Level), M = static_cast<uint16>(MapId), J = static_cast<uint16>(Job), C = static_cast<uint16>(Count);
	const uint32 Last = static_cast<uint32>(LastIndex);
	FMemory::Memcpy(Packet + 2, &L, 2);
	FMemory::Memcpy(Packet + 4, &M, 2);
	FMemory::Memcpy(Packet + 6, &J, 2);
	FMemory::Memcpy(Packet + 8, &Last, 4);
	FMemory::Memcpy(Packet + 12, &C, 2);
	SendData(Packet, sizeof(Packet));
	return true;
}

bool URagnarokNetworkSubsystem::SendPartyBookingDelete()
{
	if (!CurrentSocket || CurrentState != ERagnarokNetworkState::ConnectedMap)
	{
		return false;
	}
	uint8 Packet[2] = { 0x06, 0x08 };
	SendData(Packet, sizeof(Packet));
	return true;
}

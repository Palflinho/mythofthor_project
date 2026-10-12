// Mascote (pet) - rAthena pet.cpp / clif.cpp, PACKETVER 20250402:
//   Captura: item de domar -> ZC_START_CAPTURE 0x019e -> o jogador clica no monstro -> CZ_TRYCAPTURE_MONSTER 0x019f
//            -> ZC_TRYCAPTURE_MONSTER 0x01a0 (0 falhou, 1 capturou: o ovo vai para a mochila).
//   Chocar: Incubadora -> ZC_PETEGG_LIST 0x01a6 (lista de escolha, RagnarokNetworkSelectLists.cpp) -> CZ_SELECT_PETEGG 0x01a7.
//   Dados: ZC_PROPERTY_PET 0x01a2 <nome>.24B <renomeado>.B <nivel>.W <fome>.W <lealdade>.W <acessorio>.W <classe>.W,
//          ZC_CHANGESTATE_PET 0x01a4 <tipo>.B <id>.L <valor>.L (0 este e o seu mascote, 1 lealdade, 2 fome, 3 acessorio,
//          4 brincadeira, 5 cabelo, 6 ovo atualizado), ZC_FEED_PET 0x01a3 <ok>.B <item>.L, ZC_PET_ACT 0x01aa <id>.L <emocao>.L.
//   Pedidos: CZ_COMMAND_PET 0x01a1 <comando>.B, CZ_RENAME_PET 0x01a5 <nome>.24B, CZ_CONFIG 0x02d8 (autoalimentar).
// Fome, lealdade, comida e chance de captura sao do servidor; o cliente mostra e pede.

#include "Network/RagnarokNetworkSubsystem.h"
#include "Core/MythRoClientTables.h"

namespace
{
	template<typename T> T PetRead(const uint8* Data, int32 Offset) { T V; FMemory::Memcpy(&V, Data + Offset, sizeof(T)); return V; }

	FString PetText(const uint8* Data, int32 Size)
	{
		TArray<uint8> Buf;
		Buf.Append(Data, Size);
		Buf.Add(0);
		return FString(UTF8_TO_TCHAR(reinterpret_cast<const char*>(Buf.GetData())));
	}
}

void URagnarokNetworkSubsystem::HandlePetPacket(const uint8* PacketData, int32 PacketSize, uint16 PacketID)
{
	auto Say = [this](const FString& Text, const TCHAR* Color) { OnChatMessage.Broadcast(TEXT(""), Text, Color); };
	switch (PacketID)
	{
	case 0x019e: // ZC_START_CAPTURE: o proximo clique num monstro tenta a captura
		bCapturing = true;
		OnPet.Broadcast(0);
		break;
	case 0x01a0: // ZC_TRYCAPTURE_MONSTER <resultado>.B
		if (PacketSize >= 3)
		{
			bCapturing = false;
			Say(PacketData[2] ? TEXT("Captura bem-sucedida! O ovo do mascote está na mochila.") : TEXT("A captura falhou."),
				PacketData[2] ? TEXT("#80FF80") : TEXT("#FF8080"));
		}
		break;
	case 0x01a2: // ZC_PROPERTY_PET
		if (PacketSize >= 37)
		{
			PetInfo.Name = PetText(PacketData + 2, 24);
			PetInfo.bRenamed = PacketData[26] != 0;
			PetInfo.Level = PetRead<int16>(PacketData, 27);
			PetInfo.Hunger = PetRead<int16>(PacketData, 29);
			PetInfo.Intimacy = PetRead<int16>(PacketData, 31);
			PetInfo.Accessory = PetRead<uint16>(PacketData, 33);
			PetInfo.ClassId = PetRead<int16>(PacketData, 35);
			OnPet.Broadcast(1);
		}
		break;
	case 0x01a4: // ZC_CHANGESTATE_PET <tipo>.B <id>.L <valor>.L
		if (PacketSize >= 11)
		{
			const uint8 Type = PacketData[2];
			const int32 Id = PetRead<int32>(PacketData, 3);
			const int32 Value = PetRead<int32>(PacketData, 7);
			if (Type == 0)
			{
				PetGID = Id; // so o dono recebe o INIT
			}
			else if (Id == PetGID)
			{
				if (Type == 1) { PetInfo.Intimacy = Value; }
				else if (Type == 2) { PetInfo.Hunger = Value; }
				else if (Type == 3) { PetInfo.Accessory = Value; }
			}
			OnPet.Broadcast(1);
		}
		break;
	case 0x01a3: // ZC_FEED_PET <ok>.B <item>.L
		if (PacketSize >= 7 && PacketData[2] == 0)
		{
			// msg 591: "A comida [%s] nao esta disponivel."
			const FString Food = FMythRoClientTables::ItemName(PetRead<int32>(PacketData, 3));
			Say(FMythRoClientTables::Message(591).Replace(TEXT("%s"), *Food).TrimEnd(), TEXT("#FF8080"));
		}
		break;
	case 0x01aa: // ZC_PET_ACT <id>.L <dado>.L: emocao (valores pequenos) ou fala da pettalktable (compostos, ainda nao)
		if (PacketSize >= 10)
		{
			const int32 Data = PetRead<int32>(PacketData, 6);
			if (Data >= 0 && Data < 100)
			{
				OnEmotion.Broadcast(PetRead<uint32>(PacketData, 2), Data);
			}
		}
		break;
	default:
		break;
	}
}

bool URagnarokNetworkSubsystem::SendPetMenu(int32 Command)
{
	if (!CurrentSocket || CurrentState != ERagnarokNetworkState::ConnectedMap)
	{
		return false;
	}
	const uint8 Packet[3] = { 0xa1, 0x01, static_cast<uint8>(Command) }; // CZ_COMMAND_PET <comando>.B
	SendData(Packet, sizeof(Packet));
	return true;
}

bool URagnarokNetworkSubsystem::SendPetRename(const FString& Name)
{
	if (!CurrentSocket || CurrentState != ERagnarokNetworkState::ConnectedMap || Name.IsEmpty())
	{
		return false;
	}
	uint8 Packet[26] = { 0xa5, 0x01 }; // CZ_RENAME_PET <nome>.24B
	const FTCHARToUTF8 Utf8(*Name);
	FMemory::Memcpy(Packet + 2, Utf8.Get(), FMath::Min(Utf8.Length(), 23));
	SendData(Packet, sizeof(Packet));
	return true;
}

bool URagnarokNetworkSubsystem::SendCatchPet(int32 TargetId)
{
	if (!CurrentSocket || CurrentState != ERagnarokNetworkState::ConnectedMap)
	{
		return false;
	}
	uint8 Packet[6] = { 0x9f, 0x01 }; // CZ_TRYCAPTURE_MONSTER <alvo>.L (0 desiste: o servidor encerra a captura)
	FMemory::Memcpy(Packet + 2, &TargetId, 4);
	SendData(Packet, sizeof(Packet));
	bCapturing = false;
	return true;
}

bool URagnarokNetworkSubsystem::SendConfig(int32 Type, bool bOn)
{
	if (!CurrentSocket || CurrentState != ERagnarokNetworkState::ConnectedMap)
	{
		return false;
	}
	uint8 Packet[10] = { 0xd8, 0x02 }; // CZ_CONFIG <tipo>.L <ligado>.L; o servidor responde ZC_CONFIG 0x02d9
	const uint32 Flag = bOn ? 1 : 0;
	FMemory::Memcpy(Packet + 2, &Type, 4);
	FMemory::Memcpy(Packet + 6, &Flag, 4);
	SendData(Packet, sizeof(Packet));
	return true;
}

// Homunculo (Alquimista) - rAthena homunculus.cpp / clif.cpp, PACKETVER 20250402 (cliente "main"):
//   ZC_PROPERTY_HOMUN 0x0ba4 (85 bytes): <nome>.24B <flags>.B (1 renomeado, 2 descansando, 4 vivo) <nivel>.W <fome>.W
//     <lealdade/100>.W <atq>.W <matq>.W <precisao>.W <critico>.W <def>.W <mdef>.W <esquiva>.W <amotion>.W <hp>.L <maxhp>.L
//     <sp>.L <maxsp>.L <exp>.Q <proxima>.Q <pontos>.W <alcance>.W
//   ZC_HO_PAR_CHANGE 0x0ba5 <tipo>.W <valor>.Q (1 exp, 5 hp, 7 sp), ZC_CHANGESTATE_MER 0x0230 <tipo>.B <estado>.B <id>.L <dado>.L
//     (estado 0: este e o seu homunculo; 1 lealdade/100; 2 fome), ZC_HOSKILLINFO_LIST 0x0235, ZC_HOSKILLINFO_UPDATE 0x0239,
//     ZC_FEED_MER 0x022f <ok>.B <item>.L.
//   Pedidos: CZ_COMMAND_MER 0x022d <tipo>.W <comando>.B, CZ_RENAME_MER 0x0231, CZ_REQUEST_MOVENPC 0x0232, CZ_REQUEST_ACTNPC
//     0x0233, CZ_REQUEST_MOVETOOWNER 0x0234. As habilidades usam os mesmos pedidos do jogador (ID 8001+).
// O cliente original move e ataca com o homunculo pelos scripts de IA (AI/*.lua); aqui a IA basica fica no controle
// (seguir o dono, atacar o alvo dele) e o servidor decide tudo o mais.

#include "Network/RagnarokNetworkSubsystem.h"
#include "UI/MythRoClientTables.h"

namespace
{
	template<typename T> T HomRead(const uint8* Data, int32 Offset) { T V; FMemory::Memcpy(&V, Data + Offset, sizeof(T)); return V; }

	FString HomText(const uint8* Data, int32 Size)
	{
		TArray<uint8> Buf;
		Buf.Append(Data, Size);
		Buf.Add(0);
		return FString(UTF8_TO_TCHAR(reinterpret_cast<const char*>(Buf.GetData())));
	}
}

void URagnarokNetworkSubsystem::HandleHomunPacket(const uint8* PacketData, int32 PacketSize, uint16 PacketID)
{
	switch (PacketID)
	{
	case 0x0ba4: // ZC_PROPERTY_HOMUN
		if (PacketSize >= 85)
		{
			FRoHomunInfo& H = HomunInfo;
			H.Name = HomText(PacketData + 2, 24);
			const uint8 Flags = PacketData[26];
			H.bRenamed = (Flags & 1) != 0;
			H.bResting = (Flags & 2) != 0;
			H.bAlive = (Flags & 4) != 0;
			H.Level = HomRead<uint16>(PacketData, 27);
			H.Hunger = HomRead<uint16>(PacketData, 29);
			H.Intimacy = HomRead<uint16>(PacketData, 31);
			H.Atk = HomRead<uint16>(PacketData, 33);
			H.Matk = HomRead<uint16>(PacketData, 35);
			H.Hit = HomRead<uint16>(PacketData, 37);
			H.Crit = HomRead<uint16>(PacketData, 39);
			H.Def = HomRead<uint16>(PacketData, 41);
			H.Mdef = HomRead<uint16>(PacketData, 43);
			H.Flee = HomRead<uint16>(PacketData, 45);
			const int32 Amotion = HomRead<uint16>(PacketData, 47);
			if (Amotion > 0)
			{
				H.Amotion = Amotion; // o servidor manda 0 numa primeira copia (flag de calculo) e o valor logo depois
			}
			H.Hp = HomRead<uint32>(PacketData, 49);
			H.MaxHp = HomRead<uint32>(PacketData, 53);
			H.Sp = HomRead<uint32>(PacketData, 57);
			H.MaxSp = HomRead<uint32>(PacketData, 61);
			H.Exp = HomRead<int64>(PacketData, 65);
			H.ExpNext = HomRead<int64>(PacketData, 73);
			H.SkillPoints = HomRead<uint16>(PacketData, 81);
			H.Range = HomRead<uint16>(PacketData, 83);
			OnHomun.Broadcast(1);
		}
		break;
	case 0x0ba5: // ZC_HO_PAR_CHANGE <tipo>.W <valor>.Q
		if (PacketSize >= 12)
		{
			const uint16 Type = HomRead<uint16>(PacketData, 2);
			const int64 Value = HomRead<int64>(PacketData, 4);
			if (Type == 1) { HomunInfo.Exp = Value; }
			else if (Type == 5) { HomunInfo.Hp = Value; }
			else if (Type == 7) { HomunInfo.Sp = Value; }
			OnHomun.Broadcast(1);
		}
		break;
	case 0x0230: // ZC_CHANGESTATE_MER <tipo>.B <estado>.B <id>.L <dado>.L
		if (PacketSize >= 12)
		{
			const uint8 State = PacketData[3];
			const int32 Id = HomRead<int32>(PacketData, 4);
			const int32 Data = HomRead<int32>(PacketData, 8);
			if (State == 0) { HomunGID = Id; }
			else if (State == 1) { HomunInfo.Intimacy = Data; }
			else if (State == 2) { HomunInfo.Hunger = Data; }
			OnHomun.Broadcast(1);
		}
		break;
	case 0x0235: // ZC_HOSKILLINFO_LIST <len>.W { <id>.W <inf>.W <?>.W <nivel>.W <sp>.W <alcance>.W <nome>.24B <pode subir>.B }*
		HomunSkills.Reset();
		for (int32 Off = 4; Off + 37 <= PacketSize; Off += 37)
		{
			FRoHomunSkill S;
			S.Id = HomRead<uint16>(PacketData, Off);
			S.Inf = HomRead<uint16>(PacketData, Off + 2);
			S.Level = HomRead<uint16>(PacketData, Off + 6);
			S.Sp = HomRead<uint16>(PacketData, Off + 8);
			S.Range = HomRead<uint16>(PacketData, Off + 10);
			S.bUpgradable = PacketData[Off + 36] != 0;
			HomunSkills.Add(S);
		}
		OnHomun.Broadcast(1);
		break;
	case 0x0239: // ZC_HOSKILLINFO_UPDATE <id>.W <nivel>.W <sp>.W <alcance>.W <pode subir>.B
		if (PacketSize >= 11)
		{
			const int32 Id = HomRead<uint16>(PacketData, 2);
			if (FRoHomunSkill* S = HomunSkills.FindByPredicate([Id](const FRoHomunSkill& K) { return K.Id == Id; }))
			{
				S->Level = HomRead<int16>(PacketData, 4);
				S->Sp = HomRead<int16>(PacketData, 6);
				S->Range = HomRead<int16>(PacketData, 8);
				S->bUpgradable = PacketData[10] != 0;
			}
			OnHomun.Broadcast(1);
		}
		break;
	case 0x022f: // ZC_FEED_MER <ok>.B <item>.L
		if (PacketSize >= 7 && PacketData[2] == 0)
		{
			const FString Food = FMythRoClientTables::ItemName(HomRead<int32>(PacketData, 3));
			OnChatMessage.Broadcast(TEXT(""), FMythRoClientTables::Message(591).Replace(TEXT("%s"), *Food).TrimEnd(), TEXT("#FF8080"));
		}
		break;
	default:
		break;
	}
}

bool URagnarokNetworkSubsystem::SendHomunMenu(int32 Command)
{
	if (!CurrentSocket || CurrentState != ERagnarokNetworkState::ConnectedMap)
	{
		return false;
	}
	const uint8 Packet[5] = { 0x2d, 0x02, 0, 0, static_cast<uint8>(Command) }; // CZ_COMMAND_MER <tipo>.W (0 homunculo) <comando>.B
	SendData(Packet, sizeof(Packet));
	return true;
}

bool URagnarokNetworkSubsystem::SendHomunRename(const FString& Name)
{
	if (!CurrentSocket || CurrentState != ERagnarokNetworkState::ConnectedMap || Name.IsEmpty())
	{
		return false;
	}
	uint8 Packet[26] = { 0x31, 0x02 }; // CZ_RENAME_MER <nome>.24B
	const FTCHARToUTF8 Utf8(*Name);
	FMemory::Memcpy(Packet + 2, Utf8.Get(), FMath::Min(Utf8.Length(), 23));
	SendData(Packet, sizeof(Packet));
	return true;
}

bool URagnarokNetworkSubsystem::SendHomunMove(int32 CellX, int32 CellY)
{
	if (!CurrentSocket || CurrentState != ERagnarokNetworkState::ConnectedMap || HomunGID == 0)
	{
		return false;
	}
	// CZ_REQUEST_MOVENPC <id>.L <posicao>.3B (mesma codificacao do CZ_REQUEST_MOVE)
	uint8 Packet[9] = { 0x32, 0x02 };
	const int32 Id = static_cast<int32>(HomunGID);
	FMemory::Memcpy(Packet + 2, &Id, 4);
	Packet[6] = static_cast<uint8>((CellX >> 2) & 0xFF);
	Packet[7] = static_cast<uint8>(((CellX << 6) & 0xC0) | ((CellY >> 4) & 0x3F));
	Packet[8] = static_cast<uint8>((CellY << 4) & 0xF0);
	SendData(Packet, sizeof(Packet));
	return true;
}

bool URagnarokNetworkSubsystem::SendHomunAttack(int32 TargetId)
{
	if (!CurrentSocket || CurrentState != ERagnarokNetworkState::ConnectedMap || HomunGID == 0)
	{
		return false;
	}
	uint8 Packet[11] = { 0x33, 0x02 }; // CZ_REQUEST_ACTNPC <id>.L <alvo>.L <continuo>.B
	const int32 Id = static_cast<int32>(HomunGID);
	FMemory::Memcpy(Packet + 2, &Id, 4);
	FMemory::Memcpy(Packet + 6, &TargetId, 4);
	Packet[10] = 1;
	SendData(Packet, sizeof(Packet));
	return true;
}

bool URagnarokNetworkSubsystem::SendHomunToMaster()
{
	if (!CurrentSocket || CurrentState != ERagnarokNetworkState::ConnectedMap || HomunGID == 0)
	{
		return false;
	}
	uint8 Packet[6] = { 0x34, 0x02 }; // CZ_REQUEST_MOVETOOWNER <id>.L
	const int32 Id = static_cast<int32>(HomunGID);
	FMemory::Memcpy(Packet + 2, &Id, 4);
	SendData(Packet, sizeof(Packet));
	return true;
}

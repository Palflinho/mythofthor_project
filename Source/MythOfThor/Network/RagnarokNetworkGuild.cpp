// Cla (guilda) - rAthena guild.cpp / clif.cpp, PACKETVER 20250402 (cliente "main"):
//   ZC_UPDATE_GDID 0x02f7 (voce e de um cla), ZC_GUILD_INFO 0x0b7b, ZC_MEMBERMGR_INFO 0x0b7d, ZC_POSITION_ID_NAME_INFO 0x0166,
//   ZC_GUILD_NOTICE 0x016f, ZC_REQ_JOIN_GUILD 0x016a (convite), ZC_ACK_REQ_JOIN_GUILD 0x0169, ZC_RESULT_MAKE_GUILD 0x0167,
//   ZC_ACK_LEAVE_GUILD 0x0a83 / ZC_ACK_BAN_GUILD 0x0a82 (<CID>.L <motivo>.40B), ZC_UPDATE_CHARSTAT 0x01f2 (entrou/saiu do jogo),
//   ZC_ACK_REQ_CHANGE_MEMBERS 0x0156 (cargo mudou), ZC_ACK_DISORGANIZE_GUILD_RESULT 0x015e.
//   Pedidos: CZ_REQ_GUILD_MENU 0x014f, CZ_REQ_MAKE_GUILD 0x0165, CZ_REQ_JOIN_GUILD2 0x0916, CZ_JOIN_GUILD 0x016b,
//   CZ_REQ_LEAVE_GUILD 0x0159, CZ_REQ_BAN_GUILD 0x015b, CZ_GUILD_NOTICE 0x016e, CZ_REQ_DISORGANIZE_GUILD 0x015d.
// Quem pode convidar, expulsar e desfazer e decisao do servidor; os textos sao os da msgstringtable do cliente LATAM ("Cla").

#include "Network/RagnarokNetworkSubsystem.h"
#include "Core/MythRoClientTables.h"

namespace
{
	template<typename T> T GuildRead(const uint8* Data, int32 Offset) { T V; FMemory::Memcpy(&V, Data + Offset, sizeof(T)); return V; }

	FString GuildText(const uint8* Data, int32 Size)
	{
		TArray<uint8> Buf;
		Buf.Append(Data, Size);
		Buf.Add(0);
		return FString(UTF8_TO_TCHAR(reinterpret_cast<const char*>(Buf.GetData())));
	}

	void GuildWriteText(uint8* Dest, const FString& Text, int32 Size)
	{
		const FTCHARToUTF8 Utf8(*Text);
		FMemory::Memcpy(Dest, Utf8.Get(), FMath::Min(Utf8.Length(), Size - 1));
	}

	const TCHAR* GuildColor = TEXT("#A0FFA0");
}

void URagnarokNetworkSubsystem::ClearGuild()
{
	GuildId = 0;
	bGuildMaster = false;
	GuildName.Reset();
	GuildInfo = FRoGuildInfo();
	GuildMembers.Reset();
	GuildPositions.Reset();
	GuildPositionInfos.Reset();
	GuildSkills.Reset();
	GuildSkillPoints = 0;
	GuildBans.Reset();
	GuildRelations.Reset();
	GuildNoticeSubject.Reset();
	GuildNoticeText.Reset();
}

void URagnarokNetworkSubsystem::HandleGuildPacket(const uint8* PacketData, int32 PacketSize, uint16 PacketID)
{
	auto Say = [this](const FString& Text) { OnChatMessage.Broadcast(TEXT(""), Text, GuildColor); };
	auto Msg = [](int32 Index, const FString& Arg = FString()) { return FMythRoClientTables::Message(Index).Replace(TEXT("%s"), *Arg).TrimEnd(); };
	auto MemberName = [this](int32 CharId) -> FString
	{
		const FRoGuildMember* M = GuildMembers.FindByPredicate([CharId](const FRoGuildMember& G) { return G.CharId == CharId; });
		return M ? M->Name : FString();
	};
	switch (PacketID)
	{
	case 0x02f7: // ZC_UPDATE_GDID <cla>.L <emblema>.L <modo>.L <lider>.B <inter>.L <nome>.24B <lider CID>.L
		if (PacketSize >= 47)
		{
			GuildId = GuildRead<int32>(PacketData, 2);
			bGuildMaster = PacketData[14] != 0;
			GuildName = GuildText(PacketData + 19, 24);
			OnGuild.Broadcast(0);
		}
		break;
	case 0x0b7b: // ZC_GUILD_INFO
		if (PacketSize >= 118)
		{
			// Quem acabou de entrar nao recebe o ZC_UPDATE_GDID (o rAthena ainda nao tem o cargo dele): o cla vem daqui
			GuildId = GuildRead<int32>(PacketData, 2);
			GuildInfo.Level = GuildRead<int32>(PacketData, 6);
			GuildInfo.Online = GuildRead<int32>(PacketData, 10);
			GuildInfo.MaxMembers = GuildRead<int32>(PacketData, 14);
			GuildInfo.AvgLevel = GuildRead<int32>(PacketData, 18);
			GuildInfo.Exp = GuildRead<uint32>(PacketData, 22);
			GuildInfo.MaxExp = GuildRead<uint32>(PacketData, 26);
			GuildInfo.Name = GuildText(PacketData + 46, 24);
			GuildInfo.Land = GuildText(PacketData + 70, 16);
			GuildInfo.MasterCharId = GuildRead<int32>(PacketData, 90);
			GuildInfo.Master = GuildText(PacketData + 94, 24);
			GuildName = GuildInfo.Name;
			bGuildMaster = GuildInfo.MasterCharId == static_cast<int32>(PendingCharID);
			OnGuild.Broadcast(0);
		}
		break;
	case 0x0b7d: // ZC_MEMBERMGR_INFO <len>.W { AID.L GID.L cabelo.W cor.W sexo.W classe.W nivel.W exp.L online.L cargo.L login.L nome.24B }*
		GuildMembers.Reset();
		for (int32 Off = 4; Off + 58 <= PacketSize; Off += 58)
		{
			FRoGuildMember M;
			M.AccountId = GuildRead<int32>(PacketData, Off);
			M.CharId = GuildRead<int32>(PacketData, Off + 4);
			M.ClassId = GuildRead<int16>(PacketData, Off + 14);
			M.Level = GuildRead<int16>(PacketData, Off + 16);
			M.Exp = GuildRead<int32>(PacketData, Off + 18);
			M.bOnline = GuildRead<int32>(PacketData, Off + 22) != 0;
			M.Position = GuildRead<int32>(PacketData, Off + 26);
			M.LastLogin = GuildRead<int32>(PacketData, Off + 30);
			M.Name = GuildText(PacketData + Off + 34, 24);
			GuildMembers.Add(M);
		}
		OnGuild.Broadcast(0);
		break;
	case 0x0166: // ZC_POSITION_ID_NAME_INFO <len>.W { cargo.L nome.24B }*
		GuildPositions.Reset();
		for (int32 Off = 4; Off + 28 <= PacketSize; Off += 28)
		{
			GuildPositions.Add(GuildRead<int32>(PacketData, Off), GuildText(PacketData + Off + 4, 24));
		}
		OnGuild.Broadcast(0);
		break;
	case 0x014c: // ZC_MYGUILD_BASIC_INFO <len>.W { relacao.L cla.L nome.24B }* (32 bytes cada)
		GuildRelations.Reset();
		for (int32 Off = 4; Off + 32 <= PacketSize; Off += 32)
		{
			FRoGuildRelation Rel;
			Rel.Relation = GuildRead<int32>(PacketData, Off);
			Rel.GuildId = GuildRead<int32>(PacketData, Off + 4);
			Rel.Name = GuildText(PacketData + Off + 8, 24);
			GuildRelations.Add(Rel);
		}
		OnGuild.Broadcast(0);
		break;
	case 0x0160: // ZC_POSITION_INFO <len>.W { cargo.L direitos.L ranking.L imposto.L }*
		GuildPositionInfos.Reset();
		for (int32 Off = 4; Off + 16 <= PacketSize; Off += 16)
		{
			FRoGuildPosition P;
			P.Id = GuildRead<int32>(PacketData, Off);
			P.Rights = GuildRead<int32>(PacketData, Off + 4);
			P.Rank = GuildRead<int32>(PacketData, Off + 8);
			P.PayRate = GuildRead<int32>(PacketData, Off + 12);
			GuildPositionInfos.Add(P);
		}
		OnGuild.Broadcast(0);
		break;
	case 0x0162: // ZC_GUILD_SKILLINFO <len>.W <pontos>.W { id.W inf.L nivel.W sp.W alcance.W nome.24B sobe.B }* (37 bytes cada)
		if (PacketSize >= 6)
		{
			GuildSkillPoints = GuildRead<int16>(PacketData, 4);
			GuildSkills.Reset();
			for (int32 Off = 6; Off + 37 <= PacketSize; Off += 37)
			{
				FRoGuildSkill S;
				S.Id = GuildRead<uint16>(PacketData, Off);
				S.Level = GuildRead<uint16>(PacketData, Off + 6);
				S.Sp = GuildRead<uint16>(PacketData, Off + 8);
				S.Range = GuildRead<uint16>(PacketData, Off + 10);
				S.Name = GuildText(PacketData + Off + 12, 24);
				S.bUpgradable = PacketData[Off + 36] != 0;
				GuildSkills.Add(S);
			}
			OnGuild.Broadcast(0);
		}
		break;
	case 0x0b7c: // ZC_BAN_LIST <len>.W { CID.L motivo.40B nome.24B }* (68 bytes cada)
		GuildBans.Reset();
		for (int32 Off = 4; Off + 68 <= PacketSize; Off += 68)
		{
			FRoGuildBan B;
			B.CharId = GuildRead<int32>(PacketData, Off);
			B.Reason = GuildText(PacketData + Off + 4, 40);
			B.Name = GuildText(PacketData + Off + 44, 24);
			GuildBans.Add(B);
		}
		OnGuild.Broadcast(0);
		break;
	case 0x0156: // ZC_ACK_REQ_CHANGE_MEMBERS <len>.W { AID.L CID.L cargo.L }*
		for (int32 Off = 4; Off + 12 <= PacketSize; Off += 12)
		{
			const int32 Cid = GuildRead<int32>(PacketData, Off + 4);
			if (FRoGuildMember* M = GuildMembers.FindByPredicate([Cid](const FRoGuildMember& G) { return G.CharId == Cid; }))
			{
				M->Position = GuildRead<int32>(PacketData, Off + 8);
			}
		}
		OnGuild.Broadcast(0);
		break;
	case 0x016f: // ZC_GUILD_NOTICE <assunto>.60B <aviso>.120B
		if (PacketSize >= 182)
		{
			GuildNoticeSubject = GuildText(PacketData + 2, 60);
			GuildNoticeText = GuildText(PacketData + 62, 120);
			if (GuildId == 0)
			{
				SendGuildMenuRequest(0); // entrou agora: o aviso chega, o ZC_UPDATE_GDID nao
			}
			if (!GuildNoticeSubject.IsEmpty())
			{
				Say(FString::Printf(TEXT("[%s] %s"), *GuildNoticeSubject, *GuildNoticeText));
			}
			OnGuild.Broadcast(0);
		}
		break;
	case 0x016a: // ZC_REQ_JOIN_GUILD <cla>.L <nome>.24B
		if (PacketSize >= 30)
		{
			GuildInviteId = GuildRead<int32>(PacketData, 2);
			GuildInviteName = GuildText(PacketData + 6, 24);
			OnGuild.Broadcast(1);
		}
		break;
	case 0x0169: // ZC_ACK_REQ_JOIN_GUILD <resposta>.B: 0 ja tem cla, 1 recusou, 2 aceitou, 3 cla cheio
		if (PacketSize >= 3)
		{
			static const int32 Msgs[4] = { 378, 379, 380, 381 };
			const uint8 R = PacketData[2];
			Say(R < 4 ? Msg(Msgs[R]) : Msg(2090));
		}
		break;
	case 0x0167: // ZC_RESULT_MAKE_GUILD <resultado>.B: 0 criado, 1 ja tem cla, 2 nome existe, 3 falta o Emperium
		if (PacketSize >= 3)
		{
			static const int32 Msgs[4] = { 374, 375, 376, 405 };
			const uint8 R = PacketData[2];
			Say(Msg(R < 4 ? Msgs[R] : 375));
		}
		break;
	case 0x0a83: // ZC_ACK_LEAVE_GUILD2 <CID>.L <motivo>.40B
	case 0x0a82: // ZC_ACK_BAN_GUILD3 <motivo>.40B <CID>.L (a ordem e outra)
		if (PacketSize >= 46)
		{
			const bool bBan = PacketID == 0x0a82;
			const int32 Cid = GuildRead<int32>(PacketData, bBan ? 42 : 2);
			const FString Reason = GuildText(PacketData + (bBan ? 2 : 6), 40);
			if (Cid == static_cast<int32>(PendingCharID))
			{
				Say(Msg(bBan ? 490 : 489));
				ClearGuild();
				OnGuild.Broadcast(2);
			}
			else
			{
				Say(Msg(bBan ? 370 : 364, MemberName(Cid)));
				GuildMembers.RemoveAll([Cid](const FRoGuildMember& G) { return G.CharId == Cid; });
				OnGuild.Broadcast(0);
			}
			if (!Reason.IsEmpty())
			{
				Say(Msg(bBan ? 371 : 365, Reason));
			}
		}
		break;
	case 0x01f2: // ZC_UPDATE_CHARSTAT <AID>.L <CID>.L <online>.L <sexo>.W <cabelo>.W <cor>.W
		if (PacketSize >= 14)
		{
			const int32 Cid = GuildRead<int32>(PacketData, 6);
			const bool bOnline = GuildRead<int32>(PacketData, 10) != 0;
			if (FRoGuildMember* M = GuildMembers.FindByPredicate([Cid](const FRoGuildMember& G) { return G.CharId == Cid; }))
			{
				if (M->bOnline != bOnline)
				{
					M->bOnline = bOnline;
					Say(Msg(bOnline ? 485 : 486, M->Name));
				}
				OnGuild.Broadcast(0);
			}
		}
		break;
	case 0x015e: // ZC_ACK_DISORGANIZE_GUILD_RESULT <resultado>.L: 0 desfeito, 1 nome errado, 2 ainda ha membros
		if (PacketSize >= 6)
		{
			const int32 R = GuildRead<int32>(PacketData, 2);
			Say(Msg(R == 0 ? 400 : (R == 1 ? 401 : (R == 2 ? 402 : 366))));
			if (R == 0)
			{
				ClearGuild();
				OnGuild.Broadcast(2);
			}
		}
		break;
	default:
		break;
	}
}

bool URagnarokNetworkSubsystem::SendGuildMenuRequest(int32 Type)
{
	if (!CurrentSocket || CurrentState != ERagnarokNetworkState::ConnectedMap)
	{
		return false;
	}
	uint8 Packet[6] = { 0x4f, 0x01 }; // CZ_REQ_GUILD_MENU <tipo>.L
	FMemory::Memcpy(Packet + 2, &Type, 4);
	SendData(Packet, sizeof(Packet));
	return true;
}

bool URagnarokNetworkSubsystem::SendCreateGuild(const FString& Name)
{
	if (!CurrentSocket || CurrentState != ERagnarokNetworkState::ConnectedMap || Name.IsEmpty())
	{
		return false;
	}
	uint8 Packet[30] = { 0x65, 0x01 }; // CZ_REQ_MAKE_GUILD <CID>.L <nome>.24B
	const uint32 Cid = PendingCharID;
	FMemory::Memcpy(Packet + 2, &Cid, 4);
	GuildWriteText(Packet + 6, Name, 24);
	SendData(Packet, sizeof(Packet));
	return true;
}

bool URagnarokNetworkSubsystem::SendGuildInvite(const FString& CharName)
{
	if (!CurrentSocket || CurrentState != ERagnarokNetworkState::ConnectedMap || CharName.IsEmpty())
	{
		return false;
	}
	uint8 Packet[26] = { 0x16, 0x09 }; // CZ_REQ_JOIN_GUILD2 <nome>.24B
	GuildWriteText(Packet + 2, CharName, 24);
	SendData(Packet, sizeof(Packet));
	return true;
}

bool URagnarokNetworkSubsystem::SendGuildInviteReply(int32 InviteGuildId, bool bAccept)
{
	if (!CurrentSocket || CurrentState != ERagnarokNetworkState::ConnectedMap)
	{
		return false;
	}
	uint8 Packet[10] = { 0x6b, 0x01 }; // CZ_JOIN_GUILD <cla>.L <resposta>.L (0 recusa)
	const int32 Answer = bAccept ? 1 : 0;
	FMemory::Memcpy(Packet + 2, &InviteGuildId, 4);
	FMemory::Memcpy(Packet + 6, &Answer, 4);
	SendData(Packet, sizeof(Packet));
	GuildInviteId = 0;
	return true;
}

bool URagnarokNetworkSubsystem::SendLeaveGuild(const FString& Reason)
{
	if (!CurrentSocket || CurrentState != ERagnarokNetworkState::ConnectedMap || GuildId == 0)
	{
		return false;
	}
	uint8 Packet[54] = { 0x59, 0x01 }; // CZ_REQ_LEAVE_GUILD <cla>.L <AID>.L <CID>.L <motivo>.40B
	const int32 Aid = static_cast<int32>(GetSelfID());
	const uint32 Cid = PendingCharID;
	FMemory::Memcpy(Packet + 2, &GuildId, 4);
	FMemory::Memcpy(Packet + 6, &Aid, 4);
	FMemory::Memcpy(Packet + 10, &Cid, 4);
	GuildWriteText(Packet + 14, Reason, 40);
	SendData(Packet, sizeof(Packet));
	return true;
}

bool URagnarokNetworkSubsystem::SendGuildExpel(int32 AccountId, int32 CharId, const FString& Reason)
{
	if (!CurrentSocket || CurrentState != ERagnarokNetworkState::ConnectedMap || GuildId == 0)
	{
		return false;
	}
	uint8 Packet[54] = { 0x5b, 0x01 }; // CZ_REQ_BAN_GUILD <cla>.L <AID>.L <CID>.L <motivo>.40B
	FMemory::Memcpy(Packet + 2, &GuildId, 4);
	FMemory::Memcpy(Packet + 6, &AccountId, 4);
	FMemory::Memcpy(Packet + 10, &CharId, 4);
	GuildWriteText(Packet + 14, Reason, 40);
	SendData(Packet, sizeof(Packet));
	return true;
}

bool URagnarokNetworkSubsystem::SendGuildNotice(const FString& Subject, const FString& Notice)
{
	if (!CurrentSocket || CurrentState != ERagnarokNetworkState::ConnectedMap || GuildId == 0)
	{
		return false;
	}
	uint8 Packet[186] = { 0x6e, 0x01 }; // CZ_GUILD_NOTICE <cla>.L <assunto>.60B <aviso>.120B
	FMemory::Memcpy(Packet + 2, &GuildId, 4);
	GuildWriteText(Packet + 6, Subject, 60);
	GuildWriteText(Packet + 66, Notice, 120);
	SendData(Packet, sizeof(Packet));
	return true;
}

bool URagnarokNetworkSubsystem::SendBreakGuild(const FString& Name)
{
	if (!CurrentSocket || CurrentState != ERagnarokNetworkState::ConnectedMap || GuildId == 0)
	{
		return false;
	}
	uint8 Packet[42] = { 0x5d, 0x01 }; // CZ_REQ_DISORGANIZE_GUILD <nome do cla>.40B
	GuildWriteText(Packet + 2, Name, 40);
	SendData(Packet, sizeof(Packet));
	return true;
}

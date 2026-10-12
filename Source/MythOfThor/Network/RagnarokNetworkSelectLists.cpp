// Listas de escolha que o servidor abre depois de uma skill ou de um item (rAthena clif.cpp): destinos do Portal, Lupa
// (identificar), carta (onde encaixar), Fabricar Flechas, Forjar/Farmacologia, Autoconjurar e Refinar arma (Mestre-Ferreiro).
// O cliente so mostra a lista (UMythRoHUDWidget, MythRoSelectList.cpp) e devolve a escolha; regras, custos e chances sao do servidor.
// Formatos conferidos no packets.hpp do rAthena para o PACKETVER 20250402 (IDs de item com 4 bytes).

#include "Network/RagnarokNetworkSubsystem.h"
#include "UI/MythRoClientTables.h"

namespace
{
	template<typename T> T ReadAt(const uint8* Data, int32 Offset) { T V; FMemory::Memcpy(&V, Data + Offset, sizeof(T)); return V; }
}

void URagnarokNetworkSubsystem::HandleSelectListPacket(const uint8* PacketData, int32 PacketSize, uint16 PacketID)
{
	TArray<int32> Values;
	TArray<FString> Names;
	auto Emit = [&](EMythSelectList Type)
	{
		if (Values.Num() > 0)
		{
			OnSelectList.Broadcast(static_cast<uint8>(Type), Values, Names);
		}
	};
	switch (PacketID)
	{
	case 0x0abe: // ZC_WARPLIST <len>.W <skill>.W { <mapa>.16B }*
	{
		if (PacketSize < 6)
		{
			break;
		}
		SelectSkillId = ReadAt<uint16>(PacketData, 4);
		for (int32 Off = 6; Off + 16 <= PacketSize; Off += 16)
		{
			char Map[17] = {};
			FMemory::Memcpy(Map, PacketData + Off, 16);
			FString Name = UTF8_TO_TCHAR(Map);
			Name.RemoveFromEnd(TEXT(".gat"));
			if (!Name.IsEmpty())
			{
				Values.Add(SelectSkillId);
				Names.Add(Name);
			}
		}
		Emit(EMythSelectList::Warp);
		break;
	}
	case 0x0177: // ZC_ITEMIDENTIFY_LIST <len>.W { <indice>.W }*
	case 0x017b: // ZC_ITEMCOMPOSITION_LIST <len>.W { <indice>.W }*
		for (int32 Off = 4; Off + 2 <= PacketSize; Off += 2)
		{
			Values.Add(ReadAt<uint16>(PacketData, Off));
		}
		Emit(PacketID == 0x0177 ? EMythSelectList::Identify : EMythSelectList::Card);
		break;
	case 0x0b65: // ZC_REPAIRITEMLIST <len>.W { <indice>.W <item>.L <cartas>.16B <refino>.B <grau>.B }* (24 bytes)
		RepairEntries.Reset();
		for (int32 Off = 4; Off + 24 <= PacketSize; Off += 24)
		{
			RepairEntries.Emplace(PacketData + Off, 24);
			Values.Add(ReadAt<int32>(PacketData, Off + 2));
		}
		Emit(EMythSelectList::Repair);
		break;
	case 0x01a6: // ZC_PETEGG_LIST <len>.W { <indice>.W }* (ovos para chocar)
		for (int32 Off = 4; Off + 2 <= PacketSize; Off += 2)
		{
			Values.Add(ReadAt<uint16>(PacketData, Off));
		}
		Emit(EMythSelectList::PetEgg);
		break;
	case 0x01ad: // ZC_MAKINGARROW_LIST <len>.W { <item>.L }*
		for (int32 Off = 4; Off + 4 <= PacketSize; Off += 4)
		{
			Values.Add(ReadAt<int32>(PacketData, Off));
		}
		Emit(EMythSelectList::Arrow);
		break;
	case 0x018d: // ZC_MAKABLEITEMLIST <len>.W { <item>.L <material>.3L }*
		for (int32 Off = 4; Off + 16 <= PacketSize; Off += 16)
		{
			Values.Add(ReadAt<int32>(PacketData, Off));
		}
		Emit(EMythSelectList::Produce);
		break;
	case 0x0afb: // ZC_AUTOSPELLLIST <len>.W { <skill>.L }*
		for (int32 Off = 4; Off + 4 <= PacketSize; Off += 4)
		{
			if (const int32 Skill = ReadAt<int32>(PacketData, Off); Skill > 0)
			{
				Values.Add(Skill);
			}
		}
		Emit(EMythSelectList::Autospell);
		break;
	case 0x0221: // ZC_NOTIFY_WEAPONITEMLIST <len>.W { <indice>.W <item>.L <refino>.B <cartas>.16B }*
		for (int32 Off = 4; Off + 23 <= PacketSize; Off += 23)
		{
			Values.Add(ReadAt<int16>(PacketData, Off));
		}
		Emit(EMythSelectList::Refine);
		break;

	// Respostas: so a mensagem do cliente e o que muda na mochila (o resto vem nos pacotes de item do servidor)
	case 0x0179: // ZC_ACK_ITEMIDENTIFY <indice>.W <resultado>.B (0 ok: msg 491)
		if (PacketSize >= 5)
		{
			const int32 Index = ReadAt<uint16>(PacketData, 2);
			if (PacketData[4] == 0)
			{
				if (FRagnarokInventoryItem* Item = InventoryItems.FindByPredicate([Index](const FRagnarokInventoryItem& I) { return I.Index == Index; }))
				{
					Item->bIdentified = true;
				}
				OnChatMessage.Broadcast(TEXT(""), FMythRoClientTables::Message(491), TEXT("#00FFFF"));
				OnInventory.Broadcast(InventoryItems);
			}
		}
		break;
	case 0x017d: // ZC_ACK_ITEMCOMPOSITION <equip>.W <carta>.W <resultado>.B: a carta entra no primeiro encaixe livre
		if (PacketSize >= 7 && PacketData[6] == 0)
		{
			const int32 EquipIndex = ReadAt<uint16>(PacketData, 2);
			const int32 CardIndex = ReadAt<uint16>(PacketData, 4);
			const FRagnarokInventoryItem* Card = InventoryItems.FindByPredicate([CardIndex](const FRagnarokInventoryItem& I) { return I.Index == CardIndex; });
			FRagnarokInventoryItem* Equip = InventoryItems.FindByPredicate([EquipIndex](const FRagnarokInventoryItem& I) { return I.Index == EquipIndex; });
			if (Card && Equip)
			{
				int32* Slots[4] = { &Equip->Slot1, &Equip->Slot2, &Equip->Slot3, &Equip->Slot4 };
				for (int32* S : Slots)
				{
					if (*S == 0)
					{
						*S = Card->ItemID;
						break;
					}
				}
			}
			OnInventory.Broadcast(InventoryItems);
		}
		break;
	case 0x0223: // ZC_ACK_WEAPONREFINE <resultado>.L <item>.L: 0 ok (498), 1 falhou (499), 2 perdeu nivel (1537)
		if (PacketSize >= 6)
		{
			const int32 Result = ReadAt<int32>(PacketData, 2);
			OnChatMessage.Broadcast(TEXT(""), FMythRoClientTables::Message(Result == 0 ? 498 : (Result == 2 ? 1537 : 499)), TEXT("#00FFFF"));
		}
		break;
	default:
		break;
	}
}

bool URagnarokNetworkSubsystem::SendSelectListChoice(EMythSelectList Type, int32 Value, const FString& Name, bool bCancel)
{
	if (!CurrentSocket || CurrentState != ERagnarokNetworkState::ConnectedMap)
	{
		return false;
	}
	TArray<uint8> Packet;
	auto Put16 = [&Packet](uint16 V) { Packet.Append(reinterpret_cast<const uint8*>(&V), 2); };
	auto Put32 = [&Packet](int32 V) { Packet.Append(reinterpret_cast<const uint8*>(&V), 4); };
	switch (Type)
	{
	case EMythSelectList::Warp: // CZ_SELECT_WARPPOINT <skill>.W <mapa>.16B ("cancel" fecha)
	{
		Put16(0x011b);
		Put16(static_cast<uint16>(SelectSkillId));
		char Map[16] = {};
		const FTCHARToUTF8 Utf8(bCancel ? TEXT("cancel") : *(Name + TEXT(".gat")));
		FMemory::Memcpy(Map, Utf8.Get(), FMath::Min(Utf8.Length(), 15));
		Packet.Append(reinterpret_cast<const uint8*>(Map), 16);
		break;
	}
	case EMythSelectList::Identify: // CZ_REQ_ITEMIDENTIFY <indice>.W (-1 fecha)
		Put16(0x0178);
		Put16(bCancel ? 0xFFFF : static_cast<uint16>(Value));
		break;
	case EMythSelectList::Card: // CZ_REQ_ITEMCOMPOSITION <carta>.W <equipamento>.W
		if (bCancel || PendingCardIndex < 0)
		{
			PendingCardIndex = -1;
			return true;
		}
		Put16(0x017c);
		Put16(static_cast<uint16>(PendingCardIndex));
		Put16(static_cast<uint16>(Value));
		PendingCardIndex = -1;
		break;
	case EMythSelectList::Arrow: // CZ_REQ_MAKINGARROW <item>.L
		if (bCancel) { return true; }
		Put16(0x01ae);
		Put32(Value);
		break;
	case EMythSelectList::Produce: // CZ_REQMAKINGITEM <item>.L <material>.3L (sem pedras/estrelas por enquanto)
		if (bCancel) { return true; }
		Put16(0x018e);
		Put32(Value);
		Put32(0);
		Put32(0);
		Put32(0);
		break;
	case EMythSelectList::Autospell: // CZ_SELECTAUTOSPELL <skill>.L
		if (bCancel) { return true; }
		Put16(0x01ce);
		Put32(Value);
		break;
	case EMythSelectList::Refine: // CZ_REQ_WEAPONREFINE <indice>.L
		if (bCancel) { return true; }
		Put16(0x0222);
		Put32(Value);
		break;
	case EMythSelectList::Repair: // CZ_REQ_ITEMREPAIR2 <a entrada da lista>.24B (o indice e do inventario de quem sera consertado)
	{
		if (bCancel) { RepairEntries.Reset(); return true; }
		const int32 Found = RepairEntries.IndexOfByPredicate([Value](const TArray<uint8>& E) { return ReadAt<int32>(E.GetData(), 2) == Value; });
		if (Found == INDEX_NONE) { return true; }
		Put16(0x0b66);
		Packet.Append(RepairEntries[Found]);
		RepairEntries.Reset();
		break;
	}
	case EMythSelectList::PetEgg: // CZ_SELECT_PETEGG <indice>.W
		if (bCancel) { return true; }
		Put16(0x01a7);
		Put16(static_cast<uint16>(Value));
		break;
	}
	SendData(Packet.GetData(), Packet.Num());
	return true;
}

bool URagnarokNetworkSubsystem::SendCardListRequest(int32 CardIndex)
{
	if (!CurrentSocket || CurrentState != ERagnarokNetworkState::ConnectedMap)
	{
		return false;
	}
	// CZ_REQ_ITEMCOMPOSITION_LIST 0x017a <carta>.W: o servidor responde com ZC_ITEMCOMPOSITION_LIST (0x017b)
	PendingCardIndex = CardIndex;
	const uint16 Index = static_cast<uint16>(CardIndex);
	uint8 Packet[4] = { 0x7a, 0x01 };
	FMemory::Memcpy(Packet + 2, &Index, 2);
	SendData(Packet, sizeof(Packet));
	return true;
}

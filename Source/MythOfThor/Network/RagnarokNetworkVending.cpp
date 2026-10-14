// Loja de jogador (Vending, skill Venda do Mercador) - rAthena vending.cpp / clif.cpp, PACKETVER 20250402:
//   ZC_OPENSTORE 0x012d (pode abrir, com o maximo de itens) -> CZ_REQ_OPENSTORE2 0x01b2 (titulo + itens do carrinho)
//   ZC_ACK_OPENSTORE2 0x0a28, ZC_PC_PURCHASE_MYITEMLIST 0x0b40 (a minha loja), ZC_DELETEITEM_FROM_MCSTORE 0x09e5 (vendi)
//   ZC_STORE_ENTRY 0x0131 / ZC_DISAPPEAR_ENTRY 0x0132 (placa sobre a cabeca), CZ_REQ_BUY_FROMMC 0x0130
//   ZC_PC_PURCHASE_ITEMLIST_FROMMC 0x0b3d (lista de outra loja) -> CZ_PC_PURCHASE_ITEMLIST_FROMMC2 0x0801, ZC_PC_PURCHASE_RESULT_FROMMC 0x0135
//   CZ_REQ_CLOSESTORE 0x012e
// Precos, zeny, peso e estoque sao do servidor; o cliente so mostra e pede.

#include "Network/RagnarokNetworkSubsystem.h"
#include "Core/MythRoClientTables.h"

namespace
{
	template<typename T> T VendRead(const uint8* Data, int32 Offset) { T V; FMemory::Memcpy(&V, Data + Offset, sizeof(T)); return V; }

	FString VendText(const uint8* Data, int32 Size)
	{
		TArray<uint8> Buf;
		Buf.Append(Data, Size);
		Buf.Add(0);
		return FString(UTF8_TO_TCHAR(reinterpret_cast<const char*>(Buf.GetData())));
	}
}

void URagnarokNetworkSubsystem::HandleVendingPacket(const uint8* PacketData, int32 PacketSize, uint16 PacketID)
{
	switch (PacketID)
	{
	case 0x012d: // ZC_OPENSTORE <max itens>.W: a skill Venda deixou abrir a loja
		if (PacketSize >= 4)
		{
			VendMaxItems = VendRead<uint16>(PacketData, 2);
			OnVending.Broadcast(0);
		}
		break;
	case 0x0a28: // ZC_ACK_OPENSTORE2 <resultado>.B (0 abriu)
		if (PacketSize >= 3 && PacketData[2] != 0)
		{
			OnChatMessage.Broadcast(TEXT(""), TEXT("Could not open the shop."), TEXT("#FF6060"));
		}
		break;
	case 0x0131: // ZC_STORE_ENTRY <dono>.L <titulo>.80B
		if (PacketSize >= 86)
		{
			VendingBoards.Add(VendRead<int32>(PacketData, 2), VendText(PacketData + 6, 80));
		}
		break;
	case 0x0132: // ZC_DISAPPEAR_ENTRY <dono>.L
		if (PacketSize >= 6)
		{
			const int32 Owner = VendRead<int32>(PacketData, 2);
			VendingBoards.Remove(Owner);
			if (Owner == static_cast<int32>(GetSelfID()) && bVending)
			{
				bVending = false;
				MyVendItems.Reset();
				OnVending.Broadcast(3);
			}
		}
		break;
	case 0x0b3d: // ZC_PC_PURCHASE_ITEMLIST_FROMMC <len>.W <dono>.L <loja>.L { 64 bytes }*
	{
		if (PacketSize < 12)
		{
			break;
		}
		VendBuyAID = VendRead<int32>(PacketData, 4);
		VendBuyId = VendRead<int32>(PacketData, 8);
		VendBuyItems.Reset();
		for (int32 Off = 12; Off + 64 <= PacketSize; Off += 64)
		{
			// preco.L quantidade.W indice.W tipo.B item.L identificado.B danificado.B cartas.16B opcoes.25B local.L visual.W refino.B grau.B
			FRoVendItem I;
			I.Price = VendRead<int32>(PacketData, Off);
			I.Amount = VendRead<uint16>(PacketData, Off + 4);
			I.Index = VendRead<int16>(PacketData, Off + 6);
			I.Type = PacketData[Off + 8];
			I.ItemID = VendRead<int32>(PacketData, Off + 9);
			I.bIdentified = PacketData[Off + 13] != 0;
			for (int32 s = 0; s < 4; ++s) { I.Slots[s] = VendRead<int32>(PacketData, Off + 15 + s * 4); }
			I.Refine = PacketData[Off + 62];
			VendBuyItems.Add(I);
		}
		OnVending.Broadcast(1);
		break;
	}
	case 0x0b40: // ZC_PC_PURCHASE_MYITEMLIST <len>.W <eu>.L { 58 bytes }*: a minha loja abriu
	{
		if (PacketSize < 8)
		{
			break;
		}
		MyVendItems.Reset();
		for (int32 Off = 8; Off + 58 <= PacketSize; Off += 58)
		{
			// preco.L indice.W quantidade.W tipo.B item.L identificado.B danificado.B cartas.16B opcoes.25B refino.B grau.B
			FRoVendItem I;
			I.Price = VendRead<int32>(PacketData, Off);
			I.Index = VendRead<int16>(PacketData, Off + 4);
			I.Amount = VendRead<int16>(PacketData, Off + 6);
			I.Type = PacketData[Off + 8];
			I.ItemID = VendRead<int32>(PacketData, Off + 9);
			I.bIdentified = PacketData[Off + 13] != 0;
			for (int32 s = 0; s < 4; ++s) { I.Slots[s] = VendRead<int32>(PacketData, Off + 15 + s * 4); }
			I.Refine = PacketData[Off + 56];
			MyVendItems.Add(I);
		}
		bVending = true;
		OnVending.Broadcast(2);
		break;
	}
	case 0x09e5: // ZC_DELETEITEM_FROM_MCSTORE <indice>.W <quantidade>.W <comprador>.L <data>.L <zeny>.L: vendi
		if (PacketSize >= 18)
		{
			const int32 Index = VendRead<uint16>(PacketData, 2);
			const int32 Amount = VendRead<uint16>(PacketData, 4);
			for (int32 i = 0; i < MyVendItems.Num(); ++i)
			{
				if (MyVendItems[i].Index == Index)
				{
					OnChatMessage.Broadcast(TEXT(""), FString::Printf(TEXT("Sold: %s x %d."), *FMythRoClientTables::ItemName(MyVendItems[i].ItemID), Amount), TEXT("#00FFFF"));
					MyVendItems[i].Amount -= Amount;
					if (MyVendItems[i].Amount <= 0)
					{
						MyVendItems.RemoveAt(i);
					}
					break;
				}
			}
			OnVending.Broadcast(2);
		}
		break;
	case 0x0135: // ZC_PC_PURCHASE_RESULT_FROMMC <indice>.W <quantidade>.W <resultado>.B
		if (PacketSize >= 7 && PacketData[6] != 0)
		{
			const TCHAR* Why = TEXT("The purchase was not made.");
			switch (PacketData[6])
			{
			case 1: Why = TEXT("Not enough zeny."); break;
			case 2: Why = TEXT("Weight above the limit."); break;
			case 4: Why = TEXT("Item sold out in the shop."); break;
			case 5: Why = TEXT("Cannot buy during a trade."); break;
			case 6: Why = TEXT("The shop changed; the purchase was not made."); break;
			case 7: Why = TEXT("The shop is no longer selling."); break;
			default: break;
			}
			OnChatMessage.Broadcast(TEXT(""), Why, TEXT("#FF6060"));
		}
		break;
	default:
		break;
	}
}

bool URagnarokNetworkSubsystem::SendOpenVending(const FString& Title, const TArray<FIntVector>& Items, bool bOpen)
{
	if (!CurrentSocket || CurrentState != ERagnarokNetworkState::ConnectedMap)
	{
		return false;
	}
	// CZ_REQ_OPENSTORE2 0x01b2 <len>.W <titulo>.80B <abrir>.B { <indice no carrinho>.W <quantidade>.W <preco>.L }*
	TArray<uint8> Packet;
	Packet.SetNumZeroed(85 + Items.Num() * 8);
	const uint16 Id = 0x01b2, Len = static_cast<uint16>(Packet.Num());
	FMemory::Memcpy(Packet.GetData(), &Id, 2);
	FMemory::Memcpy(Packet.GetData() + 2, &Len, 2);
	const FTCHARToUTF8 Utf8(*Title.Left(36));
	FMemory::Memcpy(Packet.GetData() + 4, Utf8.Get(), FMath::Min(Utf8.Length(), 79));
	Packet[84] = bOpen ? 1 : 0;
	for (int32 i = 0; i < Items.Num(); ++i)
	{
		const uint16 Index = static_cast<uint16>(Items[i].X), Amount = static_cast<uint16>(Items[i].Y);
		const int32 Price = Items[i].Z;
		uint8* P = Packet.GetData() + 85 + i * 8;
		FMemory::Memcpy(P, &Index, 2);
		FMemory::Memcpy(P + 2, &Amount, 2);
		FMemory::Memcpy(P + 4, &Price, 4);
	}
	SendData(Packet.GetData(), Packet.Num());
	return true;
}

bool URagnarokNetworkSubsystem::SendVendingListReq(int32 OwnerAID)
{
	if (!CurrentSocket || CurrentState != ERagnarokNetworkState::ConnectedMap)
	{
		return false;
	}
	uint8 Packet[6] = { 0x30, 0x01 }; // CZ_REQ_BUY_FROMMC <dono>.L
	FMemory::Memcpy(Packet + 2, &OwnerAID, 4);
	SendData(Packet, sizeof(Packet));
	return true;
}

bool URagnarokNetworkSubsystem::SendVendingBuy(const TArray<FIntPoint>& Items)
{
	if (!CurrentSocket || CurrentState != ERagnarokNetworkState::ConnectedMap || Items.Num() == 0)
	{
		return false;
	}
	// CZ_PC_PURCHASE_ITEMLIST_FROMMC2 0x0801 <len>.W <dono>.L <loja>.L { <quantidade>.W <indice>.W }*
	TArray<uint8> Packet;
	Packet.SetNumZeroed(12 + Items.Num() * 4);
	const uint16 Id = 0x0801, Len = static_cast<uint16>(Packet.Num());
	FMemory::Memcpy(Packet.GetData(), &Id, 2);
	FMemory::Memcpy(Packet.GetData() + 2, &Len, 2);
	FMemory::Memcpy(Packet.GetData() + 4, &VendBuyAID, 4);
	FMemory::Memcpy(Packet.GetData() + 8, &VendBuyId, 4);
	for (int32 i = 0; i < Items.Num(); ++i)
	{
		const uint16 Count = static_cast<uint16>(Items[i].Y), Index = static_cast<uint16>(Items[i].X);
		FMemory::Memcpy(Packet.GetData() + 12 + i * 4, &Count, 2);
		FMemory::Memcpy(Packet.GetData() + 14 + i * 4, &Index, 2);
	}
	SendData(Packet.GetData(), Packet.Num());
	return true;
}

bool URagnarokNetworkSubsystem::SendCloseVending()
{
	if (!CurrentSocket || CurrentState != ERagnarokNetworkState::ConnectedMap)
	{
		return false;
	}
	const uint8 Packet[2] = { 0x2e, 0x01 }; // CZ_REQ_CLOSESTORE
	SendData(Packet, sizeof(Packet));
	return true;
}

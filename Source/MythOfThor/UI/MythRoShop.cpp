// Loja de NPC (docs/ref/14): o servidor manda a escolha comprar/vender (0x00c4), a vitrine (0x0b77) ou os itens
// vendaveis do inventario (0x00c7); o jogador monta a cesta e confirma (0x00c8 / 0x00c9). Desconto, overcharge, peso e
// zeny sao decididos pelo servidor (pc_modifybuyvalue / npc_buylist): o cliente so mostra e repassa o que o servidor aceita.
// Layout classico de duas colunas: vitrine a esquerda, cesta a direita, total e botoes embaixo.

#include "UI/MythRoHUDWidget.h"

#include "Network/RagnarokNetworkSubsystem.h"
#include "Sprites/MythRoSound.h"
#include "UI/MythRoClientTables.h"

#include "Components/CanvasPanel.h"
#include "Components/Image.h"
#include "Components/TextBlock.h"
#include "GameFramework/PlayerController.h"

namespace
{
	const FLinearColor ShopText(0.12f, 0.12f, 0.12f);
	const FLinearColor ShopLink(0.12f, 0.3f, 0.65f);
	const FLinearColor ShopGray(0.45f, 0.45f, 0.45f);
	const FLinearColor ShopGreen(0.05f, 0.5f, 0.15f);
	constexpr float ShopRowH = 24.f;
	constexpr float ShopW = 560.f;
	constexpr float ShopListX = 8.f;
	constexpr float ShopListY = 42.f;
	constexpr float ShopListW = 266.f;
	constexpr float ShopBasketX = 286.f;
	constexpr int32 ShopMaxStack = 30000; // limite por transacao (docs/ref/14 2.2)

	FString Zeny(int64 Value)
	{
		FString S = FString::Printf(TEXT("%lld"), Value);
		for (int32 i = S.Len() - 3; i > 0; i -= 3)
		{
			S.InsertAt(i, TEXT("."));
		}
		return S + TEXT(" z");
	}
}

void UMythRoHUDWidget::BuildShopWindows()
{
	// Escolha comprar / vender / cancelar (UIChooseSellBuyWnd, 180 x 110)
	const FVector2D DealSize(180.f, 17.f + 3 * 24.f + 14.f);
	DealWindow = MakeRoPanel(TEXT("Loja"), DealSize, FVector2D(0.f, -60.f));
	DealBuyLink = AddButtonHit(DealWindow, TEXT("buy"), 68.f, 26.f);
	DealSellLink = AddButtonHit(DealWindow, TEXT("sell"), 68.f, 50.f);
	DealCancelLink = AddButtonHit(DealWindow, TEXT("cancel"), 68.f, 74.f);
	DealWindow->SetVisibility(ESlateVisibility::Collapsed);

	// Vitrine (esquerda) e cesta (direita)
	const float H = ShopListY + ShopRowCount * ShopRowH + 58.f;
	ShopWindow = MakeRoPanel(TEXT("Loja"), FVector2D(ShopW, H), FVector2D(0.f, -20.f));
	ShopHeaderText = AddText(ShopWindow, 10.f, 22.f, 8, ShopGray);
	AddText(ShopWindow, ShopBasketX, 22.f, 8, ShopGray)->SetText(FText::FromString(TEXT("Cesta (clique para tirar)")));
	for (int32 i = 0; i < ShopRowCount; ++i)
	{
		const float Y = ShopListY + i * ShopRowH;
		UImage* Hit = AddColor(ShopWindow, FLinearColor(0.f, 0.f, 0.f, 0.03f), ShopListX, Y, ShopListW, ShopRowH - 1.f);
		Hit->SetVisibility(ESlateVisibility::Visible);
		ShopRowHit.Add(Hit);
		UImage* Icon = AddImage(ShopWindow, nullptr, ShopListX + 2.f, Y, 24.f, 24.f);
		Icon->SetVisibility(ESlateVisibility::Collapsed);
		ShopRowIcons.Add(Icon);
		ShopRowNames.Add(AddText(ShopWindow, ShopListX + 32.f, Y + 5.f, 8, ShopText));
		ShopRowPrices.Add(AddText(ShopWindow, ShopListX + 196.f, Y + 5.f, 8, ShopGreen));
		BasketRowTexts.Add(AddText(ShopWindow, ShopBasketX + 4.f, Y + 5.f, 8, ShopText));
	}
	ShopTotalText = AddText(ShopWindow, ShopBasketX, H - 50.f, 9, ShopText);
	ShopHintText = AddText(ShopWindow, 10.f, H - 50.f, 7, ShopGray);
	ShopConfirmLink = AddTextButton(ShopWindow, TEXT("Confirmar"), ShopBasketX, H - 32.f, 84.f);
	ShopCancelLink = AddButtonHit(ShopWindow, TEXT("cancel"), ShopBasketX + 100.f, H - 32.f);
	ShopWindow->SetVisibility(ESlateVisibility::Collapsed);
}

bool UMythRoHUDWidget::IsShopOpen() const
{
	return (DealWindow && DealWindow->IsVisible()) || (ShopWindow && ShopWindow->IsVisible());
}

void UMythRoHUDWidget::ShowShopDealType(int64 NpcID)
{
	if (!DealWindow)
	{
		return;
	}
	ShopNpcID = NpcID;
	ShopRows.Reset();
	ShopBasket.Reset();
	ShopWindow->SetVisibility(ESlateVisibility::Collapsed);
	DealWindow->SetVisibility(ESlateVisibility::SelfHitTestInvisible);
	OpenOrder.Remove(DealWindow);
	OpenOrder.Add(DealWindow);
}

void UMythRoHUDWidget::ShowShopItems(bool bSell, const TArray<FRagnarokShopItem>& Items)
{
	if (!ShopWindow)
	{
		return;
	}
	bShopSell = bSell;
	ShopRows.Reset();
	ShopBasket.Reset();
	ShopScroll = 0;
	const URagnarokNetworkSubsystem* Net = GetNet();
	for (const FRagnarokShopItem& I : Items)
	{
		FShopRow R;
		R.ItemID = I.ItemID;
		R.Index = I.Index;
		R.Price = I.Price;
		R.Discount = I.DiscountPrice;
		if (bSell && Net)
		{
			for (const FRagnarokInventoryItem& Inv : Net->GetInventoryItems())
			{
				if (Inv.Index == I.Index)
				{
					R.MaxAmount = FMath::Max(Inv.Amount, 1);
					break;
				}
			}
		}
		ShopRows.Add(R);
	}
	DealWindow->SetVisibility(ESlateVisibility::Collapsed);
	ShopWindow->SetVisibility(ESlateVisibility::SelfHitTestInvisible);
	OpenOrder.Remove(ShopWindow);
	OpenOrder.Add(ShopWindow);
	RefreshShop();
}

void UMythRoHUDWidget::RefreshShop()
{
	if (!ShopWindow)
	{
		return;
	}
	ShopHeaderText->SetText(FText::FromString(bShopSell ? TEXT("Seus itens (clique para vender 1; Shift: 10)") : TEXT("À venda (clique para comprar 1; Shift: 10)")));
	ShopHintText->SetText(FText::FromString(ShopRows.Num() > ShopRowCount ? TEXT("Roda do mouse: rolar a lista") : TEXT("")));
	int64 Total = 0;
	TArray<int32> BasketOrder;
	ShopBasket.GetKeys(BasketOrder);
	BasketOrder.Sort();
	for (int32 i = 0; i < ShopRowCount; ++i)
	{
		const int32 RowIndex = ShopScroll + i;
		const bool bHas = ShopRows.IsValidIndex(RowIndex);
		ShopRowHit[i]->SetVisibility(bHas ? ESlateVisibility::Visible : ESlateVisibility::Collapsed);
		ShopRowIcons[i]->SetVisibility(ESlateVisibility::Collapsed);
		ShopRowNames[i]->SetText(FText::GetEmpty());
		ShopRowPrices[i]->SetText(FText::GetEmpty());
		if (bHas)
		{
			const FShopRow& R = ShopRows[RowIndex];
			if (UTexture2D* Icon = FMythRoClientTables::ItemIcon(R.ItemID))
			{
				ShopRowIcons[i]->SetBrushFromTexture(Icon);
				ShopRowIcons[i]->SetVisibility(ESlateVisibility::HitTestInvisible);
			}
			FString Name = FMythRoClientTables::ItemName(R.ItemID);
			ShopRowNames[i]->SetText(FText::FromString(Name.IsEmpty() ? FString::Printf(TEXT("Item %d"), R.ItemID) : Name));
			ShopRowPrices[i]->SetText(FText::FromString(Zeny(R.Discount)));
		}
		// Cesta: uma linha por item escolhido
		BasketRowTexts[i]->SetText(FText::GetEmpty());
		if (BasketOrder.IsValidIndex(i) && ShopRows.IsValidIndex(BasketOrder[i]))
		{
			const FShopRow& R = ShopRows[BasketOrder[i]];
			const int32 Count = ShopBasket[BasketOrder[i]];
			FString Name = FMythRoClientTables::ItemName(R.ItemID);
			BasketRowTexts[i]->SetText(FText::FromString(FString::Printf(TEXT("%s x %d   %s"), *Name, Count, *Zeny(static_cast<int64>(R.Discount) * Count))));
		}
	}
	for (const TPair<int32, int32>& Pair : ShopBasket)
	{
		if (ShopRows.IsValidIndex(Pair.Key))
		{
			Total += static_cast<int64>(ShopRows[Pair.Key].Discount) * Pair.Value;
		}
	}
	const URagnarokNetworkSubsystem* Net = GetNet();
	const int64 Have = Net ? Net->GetPlayerStats().Zeny : 0;
	ShopTotalText->SetText(FText::FromString(bShopSell
		? FString::Printf(TEXT("Você recebe: %s"), *Zeny(Total))
		: FString::Printf(TEXT("Total: %s   (você tem %s)"), *Zeny(Total), *Zeny(Have))));
	ShopConfirmLink->SetText(FText::FromString(bShopSell ? TEXT("Vender") : TEXT("Comprar")));
}

bool UMythRoHUDWidget::HandleShopClick(const FVector2D& ScreenPos)
{
	URagnarokNetworkSubsystem* Net = GetNet();
	if (DealWindow && DealWindow->IsVisible())
	{
		if (IsUnder(DealBuyLink, ScreenPos) || IsUnder(DealSellLink, ScreenPos))
		{
			ShopChooseDeal(IsUnder(DealSellLink, ScreenPos) ? 1 : 0); // a vitrine chega logo depois
			return true;
		}
		if (IsUnder(DealCancelLink, ScreenPos))
		{
			CloseShop(true);
			return true;
		}
	}
	if (!ShopWindow || !ShopWindow->IsVisible())
	{
		return false;
	}
	const bool bShift = GetOwningPlayer() && (GetOwningPlayer()->IsInputKeyDown(EKeys::LeftShift) || GetOwningPlayer()->IsInputKeyDown(EKeys::RightShift));
	TArray<int32> BasketOrder;
	ShopBasket.GetKeys(BasketOrder);
	BasketOrder.Sort();
	for (int32 i = 0; i < ShopRowCount; ++i)
	{
		const int32 RowIndex = ShopScroll + i;
		if (ShopRows.IsValidIndex(RowIndex) && IsUnder(ShopRowHit[i], ScreenPos))
		{
			int32& Count = ShopBasket.FindOrAdd(RowIndex);
			Count = FMath::Min(Count + (bShift ? 10 : 1), FMath::Min(ShopRows[RowIndex].MaxAmount, ShopMaxStack));
			RefreshShop();
			return true;
		}
		if (BasketOrder.IsValidIndex(i) && IsUnder(BasketRowTexts[i], ScreenPos))
		{
			int32& Count = ShopBasket[BasketOrder[i]];
			Count -= bShift ? 10 : 1;
			if (Count <= 0)
			{
				ShopBasket.Remove(BasketOrder[i]);
			}
			RefreshShop();
			return true;
		}
	}
	if (IsUnder(ShopConfirmLink, ScreenPos))
	{
		ShopConfirm();
		return true;
	}
	if (IsUnder(ShopCancelLink, ScreenPos))
	{
		CloseShop(true);
		return true;
	}
	return false;
}

bool UMythRoHUDWidget::HandleShopWheel(const FVector2D& ScreenPos, float Delta)
{
	if (!ShopWindow || !ShopWindow->IsVisible() || !IsUnder(ShopWindow, ScreenPos))
	{
		return false;
	}
	ShopScroll = FMath::Clamp(ShopScroll + (Delta > 0.f ? -1 : 1), 0, FMath::Max(0, ShopRows.Num() - ShopRowCount));
	RefreshShop();
	return true;
}

void UMythRoHUDWidget::ShopResult(bool bSell, int32 Result)
{
	// Textos do rAthena (e_purchase_result do clif.cpp) para a compra; a venda so tem 0 e 1
	FString Msg;
	bool bOk = false;
	if (bSell)
	{
		bOk = Result == 0;
		Msg = bOk ? TEXT("Venda concluída.") : TEXT("A venda falhou.");
	}
	else
	{
		bOk = Result == 0 || Result == 12;
		switch (Result)
		{
		case 0: Msg = TEXT("Compra concluída."); break;
		case 1: Msg = TEXT("Você não tem zeny suficiente."); break;
		case 2: Msg = TEXT("Você está com peso demais."); break;
		case 3: Msg = TEXT("Você já carrega itens demais."); break;
		case 9: Msg = TEXT("Quantidade acima do que se pode comprar."); break;
		case 13: Msg = TEXT("O item está esgotado."); break;
		default: Msg = FString::Printf(TEXT("A compra falhou (%d)."), Result); break;
		}
	}
	AddChatLine(Msg, bOk ? FLinearColor(0.5f, 0.8f, 1.f) : FLinearColor(1.f, 0.4f, 0.4f));
	if (bOk)
	{
		UMythRoSound::PlayWavAtLocation(this, TEXT("effect/coin.wav"), FVector::ZeroVector);
	}
	// O servidor ja encerrou a sessao de loja (npc_shopid = 0), com sucesso ou nao
	CloseShop(false);
}

void UMythRoHUDWidget::CloseShop(bool bNotifyServer)
{
	const bool bWasOpen = IsShopOpen();
	if (DealWindow) { DealWindow->SetVisibility(ESlateVisibility::Collapsed); }
	if (ShopWindow) { ShopWindow->SetVisibility(ESlateVisibility::Collapsed); }
	ShopRows.Reset();
	ShopBasket.Reset();
	if (bNotifyServer && bWasOpen)
	{
		if (URagnarokNetworkSubsystem* Net = GetNet())
		{
			Net->SendShopClose(); // CZ_NPC_SHOP_CLOSED: libera o NPC
		}
	}
}

void UMythRoHUDWidget::ShopChooseDeal(int32 Type)
{
	if (URagnarokNetworkSubsystem* Net = GetNet())
	{
		Net->SendShopDealType(ShopNpcID, static_cast<uint8>(Type));
	}
}

void UMythRoHUDWidget::ShopAddToBasket(int32 Row, int32 Qty)
{
	if (ShopRows.IsValidIndex(Row) && Qty > 0)
	{
		ShopBasket.FindOrAdd(Row) = FMath::Min(Qty, FMath::Min(ShopRows[Row].MaxAmount, ShopMaxStack));
		RefreshShop();
	}
}

void UMythRoHUDWidget::ShopConfirm()
{
	TArray<TPair<int32, int32>> Lines;
	for (const TPair<int32, int32>& Pair : ShopBasket)
	{
		if (ShopRows.IsValidIndex(Pair.Key) && Pair.Value > 0)
		{
			Lines.Add(TPair<int32, int32>(bShopSell ? ShopRows[Pair.Key].Index : ShopRows[Pair.Key].ItemID, Pair.Value));
		}
	}
	URagnarokNetworkSubsystem* Net = GetNet();
	if (Net && Lines.Num() > 0)
	{
		// O servidor valida zeny, peso e estoque e responde (0x00ca / 0x00cb); ShopResult fecha as janelas
		if (bShopSell) { Net->SendShopSell(Lines); } else { Net->SendShopBuy(Lines); }
	}
}

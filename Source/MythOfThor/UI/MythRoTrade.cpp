// Troca entre jogadores (docs/ref/22 2, pacotes conferidos no clif.cpp) e o menu do botao direito sobre um jogador
// (Trocar, Convidar para o grupo, Cochichar). Quem pode trocar, peso, travas e a transferencia sao do servidor.

#include "UI/MythRoHUDWidget.h"

#include "Network/RagnarokNetworkSubsystem.h"
#include "UI/MythRoClientTables.h"

#include "Components/CanvasPanel.h"
#include "Components/CanvasPanelSlot.h"
#include "Components/EditableTextBox.h"
#include "Components/Image.h"
#include "Components/TextBlock.h"
#include "Blueprint/WidgetLayoutLibrary.h"
#include "Layout/Geometry.h"

namespace
{
	const FLinearColor TrText(0.12f, 0.12f, 0.12f);
	const FLinearColor TrLink(0.12f, 0.3f, 0.65f);
	const FLinearColor TrGray(0.55f, 0.55f, 0.55f);
	const FLinearColor TrGreen(0.1f, 0.5f, 0.2f);
	constexpr float TradeW = 430.f;
	constexpr float TradeRowH = 18.f;
	constexpr float TradeListY = 44.f;
	constexpr float TradeColW = 205.f;

	FString TradeItemLabel(const FRoTradeItem& I)
	{
		if (I.ItemId == 0)
		{
			return FString::Printf(TEXT("%d zeny"), I.Amount);
		}
		FString Name = FMythRoClientTables::ItemName(I.ItemId);
		if (Name.IsEmpty())
		{
			Name = FString::Printf(TEXT("Item %d"), I.ItemId);
		}
		return I.Amount > 1 ? FString::Printf(TEXT("%s x %d"), *Name, I.Amount) : Name;
	}
}

void UMythRoHUDWidget::BuildTradeWindows()
{
	const float H = TradeListY + TradeRowCount * TradeRowH + 62.f;
	TradeWindow = MakeRoPanel(TEXT("Troca"), FVector2D(TradeW, H), FVector2D(0.f, -40.f));
	TradeMineTitle = AddText(TradeWindow, 10.f, 22.f, 9, TrText);
	TradeTheirTitle = AddText(TradeWindow, 10.f + TradeColW + 10.f, 22.f, 9, TrText);
	AddColor(TradeWindow, FLinearColor(0.9f, 0.92f, 0.96f, 1.f), 8.f, TradeListY - 2.f, TradeColW, TradeRowCount * TradeRowH + 4.f);
	AddColor(TradeWindow, FLinearColor(0.9f, 0.92f, 0.96f, 1.f), 18.f + TradeColW, TradeListY - 2.f, TradeColW, TradeRowCount * TradeRowH + 4.f);
	for (int32 i = 0; i < TradeRowCount; ++i)
	{
		TradeMineRows.Add(AddText(TradeWindow, 12.f, TradeListY + i * TradeRowH, 8, TrText));
		TradeTheirRows.Add(AddText(TradeWindow, 22.f + TradeColW, TradeListY + i * TradeRowH, 8, TrText));
	}
	const float BY = H - 40.f;
	TradeHint = AddText(TradeWindow, 10.f, BY - 4.f, 7, TrGray);
	TradeLockLink = AddTextButton(TradeWindow, TEXT("Travar"), 40.f, BY + 10.f, 80.f);
	TradeCommitLink = AddButtonHit(TradeWindow, TEXT("exchange"), 170.f, BY + 10.f);
	TradeCancelLink = AddButtonHit(TradeWindow, TEXT("cancel"), 290.f, BY + 10.f);

	// Pedido recebido
	TradeAskWindow = MakeRoPanel(TEXT("Pedido de troca"), FVector2D(300.f, 92.f), FVector2D(0.f, -150.f));
	TradeAskText = AddText(TradeAskWindow, 10.f, 26.f, 9, TrText);
	TradeAcceptLink = AddButtonHit(TradeAskWindow, TEXT("agree"), 50.f, 58.f);
	TradeDeclineLink = AddButtonHit(TradeAskWindow, TEXT("disagree"), 170.f, 58.f);

	// Menu do botao direito sobre um jogador
	PlayerMenuWindow = MakeRoPanel(TEXT("Jogador"), FVector2D(170.f, 17.f + 6 * 20.f + 10.f), FVector2D::ZeroVector);
	const TCHAR* Labels[6] = { TEXT("Trocar"), TEXT("Convidar para o grupo"), TEXT("Cochichar"), TEXT("Adicionar amigo"), TEXT("Convidar para o clã"), TEXT("Cancelar") };
	for (int32 i = 0; i < 6; ++i)
	{
		UTextBlock* T = AddText(PlayerMenuWindow, 14.f, 24.f + i * 20.f, 9, TrLink);
		T->SetText(FText::FromString(Labels[i]));
		PlayerMenuItems.Add(T);
	}
}

void UMythRoHUDWidget::OnTradeRequestReceived(const FString& FromName)
{
	if (!TradeAskWindow)
	{
		return;
	}
	TradeAskText->SetText(FText::FromString(FString::Printf(TEXT("%s quer trocar com você."), *FromName)));
	TradeAskWindow->SetVisibility(ESlateVisibility::SelfHitTestInvisible);
	OpenOrder.Remove(TradeAskWindow);
	OpenOrder.Add(TradeAskWindow);
}

void UMythRoHUDWidget::RefreshTrade()
{
	URagnarokNetworkSubsystem* Net = GetNet();
	if (!TradeWindow || !Net)
	{
		return;
	}
	if (!Net->IsTradeOpen())
	{
		TradeWindow->SetVisibility(ESlateVisibility::Collapsed);
		OpenOrder.Remove(TradeWindow);
		return;
	}
	if (!TradeWindow->IsVisible())
	{
		TradeWindow->SetVisibility(ESlateVisibility::SelfHitTestInvisible);
		OpenOrder.Remove(TradeWindow);
		OpenOrder.Add(TradeWindow);
		if (TradeAskWindow) { TradeAskWindow->SetVisibility(ESlateVisibility::Collapsed); }
	}
	TradeMineTitle->SetText(FText::FromString(Net->IsTradeLockedMine() ? TEXT("Você (travado)") : TEXT("Você")));
	TradeTheirTitle->SetText(FText::FromString(FString::Printf(TEXT("%s%s"), Net->GetTradePartner().IsEmpty() ? TEXT("Jogador") : *Net->GetTradePartner(), Net->IsTradeLockedTheirs() ? TEXT(" (travado)") : TEXT(""))));
	TradeMineTitle->SetColorAndOpacity(FSlateColor(Net->IsTradeLockedMine() ? TrGreen : TrText));
	TradeTheirTitle->SetColorAndOpacity(FSlateColor(Net->IsTradeLockedTheirs() ? TrGreen : TrText));
	for (int32 i = 0; i < TradeRowCount; ++i)
	{
		TradeMineRows[i]->SetText(Net->GetTradeMine().IsValidIndex(i) ? FText::FromString(TradeItemLabel(Net->GetTradeMine()[i])) : FText::GetEmpty());
		TradeTheirRows[i]->SetText(Net->GetTradeTheirs().IsValidIndex(i) ? FText::FromString(TradeItemLabel(Net->GetTradeTheirs()[i])) : FText::GetEmpty());
	}
	const bool bBoth = Net->IsTradeLockedMine() && Net->IsTradeLockedTheirs();
	TradeLockLink->SetColorAndOpacity(FSlateColor(Net->IsTradeLockedMine() ? TrGray : TrLink));
	TradeCommitLink->SetColorAndOpacity(FSlateColor(bBoth ? TrLink : TrGray));
	TradeHint->SetText(FText::FromString(TEXT("Duplo clique num item do inventário para oferecer (Shift: todos). Travar quando terminar.")));
}

bool UMythRoHUDWidget::TryTradeAddFromInventory(int32 InventoryIndex, int32 Amount)
{
	URagnarokNetworkSubsystem* Net = GetNet();
	if (!Net || !Net->IsTradeOpen() || Net->IsTradeLockedMine())
	{
		return false;
	}
	return Net->SendTradeAddItem(InventoryIndex, Amount);
}

void UMythRoHUDWidget::ShowPlayerMenu(int64 AccountId, const FString& Name, const FVector2D& ScreenPos)
{
	if (!PlayerMenuWindow)
	{
		return;
	}
	MenuTargetAid = AccountId;
	MenuTargetName = Name;
	if (UCanvasPanelSlot* S = Cast<UCanvasPanelSlot>(PlayerMenuWindow->Slot); S && Root)
	{
		const FVector2D Local = Root->GetCachedGeometry().AbsoluteToLocal(ScreenPos);
		S->SetAnchors(FAnchors(0.f, 0.f));
		S->SetAlignment(FVector2D::ZeroVector);
		S->SetPosition(Local);
		S->SetZOrder(++TopZOrder);
	}
	PlayerMenuWindow->SetVisibility(ESlateVisibility::SelfHitTestInvisible);
	OpenOrder.Remove(PlayerMenuWindow);
	OpenOrder.Add(PlayerMenuWindow);
}

bool UMythRoHUDWidget::HandleTradeClick(const FVector2D& ScreenPos)
{
	URagnarokNetworkSubsystem* Net = GetNet();
	if (!Net)
	{
		return false;
	}
	if (PlayerMenuWindow && PlayerMenuWindow->IsVisible())
	{
		for (int32 i = 0; i < PlayerMenuItems.Num(); ++i)
		{
			if (!IsUnder(PlayerMenuItems[i], ScreenPos))
			{
				continue;
			}
			PlayerMenuWindow->SetVisibility(ESlateVisibility::Collapsed);
			OpenOrder.Remove(PlayerMenuWindow);
			switch (i)
			{
			case 0: Net->SendTradeRequest(static_cast<int32>(MenuTargetAid)); break;
			case 1: Net->SendPartyInvite(MenuTargetName); break;
			case 2:
				if (ChatInput)
				{
					ChatInput->SetText(FText::FromString(FString::Printf(TEXT("/w %s "), *MenuTargetName)));
					ChatInput->SetKeyboardFocus();
				}
				break;
			case 3: Net->SendFriendAdd(MenuTargetName); break;
			case 4: Net->SendGuildInvite(MenuTargetName); break;
			default: break;
			}
			return true;
		}
	}
	if (TradeAskWindow && TradeAskWindow->IsVisible())
	{
		if (IsUnder(TradeAcceptLink, ScreenPos) || IsUnder(TradeDeclineLink, ScreenPos))
		{
			Net->SendTradeReply(IsUnder(TradeAcceptLink, ScreenPos));
			TradeAskWindow->SetVisibility(ESlateVisibility::Collapsed);
			OpenOrder.Remove(TradeAskWindow);
			return true;
		}
	}
	if (TradeWindow && TradeWindow->IsVisible())
	{
		if (IsUnder(TradeLockLink, ScreenPos) && !Net->IsTradeLockedMine()) { Net->SendTradeLock(); return true; }
		if (IsUnder(TradeCommitLink, ScreenPos) && Net->IsTradeLockedMine() && Net->IsTradeLockedTheirs()) { Net->SendTradeCommit(); return true; }
		if (IsUnder(TradeCancelLink, ScreenPos)) { Net->SendTradeCancel(); return true; }
	}
	return false;
}

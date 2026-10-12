// Loja de jogador (Vending) do UMythRoHUDWidget. Tres janelas, como o cliente:
//  - Abrir loja: aparece quando a skill Venda deixa (ZC_OPENSTORE). Nome da loja, os itens do carrinho a esquerda e a loja a
//    direita; clique num item do carrinho poe na loja com o preco e a quantidade das caixas (vazia = a pilha toda), clique na
//    loja tira. "Abrir" manda; "Cancelar" desiste (o servidor libera a skill).
//  - Minha loja: o que esta a venda e quanto resta; "Fechar loja".
//  - Loja de outro (clique na placa sobre a cabeca): clique soma 1 (Shift: 10), total em zeny, "Comprar".
// Pacotes em Network/RagnarokNetworkVending.cpp; preco, zeny, peso e estoque sao do servidor.

#include "UI/MythRoHUDWidget.h"

#include "Network/RagnarokNetworkSubsystem.h"
#include "UI/MythRoClientTables.h"

#include "Blueprint/WidgetTree.h"
#include "Components/CanvasPanel.h"
#include "Components/CanvasPanelSlot.h"
#include "Components/EditableTextBox.h"
#include "Components/Image.h"
#include "Components/TextBlock.h"
#include "GameFramework/PlayerController.h"

namespace
{
	const FLinearColor VendText(0.12f, 0.12f, 0.12f);
	const FLinearColor VendGray(0.45f, 0.45f, 0.45f);
	constexpr float VendRowH = 20.f;
	constexpr float VendListY = 62.f;
	constexpr float VendW = 520.f;
	constexpr float VendColW = 246.f;

	FString VendItemName(int32 ItemID, int32 Refine)
	{
		FString Name = FMythRoClientTables::ItemName(ItemID);
		if (Name.IsEmpty())
		{
			Name = FString::Printf(TEXT("Item %d"), ItemID);
		}
		return Refine > 0 ? FString::Printf(TEXT("+%d %s"), Refine, *Name) : Name;
	}

	/** Zeny com separador de milhar (1.000.000). */
	FString VendZeny(int64 Z)
	{
		FString S = FString::Printf(TEXT("%lld"), Z);
		for (int32 i = S.Len() - 3; i > 0; i -= 3)
		{
			S.InsertAt(i, TEXT("."));
		}
		return S;
	}
}

void UMythRoHUDWidget::BuildVendingWindows()
{
	auto Box = [this](UCanvasPanel* Win, float X, float Y, float W, const TCHAR* Hint) -> UEditableTextBox*
	{
		UEditableTextBox* B = WidgetTree->ConstructWidget<UEditableTextBox>();
		B->SetHintText(FText::FromString(Hint));
		if (UCanvasPanelSlot* S = Win->AddChildToCanvas(B))
		{
			S->SetPosition(FVector2D(X, Y) * UIScale);
			S->SetSize(FVector2D(W, 18.f) * UIScale);
		}
		return B;
	};

	// Abrir loja
	const float SetupH = VendListY + VendRows * VendRowH + 40.f;
	VendSetupWindow = MakeRoPanel(TEXT("Abrir loja"), FVector2D(VendW, SetupH), FVector2D(0.f, -40.f));
	VendTitleBox = Box(VendSetupWindow, 8.f, 22.f, 250.f, TEXT("Nome da loja"));
	VendPriceBox = Box(VendSetupWindow, 270.f, 22.f, 120.f, TEXT("Preço (z)"));
	VendAmountBox = Box(VendSetupWindow, 398.f, 22.f, 114.f, TEXT("Qtd. (vazio: tudo)"));
	AddText(VendSetupWindow, 8.f, 44.f, 8, VendGray)->SetText(FText::FromString(TEXT("No carrinho (clique: põe na loja)")));
	AddText(VendSetupWindow, 266.f, 44.f, 8, VendGray)->SetText(FText::FromString(TEXT("Na loja (clique: tira)")));
	for (int32 i = 0; i < VendRows; ++i)
	{
		const float Y = VendListY + i * VendRowH;
		for (int32 Side = 0; Side < 2; ++Side)
		{
			const float X = Side == 0 ? 8.f : 266.f;
			UImage* Hit = AddColor(VendSetupWindow, FLinearColor(0.f, 0.f, 0.f, 0.03f), X, Y, VendColW, VendRowH - 1.f);
			Hit->SetVisibility(ESlateVisibility::Visible);
			UTextBlock* Name = AddText(VendSetupWindow, X + 4.f, Y + 3.f, 8, VendText);
			(Side == 0 ? VendCartHit : VendShopHit).Add(Hit);
			(Side == 0 ? VendCartNames : VendShopNames).Add(Name);
		}
	}
	VendSetupInfo = AddText(VendSetupWindow, 8.f, SetupH - 24.f, 8, VendGray);
	VendOpenBtn = AddTextButton(VendSetupWindow, TEXT("Abrir"), VendW - 150.f, SetupH - 28.f, 64.f);
	VendCancelBtn = AddTextButton(VendSetupWindow, TEXT("Cancelar"), VendW - 80.f, SetupH - 28.f, 72.f);

	// Minha loja aberta
	const float ShopH = 40.f + VendRows * VendRowH + 36.f;
	VendShopWindow = MakeRoPanel(TEXT("Minha loja"), FVector2D(300.f, ShopH), FVector2D(-300.f, 0.f));
	for (int32 i = 0; i < VendRows; ++i)
	{
		VendMyNames.Add(AddText(VendShopWindow, 8.f, 24.f + i * VendRowH, 8, VendText));
	}
	VendCloseShopBtn = AddTextButton(VendShopWindow, TEXT("Fechar loja"), 300.f - 98.f, ShopH - 28.f, 90.f);

	// Loja de outro jogador
	const float BuyH = 40.f + VendRows * VendRowH + 40.f;
	VendBuyWindow = MakeRoPanel(TEXT("Loja"), FVector2D(380.f, BuyH), FVector2D(0.f, -40.f));
	AddText(VendBuyWindow, 8.f, 22.f, 8, VendGray)->SetText(FText::FromString(TEXT("Clique: +1 (Shift: +10); clique com Ctrl: zera")));
	for (int32 i = 0; i < VendRows; ++i)
	{
		const float Y = 40.f + i * VendRowH;
		UImage* Hit = AddColor(VendBuyWindow, FLinearColor(0.f, 0.f, 0.f, 0.03f), 8.f, Y, 364.f, VendRowH - 1.f);
		Hit->SetVisibility(ESlateVisibility::Visible);
		VendBuyHit.Add(Hit);
		VendBuyNames.Add(AddText(VendBuyWindow, 12.f, Y + 3.f, 8, VendText));
	}
	VendBuyTotal = AddText(VendBuyWindow, 8.f, BuyH - 24.f, 8, VendText);
	VendBuyBtn = AddTextButton(VendBuyWindow, TEXT("Comprar"), 380.f - 160.f, BuyH - 28.f, 72.f);
	VendBuyCancelBtn = AddTextButton(VendBuyWindow, TEXT("Cancelar"), 380.f - 82.f, BuyH - 28.f, 74.f);
}

void UMythRoHUDWidget::OnVendingEvent(uint8 Event)
{
	auto Show = [this](UCanvasPanel* Win)
	{
		Win->SetVisibility(ESlateVisibility::SelfHitTestInvisible);
		OpenOrder.Remove(Win);
		OpenOrder.Add(Win);
		if (UCanvasPanelSlot* S = Cast<UCanvasPanelSlot>(Win->Slot))
		{
			S->SetZOrder(++TopZOrder);
		}
	};
	switch (Event)
	{
	case 0: // a skill Venda deixou abrir: monta a loja
		VendPlanned.Reset();
		Show(VendSetupWindow);
		VendTitleBox->SetKeyboardFocus();
		break;
	case 1: // chegou a lista de outra loja
		VendBuyChosen.Reset();
		Show(VendBuyWindow);
		break;
	case 2: // a minha loja abriu ou vendeu algo
		VendSetupWindow->SetVisibility(ESlateVisibility::Collapsed);
		Show(VendShopWindow);
		break;
	case 3: // a minha loja fechou
		VendShopWindow->SetVisibility(ESlateVisibility::Collapsed);
		break;
	default:
		break;
	}
	RefreshVending();
}

void UMythRoHUDWidget::RefreshVending()
{
	const URagnarokNetworkSubsystem* Net = GetNet();
	if (!Net || !VendSetupWindow)
	{
		return;
	}
	if (VendSetupWindow->IsVisible())
	{
		const TArray<FRagnarokInventoryItem>& Cart = Net->GetCartItems();
		for (int32 i = 0; i < VendRows; ++i)
		{
			const FRagnarokInventoryItem* C = Cart.IsValidIndex(i) ? &Cart[i] : nullptr;
			VendCartHit[i]->SetVisibility(C ? ESlateVisibility::Visible : ESlateVisibility::Collapsed);
			VendCartNames[i]->SetText(C ? FText::FromString(FString::Printf(TEXT("%s  x %d"), *VendItemName(C->ItemID, C->Refine), C->Amount)) : FText::GetEmpty());
			const FIntVector* P = VendPlanned.IsValidIndex(i) ? &VendPlanned[i] : nullptr;
			const FRagnarokInventoryItem* PlannedItem = P ? Cart.FindByPredicate([P](const FRagnarokInventoryItem& VI) { return VI.Index == P->X; }) : nullptr;
			VendShopHit[i]->SetVisibility(P ? ESlateVisibility::Visible : ESlateVisibility::Collapsed);
			VendShopNames[i]->SetText(P ? FText::FromString(FString::Printf(TEXT("%s  x %d  -  %s z"), PlannedItem ? *VendItemName(PlannedItem->ItemID, PlannedItem->Refine) : TEXT("?"), P->Y, *VendZeny(P->Z))) : FText::GetEmpty());
		}
		VendSetupInfo->SetText(FText::FromString(FString::Printf(TEXT("Itens na loja: %d / %d"), VendPlanned.Num(), Net->GetVendMaxItems())));
	}
	if (VendShopWindow->IsVisible())
	{
		const TArray<FRoVendItem>& Mine = Net->GetMyVendItems();
		for (int32 i = 0; i < VendRows; ++i)
		{
			const FRoVendItem* M = Mine.IsValidIndex(i) ? &Mine[i] : nullptr;
			VendMyNames[i]->SetText(M ? FText::FromString(FString::Printf(TEXT("%s  x %d  -  %s z"), *VendItemName(M->ItemID, M->Refine), M->Amount, *VendZeny(M->Price))) : FText::GetEmpty());
		}
	}
	if (VendBuyWindow->IsVisible())
	{
		const TArray<FRoVendItem>& Items = Net->GetVendBuyItems();
		int64 Total = 0;
		for (int32 i = 0; i < VendRows; ++i)
		{
			const FRoVendItem* VI = Items.IsValidIndex(i) ? &Items[i] : nullptr;
			const int32 Chosen = VI ? VendBuyChosen.FindRef(VI->Index) : 0;
			VendBuyHit[i]->SetVisibility(VI ? ESlateVisibility::Visible : ESlateVisibility::Collapsed);
			VendBuyHit[i]->SetColorAndOpacity(Chosen > 0 ? FLinearColor(0.78f, 0.86f, 0.98f, 1.f) : FLinearColor(0.f, 0.f, 0.f, 0.03f));
			VendBuyNames[i]->SetText(VI ? FText::FromString(FString::Printf(TEXT("%s  -  %s z  (resta %d)%s"), *VendItemName(VI->ItemID, VI->Refine), *VendZeny(VI->Price), VI->Amount,
				Chosen > 0 ? *FString::Printf(TEXT("   comprar: %d"), Chosen) : TEXT(""))) : FText::GetEmpty());
		}
		for (const FRoVendItem& VI : Items)
		{
			Total += static_cast<int64>(VI.Price) * VendBuyChosen.FindRef(VI.Index);
		}
		VendBuyTotal->SetText(FText::FromString(FString::Printf(TEXT("Total: %s z"), *VendZeny(Total))));
	}
}

bool UMythRoHUDWidget::HandleVendingClick(const FVector2D& ScreenPos)
{
	URagnarokNetworkSubsystem* Net = GetNet();
	if (!Net || !VendSetupWindow)
	{
		return false;
	}
	APlayerController* PC = GetOwningPlayer();
	const bool bShift = PC && (PC->IsInputKeyDown(EKeys::LeftShift) || PC->IsInputKeyDown(EKeys::RightShift));
	const bool bCtrl = PC && (PC->IsInputKeyDown(EKeys::LeftControl) || PC->IsInputKeyDown(EKeys::RightControl));
	if (VendSetupWindow->IsVisible())
	{
		if (IsUnder(VendOpenBtn, ScreenPos))
		{
			const FString Title = VendTitleBox->GetText().ToString().TrimStartAndEnd();
			if (Title.IsEmpty() || VendPlanned.Num() == 0)
			{
				AddChatLine(TEXT("Dê um nome à loja e ponha pelo menos um item."), FLinearColor(1.f, 0.45f, 0.45f));
				return true;
			}
			Net->SendOpenVending(Title, VendPlanned, true);
			return true;
		}
		if (IsUnder(VendCancelBtn, ScreenPos))
		{
			Net->SendOpenVending(TEXT("-"), {}, false);
			VendSetupWindow->SetVisibility(ESlateVisibility::Collapsed);
			return true;
		}
		const TArray<FRagnarokInventoryItem>& Cart = Net->GetCartItems();
		for (int32 i = 0; i < VendRows; ++i)
		{
			if (Cart.IsValidIndex(i) && IsUnder(VendCartHit[i], ScreenPos))
			{
				const int32 Price = FCString::Atoi(*VendPriceBox->GetText().ToString().Replace(TEXT("."), TEXT("")));
				const int32 Wanted = FCString::Atoi(*VendAmountBox->GetText().ToString());
				if (Price <= 0)
				{
					AddChatLine(TEXT("Digite o preço antes de pôr o item na loja."), FLinearColor(1.f, 0.45f, 0.45f));
					return true;
				}
				if (VendPlanned.Num() >= Net->GetVendMaxItems() || VendPlanned.ContainsByPredicate([&](const FIntVector& P) { return P.X == Cart[i].Index; }))
				{
					return true;
				}
				VendPlanned.Add(FIntVector(Cart[i].Index, Wanted > 0 ? FMath::Min(Wanted, Cart[i].Amount) : Cart[i].Amount, Price));
				RefreshVending();
				return true;
			}
			if (VendPlanned.IsValidIndex(i) && IsUnder(VendShopHit[i], ScreenPos))
			{
				VendPlanned.RemoveAt(i);
				RefreshVending();
				return true;
			}
		}
		if (IsUnder(VendSetupWindow, ScreenPos))
		{
			return true;
		}
	}
	if (VendShopWindow->IsVisible() && IsUnder(VendCloseShopBtn, ScreenPos))
	{
		Net->SendCloseVending();
		return true;
	}
	if (VendBuyWindow->IsVisible())
	{
		if (IsUnder(VendBuyCancelBtn, ScreenPos))
		{
			VendBuyWindow->SetVisibility(ESlateVisibility::Collapsed);
			return true;
		}
		if (IsUnder(VendBuyBtn, ScreenPos))
		{
			TArray<FIntPoint> Order;
			for (const TPair<int32, int32>& C : VendBuyChosen)
			{
				if (C.Value > 0)
				{
					Order.Add(FIntPoint(C.Key, C.Value));
				}
			}
			if (Net->SendVendingBuy(Order))
			{
				VendBuyWindow->SetVisibility(ESlateVisibility::Collapsed);
			}
			return true;
		}
		const TArray<FRoVendItem>& Items = Net->GetVendBuyItems();
		for (int32 i = 0; i < VendRows; ++i)
		{
			if (Items.IsValidIndex(i) && IsUnder(VendBuyHit[i], ScreenPos))
			{
				int32& Chosen = VendBuyChosen.FindOrAdd(Items[i].Index);
				Chosen = bCtrl ? 0 : FMath::Min(Chosen + (bShift ? 10 : 1), Items[i].Amount);
				RefreshVending();
				return true;
			}
		}
	}
	return false;
}

void UMythRoHUDWidget::VendTestOpen(const FString& Title, int32 Price)
{
	URagnarokNetworkSubsystem* Net = GetNet();
	if (!Net)
	{
		return;
	}
	VendTitleBox->SetText(FText::FromString(Title));
	VendPlanned.Reset();
	for (const FRagnarokInventoryItem& C : Net->GetCartItems())
	{
		if (VendPlanned.Num() < Net->GetVendMaxItems())
		{
			VendPlanned.Add(FIntVector(C.Index, C.Amount, Price));
		}
	}
	Net->SendOpenVending(Title, VendPlanned, true);
}

void UMythRoHUDWidget::VendTestBuy(int32 Row, int32 Count)
{
	URagnarokNetworkSubsystem* Net = GetNet();
	if (Net && Net->GetVendBuyItems().IsValidIndex(Row))
	{
		Net->SendVendingBuy({ FIntPoint(Net->GetVendBuyItems()[Row].Index, Count) });
	}
}

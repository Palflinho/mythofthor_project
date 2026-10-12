// Carrinho do Mercador: o servidor manda a lista (0x0b09/0x0b0a com invType 1, entre 0x0b08 e 0x0b0b), atualiza item a item
// (0x0b45 entrou, 0x0125 saiu), o peso e a quantidade (0x0121) e tira o carrinho (0x012b). Aqui so mostramos as duas listas e
// pedimos as transferencias (CZ_MOVE_ITEM_FROM_BODY_TO_CART 0x0126 / FROM_CART_TO_BODY 0x0127); peso e limites sao do servidor.

#include "UI/MythRoHUDWidget.h"

#include "Network/RagnarokNetworkSubsystem.h"
#include "Core/MythRoClientTables.h"

#include "Components/CanvasPanel.h"
#include "Components/Image.h"
#include "Components/TextBlock.h"
#include "GameFramework/PlayerController.h"

namespace
{
	const FLinearColor CartText(0.12f, 0.12f, 0.12f);
	const FLinearColor CartGray(0.45f, 0.45f, 0.45f);
	constexpr float CartRowH = 24.f;
	constexpr float CartW = 560.f;
	constexpr float CartListY = 44.f;
	constexpr float CartListW = 266.f;
	constexpr float CartColA = 8.f;
	constexpr float CartColB = 286.f;
	constexpr int32 CartRows = 12;

	FString CartItemLabel(const FRagnarokInventoryItem& I)
	{
		FString Name = FMythRoClientTables::ItemName(I.ItemID);
		if (Name.IsEmpty())
		{
			Name = FString::Printf(TEXT("Item %d"), I.ItemID);
		}
		if (I.Refine > 0)
		{
			Name = FString::Printf(TEXT("+%d %s"), I.Refine, *Name);
		}
		return I.Amount > 1 ? FString::Printf(TEXT("%s  x %d"), *Name, I.Amount) : Name;
	}

	/** Itens da mochila que podem ir para o carrinho (equipados nao vao). */
	TArray<const FRagnarokInventoryItem*> CartBagItems(const URagnarokNetworkSubsystem& Net)
	{
		TArray<const FRagnarokInventoryItem*> Bag;
		for (const FRagnarokInventoryItem& I : Net.GetInventoryItems())
		{
			if (!I.bIsEquipped)
			{
				Bag.Add(&I);
			}
		}
		return Bag;
	}
}

void UMythRoHUDWidget::BuildCartWindow()
{
	const float H = CartListY + CartRows * CartRowH + 40.f;
	CartWindow = MakeRoPanel(TEXT("Carrinho"), FVector2D(CartW, H), FVector2D(40.f, -20.f));
	AddText(CartWindow, CartColA, 22.f, 8, CartGray)->SetText(FText::FromString(TEXT("No carrinho (clique: pega 1; Shift: 10)")));
	AddText(CartWindow, CartColB, 22.f, 8, CartGray)->SetText(FText::FromString(TEXT("Na mochila (clique: põe 1; Shift: 10)")));
	for (int32 i = 0; i < CartRows; ++i)
	{
		const float Y = CartListY + i * CartRowH;
		for (int32 Side = 0; Side < 2; ++Side)
		{
			const float X = Side == 0 ? CartColA : CartColB;
			UImage* Hit = AddColor(CartWindow, FLinearColor(0.f, 0.f, 0.f, 0.03f), X, Y, CartListW, CartRowH - 1.f);
			Hit->SetVisibility(ESlateVisibility::Visible);
			UImage* Icon = AddImage(CartWindow, nullptr, X + 2.f, Y, 24.f, 24.f);
			Icon->SetVisibility(ESlateVisibility::Collapsed);
			UTextBlock* Name = AddText(CartWindow, X + 32.f, Y + 5.f, 8, CartText);
			(Side == 0 ? CartHit : CartBagHit).Add(Hit);
			(Side == 0 ? CartIcons : CartBagIcons).Add(Icon);
			(Side == 0 ? CartNames : CartBagNames).Add(Name);
		}
	}
	CartInfoText = AddText(CartWindow, CartColA, H - 26.f, 8, CartGray);
	CartCloseLink = AddButtonHit(CartWindow, TEXT("close"), CartW - 56.f, H - 30.f);
	CartWindow->SetVisibility(ESlateVisibility::Collapsed);
}

void UMythRoHUDWidget::ToggleCartWindow()
{
	const URagnarokNetworkSubsystem* Net = GetNet();
	if (!CartWindow)
	{
		return;
	}
	if (CartWindow->IsVisible())
	{
		CartWindow->SetVisibility(ESlateVisibility::Collapsed);
		return;
	}
	if (!Net || !Net->HasCart())
	{
		AddChatLine(TEXT("Você não tem um carrinho."), FLinearColor(1.f, 0.45f, 0.45f));
		return;
	}
	CartScroll = 0;
	CartBagScroll = 0;
	CartWindow->SetVisibility(ESlateVisibility::SelfHitTestInvisible);
	OpenOrder.Remove(CartWindow);
	OpenOrder.Add(CartWindow);
	RefreshCart();
}

void UMythRoHUDWidget::RefreshCart()
{
	const URagnarokNetworkSubsystem* Net = GetNet();
	if (EquipCartBtn)
	{
		// O botao do carrinho no Equipamento so existe com carrinho, na aba Geral
		EquipCartBtn->SetVisibility(Net && Net->HasCart() && EquipTab == 0 ? ESlateVisibility::Visible : ESlateVisibility::Collapsed);
	}
	if (!CartWindow || !Net || !CartWindow->IsVisible())
	{
		return;
	}
	if (!Net->HasCart())
	{
		CartWindow->SetVisibility(ESlateVisibility::Collapsed); // o carrinho foi tirado
		return;
	}
	const TArray<FRagnarokInventoryItem>& Cart = Net->GetCartItems();
	const TArray<const FRagnarokInventoryItem*> Bag = CartBagItems(*Net);
	CartScroll = FMath::Clamp(CartScroll, 0, FMath::Max(0, Cart.Num() - CartRows));
	CartBagScroll = FMath::Clamp(CartBagScroll, 0, FMath::Max(0, Bag.Num() - CartRows));
	for (int32 i = 0; i < CartRows; ++i)
	{
		const FRagnarokInventoryItem* C = Cart.IsValidIndex(CartScroll + i) ? &Cart[CartScroll + i] : nullptr;
		CartHit[i]->SetVisibility(C ? ESlateVisibility::Visible : ESlateVisibility::Collapsed);
		CartNames[i]->SetText(C ? FText::FromString(CartItemLabel(*C)) : FText::GetEmpty());
		UTexture2D* CIcon = C ? FMythRoClientTables::ItemIcon(C->ItemID) : nullptr;
		if (CIcon) { CartIcons[i]->SetBrushFromTexture(CIcon); }
		CartIcons[i]->SetVisibility(CIcon ? ESlateVisibility::HitTestInvisible : ESlateVisibility::Collapsed);

		const FRagnarokInventoryItem* B = Bag.IsValidIndex(CartBagScroll + i) ? Bag[CartBagScroll + i] : nullptr;
		CartBagHit[i]->SetVisibility(B ? ESlateVisibility::Visible : ESlateVisibility::Collapsed);
		CartBagNames[i]->SetText(B ? FText::FromString(CartItemLabel(*B)) : FText::GetEmpty());
		UTexture2D* BIcon = B ? FMythRoClientTables::ItemIcon(B->ItemID) : nullptr;
		if (BIcon) { CartBagIcons[i]->SetBrushFromTexture(BIcon); }
		CartBagIcons[i]->SetVisibility(BIcon ? ESlateVisibility::HitTestInvisible : ESlateVisibility::Collapsed);
	}
	// Peso como o cliente mostra (o servidor manda em decimos)
	CartInfoText->SetText(FText::FromString(FString::Printf(TEXT("Itens %d / %d    Peso %d / %d"),
		Net->GetCartCount(), Net->GetCartMax(), Net->GetCartWeight() / 10, Net->GetCartMaxWeight() / 10)));
}

void UMythRoHUDWidget::CartMove(bool bToCart, int32 Row, int32 Amount)
{
	URagnarokNetworkSubsystem* Net = GetNet();
	if (!Net)
	{
		return;
	}
	if (bToCart)
	{
		const TArray<const FRagnarokInventoryItem*> Bag = CartBagItems(*Net);
		if (Bag.IsValidIndex(Row))
		{
			Net->SendCartMove(true, Bag[Row]->Index, FMath::Min(Amount, Bag[Row]->Amount));
		}
	}
	else if (Net->GetCartItems().IsValidIndex(Row))
	{
		const FRagnarokInventoryItem& I = Net->GetCartItems()[Row];
		Net->SendCartMove(false, I.Index, FMath::Min(Amount, I.Amount));
	}
}

bool UMythRoHUDWidget::HandleCartClick(const FVector2D& ScreenPos)
{
	if (EquipWindow && EquipWindow->IsVisible() && IsUnder(EquipCartBtn, ScreenPos))
	{
		ToggleCartWindow();
		return true;
	}
	if (!CartWindow || !CartWindow->IsVisible())
	{
		return false;
	}
	if (IsUnder(CartCloseLink, ScreenPos))
	{
		CartWindow->SetVisibility(ESlateVisibility::Collapsed);
		return true;
	}
	const bool bShift = GetOwningPlayer() && (GetOwningPlayer()->IsInputKeyDown(EKeys::LeftShift) || GetOwningPlayer()->IsInputKeyDown(EKeys::RightShift));
	for (int32 i = 0; i < CartRows; ++i)
	{
		if (IsUnder(CartHit[i], ScreenPos))
		{
			CartMove(false, CartScroll + i, bShift ? 10 : 1);
			return true;
		}
		if (IsUnder(CartBagHit[i], ScreenPos))
		{
			CartMove(true, CartBagScroll + i, bShift ? 10 : 1);
			return true;
		}
	}
	return false;
}

bool UMythRoHUDWidget::HandleCartWheel(const FVector2D& ScreenPos, float Delta)
{
	if (!CartWindow || !CartWindow->IsVisible() || !IsUnder(CartWindow, ScreenPos))
	{
		return false;
	}
	const FGeometry& Geo = CartWindow->GetCachedGeometry();
	const bool bLeft = Geo.AbsoluteToLocal(ScreenPos).X < Geo.GetLocalSize().X * 0.5f;
	(bLeft ? CartScroll : CartBagScroll) += Delta > 0.f ? -1 : 1;
	RefreshCart();
	return true;
}

// Armazem da Kafra (docs/ref/15): o servidor manda a lista (0x0b09/0x0b39 com invType 2, entre 0x0b08 e 0x0b0b),
// atualiza item a item (0x0b44 entrou, 0x00f6 saiu) e fecha (0x00f8). Aqui so mostramos as duas listas e pedimos as
// transferencias (0x0364 / 0x0365); peso, taxa e travas de movimento sao do servidor.

#include "UI/MythRoHUDWidget.h"

#include "Network/RagnarokNetworkSubsystem.h"
#include "UI/MythRoClientTables.h"

#include "Components/CanvasPanel.h"
#include "Components/Image.h"
#include "Components/TextBlock.h"
#include "GameFramework/PlayerController.h"

namespace
{
	const FLinearColor StoreText(0.12f, 0.12f, 0.12f);
	const FLinearColor StoreLink(0.12f, 0.3f, 0.65f);
	const FLinearColor StoreGray(0.45f, 0.45f, 0.45f);
	constexpr float StoreRowH = 24.f;
	constexpr float StoreW = 560.f;
	constexpr float StoreListY = 44.f;
	constexpr float StoreListW = 266.f;
	constexpr float StoreColA = 8.f;
	constexpr float StoreColB = 286.f;

	FString ItemLabel(const FRagnarokInventoryItem& I)
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
}

void UMythRoHUDWidget::BuildStorageWindow()
{
	const float H = StoreListY + StorageRowCount * StoreRowH + 40.f;
	StorageWindow = MakeRoPanel(TEXT("Armazém"), FVector2D(StoreW, H), FVector2D(0.f, -20.f));
	AddText(StorageWindow, StoreColA, 22.f, 8, StoreGray)->SetText(FText::FromString(TEXT("No armazém (clique: pega 1; Shift: 10)")));
	AddText(StorageWindow, StoreColB, 22.f, 8, StoreGray)->SetText(FText::FromString(TEXT("Na mochila (clique: guarda 1; Shift: 10)")));
	for (int32 i = 0; i < StorageRowCount; ++i)
	{
		const float Y = StoreListY + i * StoreRowH;
		for (int32 Side = 0; Side < 2; ++Side)
		{
			const float X = Side == 0 ? StoreColA : StoreColB;
			UImage* Hit = AddColor(StorageWindow, FLinearColor(0.f, 0.f, 0.f, 0.03f), X, Y, StoreListW, StoreRowH - 1.f);
			Hit->SetVisibility(ESlateVisibility::Visible);
			UImage* Icon = AddImage(StorageWindow, nullptr, X + 2.f, Y, 24.f, 24.f);
			Icon->SetVisibility(ESlateVisibility::Collapsed);
			UTextBlock* Name = AddText(StorageWindow, X + 32.f, Y + 5.f, 8, StoreText);
			(Side == 0 ? StoreHit : BagHit).Add(Hit);
			(Side == 0 ? StoreIcons : BagIcons).Add(Icon);
			(Side == 0 ? StoreNames : BagNames).Add(Name);
		}
	}
	StorageCountText = AddText(StorageWindow, StoreColA, H - 26.f, 8, StoreGray);
	StorageCloseLink = AddText(StorageWindow, StoreW - 70.f, H - 28.f, 9, StoreLink);
	StorageCloseLink->SetText(FText::FromString(TEXT("Fechar")));
	StorageWindow->SetVisibility(ESlateVisibility::Collapsed);
}

bool UMythRoHUDWidget::IsStorageOpen() const
{
	return StorageWindow && StorageWindow->IsVisible();
}

void UMythRoHUDWidget::OpenStorage()
{
	if (!StorageWindow)
	{
		return;
	}
	StoreScroll = 0;
	BagScroll = 0;
	StorageWindow->SetVisibility(ESlateVisibility::SelfHitTestInvisible);
	OpenOrder.Remove(StorageWindow);
	OpenOrder.Add(StorageWindow);
	RefreshStorage();
}

void UMythRoHUDWidget::RefreshStorage()
{
	const URagnarokNetworkSubsystem* Net = GetNet();
	if (!StorageWindow || !Net || !StorageWindow->IsVisible())
	{
		return;
	}
	const TArray<FRagnarokInventoryItem>& Store = Net->GetStorageItems();
	TArray<const FRagnarokInventoryItem*> Bag;
	for (const FRagnarokInventoryItem& I : Net->GetInventoryItems())
	{
		if (!I.bIsEquipped)
		{
			Bag.Add(&I); // equipado nao vai para o armazem
		}
	}
	StoreScroll = FMath::Clamp(StoreScroll, 0, FMath::Max(0, Store.Num() - StorageRowCount));
	BagScroll = FMath::Clamp(BagScroll, 0, FMath::Max(0, Bag.Num() - StorageRowCount));
	for (int32 i = 0; i < StorageRowCount; ++i)
	{
		const FRagnarokInventoryItem* S = Store.IsValidIndex(StoreScroll + i) ? &Store[StoreScroll + i] : nullptr;
		StoreHit[i]->SetVisibility(S ? ESlateVisibility::Visible : ESlateVisibility::Collapsed);
		StoreNames[i]->SetText(S ? FText::FromString(ItemLabel(*S)) : FText::GetEmpty());
		UTexture2D* SIcon = S ? FMythRoClientTables::ItemIcon(S->ItemID) : nullptr;
		if (SIcon) { StoreIcons[i]->SetBrushFromTexture(SIcon); }
		StoreIcons[i]->SetVisibility(SIcon ? ESlateVisibility::HitTestInvisible : ESlateVisibility::Collapsed);

		const FRagnarokInventoryItem* B = Bag.IsValidIndex(BagScroll + i) ? Bag[BagScroll + i] : nullptr;
		BagHit[i]->SetVisibility(B ? ESlateVisibility::Visible : ESlateVisibility::Collapsed);
		BagNames[i]->SetText(B ? FText::FromString(ItemLabel(*B)) : FText::GetEmpty());
		UTexture2D* BIcon = B ? FMythRoClientTables::ItemIcon(B->ItemID) : nullptr;
		if (BIcon) { BagIcons[i]->SetBrushFromTexture(BIcon); }
		BagIcons[i]->SetVisibility(BIcon ? ESlateVisibility::HitTestInvisible : ESlateVisibility::Collapsed);
	}
	StorageCountText->SetText(FText::FromString(FString::Printf(TEXT("%d / %d espaços usados"), Store.Num(), Net->GetStorageMax())));
}

void UMythRoHUDWidget::StorageMove(bool bToStorage, int32 Row, int32 Amount)
{
	URagnarokNetworkSubsystem* Net = GetNet();
	if (!Net)
	{
		return;
	}
	if (bToStorage)
	{
		int32 N = 0;
		for (const FRagnarokInventoryItem& I : Net->GetInventoryItems())
		{
			if (!I.bIsEquipped && N++ == Row)
			{
				Net->SendStorageMove(true, I.Index, FMath::Min(Amount, I.Amount));
				return;
			}
		}
	}
	else if (Net->GetStorageItems().IsValidIndex(Row))
	{
		const FRagnarokInventoryItem& I = Net->GetStorageItems()[Row];
		Net->SendStorageMove(false, I.Index, FMath::Min(Amount, I.Amount));
	}
}

bool UMythRoHUDWidget::HandleStorageClick(const FVector2D& ScreenPos)
{
	if (!IsStorageOpen())
	{
		return false;
	}
	if (IsUnder(StorageCloseLink, ScreenPos))
	{
		CloseStorage(true);
		return true;
	}
	const bool bShift = GetOwningPlayer() && (GetOwningPlayer()->IsInputKeyDown(EKeys::LeftShift) || GetOwningPlayer()->IsInputKeyDown(EKeys::RightShift));
	for (int32 i = 0; i < StorageRowCount; ++i)
	{
		if (IsUnder(StoreHit[i], ScreenPos))
		{
			StorageMove(false, StoreScroll + i, bShift ? 10 : 1);
			return true;
		}
		if (IsUnder(BagHit[i], ScreenPos))
		{
			StorageMove(true, BagScroll + i, bShift ? 10 : 1);
			return true;
		}
	}
	return false;
}

bool UMythRoHUDWidget::HandleStorageWheel(const FVector2D& ScreenPos, float Delta)
{
	if (!IsStorageOpen() || !IsUnder(StorageWindow, ScreenPos))
	{
		return false;
	}
	const FGeometry& Geo = StorageWindow->GetCachedGeometry();
	const bool bLeft = Geo.AbsoluteToLocal(ScreenPos).X < Geo.GetLocalSize().X * 0.5f;
	(bLeft ? StoreScroll : BagScroll) += Delta > 0.f ? -1 : 1;
	RefreshStorage();
	return true;
}

void UMythRoHUDWidget::CloseStorage(bool bNotifyServer)
{
	const bool bWasOpen = IsStorageOpen();
	if (StorageWindow)
	{
		StorageWindow->SetVisibility(ESlateVisibility::Collapsed);
	}
	if (bNotifyServer && bWasOpen)
	{
		if (URagnarokNetworkSubsystem* Net = GetNet())
		{
			Net->SendStorageClose();
		}
	}
}

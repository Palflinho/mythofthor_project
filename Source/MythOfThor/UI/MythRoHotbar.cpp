// Barra de atalhos F1..F9 do UMythRoHUDWidget, como no cliente original: os atalhos ficam no servidor
// (ZC_SHORTCUT_KEY_LIST / CZ_SHORTCUT_KEY_CHANGE2); arrastar uma skill ou um item da janela para um espaco grava o
// atalho; arrastar um atalho para fora da barra o apaga.

#include "UI/MythRoHUDWidget.h"
#include "Framework/Application/SlateApplication.h"

#include "Core/MythRoClientTables.h"
#include "UI/MythRoUITextures.h"
#include "Network/RagnarokNetworkSubsystem.h"

#include "Blueprint/WidgetTree.h"
#include "Components/CanvasPanel.h"
#include "Components/CanvasPanelSlot.h"
#include "Components/Image.h"
#include "Components/TextBlock.h"

namespace
{
	FSlateBrush HotbarBrush(UTexture2D* Tex, const FVector2D& Size)
	{
		FSlateBrush Brush;
		Brush.SetResourceObject(Tex);
		Brush.ImageSize = Size;
		Brush.DrawAs = ESlateBrushDrawType::Image;
		return Brush;
	}
}

bool UMythRoHUDWidget::HandleHotbarClick(const FVector2D& ScreenPos)
{
	if (!HotbarWindow || !HotbarWindow->IsVisible())
	{
		return false;
	}
	if (IsUnder(HotbarUp, ScreenPos)) { HotbarFirstRow = FMath::Max(0, HotbarFirstRow - 1); RefreshHotbar(); return true; }
	if (IsUnder(HotbarDown, ScreenPos)) { HotbarFirstRow = FMath::Min(2, HotbarFirstRow + 1); RefreshHotbar(); return true; }
	for (int32 r = 0; r < HotbarRowBoxes.Num(); ++r)
	{
		if (IsUnder(HotbarRowBoxes[r], ScreenPos))
		{
			HotbarActiveRow = HotbarFirstRow + r;
			RefreshHotbar();
			return true;
		}
	}
	return false;
}

int32 UMythRoHUDWidget::HotbarSlotAt(const FVector2D& ScreenPos) const
{
	for (int32 i = 0; i < HotbarSlots.Num(); ++i)
	{
		if (IsUnder(HotbarSlots[i], ScreenPos))
		{
			return (HotbarFirstRow + i / 9) * 9 + i % 9; // indice do atalho no servidor (a linha de cima pode ser a 2, 3 ou 4)
		}
	}
	return INDEX_NONE;
}

void UMythRoHUDWidget::RefreshHotbar()
{
	const URagnarokNetworkSubsystem* Net = GetNet();
	for (int32 r = 0; r < HotbarRowTexts.Num(); ++r)
	{
		const int32 Row = HotbarFirstRow + r;
		HotbarRowTexts[r]->SetText(FText::AsNumber(Row + 1));
		HotbarRowBoxes[r]->SetColorAndOpacity(Row == HotbarActiveRow ? FLinearColor(0.35f, 0.55f, 0.9f, 1.f) : FLinearColor(0.93f, 0.85f, 0.9f, 1.f));
		HotbarRowTexts[r]->SetColorAndOpacity(FSlateColor(Row == HotbarActiveRow ? FLinearColor::White : FLinearColor(0.4f, 0.3f, 0.4f)));
	}
	for (int32 i = 0; i < HotbarIcons.Num(); ++i)
	{
		UTexture2D* Icon = nullptr;
		FString Count;
		const int32 SlotIndex = (HotbarFirstRow + i / 9) * 9 + i % 9;
		if (Net && Net->GetHotkeys().IsValidIndex(SlotIndex) && Net->GetHotkeys()[SlotIndex].ID != 0)
		{
			const FRagnarokHotkey& H = Net->GetHotkeys()[SlotIndex];
			if (H.bIsSkill)
			{
				Icon = FMythRoClientTables::SkillIcon(H.ID);
				Count = FString::Printf(TEXT("%d"), FMath::Max(1, H.Count));
			}
			else
			{
				Icon = FMythRoClientTables::ItemIcon(H.ID);
				int32 Total = 0; // quantidade atual no inventario (o atalho guarda so o ID)
				for (const FRagnarokInventoryItem& Item : Net->GetInventoryItems())
				{
					if (Item.ItemID == H.ID && !Item.bIsEquipped)
					{
						Total += Item.Amount;
					}
				}
				Count = FString::Printf(TEXT("%d"), Total);
			}
		}
		HotbarIcons[i]->SetBrush(HotbarBrush(Icon, FVector2D(24.f, 24.f) * UIScale));
		HotbarIcons[i]->SetVisibility(Icon ? ESlateVisibility::HitTestInvisible : ESlateVisibility::Collapsed);
		HotbarCounts[i]->SetText(Icon ? FText::FromString(Count) : FText::GetEmpty());
	}
}

bool UMythRoHUDWidget::BeginHotDrag(const FVector2D& ScreenPos)
{
	const URagnarokNetworkSubsystem* Net = GetNet();
	if (!Net)
	{
		return false;
	}
	bDragIsSkill = false;
	DragID = 0;
	DragCount = 0;
	DragFromHotbar = INDEX_NONE;
	UTexture2D* Icon = nullptr;
	DragItemIndex = INDEX_NONE;

	// 1) skill da janela de habilidades
	if (SkillsWindow && SkillsWindow->IsVisible())
	{
		for (int32 r = 0; r < SkillIcons.Num() && r < SkillRowID.Num(); ++r)
		{
			if (SkillRowID[r] != 0 && IsUnder(SkillIcons[r], ScreenPos))
			{
				bDragIsSkill = true;
				DragID = SkillRowID[r];
				DragCount = SkillRowLevel[r];
				Icon = FMythRoClientTables::SkillIcon(DragID);
				break;
			}
		}
	}
	// 2) item da janela de itens
	if (!Icon && ItemsWindow && ItemsWindow->IsVisible())
	{
		for (int32 c = 0; c < ItemCells.Num(); ++c)
		{
			if (ItemCellIndex[c] < 0 || !IsUnder(ItemCells[c], ScreenPos))
			{
				continue;
			}
			for (const FRagnarokInventoryItem& Item : Net->GetInventoryItems())
			{
				if (Item.Index == ItemCellIndex[c])
				{
					DragID = Item.ItemID;
					DragCount = Item.Amount;
					DragItemIndex = Item.bIsEquipped ? INDEX_NONE : Item.Index;
					Icon = FMythRoClientTables::ItemIcon(DragID);
					break;
				}
			}
			break;
		}
	}
	// 3) atalho ja na barra (mover ou tirar)
	if (!Icon)
	{
		const int32 HotSlot = HotbarSlotAt(ScreenPos);
		if (HotSlot != INDEX_NONE && Net->GetHotkeys().IsValidIndex(HotSlot) && Net->GetHotkeys()[HotSlot].ID != 0)
		{
			const FRagnarokHotkey& H = Net->GetHotkeys()[HotSlot];
			bDragIsSkill = H.bIsSkill;
			DragID = H.ID;
			DragCount = H.Count;
			DragFromHotbar = HotSlot;
			Icon = H.bIsSkill ? FMythRoClientTables::SkillIcon(H.ID) : FMythRoClientTables::ItemIcon(H.ID);
		}
	}
	if (!Icon || DragID == 0)
	{
		return false;
	}
	if (!DragIcon)
	{
		DragIcon = WidgetTree->ConstructWidget<UImage>();
		DragIcon->SetVisibility(ESlateVisibility::HitTestInvisible);
		if (UCanvasPanelSlot* S = Root->AddChildToCanvas(DragIcon))
		{
			S->SetSize(FVector2D(24.f, 24.f) * UIScale);
			S->SetZOrder(1000);
		}
	}
	DragIcon->SetBrush(HotbarBrush(Icon, FVector2D(24.f, 24.f) * UIScale));
	DragIcon->SetRenderOpacity(0.8f);
	DragIcon->SetVisibility(ESlateVisibility::HitTestInvisible);
	bHotDragging = true;
	UpdateHotDrag(ScreenPos);
	return true;
}

void UMythRoHUDWidget::UpdateHotDrag(const FVector2D& ScreenPos)
{
	if (!DragIcon || !Root)
	{
		return;
	}
	if (UCanvasPanelSlot* S = Cast<UCanvasPanelSlot>(DragIcon->Slot))
	{
		S->SetPosition(Root->GetCachedGeometry().AbsoluteToLocal(ScreenPos) - FVector2D(12.f, 12.f) * UIScale);
	}
}

bool UMythRoHUDWidget::EndHotDrag(const FVector2D& ScreenPos)
{
	if (!bHotDragging)
	{
		return false;
	}
	bHotDragging = false;
	if (DragIcon)
	{
		DragIcon->SetVisibility(ESlateVisibility::Collapsed);
	}
	URagnarokNetworkSubsystem* Net = GetNet();
	if (!Net)
	{
		return true;
	}
	// Item da mochila largado fora de qualquer janela: joga no chao (1; com Shift a pilha toda)
	if (!bDragIsSkill && DragFromHotbar == INDEX_NONE && DragItemIndex != INDEX_NONE && HotbarSlotAt(ScreenPos) == INDEX_NONE && !IsOverAnyWindow(ScreenPos))
	{
		const bool bShift = GetOwningPlayer() && (GetOwningPlayer()->IsInputKeyDown(EKeys::LeftShift) || GetOwningPlayer()->IsInputKeyDown(EKeys::RightShift));
		Net->SendDropItem(DragItemIndex, bShift ? DragCount : 1);
		DragItemIndex = INDEX_NONE;
		return true;
	}
	const int32 HotSlot = HotbarSlotAt(ScreenPos);
	if (HotSlot != INDEX_NONE)
	{
		if (DragFromHotbar != INDEX_NONE && DragFromHotbar != HotSlot)
		{
			Net->SetHotkey(DragFromHotbar, false, 0, 0); // mover: o espaco antigo fica vazio
		}
		Net->SetHotkey(HotSlot, bDragIsSkill, DragID, bDragIsSkill ? DragCount : 0);
	}
	else if (DragFromHotbar != INDEX_NONE)
	{
		Net->SetHotkey(DragFromHotbar, false, 0, 0); // arrastado para fora da barra: apaga o atalho
	}
	RefreshHotbar();
	return true;
}

bool UMythRoHUDWidget::IsPointerOverUi() const
{
	if (!FSlateApplication::IsInitialized())
	{
		return false;
	}
	const FVector2D Pos = FSlateApplication::Get().GetCursorPos();
	return IsOverAnyWindow(Pos) || HotbarSlotAt(Pos) != INDEX_NONE || IsNpcDialogOpen();
}

bool UMythRoHUDWidget::IsOverAnyWindow(const FVector2D& ScreenPos) const
{
	for (const TWeakObjectPtr<UCanvasPanel>& Win : OpenOrder)
	{
		if (Win.IsValid() && Win->IsVisible() && IsUnder(Win.Get(), ScreenPos))
		{
			return true;
		}
	}
	return (BasicWindow && BasicWindow->IsVisible() && IsUnder(BasicWindow, ScreenPos)) || (MinimapWindow && MinimapWindow->IsVisible() && IsUnder(MinimapWindow, ScreenPos));
}

void UMythRoHUDWidget::UpdateCooldowns()
{
	UpdateBooking();
	if (GuildSkillRefreshAt > 0.0 && FPlatformTime::Seconds() >= GuildSkillRefreshAt)
	{
		GuildSkillRefreshAt = 0.0;
		if (URagnarokNetworkSubsystem* GNet = GetNet()) { GNet->SendGuildMenuRequest(3); }
	}
	const URagnarokNetworkSubsystem* Net = GetNet();
	for (int32 i = 0; i < HotbarCdBacks.Num() && i < HotbarCdTexts.Num(); ++i)
	{
		float Left = 0.f, Total = 0.f;
		const int32 SlotIndex = (HotbarFirstRow + i / 9) * 9 + i % 9;
		if (Net && Net->GetHotkeys().IsValidIndex(SlotIndex) && Net->GetHotkeys()[SlotIndex].bIsSkill)
		{
			Left = Net->GetSkillCooldown(Net->GetHotkeys()[SlotIndex].ID, &Total);
		}
		if (Left <= 0.f || Total <= 0.f)
		{
			HotbarCdBacks[i]->SetVisibility(ESlateVisibility::Collapsed);
			HotbarCdTexts[i]->SetText(FText::GetEmpty());
			continue;
		}
		// O escurecimento encolhe de cima para baixo conforme a recarga passa
		const float Frac = FMath::Clamp(Left / Total, 0.f, 1.f);
		const float X = 12.f + 30.6f * (i % 9);
		const float Y = (i / 9) * 34.f;
		if (UCanvasPanelSlot* S = Cast<UCanvasPanelSlot>(HotbarCdBacks[i]->Slot))
		{
			S->SetPosition(FVector2D(X + 2.f, Y + 5.f + 24.f * (1.f - Frac)) * UIScale);
			S->SetSize(FVector2D(24.f, 24.f * Frac) * UIScale);
		}
		HotbarCdBacks[i]->SetVisibility(ESlateVisibility::HitTestInvisible);
		HotbarCdTexts[i]->SetText(FText::FromString(Left >= 10.f ? FString::FromInt(FMath::CeilToInt(Left)) : FString::Printf(TEXT("%.1f"), Left)));
	}
}

// Barra de atalhos F1..F9 do UMythRoHUDWidget, como no cliente original: os atalhos ficam no servidor
// (ZC_SHORTCUT_KEY_LIST / CZ_SHORTCUT_KEY_CHANGE2); arrastar uma skill ou um item da janela para um espaco grava o
// atalho; arrastar um atalho para fora da barra o apaga.

#include "UI/MythRoHUDWidget.h"

#include "UI/MythRoClientTables.h"
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

int32 UMythRoHUDWidget::HotbarSlotAt(const FVector2D& ScreenPos) const
{
	for (int32 i = 0; i < HotbarSlots.Num(); ++i)
	{
		if (IsUnder(HotbarSlots[i], ScreenPos))
		{
			return i;
		}
	}
	return INDEX_NONE;
}

void UMythRoHUDWidget::RefreshHotbar()
{
	const URagnarokNetworkSubsystem* Net = GetNet();
	for (int32 i = 0; i < HotbarIcons.Num(); ++i)
	{
		UTexture2D* Icon = nullptr;
		FString Count;
		if (Net && Net->GetHotkeys().IsValidIndex(i) && Net->GetHotkeys()[i].ID != 0)
		{
			const FRagnarokHotkey& H = Net->GetHotkeys()[i];
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

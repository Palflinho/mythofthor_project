// Lista de escolha que o servidor abre depois de uma skill ou item (FOnSelectList): destinos do Portal, Lupa, onde encaixar a
// carta, Fabricar Flechas, Forjar/Farmacologia, Autoconjurar e Refinar arma. Como o cliente original: uma janela pequena com
// a lista (icone e nome), clique escolhe a linha, duplo clique ou OK confirma, Cancelar fecha. A regra fica no rAthena.

#include "UI/MythRoHUDWidget.h"

#include "Network/RagnarokNetworkSubsystem.h"
#include "Core/MythRoClientTables.h"
#include "UI/MythRoUITextures.h"

#include "Components/CanvasPanel.h"
#include "Components/CanvasPanelSlot.h"
#include "Components/Image.h"
#include "Components/TextBlock.h"

namespace
{
	const FLinearColor SelText(0.08f, 0.10f, 0.18f);
	const FLinearColor SelPick(0.78f, 0.86f, 0.98f, 1.f);
	constexpr float SelW = 260.f;
	constexpr float SelRowH = 24.f;
	constexpr int32 SelRows = 8;
	constexpr float SelListY = 22.f;
	constexpr float SelH = SelListY + SelRows * SelRowH + 32.f;

	FSlateBrush SelBrush(UTexture2D* Tex, const FVector2D& Size)
	{
		FSlateBrush B;
		B.SetResourceObject(Tex);
		B.ImageSize = Size;
		B.DrawAs = ESlateBrushDrawType::Image;
		return B;
	}
}

void UMythRoHUDWidget::BuildSelectListWindow()
{
	SelectWindow = MakeRoPanel(TEXT("Escolha"), FVector2D(SelW, SelH), FVector2D(0.f, -60.f));
	// O texto da barra de titulo e o primeiro UTextBlock do painel (AddTitleBar): trocado a cada lista
	for (int32 c = 0; c < SelectWindow->GetChildrenCount() && !SelectTitle; ++c)
	{
		SelectTitle = Cast<UTextBlock>(SelectWindow->GetChildAt(c));
	}
	for (int32 i = 0; i < SelRows; ++i)
	{
		const float Y = SelListY + i * SelRowH;
		UImage* Back = AddColor(SelectWindow, FLinearColor::Transparent, 4.f, Y, SelW - 8.f, SelRowH - 1.f);
		Back->SetVisibility(ESlateVisibility::Visible);
		SelectRowBacks.Add(Back);
		UImage* Icon = AddImage(SelectWindow, nullptr, 8.f, Y, 24.f, 24.f);
		Icon->SetVisibility(ESlateVisibility::Collapsed);
		SelectIcons.Add(Icon);
		SelectTexts.Add(AddText(SelectWindow, 38.f, Y + 5.f, 8, SelText));
	}
	SelectOk = AddImage(SelectWindow, FMythRoUITextures::Get(TEXT("btn_ok.bmp")), SelW - 98.f, SelH - 26.f, 42.f, 20.f);
	SelectCancel = AddImage(SelectWindow, FMythRoUITextures::Get(TEXT("btn_cancel.bmp")), SelW - 50.f, SelH - 26.f, 42.f, 20.f);
	SelectOk->SetVisibility(ESlateVisibility::Visible);
	SelectCancel->SetVisibility(ESlateVisibility::Visible);
}

void UMythRoHUDWidget::OpenSelectList(uint8 ListType, const TArray<int32>& Values, const TArray<FString>& Names)
{
	if (!SelectWindow)
	{
		return;
	}
	SelectType = ListType;
	SelectValues = Values;
	SelectNames = Names;
	SelectScroll = 0;
	SelectRow = 0;
	// Titulo na barra da janela, como o cliente (a carta leva o nome dela: msg 522 + "(nome)")
	FString Title;
	switch (static_cast<EMythSelectList>(ListType))
	{
	case EMythSelectList::Warp: Title = TEXT("Portal"); break;
	case EMythSelectList::Identify: Title = TEXT("Identificar item"); break;
	case EMythSelectList::Card: Title = FMythRoClientTables::Message(522); if (Title.IsEmpty()) { Title = TEXT("Encaixar carta"); } break;
	case EMythSelectList::Arrow: Title = TEXT("Fabricar flechas"); break;
	case EMythSelectList::Produce: Title = TEXT("Fabricar"); break;
	case EMythSelectList::Autospell: Title = TEXT("Autoconjurar"); break;
	case EMythSelectList::Refine: Title = TEXT("Refinar arma"); break;
	case EMythSelectList::PetEgg: Title = FMythRoClientTables::Message(599).TrimEnd(); break;
	case EMythSelectList::Repair: Title = FMythRoClientTables::Message(812).TrimEnd(); break;
	}
	if (SelectTitle)
	{
		SelectTitle->SetText(FText::FromString(Title));
	}
	SelectWindow->SetVisibility(ESlateVisibility::SelfHitTestInvisible);
	OpenOrder.Remove(SelectWindow);
	OpenOrder.Add(SelectWindow);
	if (UCanvasPanelSlot* S = Cast<UCanvasPanelSlot>(SelectWindow->Slot))
	{
		S->SetZOrder(++TopZOrder);
	}
	RefreshSelectList();
}

void UMythRoHUDWidget::RefreshSelectList()
{
	const URagnarokNetworkSubsystem* Net = GetNet();
	SelectScroll = FMath::Clamp(SelectScroll, 0, FMath::Max(0, SelectValues.Num() - SelRows));
	for (int32 i = 0; i < SelRows; ++i)
	{
		const int32 Index = SelectScroll + i;
		const bool bHas = SelectValues.IsValidIndex(Index);
		FString Label;
		UTexture2D* Icon = nullptr;
		if (bHas)
		{
			const int32 Value = SelectValues[Index];
			switch (static_cast<EMythSelectList>(SelectType))
			{
			case EMythSelectList::Warp:
				Label = SelectNames.IsValidIndex(Index) ? SelectNames[Index] : FString();
				break;
			case EMythSelectList::Identify:
			case EMythSelectList::Card:
			case EMythSelectList::Refine:
			case EMythSelectList::PetEgg:
				// Valor = indice na mochila
				if (Net)
				{
					for (const FRagnarokInventoryItem& Item : Net->GetInventoryItems())
					{
						if (Item.Index == Value)
						{
							const FMythRoItemInfo* Info = FMythRoClientTables::ItemInfo(Item.ItemID);
							Label = !Item.bIdentified && Info && !Info->UnidentifiedName.IsEmpty() ? Info->UnidentifiedName : FMythRoClientTables::ItemName(Item.ItemID);
							if (Item.Refine > 0)
							{
								Label = FString::Printf(TEXT("+%d %s"), Item.Refine, *Label);
							}
							Icon = FMythRoClientTables::ItemIcon(Item.ItemID);
							break;
						}
					}
				}
				break;
			case EMythSelectList::Arrow:
			case EMythSelectList::Produce:
			case EMythSelectList::Repair:
				Label = FMythRoClientTables::ItemName(Value);
				Icon = FMythRoClientTables::ItemIcon(Value);
				break;
			case EMythSelectList::Autospell:
				Label = FMythRoClientTables::SkillName(Value);
				Icon = FMythRoClientTables::SkillIcon(Value);
				break;
			}
			if (Label.IsEmpty())
			{
				Label = FString::Printf(TEXT("#%d"), Value);
			}
		}
		SelectTexts[i]->SetText(FText::FromString(Label));
		if (Icon)
		{
			SelectIcons[i]->SetBrush(SelBrush(Icon, FVector2D(24.f, 24.f) * UIScale));
		}
		SelectIcons[i]->SetVisibility(Icon ? ESlateVisibility::HitTestInvisible : ESlateVisibility::Collapsed);
		SelectRowBacks[i]->SetVisibility(bHas ? ESlateVisibility::Visible : ESlateVisibility::Collapsed);
		SelectRowBacks[i]->SetColorAndOpacity(Index == SelectRow ? SelPick : FLinearColor::Transparent);
	}
}

void UMythRoHUDWidget::SelectListChoose(int32 Row)
{
	URagnarokNetworkSubsystem* Net = GetNet();
	if (!SelectWindow || !SelectWindow->IsVisible() || !Net)
	{
		return;
	}
	const EMythSelectList Type = static_cast<EMythSelectList>(SelectType);
	if (SelectValues.IsValidIndex(Row))
	{
		Net->SendSelectListChoice(Type, SelectValues[Row], SelectNames.IsValidIndex(Row) ? SelectNames[Row] : FString(), false);
	}
	else
	{
		Net->SendSelectListChoice(Type, 0, FString(), true);
	}
	SelectWindow->SetVisibility(ESlateVisibility::Collapsed);
}

bool UMythRoHUDWidget::HandleSelectListClick(const FVector2D& ScreenPos, bool bDouble)
{
	if (!SelectWindow || !SelectWindow->IsVisible())
	{
		return false;
	}
	if (IsUnder(SelectOk, ScreenPos))
	{
		SelectListChoose(SelectRow);
		return true;
	}
	if (IsUnder(SelectCancel, ScreenPos))
	{
		SelectListChoose(-1);
		return true;
	}
	for (int32 i = 0; i < SelRows; ++i)
	{
		const int32 Index = SelectScroll + i;
		if (SelectValues.IsValidIndex(Index) && IsUnder(SelectRowBacks[i], ScreenPos))
		{
			SelectRow = Index;
			if (bDouble)
			{
				SelectListChoose(Index);
			}
			else
			{
				RefreshSelectList();
			}
			return true;
		}
	}
	return false;
}

bool UMythRoHUDWidget::HandleSelectListWheel(const FVector2D& ScreenPos, float Delta)
{
	if (!SelectWindow || !SelectWindow->IsVisible() || !IsUnder(SelectWindow, ScreenPos))
	{
		return false;
	}
	SelectScroll += Delta > 0.f ? -1 : 1;
	RefreshSelectList();
	return true;
}

// Janela de descricao do item (botao direito no inventario ou no equipamento), como no cliente original:
// ilustracao a esquerda, nome com slots e refino, e o texto do itemInfo.lua com as cores ^RRGGBB.

#include "UI/MythRoHUDWidget.h"

#include "UI/MythRoClientTables.h"
#include "UI/MythRoUITextures.h"
#include "Network/RagnarokNetworkSubsystem.h"

#include "Blueprint/WidgetTree.h"
#include "Components/CanvasPanel.h"
#include "Components/CanvasPanelSlot.h"
#include "Components/HorizontalBox.h"
#include "Components/Image.h"
#include "Components/ScrollBox.h"
#include "Components/TextBlock.h"

namespace
{
	const FLinearColor InfoText(0.12f, 0.12f, 0.12f);
	constexpr float InfoW = 300.f;
	constexpr float InfoH = 210.f;
	constexpr float IllustW = 75.f;  // collection/*.bmp: 75x100
	constexpr float IllustH = 100.f;

	/** "^0000ffTexto^000000 resto": trechos com a cor do codigo (^000000 = cor padrao do texto). */
	TArray<TPair<FString, FLinearColor>> SplitColors(const FString& Line)
	{
		TArray<TPair<FString, FLinearColor>> Out;
		FLinearColor Color = InfoText;
		FString Current;
		for (int32 i = 0; i < Line.Len(); ++i)
		{
			if (Line[i] == TEXT('^') && i + 6 < Line.Len() + 0 && i + 7 <= Line.Len())
			{
				const FString Hex = Line.Mid(i + 1, 6);
				bool bHex = true;
				for (TCHAR C : Hex) { bHex &= FChar::IsHexDigit(C); }
				if (bHex)
				{
					if (!Current.IsEmpty()) { Out.Add({ Current, Color }); Current.Empty(); }
					Color = Hex == TEXT("000000") ? InfoText : FLinearColor(FColor::FromHex(Hex));
					i += 6;
					continue;
				}
			}
			Current.AppendChar(Line[i]);
		}
		if (!Current.IsEmpty()) { Out.Add({ Current, Color }); }
		return Out;
	}
}

void UMythRoHUDWidget::BuildItemInfoWindow()
{
	ItemInfoWindow = MakeRoPanel(TEXT("Informações do item"), FVector2D(InfoW, InfoH), FVector2D(-120.f, -60.f));
	ItemInfoImage = AddImage(ItemInfoWindow, nullptr, 8.f, 24.f, IllustW, IllustH);
	ItemInfoName = AddText(ItemInfoWindow, IllustW + 16.f, 22.f, 9, InfoText);
	ItemInfoBody = WidgetTree->ConstructWidget<UScrollBox>();
	if (UCanvasPanelSlot* S = ItemInfoWindow->AddChildToCanvas(ItemInfoBody))
	{
		S->SetPosition(FVector2D(IllustW + 16.f, 40.f) * UIScale);
		S->SetSize(FVector2D(InfoW - IllustW - 24.f, InfoH - 48.f) * UIScale);
	}
	ItemInfoWindow->SetVisibility(ESlateVisibility::Collapsed);
}

void UMythRoHUDWidget::ShowItemInfo(const FRagnarokInventoryItem& Item)
{
	if (!ItemInfoWindow)
	{
		return;
	}
	const FMythRoItemInfo* Info = FMythRoClientTables::ItemInfo(Item.ItemID);
	FString Name = Info ? (Item.bIdentified ? Info->Name : Info->UnidentifiedName) : FMythRoClientTables::ItemName(Item.ItemID);
	if (Name.IsEmpty())
	{
		Name = FString::Printf(TEXT("Item %d"), Item.ItemID);
	}
	if (Item.Refine > 0)
	{
		Name = FString::Printf(TEXT("+%d %s"), Item.Refine, *Name);
	}
	if (Info && Info->Slots > 0 && Item.bIdentified)
	{
		Name += FString::Printf(TEXT(" [%d]"), Info->Slots);
	}
	UTexture2D* Illust = FMythRoClientTables::ItemIllustration(Item.ItemID);
	if (!Illust)
	{
		Illust = FMythRoClientTables::ItemIcon(Item.ItemID); // sem ilustracao: o icone do item
	}
	const TArray<FString> Empty;
	ShowInfoLines(Name, Illust, Info ? (Item.bIdentified ? Info->Desc : Info->UnidentifiedDesc) : Empty);
}

void UMythRoHUDWidget::ShowSkillInfo(int32 SkillID)
{
	// Botao direito numa habilidade: a descricao do skilldesctable (tipo, alvo, resultado) na mesma janela dos itens
	const TArray<FString>* Desc = FMythRoClientTables::SkillDescription(SkillID);
	const TArray<FString> Empty;
	FString Name = FMythRoClientTables::SkillName(SkillID);
	if (Name.IsEmpty())
	{
		Name = FString::Printf(TEXT("Habilidade %d"), SkillID);
	}
	ShowInfoLines(Name, FMythRoClientTables::SkillIcon(SkillID), Desc ? *Desc : Empty);
}

void UMythRoHUDWidget::ShowInfoLines(const FString& Name, UTexture2D* Illust, const TArray<FString>& Lines)
{
	if (!ItemInfoWindow)
	{
		return;
	}
	ItemInfoName->SetText(FText::FromString(Name));
	ItemInfoImage->SetBrushFromTexture(Illust);
	ItemInfoImage->SetVisibility(Illust ? ESlateVisibility::HitTestInvisible : ESlateVisibility::Collapsed);

	ItemInfoBody->ClearChildren();
	const int32 FontSize = FMath::RoundToInt(8 * UIScale);
	const float WrapWidth = (InfoW - IllustW - 32.f) * UIScale;
	for (const FString& Line : Lines)
	{
		const TArray<TPair<FString, FLinearColor>> Parts = SplitColors(Line);
		auto MakeText = [this, FontSize](const FString& Text, const FLinearColor& Color)
		{
			UTextBlock* T = WidgetTree->ConstructWidget<UTextBlock>();
			T->SetFont(FMythRoUITextures::RoFont(FontSize));
			T->SetColorAndOpacity(FSlateColor(Color));
			T->SetText(FText::FromString(Text));
			return T;
		};
		if (Parts.Num() <= 1)
		{
			// Linha de uma cor so (descricao): quebra no fim da coluna
			UTextBlock* T = MakeText(Parts.Num() ? Parts[0].Key : FString(TEXT(" ")), Parts.Num() ? Parts[0].Value : InfoText);
			T->SetAutoWrapText(true);
			T->SetWrapTextAt(WrapWidth);
			ItemInfoBody->AddChild(T);
		}
		else
		{
			// Linha com cores (ex.: "Peso: ^777777 10^000000"): trechos lado a lado
			UHorizontalBox* Row = WidgetTree->ConstructWidget<UHorizontalBox>();
			for (const TPair<FString, FLinearColor>& P : Parts)
			{
				Row->AddChild(MakeText(P.Key, P.Value));
			}
			ItemInfoBody->AddChild(Row);
		}
	}
	ItemInfoBody->ScrollToStart();
	ItemInfoWindow->SetVisibility(ESlateVisibility::SelfHitTestInvisible);
	OpenOrder.Remove(ItemInfoWindow);
	OpenOrder.Add(ItemInfoWindow);
}

bool UMythRoHUDWidget::HandleItemInfoClick(const FVector2D& ScreenPos)
{
	URagnarokNetworkSubsystem* Net = GetNet();
	if (!Net)
	{
		return false;
	}
	auto ShowIndex = [this, Net](int32 Index)
	{
		for (const FRagnarokInventoryItem& Item : Net->GetInventoryItems())
		{
			if (Item.Index == Index)
			{
				ShowItemInfo(Item);
				return true;
			}
		}
		return false;
	};
	if (ItemsWindow && ItemsWindow->IsVisible())
	{
		for (int32 i = 0; i < ItemCells.Num() && i < ItemCellIndex.Num(); ++i)
		{
			if (ItemCellIndex[i] >= 0 && IsUnder(ItemCells[i], ScreenPos))
			{
				return ShowIndex(ItemCellIndex[i]);
			}
		}
	}
	if (SkillsWindow && SkillsWindow->IsVisible())
	{
		for (int32 r = 0; r < SkillIcons.Num() && r < SkillRowID.Num(); ++r)
		{
			if (SkillRowID[r] != 0 && (IsUnder(SkillIcons[r], ScreenPos) || IsUnder(SkillNames[r], ScreenPos)))
			{
				ShowSkillInfo(SkillRowID[r]);
				return true;
			}
		}
	}
	if (EquipWindow && EquipWindow->IsVisible())
	{
		if (EquipAmmoIndex >= 0 && IsUnder(EquipAmmoIcon, ScreenPos))
		{
			return ShowIndex(EquipAmmoIndex);
		}
		for (int32 i = 0; i < EquipCellIndex.Num(); ++i)
		{
			if (EquipCellIndex[i] >= 0 && EquipIcons.IsValidIndex(2 * i) && IsUnder(EquipIcons[2 * i], ScreenPos))
			{
				return ShowIndex(EquipCellIndex[i]);
			}
		}
	}
	return false;
}

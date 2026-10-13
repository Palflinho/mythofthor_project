// Loja de Testes (janela de Game Master): lista todos os equipamentos, cartas, consumiveis e outros itens do jogo
// (Config/item_catalog.json, tools/gen_item_catalog.py) com busca por nome ou ID. Clicar numa linha pede o item ao servidor
// com o comando "@item <id> <qtd>" (so funciona em conta GM; o rAthena decide). Shift + clique leva 10.

#include "UI/MythRoHUDWidget.h"

#include "Network/RagnarokNetworkSubsystem.h"
#include "Core/MythRoClientTables.h"
#include "UI/MythRoUITextures.h"

#include "Blueprint/WidgetTree.h"
#include "Components/CanvasPanel.h"
#include "Components/CanvasPanelSlot.h"
#include "Components/EditableTextBox.h"
#include "Components/Image.h"
#include "Components/TextBlock.h"
#include "Dom/JsonObject.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "Serialization/JsonReader.h"
#include "Serialization/JsonSerializer.h"

namespace
{
	constexpr float TsW = 300.f;
	constexpr float TsRowH = 22.f;
	constexpr int32 TsRows = 13;
	constexpr float TsTabY = 20.f;
	constexpr float TsSearchY = 42.f;
	constexpr float TsListY = 66.f;
	constexpr float TsH = TsListY + TsRows * TsRowH + 22.f;
	const FLinearColor TsText(0.08f, 0.10f, 0.18f);
	const FLinearColor TsHover(0.78f, 0.86f, 0.98f, 1.f);

	struct FCatalogItem
	{
		int32 Id = 0;
		FString Name;
		FString NameEn;
		uint8 Kind = 0; // 0 equip, 1 card, 2 use, 3 etc
	};

	const TArray<FCatalogItem>& Catalog()
	{
		static TArray<FCatalogItem> Items;
		static bool bLoaded = false;
		if (bLoaded)
		{
			return Items;
		}
		bLoaded = true;
		FString Text;
		if (!FFileHelper::LoadFileToString(Text, *FPaths::Combine(FPaths::ProjectConfigDir(), TEXT("item_catalog.json"))))
		{
			return Items;
		}
		TArray<TSharedPtr<FJsonValue>> Arr;
		TSharedRef<TJsonReader<>> Reader = TJsonReaderFactory<>::Create(Text);
		if (!FJsonSerializer::Deserialize(Reader, Arr))
		{
			return Items;
		}
		Items.Reserve(Arr.Num());
		for (const TSharedPtr<FJsonValue>& V : Arr)
		{
			const TSharedPtr<FJsonObject>* O = nullptr;
			if (!V->TryGetObject(O) || !O)
			{
				continue;
			}
			FCatalogItem It;
			It.Id = (*O)->GetIntegerField(TEXT("i"));
			It.NameEn = (*O)->GetStringField(TEXT("n"));
			// Nome em ingles (do item_db) e, quando o cliente tem um nome proprio, o nome em portugues entre parenteses
			const FString Pt = (*O)->GetStringField(TEXT("p"));
			It.Name = (Pt.IsEmpty() || Pt == It.NameEn || Pt == TEXT("Carta")) ? It.NameEn : FString::Printf(TEXT("%s (%s)"), *It.NameEn, *Pt);
			const FString Kind = (*O)->GetStringField(TEXT("t"));
			It.Kind = Kind == TEXT("equip") ? 0 : Kind == TEXT("card") ? 1 : Kind == TEXT("use") ? 2 : 3;
			Items.Add(MoveTemp(It));
		}
		return Items;
	}
}

void UMythRoHUDWidget::BuildTestShopWindow()
{
	TestShopWindow = MakeRoPanel(TEXT("Loja de Testes (GM)"), FVector2D(TsW, TsH), FVector2D(-200.f, -20.f));
	static const TCHAR* Tabs[] = { TEXT("Equipamentos"), TEXT("Cartas"), TEXT("Consumíveis"), TEXT("Outros") };
	for (int32 t = 0; t < 4; ++t)
	{
		const float W = TsW / 4.f;
		UImage* Back = AddColor(TestShopWindow, FLinearColor(0.7f, 0.76f, 0.9f, 1.f), 2.f + t * W, TsTabY, W - 2.f, 20.f);
		Back->SetVisibility(ESlateVisibility::Visible);
		TestShopTabBacks.Add(Back);
		UTextBlock* Label = AddText(TestShopWindow, 2.f + t * W, TsTabY + 4.f, 7, TsText);
		Label->SetText(FText::FromString(Tabs[t]));
		Label->SetJustification(ETextJustify::Center);
		if (UCanvasPanelSlot* S = Cast<UCanvasPanelSlot>(Label->Slot)) { S->SetAutoSize(false); S->SetSize(FVector2D(W - 2.f, 14.f) * UIScale); }
	}
	TestShopSearch = WidgetTree->ConstructWidget<UEditableTextBox>();
	TestShopSearch->SetHintText(FText::FromString(TEXT("Buscar por nome ou ID")));
	TestShopSearch->OnTextChanged.AddDynamic(this, &UMythRoHUDWidget::OnTestShopSearch);
	if (UCanvasPanelSlot* S = TestShopWindow->AddChildToCanvas(TestShopSearch))
	{
		S->SetPosition(FVector2D(4.f, TsSearchY) * UIScale);
		S->SetSize(FVector2D(TsW - 8.f, 20.f) * UIScale);
	}
	for (int32 i = 0; i < TsRows; ++i)
	{
		const float Y = TsListY + i * TsRowH;
		UImage* Back = AddColor(TestShopWindow, FLinearColor::Transparent, 4.f, Y, TsW - 8.f, TsRowH - 1.f);
		Back->SetVisibility(ESlateVisibility::Visible);
		TestShopRowBacks.Add(Back);
		UImage* Icon = AddImage(TestShopWindow, nullptr, 8.f, Y - 1.f, 24.f, 24.f);
		Icon->SetVisibility(ESlateVisibility::Collapsed);
		TestShopIcons.Add(Icon);
		TestShopTexts.Add(AddText(TestShopWindow, 38.f, Y + 5.f, 7, TsText));
	}
	TestShopHint = AddText(TestShopWindow, 6.f, TsH - 17.f, 7, FLinearColor(0.12f, 0.27f, 0.55f));
	TestShopRefilter();
}

void UMythRoHUDWidget::TestShopRefilter()
{
	const TArray<FCatalogItem>& Items = Catalog();
	const FString Query = TestShopSearch ? TestShopSearch->GetText().ToString().TrimStartAndEnd() : FString();
	const bool bNumeric = !Query.IsEmpty() && Query.IsNumeric();
	TestShopFiltered.Reset();
	for (int32 i = 0; i < Items.Num(); ++i)
	{
		const FCatalogItem& It = Items[i];
		if (It.Kind != TestShopTab)
		{
			continue;
		}
		if (!Query.IsEmpty())
		{
			const bool bMatch = bNumeric ? FString::FromInt(It.Id).StartsWith(Query) : (It.Name.Contains(Query) || It.NameEn.Contains(Query));
			if (!bMatch)
			{
				continue;
			}
		}
		TestShopFiltered.Add(i);
	}
	TestShopScroll = 0;
	RefreshTestShop();
}

void UMythRoHUDWidget::OnTestShopSearch(const FText& Text)
{
	TestShopRefilter();
}

void UMythRoHUDWidget::RefreshTestShop()
{
	if (!TestShopWindow)
	{
		return;
	}
	const TArray<FCatalogItem>& Items = Catalog();
	TestShopScroll = FMath::Clamp(TestShopScroll, 0, FMath::Max(0, TestShopFiltered.Num() - TsRows));
	for (int32 t = 0; t < TestShopTabBacks.Num(); ++t)
	{
		TestShopTabBacks[t]->SetColorAndOpacity(t == TestShopTab ? FLinearColor(0.95f, 0.97f, 1.f) : FLinearColor(0.7f, 0.76f, 0.9f));
	}
	for (int32 i = 0; i < TsRows; ++i)
	{
		const int32 Index = TestShopScroll + i;
		if (!TestShopFiltered.IsValidIndex(Index))
		{
			TestShopTexts[i]->SetText(FText::GetEmpty());
			TestShopIcons[i]->SetVisibility(ESlateVisibility::Collapsed);
			continue;
		}
		const FCatalogItem& It = Items[TestShopFiltered[Index]];
		TestShopTexts[i]->SetText(FText::FromString(FString::Printf(TEXT("%s  #%d"), *It.Name.Left(40), It.Id)));
		UTexture2D* Icon = FMythRoClientTables::ItemIcon(It.Id);
		if (Icon)
		{
			FSlateBrush B;
			B.SetResourceObject(Icon);
			B.ImageSize = FVector2D(24.f, 24.f) * UIScale;
			TestShopIcons[i]->SetBrush(B);
		}
		TestShopIcons[i]->SetVisibility(Icon ? ESlateVisibility::HitTestInvisible : ESlateVisibility::Collapsed);
	}
	if (TestShopHint)
	{
		TestShopHint->SetText(FText::FromString(FString::Printf(TEXT("%d itens  |  clique pega 1, Shift pega 10  |  roda do mouse rola"), TestShopFiltered.Num())));
	}
}

bool UMythRoHUDWidget::HandleTestShopClick(const FVector2D& ScreenPos, bool bShift)
{
	if (!TestShopWindow || !TestShopWindow->IsVisible())
	{
		return false;
	}
	for (int32 t = 0; t < TestShopTabBacks.Num(); ++t)
	{
		if (IsUnder(TestShopTabBacks[t], ScreenPos))
		{
			TestShopTab = t;
			TestShopRefilter();
			return true;
		}
	}
	const TArray<FCatalogItem>& Items = Catalog();
	for (int32 i = 0; i < TestShopRowBacks.Num(); ++i)
	{
		const int32 Index = TestShopScroll + i;
		if (IsUnder(TestShopRowBacks[i], ScreenPos) && TestShopFiltered.IsValidIndex(Index))
		{
			const FCatalogItem& It = Items[TestShopFiltered[Index]];
			if (URagnarokNetworkSubsystem* Net = GetNet())
			{
				Net->SendGlobalMessage(FString::Printf(TEXT("@item %d %d"), It.Id, bShift ? 10 : 1));
			}
			return true;
		}
	}
	return false;
}

bool UMythRoHUDWidget::HandleTestShopWheel(const FVector2D& ScreenPos, float Delta)
{
	if (!TestShopWindow || !TestShopWindow->IsVisible() || !IsUnder(TestShopWindow, ScreenPos))
	{
		return false;
	}
	TestShopScroll += (Delta > 0.f ? -3 : 3);
	RefreshTestShop();
	return true;
}

void UMythRoHUDWidget::TestTestShop(int32 Tab, const FString& Query)
{
	if (!TestShopWindow)
	{
		return;
	}
	if (!TestShopWindow->IsVisible())
	{
		ToggleWindow(EMythRoWindow::TestShop);
	}
	TestShopTab = FMath::Clamp(Tab, 0, 3);
	if (TestShopSearch)
	{
		TestShopSearch->SetText(FText::FromString(Query));
	}
	TestShopRefilter();
}

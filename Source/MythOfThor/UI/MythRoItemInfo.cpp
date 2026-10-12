// Janela de descricao do item (botao direito no inventario ou no equipamento), como no cliente original:
// ilustracao a esquerda, nome com slots e refino, e o texto do itemInfo.lua com as cores ^RRGGBB.

#include "UI/MythRoHUDWidget.h"

#include "Core/MythRoClientTables.h"
#include "UI/MythRoUITextures.h"
#include "Network/RagnarokNetworkSubsystem.h"

#include "Blueprint/WidgetTree.h"
#include "Components/CanvasPanel.h"
#include "Components/CanvasPanelSlot.h"
#include "Brushes/SlateRoundedBoxBrush.h"
#include "Components/Border.h"
#include "Components/HorizontalBox.h"
#include "Components/Image.h"
#include "Components/ScrollBox.h"
#include "Components/TextBlock.h"
#include "Components/VerticalBox.h"
#include "Framework/Application/SlateApplication.h"
#include "UI/MythRoSkillTree.h"

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
	// Ilustracao (75x100) para itens; icones pequenos (habilidades 24x24, itens sem ilustracao) ficam no tamanho natural
	const bool bIcon = Illust && Illust->GetSizeX() <= 48;
	const FVector2D ImgSize = bIcon ? FVector2D(Illust->GetSizeX(), Illust->GetSizeY()) : FVector2D(IllustW, IllustH);
	{ FSlateBrush B; B.SetResourceObject(Illust); B.ImageSize = ImgSize * UIScale; B.DrawAs = ESlateBrushDrawType::Image; ItemInfoImage->SetBrush(B); }
	if (UCanvasPanelSlot* IS = Cast<UCanvasPanelSlot>(ItemInfoImage->Slot))
	{
		IS->SetSize(ImgSize * UIScale);
	}
	ItemInfoImage->SetVisibility(Illust ? ESlateVisibility::HitTestInvisible : ESlateVisibility::Collapsed);
	// Com icone pequeno o texto sobe para perto dele (sem a coluna vazia de 75 px da ilustracao)
	const float TextX = (Illust ? (bIcon ? ImgSize.X : IllustW) : 0.f) + 16.f;
	if (UCanvasPanelSlot* NS = Cast<UCanvasPanelSlot>(ItemInfoName->Slot)) { NS->SetPosition(FVector2D(TextX, 22.f) * UIScale); }
	if (UCanvasPanelSlot* BS = Cast<UCanvasPanelSlot>(ItemInfoBody->Slot))
	{
		BS->SetPosition(FVector2D(TextX, 40.f) * UIScale);
		BS->SetSize(FVector2D(InfoW - TextX - 8.f, InfoH - 48.f) * UIScale);
	}

	ItemInfoBody->ClearChildren();
	const int32 FontSize = FMath::RoundToInt(8 * UIScale);
	const float WrapWidth = (InfoW - TextX - 16.f) * UIScale;
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

void UMythRoHUDWidget::UpdateSkillHover()
{
	// Mouse sobre uma skill da arvore: dica curta com nome, nivel e, se ainda nao foi aprendida, os
	// pre-requisitos (diretos e os deles) ficam com borda na arvore - laranja os que faltam, verde os ja cumpridos
	int32 Hovered = 0;
	if (SkillsWindow && SkillsWindow->IsVisible() && FSlateApplication::IsInitialized())
	{
		const FVector2D Mouse = FSlateApplication::Get().GetCursorPos();
		for (int32 i = 0; i < SkillRowID.Num() && i < SkillCellFrames.Num(); ++i)
		{
			if (SkillRowID[i] != 0 && (IsUnder(SkillCellFrames[i], Mouse) || IsUnder(SkillIcons[i], Mouse)))
			{
				Hovered = SkillRowID[i];
				break;
			}
		}
	}
	if (!SkillTip)
	{
		SkillTip = WidgetTree->ConstructWidget<UBorder>();
		SkillTip->SetBrush(FSlateRoundedBoxBrush(FLinearColor(1.f, 1.f, 1.f, 0.97f), 3.f, FLinearColor(0.55f, 0.6f, 0.75f, 1.f), 1.f));
		SkillTip->SetPadding(FMargin(8.f, 6.f));
		SkillTip->SetVisibility(ESlateVisibility::Collapsed);
		SkillTipBox = WidgetTree->ConstructWidget<UVerticalBox>();
		SkillTip->SetContent(SkillTipBox);
		if (UCanvasPanelSlot* S = Root->AddChildToCanvas(SkillTip))
		{
			S->SetAutoSize(true);
			S->SetZOrder(100000);
		}
	}
	if (Hovered == 0)
	{
		SkillTip->SetVisibility(ESlateVisibility::Collapsed);
		if (SkillHoverID != 0)
		{
			for (UImage* M : SkillNeedMarks) { M->SetVisibility(ESlateVisibility::Collapsed); }
			SkillHoverID = 0;
		}
		return;
	}
	// A dica acompanha o mouse (sem passar da borda da tela)
	if (UCanvasPanelSlot* S = Cast<UCanvasPanelSlot>(SkillTip->Slot))
	{
		const FGeometry& RootGeo = Root->GetCachedGeometry();
		FVector2D Pos = RootGeo.AbsoluteToLocal(FSlateApplication::Get().GetCursorPos()) + FVector2D(18.f, 18.f);
		const FVector2D Size = SkillTip->GetDesiredSize();
		const FVector2D View = RootGeo.GetLocalSize();
		Pos.X = FMath::Min(Pos.X, View.X - Size.X - 4.f);
		if (Pos.Y + Size.Y > View.Y - 4.f) { Pos.Y -= Size.Y + 30.f; }
		S->SetPosition(Pos);
	}
	if (Hovered == SkillHoverID)
	{
		return;
	}
	SkillHoverID = Hovered;

	const URagnarokNetworkSubsystem* Net = GetNet();
	TMap<int32, int32> Have;
	if (Net)
	{
		for (const FRagnarokSkillInfo& S : Net->GetSkillList()) { Have.Add(S.SkillID, S.Level); }
	}
	const FMythTreeSkill* Tree = FMythRoSkillTree::Skill(Hovered);
	const int32 Level = Have.FindRef(Hovered);
	FString Name = Tree ? Tree->Name : FMythRoClientTables::SkillName(Hovered);
	if (Name.IsEmpty()) { Name = FMythRoClientTables::SkillName(Hovered); }

	// Pre-requisitos: o mais alto pedido de cada skill na cadeia inteira
	TMap<int32, int32> Needs;
	if (Level == 0 && Tree)
	{
		TArray<FIntPoint> Stack = Tree->Need;
		while (Stack.Num() > 0)
		{
			const FIntPoint N = Stack.Pop();
			int32& Want = Needs.FindOrAdd(N.X);
			if (N.Y <= Want) { continue; }
			Want = N.Y;
			if (const FMythTreeSkill* Sub = FMythRoSkillTree::Skill(N.X)) { Stack.Append(Sub->Need); }
		}
	}
	for (int32 i = 0; i < SkillNeedMarks.Num(); ++i) { SkillNeedMarks[i]->SetVisibility(ESlateVisibility::Collapsed); }
	for (const TPair<int32, int32>& N : Needs)
	{
		if (const int32* Cell = SkillCellOf.Find(N.Key); Cell && SkillNeedMarks.IsValidIndex(*Cell))
		{
			const bool bMet = Have.FindRef(N.Key) >= N.Value;
			SkillNeedMarks[*Cell]->SetColorAndOpacity(bMet ? FLinearColor(0.3f, 0.75f, 0.35f, 1.f) : FLinearColor(1.f, 0.55f, 0.1f, 1.f));
			SkillNeedMarks[*Cell]->SetVisibility(ESlateVisibility::HitTestInvisible);
		}
	}

	// Conteudo da dica
	SkillTipBox->ClearChildren();
	const int32 FontSize = FMath::RoundToInt(8 * UIScale);
	auto AddLine = [this, FontSize](const FString& Line, bool bBold = false)
	{
		const TArray<TPair<FString, FLinearColor>> Parts = SplitColors(Line);
		UHorizontalBox* Row = WidgetTree->ConstructWidget<UHorizontalBox>();
		for (const TPair<FString, FLinearColor>& P : Parts)
		{
			UTextBlock* T = WidgetTree->ConstructWidget<UTextBlock>();
			T->SetFont(FMythRoUITextures::RoFont(FontSize, bBold));
			T->SetColorAndOpacity(FSlateColor(P.Value));
			T->SetText(FText::FromString(P.Key));
			T->SetWrapTextAt(Parts.Num() == 1 ? 320.f * UIScale : 0.f); // largura fixa (o AutoWrap dentro da caixa horizontal quebrava cedo)
			Row->AddChild(T);
		}
		SkillTipBox->AddChild(Row);
	};
	AddLine(FString::Printf(TEXT("%s  ^777777Nv %d/%d^000000"), *Name, Level, Tree ? Tree->MaxLevel : FMath::Max(Level, 1)), true);
	// So os pre-requisitos; a descricao fica no botao direito (ShowSkillInfo)
	if (Needs.Num() > 0)
	{
		AddLine(TEXT("Requer:"), true);
		for (const TPair<int32, int32>& N : Needs)
		{
			const FMythTreeSkill* NeedTree = FMythRoSkillTree::Skill(N.Key);
			const FString NeedName = NeedTree ? NeedTree->Name : FMythRoClientTables::SkillName(N.Key);
			const bool bMet = Have.FindRef(N.Key) >= N.Value;
			AddLine(FString::Printf(TEXT("%s%s Nv %d"), bMet ? TEXT("^2a8a2a") : TEXT("^d05000"), *NeedName, N.Value));
		}
	}
	SkillTip->SetVisibility(ESlateVisibility::HitTestInvisible);
}

void UMythRoHUDWidget::SkillTestHover(int32 SkillId)
{
	if (const int32* Cell = SkillCellOf.Find(SkillId); Cell && SkillIcons.IsValidIndex(*Cell) && FSlateApplication::IsInitialized())
	{
		const FGeometry& G = SkillCellFrames[*Cell]->GetCachedGeometry();
		FSlateApplication::Get().SetCursorPos(G.LocalToAbsolute(G.GetLocalSize() * 0.5f));
	}
}

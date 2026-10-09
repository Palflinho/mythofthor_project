// Janelas classicas do cliente original (Atributos, Itens, Equipamento, Habilidades) do UMythRoHUDWidget.
// Regra do projeto: tudo o que e mostrado aqui vem do rAthena (URagnarokNetworkSubsystem); o cliente so desenha
// e envia pedidos (CZ_STATUS_CHANGE, CZ_USE_ITEM, CZ_REQ_WEAR_EQUIP, CZ_REQ_TAKEOFF_EQUIP).

#include "UI/MythRoHUDWidget.h"

#include "UI/MythRoClientTables.h"
#include "UI/MythRoUITextures.h"
#include "Network/RagnarokNetworkSubsystem.h"

#include "Blueprint/WidgetTree.h"
#include "Components/CanvasPanel.h"
#include "Components/CanvasPanelSlot.h"
#include "Components/Image.h"
#include "Components/TextBlock.h"
#include "Engine/GameInstance.h"

namespace
{
	const FLinearColor RoWinText(0.08f, 0.10f, 0.18f);
	const FLinearColor RoWinLabel(0.12f, 0.27f, 0.55f);
	const FLinearColor RoWinBody(0.965f, 0.965f, 0.975f, 0.96f);
	const FLinearColor RoWinFaint(0.45f, 0.48f, 0.55f);

	FString RoWinArt(const TCHAR* Name) { return FString::Printf(TEXT("basic_interface/%s.bmp"), Name); }

	FSlateBrush RoWinBrush(UTexture2D* Tex, const FVector2D& Size)
	{
		FSlateBrush Brush;
		Brush.SetResourceObject(Tex);
		Brush.ImageSize = Size;
		Brush.DrawAs = ESlateBrushDrawType::Image;
		return Brush;
	}

	// Ordem dos atributos no CZ_STATUS_CHANGE do rAthena (SP_STR = 13 ... SP_LUK = 18)
	const TCHAR* RoStatNames[6] = { TEXT("Str"), TEXT("Agi"), TEXT("Vit"), TEXT("Int"), TEXT("Dex"), TEXT("Luk") };
	constexpr int32 RoSpStr = 13;

	// Encaixes da janela de equipamento (arte equipwin_bg): coluna esquerda e direita, mascara EQP_* do rAthena
	struct FRoEquipSlot { uint32 Mask; bool bRight; float CenterY; };
	const FRoEquipSlot RoEquipSlots[10] = {
		{ 0x0100, false, 17.5f },  // cabeca (topo)
		{ 0x0200, false, 42.5f },  // cabeca (meio)
		{ 0x0002, false, 69.f },   // mao direita
		{ 0x0004, false, 95.f },   // capa
		{ 0x0008, false, 121.f },  // acessorio (esq.)
		{ 0x0001, true, 17.5f },   // cabeca (baixo)
		{ 0x0010, true, 42.5f },   // vestimenta
		{ 0x0020, true, 69.f },    // mao esquerda
		{ 0x0040, true, 95.f },    // calcados
		{ 0x0080, true, 121.f },   // acessorio (dir.)
	};

	// Abas da janela de itens pelo tipo do item (IT_* do rAthena): 0 item, 1 equip, 2 etc
	int32 RoItemTab(int32 Type)
	{
		switch (Type)
		{
		case 0: case 2: case 11: case 18: return 0;          // cura, usavel, consumo com atraso, cash
		case 4: case 5: case 7: case 8: case 12: return 1;   // armadura, arma, ovo, acessorio de pet, sombra
		default: return 2;                                   // etc, carta, municao
		}
	}

	constexpr int32 RoItemCols = 8;
	constexpr int32 RoItemRows = 3;
	constexpr int32 RoSkillRows = 7;
}

URagnarokNetworkSubsystem* UMythRoHUDWidget::GetNet() const
{
	const UGameInstance* GI = GetGameInstance();
	return GI ? GI->GetSubsystem<URagnarokNetworkSubsystem>() : nullptr;
}

bool UMythRoHUDWidget::IsUnder(const UWidget* Widget, const FVector2D& ScreenPos)
{
	if (!Widget || !Widget->IsVisible())
	{
		return false;
	}
	const FGeometry& Geo = Widget->GetCachedGeometry();
	const FVector2D Local = Geo.AbsoluteToLocal(ScreenPos);
	return Local.X >= 0.f && Local.Y >= 0.f && Local.X <= Geo.GetLocalSize().X && Local.Y <= Geo.GetLocalSize().Y;
}

UCanvasPanel* UMythRoHUDWidget::MakeRoPanel(const FString& Title, const FVector2D& Size, const FVector2D& Offset)
{
	UCanvasPanel* Win = MakeWindow(FVector2D(0.5f, 0.5f), FVector2D(0.5f, 0.5f), Offset, Size);
	AddColor(Win, RoWinBody, 0.f, 17.f, Size.X, Size.Y - 17.f);
	AddTitleBar(Win, Size.X, Title);
	MakeDraggable(Win, Size.X, 17.f);
	// Botao de fechar (sys_close) por cima da alca de arrastar; o clique e testado antes do arrasto
	UImage* Close = AddImage(Win, FMythRoUITextures::Get(RoWinArt(TEXT("sys_close_off"))), Size.X - 15.f, 3.f, 11.f, 11.f);
	CloseButtons.Add(Close, Win);
	Win->SetVisibility(ESlateVisibility::Collapsed);
	return Win;
}

// ------------------------------------------------------------------------------------------------ construcao

void UMythRoHUDWidget::BuildStatusWindow()
{
	// Janela de atributos (Alt+A): titulo + statwin_bg (280x103)
	StatusWindow = MakeRoPanel(TEXT("Atributos"), FVector2D(280.f, 120.f), FVector2D(-340.f, -170.f));
	AddImage(StatusWindow, FMythRoUITextures::Get(RoWinArt(TEXT("statwin_bg"))), 0.f, 17.f);
	UTexture2D* Arrow = FMythRoUITextures::Get(RoWinArt(TEXT("arw_up")));
	for (int32 i = 0; i < 6; ++i)
	{
		const float Y = 17.f + 4.f + 16.f * i;
		AddText(StatusWindow, 8.f, Y, 8, RoWinLabel)->SetText(FText::FromString(RoStatNames[i]));
		StatTexts.Add(AddText(StatusWindow, 40.f, Y, 8, RoWinText));
		StatUpArrows.Add(AddImage(StatusWindow, Arrow, 91.f, Y + 1.f, 11.f, 11.f));
	}
	// Derivados (duas colunas) e as duas linhas longas de baixo, sobre as linhas da arte
	const TCHAR* Left[4] = { TEXT("Atk"), TEXT("Matk"), TEXT("Hit"), TEXT("Critical") };
	const TCHAR* Right[4] = { TEXT("Def"), TEXT("Mdef"), TEXT("Flee"), TEXT("Aspd") };
	for (int32 i = 0; i < 4; ++i)
	{
		const float Y = 17.f + 5.f + 16.f * i;
		AddText(StatusWindow, 120.f, Y, 7, RoWinLabel)->SetText(FText::FromString(Left[i]));
		DerivedTexts.Add(AddText(StatusWindow, 152.f, Y, 7, RoWinText));
		AddText(StatusWindow, 202.f, Y, 7, RoWinLabel)->SetText(FText::FromString(Right[i]));
		DerivedTexts.Add(AddText(StatusWindow, 232.f, Y, 7, RoWinText));
	}
	AddText(StatusWindow, 120.f, 17.f + 5.f + 64.f, 7, RoWinLabel)->SetText(FText::FromString(TEXT("Pontos de Atributo")));
	DerivedTexts.Add(AddText(StatusWindow, 232.f, 17.f + 5.f + 64.f, 7, RoWinText));
	AddText(StatusWindow, 120.f, 17.f + 5.f + 80.f, 7, RoWinLabel)->SetText(FText::FromString(TEXT("Cla")));
	DerivedTexts.Add(AddText(StatusWindow, 232.f, 17.f + 5.f + 80.f, 7, RoWinText));
}

void UMythRoHUDWidget::BuildItemsWindow()
{
	// Inventario (Alt+E): abas verticais (tab_itm_0N) + grade 8x3 de celulas 32x32 (itemwin_mid) + linha de info
	const float W = 22.f + RoItemCols * 32.f + 2.f;
	const float H = 17.f + RoItemRows * 32.f + 4.f + 16.f;
	ItemsWindow = MakeRoPanel(TEXT("Itens"), FVector2D(W, H), FVector2D(340.f, -170.f));
	ItemTabImage = AddImage(ItemsWindow, FMythRoUITextures::Get(RoWinArt(TEXT("tab_itm_01"))), 0.f, 17.f, 20.f, 82.f);
	UTexture2D* CellTex = FMythRoUITextures::Get(RoWinArt(TEXT("itemwin_mid")));
	for (int32 i = 0; i < RoItemCols * RoItemRows; ++i)
	{
		const float X = 22.f + (i % RoItemCols) * 32.f;
		const float Y = 19.f + (i / RoItemCols) * 32.f;
		UImage* Cell = AddImage(ItemsWindow, CellTex, X, Y, 32.f, 32.f);
		Cell->SetVisibility(ESlateVisibility::Visible); // recebe hover/clique (nao passa para o jogo)
		ItemCells.Add(Cell);
		UImage* Icon = AddImage(ItemsWindow, nullptr, X + 4.f, Y + 4.f, 24.f, 24.f);
		Icon->SetVisibility(ESlateVisibility::Collapsed);
		ItemIcons.Add(Icon);
		UTextBlock* Amount = AddText(ItemsWindow, X + 17.f, Y + 20.f, 6, RoWinText);
		Amount->SetShadowOffset(FVector2D(1.f, 1.f));
		Amount->SetShadowColorAndOpacity(FLinearColor(1.f, 1.f, 1.f, 0.9f));
		ItemAmounts.Add(Amount);
		ItemCellIndex.Add(-1);
	}
	ItemInfoText = AddText(ItemsWindow, 24.f, H - 15.f, 7, RoWinText);
	ItemCountText = AddText(ItemsWindow, W - 60.f, H - 15.f, 7, RoWinFaint);
}

void UMythRoHUDWidget::BuildEquipWindow()
{
	// Equipamento (Alt+Q): equipwin_bg (280x130) com os rotulos dos 10 encaixes; icone sobre o circulo e nome
	EquipWindow = MakeRoPanel(TEXT("Equipamento"), FVector2D(280.f, 147.f), FVector2D(-340.f, 120.f));
	AddImage(EquipWindow, FMythRoUITextures::Get(RoWinArt(TEXT("equipwin_bg"))), 0.f, 17.f);
	for (const FRoEquipSlot& S : RoEquipSlots)
	{
		const float Y = 17.f + S.CenterY;
		// Fundo branco cobre o rotulo da arte quando ha item no encaixe
		UImage* Back = AddColor(EquipWindow, RoWinBody, S.bRight ? 160.f : 32.f, Y - 8.f, 86.f, 14.f);
		Back->SetVisibility(ESlateVisibility::Collapsed);
		UImage* Icon = AddImage(EquipWindow, nullptr, S.bRight ? 250.f : 6.f, Y - 12.f, 24.f, 24.f);
		Icon->SetVisibility(ESlateVisibility::Collapsed);
		EquipIcons.Add(Icon);
		UTextBlock* Name = AddText(EquipWindow, S.bRight ? 162.f : 34.f, Y - 7.f, 7, RoWinText);
		EquipNames.Add(Name);
		EquipIcons.Add(Back); // pares (icone, fundo) no mesmo array: indice 2*i e 2*i+1
		EquipCellIndex.Add(-1);
	}
}

void UMythRoHUDWidget::BuildSkillsWindow()
{
	// Habilidades (Alt+S): barra de cima (skill_upbar), linhas de 32 px com icone/nome/nivel, barra de baixo com os pontos
	const float W = 280.f;
	const float H = 17.f + 20.f + RoSkillRows * 32.f + 32.f;
	SkillsWindow = MakeRoPanel(TEXT("Habilidades"), FVector2D(W, H), FVector2D(340.f, 150.f));
	AddImage(SkillsWindow, FMythRoUITextures::Get(RoWinArt(TEXT("skill_upbar_left"))), 0.f, 17.f, 12.f, 20.f);
	AddImage(SkillsWindow, FMythRoUITextures::Get(RoWinArt(TEXT("skill_upbar_mid"))), 12.f, 17.f, W - 24.f, 20.f);
	AddImage(SkillsWindow, FMythRoUITextures::Get(RoWinArt(TEXT("skill_upbar_right"))), W - 12.f, 17.f, 12.f, 20.f);
	AddText(SkillsWindow, 10.f, 20.f, 7, RoWinLabel)->SetText(FText::FromString(TEXT("Lista de habilidades")));
	for (int32 i = 0; i < RoSkillRows; ++i)
	{
		const float Y = 37.f + i * 32.f;
		if (i % 2 == 1)
		{
			AddColor(SkillsWindow, FLinearColor(0.90f, 0.92f, 0.96f, 0.9f), 0.f, Y, W, 32.f);
		}
		UImage* Icon = AddImage(SkillsWindow, nullptr, 6.f, Y + 4.f, 24.f, 24.f);
		Icon->SetVisibility(ESlateVisibility::Collapsed);
		SkillIcons.Add(Icon);
		SkillNames.Add(AddText(SkillsWindow, 38.f, Y + 3.f, 8, RoWinText));
		SkillLevels.Add(AddText(SkillsWindow, 38.f, Y + 17.f, 7, RoWinLabel));
	}
	const float BarY = H - 32.f;
	AddImage(SkillsWindow, FMythRoUITextures::Get(RoWinArt(TEXT("skill_botbar_left"))), 0.f, BarY, 12.f, 32.f);
	AddImage(SkillsWindow, FMythRoUITextures::Get(RoWinArt(TEXT("skill_botbar_mid"))), 12.f, BarY, W - 24.f, 32.f);
	AddImage(SkillsWindow, FMythRoUITextures::Get(RoWinArt(TEXT("skill_botbar_right"))), W - 12.f, BarY, 12.f, 32.f);
	SkillPointsText = AddText(SkillsWindow, 12.f, BarY + 9.f, 8, RoWinText);
}

// ------------------------------------------------------------------------------------------------ abrir / fechar

void UMythRoHUDWidget::ToggleWindow(EMythRoWindow Window)
{
	UCanvasPanel* Target = nullptr;
	switch (Window)
	{
	case EMythRoWindow::Status:  Target = StatusWindow; break;
	case EMythRoWindow::Items:   Target = ItemsWindow; break;
	case EMythRoWindow::Equip:   Target = EquipWindow; break;
	case EMythRoWindow::Skills:  Target = SkillsWindow; break;
	case EMythRoWindow::Basic:   Target = BasicWindow; break;
	case EMythRoWindow::Minimap: Target = MinimapWindow; break;
	}
	if (!Target)
	{
		return;
	}
	const bool bOpen = Target->GetVisibility() == ESlateVisibility::Collapsed;
	Target->SetVisibility(bOpen ? ESlateVisibility::SelfHitTestInvisible : ESlateVisibility::Collapsed);
	OpenOrder.Remove(Target);
	if (bOpen)
	{
		OpenOrder.Add(Target);
		if (UCanvasPanelSlot* S = Cast<UCanvasPanelSlot>(Target->Slot))
		{
			S->SetZOrder(++TopZOrder); // a janela aberta vem para a frente
		}
		WindowRefreshTimer = 0.f;
		RefreshWindows();
	}
}

bool UMythRoHUDWidget::CloseTopWindow()
{
	while (OpenOrder.Num() > 0)
	{
		UCanvasPanel* Win = OpenOrder.Pop().Get();
		// Informacoes basicas e minimapa ficam abertos por padrao: o Esc fecha so as janelas de Alt+A/E/Q/S
		if (Win && Win != BasicWindow && Win != MinimapWindow && Win->GetVisibility() != ESlateVisibility::Collapsed)
		{
			Win->SetVisibility(ESlateVisibility::Collapsed);
			return true;
		}
	}
	return false;
}

// ------------------------------------------------------------------------------------------------ dados do servidor

void UMythRoHUDWidget::RefreshWindows()
{
	if (StatusWindow && StatusWindow->IsVisible()) { RefreshStatusWindow(); }
	if (ItemsWindow && ItemsWindow->IsVisible()) { RefreshItemsWindow(); }
	if (EquipWindow && EquipWindow->IsVisible()) { RefreshEquipWindow(); }
	if (SkillsWindow && SkillsWindow->IsVisible()) { RefreshSkillsWindow(); }
}

void UMythRoHUDWidget::RefreshStatusWindow()
{
	const URagnarokNetworkSubsystem* Net = GetNet();
	const FRagnarokPlayerStats Ps = Net ? Net->GetPlayerStats() : FRagnarokPlayerStats();
	const int32 Base[6] = { Ps.Str, Ps.Agi, Ps.Vit, Ps.Int, Ps.Dex, Ps.Luk };
	for (int32 i = 0; i < 6 && i < StatTexts.Num(); ++i)
	{
		StatTexts[i]->SetText(FText::AsNumber(Base[i]));
		// A seta so aparece com pontos livres (o custo real e validado pelo servidor)
		StatUpArrows[i]->SetVisibility(Ps.StatusPoints > 0 ? ESlateVisibility::Visible : ESlateVisibility::Collapsed);
	}
	auto Plus = [](int32 A, int32 B) { return FText::FromString(FString::Printf(TEXT("%d + %d"), A, B)); };
	if (DerivedTexts.Num() >= 10)
	{
		DerivedTexts[0]->SetText(Plus(Ps.Atk, Ps.Atk2));
		DerivedTexts[1]->SetText(Plus(Ps.Def, Ps.Def2));
		DerivedTexts[2]->SetText(FText::FromString(FString::Printf(TEXT("%d ~ %d"), Ps.MatkMin, Ps.MatkMax)));
		DerivedTexts[3]->SetText(Plus(Ps.Mdef, Ps.Mdef2));
		DerivedTexts[4]->SetText(FText::AsNumber(Ps.Hit));
		DerivedTexts[5]->SetText(Plus(Ps.Flee, Ps.Flee2));
		DerivedTexts[6]->SetText(FText::AsNumber(Ps.Critical));
		DerivedTexts[7]->SetText(FText::AsNumber(Ps.Aspd));
		DerivedTexts[8]->SetText(FText::AsNumber(Ps.StatusPoints));
		DerivedTexts[9]->SetText(FText::FromString(TEXT("-")));
	}
}

void UMythRoHUDWidget::RefreshItemsWindow()
{
	const URagnarokNetworkSubsystem* Net = GetNet();
	if (ItemTabImage)
	{
		ItemTabImage->SetBrush(RoWinBrush(FMythRoUITextures::Get(RoWinArt(*FString::Printf(TEXT("tab_itm_%02d"), ItemTab + 1))), FVector2D(20.f, 82.f) * UIScale));
	}
	int32 Cell = 0;
	int32 Total = 0;
	if (Net)
	{
		for (const FRagnarokInventoryItem& Item : Net->GetInventoryItems())
		{
			if (Item.bIsEquipped || Item.Amount <= 0)
			{
				continue; // equipados aparecem na janela de equipamento
			}
			++Total;
			if (RoItemTab(Item.Type) != ItemTab || Cell >= ItemCells.Num())
			{
				continue;
			}
			UTexture2D* Icon = FMythRoClientTables::ItemIcon(Item.ItemID);
			ItemIcons[Cell]->SetBrush(RoWinBrush(Icon, FVector2D(24.f, 24.f) * UIScale));
			ItemIcons[Cell]->SetVisibility(Icon ? ESlateVisibility::HitTestInvisible : ESlateVisibility::Collapsed);
			ItemAmounts[Cell]->SetText(Item.Amount > 1 ? FText::AsNumber(Item.Amount) : FText::GetEmpty());
			ItemCellIndex[Cell] = Item.Index;
			++Cell;
		}
	}
	for (; Cell < ItemCells.Num(); ++Cell)
	{
		ItemIcons[Cell]->SetVisibility(ESlateVisibility::Collapsed);
		ItemAmounts[Cell]->SetText(FText::GetEmpty());
		ItemCellIndex[Cell] = -1;
	}
	if (ItemCountText)
	{
		ItemCountText->SetText(FText::FromString(FString::Printf(TEXT("%d/100"), Total)));
	}
	// Linha de informacao: nome do item sob o cursor (como a dica do cliente original)
	FString Info;
	if (!Net || Net->GetNetworkState() != ERagnarokNetworkState::ConnectedMap)
	{
		Info = TEXT("Inventario vem do servidor (desconectado)");
	}
	for (int32 i = 0; i < ItemCells.Num() && Net; ++i)
	{
		if (ItemCellIndex[i] >= 0 && ItemCells[i]->IsHovered())
		{
			for (const FRagnarokInventoryItem& Item : Net->GetInventoryItems())
			{
				if (Item.Index == ItemCellIndex[i])
				{
					Info = FMythRoClientTables::ItemName(Item.ItemID);
					if (Item.Refine > 0)
					{
						Info = FString::Printf(TEXT("+%d %s"), Item.Refine, *Info);
					}
					if (Item.Amount > 1)
					{
						Info += FString::Printf(TEXT(": %d un."), Item.Amount);
					}
					break;
				}
			}
		}
	}
	if (ItemInfoText)
	{
		ItemInfoText->SetText(FText::FromString(Info));
	}
}

void UMythRoHUDWidget::RefreshEquipWindow()
{
	const URagnarokNetworkSubsystem* Net = GetNet();
	for (int32 i = 0; i < UE_ARRAY_COUNT(RoEquipSlots) && i < EquipNames.Num(); ++i)
	{
		const FRagnarokInventoryItem* Found = nullptr;
		if (Net)
		{
			for (const FRagnarokInventoryItem& Item : Net->GetInventoryItems())
			{
				if (Item.bIsEquipped && (static_cast<uint32>(Item.EquipLocation) & RoEquipSlots[i].Mask) != 0)
				{
					Found = &Item;
					break;
				}
			}
		}
		UImage* Icon = EquipIcons[2 * i];
		UImage* Back = EquipIcons[2 * i + 1];
		if (Found)
		{
			UTexture2D* Tex = FMythRoClientTables::ItemIcon(Found->ItemID);
			Icon->SetBrush(RoWinBrush(Tex, FVector2D(24.f, 24.f) * UIScale));
			Icon->SetVisibility(Tex ? ESlateVisibility::Visible : ESlateVisibility::Collapsed);
			Back->SetVisibility(ESlateVisibility::HitTestInvisible);
			FString Name = FMythRoClientTables::ItemName(Found->ItemID);
			if (Found->Refine > 0)
			{
				Name = FString::Printf(TEXT("+%d %s"), Found->Refine, *Name);
			}
			EquipNames[i]->SetText(FText::FromString(Name.Left(16)));
			EquipCellIndex[i] = Found->Index;
		}
		else
		{
			Icon->SetVisibility(ESlateVisibility::Collapsed);
			Back->SetVisibility(ESlateVisibility::Collapsed);
			EquipNames[i]->SetText(FText::GetEmpty());
			EquipCellIndex[i] = -1;
		}
	}
}

void UMythRoHUDWidget::RefreshSkillsWindow()
{
	const URagnarokNetworkSubsystem* Net = GetNet();
	int32 Row = 0;
	if (Net)
	{
		for (const FRagnarokSkillInfo& Skill : Net->GetSkillList())
		{
			if (Row >= SkillNames.Num())
			{
				break;
			}
			UTexture2D* Icon = FMythRoClientTables::SkillIcon(Skill.SkillID);
			SkillIcons[Row]->SetBrush(RoWinBrush(Icon, FVector2D(24.f, 24.f) * UIScale));
			SkillIcons[Row]->SetVisibility(Icon ? ESlateVisibility::HitTestInvisible : ESlateVisibility::Collapsed);
			FString Name = FMythRoClientTables::SkillName(Skill.SkillID);
			if (Name.IsEmpty())
			{
				Name = Skill.SkillName;
			}
			SkillNames[Row]->SetText(FText::FromString(Name));
			SkillLevels[Row]->SetText(FText::FromString(Skill.SpCost > 0
				? FString::Printf(TEXT("Nv %d    Sp %d"), Skill.Level, Skill.SpCost)
				: FString::Printf(TEXT("Nv %d"), Skill.Level)));
			++Row;
		}
	}
	for (; Row < SkillNames.Num(); ++Row)
	{
		SkillIcons[Row]->SetVisibility(ESlateVisibility::Collapsed);
		SkillNames[Row]->SetText(FText::GetEmpty());
		SkillLevels[Row]->SetText(FText::GetEmpty());
	}
	if (SkillPointsText)
	{
		const int32 Points = Net ? Net->GetPlayerStats().SkillPoints : 0;
		SkillPointsText->SetText(FText::FromString(FString::Printf(TEXT("Pontos de Habilidade: %d"), Points)));
	}
}

// ------------------------------------------------------------------------------------------------ cliques

bool UMythRoHUDWidget::HandleWindowClick(const FVector2D& ScreenPos)
{
	for (const TPair<TWeakObjectPtr<UImage>, TWeakObjectPtr<UCanvasPanel>>& Pair : CloseButtons)
	{
		UCanvasPanel* Win = Pair.Value.Get();
		if (Win && Win->IsVisible() && IsUnder(Pair.Key.Get(), ScreenPos))
		{
			Win->SetVisibility(ESlateVisibility::Collapsed);
			OpenOrder.Remove(Win);
			return true;
		}
	}
	URagnarokNetworkSubsystem* Net = GetNet();
	if (StatusWindow && StatusWindow->IsVisible())
	{
		for (int32 i = 0; i < StatUpArrows.Num(); ++i)
		{
			if (IsUnder(StatUpArrows[i], ScreenPos))
			{
				if (Net)
				{
					Net->SendStatusUp(RoSpStr + i, 1); // CZ_STATUS_CHANGE: o servidor valida e devolve o novo valor
				}
				return true;
			}
		}
	}
	if (ItemsWindow && ItemsWindow->IsVisible() && IsUnder(ItemTabImage, ScreenPos))
	{
		const FGeometry& Geo = ItemTabImage->GetCachedGeometry();
		const float LocalY = Geo.AbsoluteToLocal(ScreenPos).Y / FMath::Max(Geo.GetLocalSize().Y, 1.f);
		ItemTab = FMath::Clamp(FMath::FloorToInt(LocalY * 3.f), 0, 2);
		RefreshItemsWindow();
		return true;
	}
	return false;
}

FReply UMythRoHUDWidget::NativeOnMouseButtonDoubleClick(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent)
{
	const FVector2D Screen = InMouseEvent.GetScreenSpacePosition();
	URagnarokNetworkSubsystem* Net = GetNet();
	if (Net && ItemsWindow && ItemsWindow->IsVisible())
	{
		for (int32 i = 0; i < ItemCells.Num(); ++i)
		{
			if (ItemCellIndex[i] < 0 || !IsUnder(ItemCells[i], Screen))
			{
				continue;
			}
			// Duplo clique: usa o consumivel ou veste o equipamento (como no cliente original)
			for (const FRagnarokInventoryItem& Item : Net->GetInventoryItems())
			{
				if (Item.Index == ItemCellIndex[i])
				{
					if (RoItemTab(Item.Type) == 1)
					{
						Net->EquipItem(Item.Index, Item.EquipLocation);
					}
					else if (RoItemTab(Item.Type) == 0)
					{
						Net->UseItem(Item.Index);
					}
					break;
				}
			}
			return FReply::Handled();
		}
	}
	if (Net && EquipWindow && EquipWindow->IsVisible())
	{
		for (int32 i = 0; i < EquipCellIndex.Num(); ++i)
		{
			if (EquipCellIndex[i] >= 0 && IsUnder(EquipIcons[2 * i], Screen))
			{
				Net->UnequipItem(EquipCellIndex[i]); // duplo clique no encaixe tira o equipamento
				return FReply::Handled();
			}
		}
	}
	return Super::NativeOnMouseButtonDoubleClick(InGeometry, InMouseEvent);
}

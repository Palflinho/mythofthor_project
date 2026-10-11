// Janelas classicas do cliente original (Atributos, Itens, Equipamento, Habilidades) do UMythRoHUDWidget.
// Regra do projeto: tudo o que e mostrado aqui vem do rAthena (URagnarokNetworkSubsystem); o cliente so desenha
// e envia pedidos (CZ_STATUS_CHANGE, CZ_USE_ITEM, CZ_REQ_WEAR_EQUIP, CZ_REQ_TAKEOFF_EQUIP).

#include "UI/MythRoHUDWidget.h"

#include "UI/MythRoClientTables.h"
#include "UI/MythRoSkillTree.h"
#include "UI/MythRoUITextures.h"
#include "Network/RagnarokNetworkSubsystem.h"
#include "Player/MythPlayerController.h"

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
	const FLinearColor RoWinBody(0.965f, 0.965f, 0.98f, 0.93f);
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
	// Arvore de habilidades: grade de 7 colunas, celula de 40 x 46 (moldura 32, icone 24, nivel embaixo)
	constexpr float RoTreeLeft = 8.f;
	constexpr float RoTreeTop = 42.f;
	constexpr float RoTreeCellW = 40.f;
	constexpr float RoTreeCellH = 46.f;
	constexpr int32 RoTreeRows = 9;
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
	// Fundo "Visible": o clique na janela chega ao HUD e nao atravessa para o jogo (nao anda nem ataca)
	AddRounded(Win, RoWinBody, FLinearColor(0.55f, 0.58f, 0.66f, 0.9f), 0.f, 17.f, Size.X, Size.Y - 17.f, 0.f, 7.f)->SetVisibility(ESlateVisibility::Visible);
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
	// Habilidades (Alt+S) em arvore, como a janela do cliente moderno: abas por etapa da classe, grade de 7 colunas
	// na posicao do skilltreeview.lub, linhas ate os pre-requisitos e o nivel/"+" em cada skill
	const float W = RoTreeLeft * 2.f + FMythRoSkillTree::Columns * RoTreeCellW;
	const float H = RoTreeTop + RoTreeRows * RoTreeCellH + 40.f;
	SkillsWindow = MakeRoPanel(TEXT("Habilidades"), FVector2D(W, H), FVector2D(340.f, 110.f));
	AddImage(SkillsWindow, FMythRoUITextures::Get(RoWinArt(TEXT("skill_upbar_left"))), 0.f, 17.f, 12.f, 20.f);
	AddImage(SkillsWindow, FMythRoUITextures::Get(RoWinArt(TEXT("skill_upbar_mid"))), 12.f, 17.f, W - 24.f, 20.f);
	AddImage(SkillsWindow, FMythRoUITextures::Get(RoWinArt(TEXT("skill_upbar_right"))), W - 12.f, 17.f, 12.f, 20.f);
	for (int32 t = 0; t < 4; ++t)
	{
		UImage* Back = AddColor(SkillsWindow, FLinearColor::Transparent, 6.f + t * 78.f, 19.f, 76.f, 16.f);
		Back->SetVisibility(ESlateVisibility::Collapsed);
		SkillTabBacks.Add(Back);
		SkillTabTexts.Add(AddText(SkillsWindow, 12.f + t * 78.f, 20.f, 7, RoWinText));
	}
	// Linhas primeiro (ficam por baixo dos icones)
	for (int32 i = 0; i < 90; ++i)
	{
		UImage* Line = AddColor(SkillsWindow, FLinearColor(0.62f, 0.66f, 0.74f, 1.f), 0.f, 0.f, 1.f, 1.f);
		Line->SetVisibility(ESlateVisibility::Collapsed);
		SkillLinePool.Add(Line);
	}
	for (int32 i = 0; i < RoTreeRows * FMythRoSkillTree::Columns; ++i)
	{
		const float X = RoTreeLeft + (i % FMythRoSkillTree::Columns) * RoTreeCellW;
		const float Y = RoTreeTop + (i / FMythRoSkillTree::Columns) * RoTreeCellH;
		UImage* Frame = AddImage(SkillsWindow, FMythRoUITextures::Get(RoWinArt(TEXT("itemwin_mid"))), X + 5.f, Y, 32.f, 32.f);
		Frame->SetVisibility(ESlateVisibility::Collapsed);
		SkillCellFrames.Add(Frame);
		UImage* Icon = AddImage(SkillsWindow, nullptr, X + 9.f, Y + 4.f, 24.f, 24.f);
		Icon->SetVisibility(ESlateVisibility::Collapsed);
		SkillIcons.Add(Icon);
		SkillNames.Add(AddText(SkillsWindow, X + 7.f, Y + 32.f, 7, RoWinText)); // nivel "3/10"
		SkillLevels.Add(nullptr);
		UImage* Up = AddImage(SkillsWindow, FMythRoUITextures::Get(RoWinArt(TEXT("lv_up_on"))), X + 27.f, Y + 18.f, 14.f, 14.f);
		Up->SetVisibility(ESlateVisibility::Collapsed);
		SkillUpButtons.Add(Up);
	}
	const float BarY = H - 32.f;
	SkillBottomBar.Add(AddImage(SkillsWindow, FMythRoUITextures::Get(RoWinArt(TEXT("skill_botbar_left"))), 0.f, BarY, 12.f, 32.f));
	SkillBottomBar.Add(AddImage(SkillsWindow, FMythRoUITextures::Get(RoWinArt(TEXT("skill_botbar_mid"))), 12.f, BarY, W - 24.f, 32.f));
	SkillBottomBar.Add(AddImage(SkillsWindow, FMythRoUITextures::Get(RoWinArt(TEXT("skill_botbar_right"))), W - 12.f, BarY, 12.f, 32.f));
	SkillPointsText = AddText(SkillsWindow, 12.f, BarY + 9.f, 8, RoWinText);
	SkillBottomBar.Add(SkillPointsText);
}

void UMythRoHUDWidget::ResizeSkillTree(int32 Rows)
{
	// A janela cresce/encolhe com a arvore da aba (Super Aprendiz usa 9 linhas; a 1a classe, 2 ou 3)
	if (!SkillsWindow)
	{
		return;
	}
	const float W = RoTreeLeft * 2.f + FMythRoSkillTree::Columns * RoTreeCellW;
	const float H = RoTreeTop + Rows * RoTreeCellH + 40.f;
	const float BarY = H - 32.f;
	if (UCanvasPanelSlot* S = Cast<UCanvasPanelSlot>(SkillsWindow->Slot))
	{
		S->SetSize(FVector2D(W, H) * UIScale);
	}
	if (UCanvasPanelSlot* S = SkillsWindow->GetChildrenCount() > 0 ? Cast<UCanvasPanelSlot>(SkillsWindow->GetChildAt(0)->Slot) : nullptr)
	{
		S->SetSize(FVector2D(W, H - 17.f) * UIScale); // fundo branco do MakeRoPanel
	}
	for (UWidget* Part : SkillBottomBar)
	{
		if (UCanvasPanelSlot* S = Part ? Cast<UCanvasPanelSlot>(Part->Slot) : nullptr)
		{
			const FVector2D Pos = S->GetPosition();
			S->SetPosition(FVector2D(Pos.X, (Part == SkillPointsText ? BarY + 9.f : BarY) * UIScale));
		}
	}
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
	case EMythRoWindow::Party:   Target = PartyWindow; break;
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
			if (Win == ShopWindow || Win == DealWindow)
			{
				CloseShop(true);
				return true;
			}
			if (Win == StorageWindow)
			{
				CloseStorage(true);
				return true;
			}
			if (Win == PartyInviteWindow)
			{
				AnswerPartyInvite(false);
				return true;
			}
			Win->SetVisibility(ESlateVisibility::Collapsed);
			return true;
		}
	}
	return false;
}

// ------------------------------------------------------------------------------------------------ dados do servidor

void UMythRoHUDWidget::RefreshWindows()
{
	RefreshHotbar();
	if (StatusWindow && StatusWindow->IsVisible()) { RefreshStatusWindow(); }
	if (ItemsWindow && ItemsWindow->IsVisible()) { RefreshItemsWindow(); }
	if (EquipWindow && EquipWindow->IsVisible()) { RefreshEquipWindow(); }
	if (SkillsWindow && SkillsWindow->IsVisible()) { RefreshSkillsWindow(); }
	if (StorageWindow && StorageWindow->IsVisible()) { RefreshStorage(); }
	if (PartyWindow && PartyWindow->IsVisible()) { RefreshParty(); }
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
	const int32 Cells = SkillIcons.Num();
	SkillRowID.Init(0, Cells);
	SkillRowLevel.Init(0, Cells);
	const URagnarokNetworkSubsystem* Net = GetNet();
	const int32 Points = Net ? Net->GetPlayerStats().SkillPoints : 0;
	const int32 Job = Net ? Net->GetPlayerStats().JobClass : 0;

	// Abas: uma por etapa da classe (Aprendiz, 1a, 2a, trans); sem escolha, abre na classe atual
	SkillTabJobs = FMythRoSkillTree::JobChain(Job);
	if (!SkillTabJobs.IsValidIndex(SkillTab))
	{
		SkillTab = SkillTabJobs.Num() - 1;
	}
	for (int32 t = 0; t < SkillTabTexts.Num(); ++t)
	{
		const bool bHas = SkillTabJobs.IsValidIndex(t);
		SkillTabTexts[t]->SetText(bHas ? FText::FromString(FMythRoSkillTree::JobTabName(SkillTabJobs[t])) : FText::GetEmpty());
		SkillTabTexts[t]->SetColorAndOpacity(FSlateColor(t == SkillTab ? RoWinText : RoWinLabel));
		SkillTabBacks[t]->SetVisibility(bHas ? ESlateVisibility::Visible : ESlateVisibility::Collapsed);
		SkillTabBacks[t]->SetColorAndOpacity(t == SkillTab ? FLinearColor(1.f, 1.f, 1.f, 0.85f) : FLinearColor(0.85f, 0.88f, 0.94f, 0.6f));
	}

	// Nivel de cada skill que o servidor mandou (ZC_SKILLINFO_LIST): ausente = ainda bloqueada
	TMap<int32, const FRagnarokSkillInfo*> Known;
	if (Net)
	{
		for (const FRagnarokSkillInfo& S : Net->GetSkillList())
		{
			Known.Add(S.SkillID, &S);
		}
	}
	const TMap<int32, int32>* Grid = SkillTabJobs.IsValidIndex(SkillTab) ? FMythRoSkillTree::Grid(SkillTabJobs[SkillTab]) : nullptr;
	TMap<int32, int32> CellOfSkill;
	int32 LastCell = 0;
	if (Grid)
	{
		for (const TPair<int32, int32>& Cell : *Grid)
		{
			LastCell = FMath::Max(LastCell, Cell.Key);
		}
	}
	ResizeSkillTree(FMath::Clamp(LastCell / FMythRoSkillTree::Columns + 1, 3, RoTreeRows));
	for (int32 i = 0; i < Cells; ++i)
	{
		const int32* SkillID = Grid ? Grid->Find(i) : nullptr;
		const bool bUsed = SkillID && *SkillID > 0;
		SkillCellFrames[i]->SetVisibility(bUsed ? ESlateVisibility::HitTestInvisible : ESlateVisibility::Collapsed);
		if (!bUsed)
		{
			SkillIcons[i]->SetVisibility(ESlateVisibility::Collapsed);
			SkillUpButtons[i]->SetVisibility(ESlateVisibility::Collapsed);
			SkillNames[i]->SetText(FText::GetEmpty());
			continue;
		}
		CellOfSkill.Add(*SkillID, i);
		const FRagnarokSkillInfo* Info = Known.FindRef(*SkillID);
		const FMythTreeSkill* Tree = FMythRoSkillTree::Skill(*SkillID);
		const int32 Level = Info ? Info->Level : 0;
		SkillRowID[i] = *SkillID;
		SkillRowLevel[i] = Level;
		UTexture2D* Icon = FMythRoClientTables::SkillIcon(*SkillID);
		SkillIcons[i]->SetBrush(RoWinBrush(Icon, FVector2D(24.f, 24.f) * UIScale));
		SkillIcons[i]->SetVisibility(Icon ? ESlateVisibility::Visible : ESlateVisibility::Collapsed); // Visible: arrastavel
		// Aprendida: cor cheia; liberada (o servidor mandou com nivel 0): um pouco apagada; bloqueada: cinza
		SkillIcons[i]->SetColorAndOpacity(Level > 0 ? FLinearColor::White : (Info ? FLinearColor(1.f, 1.f, 1.f, 0.7f) : FLinearColor(0.35f, 0.35f, 0.38f, 0.55f)));
		SkillNames[i]->SetText(FText::FromString(FString::Printf(TEXT("%d/%d"), Level, Tree ? Tree->MaxLevel : Level)));
		SkillNames[i]->SetColorAndOpacity(FSlateColor(Level > 0 ? RoWinText : RoWinLabel));
		SkillUpButtons[i]->SetVisibility(Points > 0 && Info && Info->bUpgradable ? ESlateVisibility::Visible : ESlateVisibility::Collapsed);
	}

	// Linhas ate os pre-requisitos da mesma aba: desce do pre-requisito, anda na horizontal no vao acima da skill e
	// desce ate ela. Azul quando o pre-requisito ja esta no nivel pedido
	auto Place = [this](UImage* Img, float X, float Y, float W, float H, const FLinearColor& Color)
	{
		if (UCanvasPanelSlot* S = Cast<UCanvasPanelSlot>(Img->Slot))
		{
			S->SetPosition(FVector2D(X, Y) * UIScale);
			S->SetSize(FVector2D(FMath::Max(W, 2.f), FMath::Max(H, 2.f)) * UIScale);
		}
		Img->SetColorAndOpacity(Color);
		Img->SetVisibility(ESlateVisibility::HitTestInvisible);
	};
	int32 Used = 0;
	for (const TPair<int32, int32>& Pair : CellOfSkill)
	{
		const FMythTreeSkill* Tree = FMythRoSkillTree::Skill(Pair.Key);
		if (!Tree)
		{
			continue;
		}
		for (const FIntPoint& Need : Tree->Need)
		{
			const int32* From = CellOfSkill.Find(Need.X);
			if (!From || Used + 3 > SkillLinePool.Num())
			{
				continue;
			}
			const FRagnarokSkillInfo* Have = Known.FindRef(Need.X);
			const FLinearColor Color = Have && Have->Level >= Need.Y ? FLinearColor(0.35f, 0.55f, 0.9f, 1.f) : FLinearColor(0.68f, 0.71f, 0.78f, 1.f);
			const float FX = RoTreeLeft + (*From % FMythRoSkillTree::Columns) * RoTreeCellW + 20.f;
			const float FY = RoTreeTop + (*From / FMythRoSkillTree::Columns) * RoTreeCellH;
			const float TX = RoTreeLeft + (Pair.Value % FMythRoSkillTree::Columns) * RoTreeCellW + 20.f;
			const float TY = RoTreeTop + (Pair.Value / FMythRoSkillTree::Columns) * RoTreeCellH;
			if (FMath::IsNearlyEqual(FY, TY))
			{
				Place(SkillLinePool[Used++], FMath::Min(FX, TX), FY + 16.f, FMath::Abs(TX - FX), 2.f, Color); // mesma linha
				continue;
			}
			const float MidY = TY - 6.f; // vao entre a linha de cima e a skill
			Place(SkillLinePool[Used++], FX - 1.f, FY + 32.f, 2.f, MidY - (FY + 32.f), Color);
			Place(SkillLinePool[Used++], FMath::Min(FX, TX) - 1.f, MidY, FMath::Abs(TX - FX) + 2.f, 2.f, Color);
			Place(SkillLinePool[Used++], TX - 1.f, MidY, 2.f, TY - MidY, Color);
		}
	}
	for (int32 i = Used; i < SkillLinePool.Num(); ++i)
	{
		SkillLinePool[i]->SetVisibility(ESlateVisibility::Collapsed);
	}

	if (SkillPointsText)
	{
		// Barra de baixo: pontos e a skill selecionada (clique) com nome, nivel e SP
		FString Line = FString::Printf(TEXT("Pontos: %d"), Points);
		if (SkillRowID.IsValidIndex(SkillScroll) && SkillRowID[SkillScroll] > 0)
		{
			const int32 Id = SkillRowID[SkillScroll];
			const FMythTreeSkill* Tree = FMythRoSkillTree::Skill(Id);
			const FRagnarokSkillInfo* Info = Known.FindRef(Id);
			FString Name = Tree ? Tree->Name : FMythRoClientTables::SkillName(Id);
			Line += FString::Printf(TEXT("     %s  Nv %d/%d"), *Name, SkillRowLevel[SkillScroll], Tree ? Tree->MaxLevel : 0);
			if (Info && Info->SpCost > 0)
			{
				Line += FString::Printf(TEXT("  SP %d"), Info->SpCost);
			}
			if (!Info && Tree && Tree->Need.Num() > 0)
			{
				Line += TEXT("  (requer: ");
				for (int32 n = 0; n < Tree->Need.Num(); ++n)
				{
					const FMythTreeSkill* Req = FMythRoSkillTree::Skill(Tree->Need[n].X);
					Line += FString::Printf(TEXT("%s%s %d"), n ? TEXT(", ") : TEXT(""), Req ? *Req->Name : TEXT("?"), Tree->Need[n].Y);
				}
				Line += TEXT(")");
			}
		}
		SkillPointsText->SetText(FText::FromString(Line));
	}
}

// ------------------------------------------------------------------------------------------------ cliques

bool UMythRoHUDWidget::HandleWindowClick(const FVector2D& ScreenPos)
{
	if (HandleShopClick(ScreenPos))
	{
		return true;
	}
	if (HandleStorageClick(ScreenPos))
	{
		return true;
	}
	if (HandlePartyClick(ScreenPos))
	{
		return true;
	}
	if (HandleGameMenuClick(ScreenPos) || HandleBasicMenuClick(ScreenPos))
	{
		return true;
	}
	for (const TPair<TWeakObjectPtr<UImage>, TWeakObjectPtr<UCanvasPanel>>& Pair : CloseButtons)
	{
		UCanvasPanel* Win = Pair.Value.Get();
		if (Win && Win->IsVisible() && IsUnder(Pair.Key.Get(), ScreenPos))
		{
			if (Win == ShopWindow || Win == DealWindow)
			{
				CloseShop(true); // fechar a loja libera o NPC no servidor
				return true;
			}
			if (Win == StorageWindow)
			{
				CloseStorage(true);
				return true;
			}
			if (Win == PartyInviteWindow)
			{
				AnswerPartyInvite(false);
				return true;
			}
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
	if (SkillsWindow && SkillsWindow->IsVisible())
	{
		for (int32 t = 0; t < SkillTabBacks.Num(); ++t)
		{
			if (SkillTabJobs.IsValidIndex(t) && IsUnder(SkillTabBacks[t], ScreenPos))
			{
				SkillTab = t; // aba de outra etapa da classe
				SkillScroll = -1;
				RefreshSkillsWindow();
				return true;
			}
		}
		for (int32 r = 0; r < SkillIcons.Num() && r < SkillRowID.Num(); ++r)
		{
			if (SkillRowID[r] != 0 && IsUnder(SkillIcons[r], ScreenPos) && !IsUnder(SkillUpButtons[r], ScreenPos))
			{
				SkillScroll = r; // selecionada: nome, nivel e requisitos na barra de baixo (o arrasto continua valendo)
				RefreshSkillsWindow();
				break;
			}
		}
		for (int32 r = 0; r < SkillUpButtons.Num() && r < SkillRowID.Num(); ++r)
		{
			if (SkillRowID[r] != 0 && SkillUpButtons[r]->IsVisible() && IsUnder(SkillUpButtons[r], ScreenPos))
			{
				if (Net)
				{
					Net->SendSkillUp(SkillRowID[r]); // CZ_UPGRADE_SKILLLEVEL: o servidor valida e devolve a skill atualizada
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
	if (HandleNpcClick(Screen, true))
	{
		return FReply::Handled();
	}
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
	if (SkillsWindow && SkillsWindow->IsVisible())
	{
		for (int32 r = 0; r < SkillIcons.Num() && r < SkillRowID.Num(); ++r)
		{
			if (SkillRowID[r] != 0 && (IsUnder(SkillIcons[r], Screen) || IsUnder(SkillNames[r], Screen)))
			{
				if (AMythPlayerController* PC = Cast<AMythPlayerController>(GetOwningPlayer()))
				{
					PC->BeginSkill(SkillRowID[r], SkillRowLevel[r]); // duplo clique na skill: usa
				}
				return FReply::Handled();
			}
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

FReply UMythRoHUDWidget::NativeOnMouseWheel(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent)
{
	if (HandleShopWheel(InMouseEvent.GetScreenSpacePosition(), InMouseEvent.GetWheelDelta()))
	{
		return FReply::Handled();
	}
	if (HandleStorageWheel(InMouseEvent.GetScreenSpacePosition(), InMouseEvent.GetWheelDelta()))
	{
		return FReply::Handled();
	}
	// Roda do mouse sobre a arvore de habilidades: troca de aba (a grade inteira cabe na janela)
	if (SkillsWindow && SkillsWindow->IsVisible() && IsUnder(SkillsWindow, InMouseEvent.GetScreenSpacePosition()))
	{
		if (SkillTabJobs.Num() > 0)
		{
			SkillTab = FMath::Clamp(SkillTab + (InMouseEvent.GetWheelDelta() > 0.f ? -1 : 1), 0, SkillTabJobs.Num() - 1);
			SkillScroll = -1;
			RefreshSkillsWindow();
		}
		return FReply::Handled();
	}
	return Super::NativeOnMouseWheel(InGeometry, InMouseEvent);
}

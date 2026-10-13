#include "UI/MythRoHUDWidget.h"
#include "UI/MythRoUITextures.h"
#include "Components/CanvasPanel.h"
#include "Components/CanvasPanelSlot.h"
#include "Components/Image.h"
#include "Components/TextBlock.h"

// Menu Principal centralizado (hub): botao flutuante no canto superior direito, ao lado do minimapa, que abre/fecha um painel
// modal no centro da tela com uma grade de atalhos para as janelas do jogo. Escolher uma janela a abre e fecha o menu, a menos
// que "Manter aberto" esteja marcado.

namespace
{
	constexpr float HubCols = 3.f;
	constexpr float HubCellW = 86.f;
	constexpr float HubCellH = 72.f;
	constexpr float HubPad = 14.f;
	constexpr float HubTitleH = 26.f;
	constexpr float HubFootH = 26.f;
	constexpr int32 HubRows = 6;
	constexpr float HubW = HubPad * 2.f + HubCols * HubCellW;
	constexpr float HubH = HubTitleH + HubRows * HubCellH + HubFootH + HubPad;

	struct FHubEntry
	{
		const TCHAR* Icon;
		const TCHAR* Label;
		EMythRoWindow Window;
		bool bGameMenu;
	};

	const FHubEntry HubEntries[] = {
		{ TEXT("item"), TEXT("Inventário"), EMythRoWindow::Items, false },
		{ TEXT("equip"), TEXT("Equipamentos"), EMythRoWindow::Equip, false },
		{ TEXT("status"), TEXT("Status"), EMythRoWindow::Status, false },
		{ TEXT("skill"), TEXT("Habilidades"), EMythRoWindow::Skills, false },
		{ TEXT("party"), TEXT("Grupo"), EMythRoWindow::Party, false },
		{ TEXT("quest"), TEXT("Missões"), EMythRoWindow::Quests, false },
		{ TEXT("mail"), TEXT("Correio"), EMythRoWindow::Mail, false },
		{ TEXT("map"), TEXT("Mapa"), EMythRoWindow::Minimap, false },
		{ TEXT("guild"), TEXT("Clã"), EMythRoWindow::Guild, false },
		{ TEXT("navigation"), TEXT("Navegação"), EMythRoWindow::Navi, false },
		{ TEXT("sns"), TEXT("Amigos"), EMythRoWindow::Friends, false },
		{ TEXT("achievement"), TEXT("Conquistas"), EMythRoWindow::Achievements, false },
		{ TEXT("bank"), TEXT("Banco"), EMythRoWindow::Bank, false },
		{ TEXT("keyboard"), TEXT("Atalhos"), EMythRoWindow::Shortcuts, false },
		{ TEXT("shop"), TEXT("Loja de Testes"), EMythRoWindow::TestShop, false },
		{ TEXT("option"), TEXT("Configurações"), EMythRoWindow::Status, true },
	};
}

void UMythRoHUDWidget::BuildMainMenu()
{
	// --- Botao flutuante (tres barras), a esquerda do minimapa
	MainMenuButton = MakeWindow(FVector2D(1.f, 0.f), FVector2D(1.f, 0.f), FVector2D(-136.f * UIScale, 6.f * UIScale), FVector2D(30.f, 30.f));
	MainMenuButtonBack = AddRounded(MainMenuButton, FLinearColor(0.05f, 0.06f, 0.11f, 0.82f), FLinearColor(0.5f, 0.55f, 0.7f, 0.8f), 0.f, 0.f, 30.f, 30.f, 7.f, 7.f);
	MainMenuButtonBack->SetVisibility(ESlateVisibility::Visible);
	MainMenuButtonBack->SetToolTipText(FText::FromString(TEXT("Menu principal")));
	for (int32 i = 0; i < 3; ++i)
	{
		AddColor(MainMenuButton, FLinearColor(0.9f, 0.93f, 1.f), 7.f, 9.f + i * 5.f, 16.f, 2.f);
	}

	// --- Painel modal central
	MainMenuPanel = MakeWindow(FVector2D(0.5f, 0.5f), FVector2D(0.5f, 0.5f), FVector2D(0.f, 0.f), FVector2D(HubW, HubH));
	UImage* Back = AddRounded(MainMenuPanel, FLinearColor(0.05f, 0.06f, 0.11f, 0.92f), FLinearColor(0.55f, 0.6f, 0.8f, 0.9f), 0.f, 0.f, HubW, HubH, 10.f, 10.f);
	Back->SetVisibility(ESlateVisibility::Visible); // o clique no painel nao atravessa para o jogo
	UTextBlock* Title = AddText(MainMenuPanel, HubPad, 6.f, 9, FLinearColor(0.95f, 0.96f, 1.f), true);
	Title->SetText(FText::FromString(TEXT("Menu Principal")));
	MainMenuClose = AddImage(MainMenuPanel, FMythRoUITextures::Get(TEXT("basic_interface/sys_close_off.bmp")), HubW - 24.f, 8.f, 11.f, 11.f);
	MainMenuClose->SetVisibility(ESlateVisibility::Visible);
	for (int32 i = 0; i < UE_ARRAY_COUNT(HubEntries); ++i)
	{
		const float X = HubPad + (i % 3) * HubCellW;
		const float Y = HubTitleH + (i / 3) * HubCellH;
		UImage* Cell = AddRounded(MainMenuPanel, FLinearColor(0.12f, 0.14f, 0.24f, 0.85f), FLinearColor(0.4f, 0.45f, 0.65f, 0.8f), X + 3.f, Y + 3.f, HubCellW - 6.f, HubCellH - 6.f, 6.f, 6.f);
		Cell->SetVisibility(ESlateVisibility::Visible);
		MainMenuCells.Add(Cell);
		AddImage(MainMenuPanel, FMythRoUITextures::Get(FString::Printf(TEXT("menu_icon/bt_%s.bmp"), HubEntries[i].Icon)), X + (HubCellW - 32.f) * 0.5f, Y + 10.f, 32.f, 34.f);
		UTextBlock* Label = AddText(MainMenuPanel, X, Y + 49.f, 7, FLinearColor(0.9f, 0.93f, 1.f));
		Label->SetText(FText::FromString(HubEntries[i].Label));
		Label->SetJustification(ETextJustify::Center);
		if (UCanvasPanelSlot* S = Cast<UCanvasPanelSlot>(Label->Slot)) { S->SetAutoSize(false); S->SetSize(FVector2D(HubCellW, 14.f) * UIScale); }
	}
	MainMenuPinBox = AddColor(MainMenuPanel, FLinearColor(0.9f, 0.93f, 1.f), HubPad + 2.f, HubH - HubFootH - 2.f + 6.f, 12.f, 12.f);
	MainMenuPinBox->SetVisibility(ESlateVisibility::Visible);
	MainMenuPinMark = AddColor(MainMenuPanel, FLinearColor(0.15f, 0.55f, 0.95f), HubPad + 4.f, HubH - HubFootH - 2.f + 8.f, 8.f, 8.f);
	MainMenuPinMark->SetVisibility(ESlateVisibility::Collapsed);
	UTextBlock* Pin = AddText(MainMenuPanel, HubPad + 20.f, HubH - HubFootH - 2.f + 6.f, 7, FLinearColor(0.85f, 0.88f, 1.f));
	Pin->SetText(FText::FromString(TEXT("Manter aberto")));
	MainMenuPanel->SetVisibility(ESlateVisibility::Collapsed);
}

void UMythRoHUDWidget::ToggleMainMenu()
{
	if (!MainMenuPanel)
	{
		return;
	}
	MainMenuPanel->SetVisibility(MainMenuPanel->IsVisible() ? ESlateVisibility::Collapsed : ESlateVisibility::SelfHitTestInvisible);
}

void UMythRoHUDWidget::TestMainMenu(int32 Index)
{
	if (Index < 0)
	{
		ToggleMainMenu();
		return;
	}
	if (MainMenuPanel && !MainMenuPanel->IsVisible())
	{
		ToggleMainMenu();
	}
	MainMenuPick(Index);
}

void UMythRoHUDWidget::MainMenuPick(int32 i)
{
	if (!MainMenuPanel || i < 0 || i >= UE_ARRAY_COUNT(HubEntries))
	{
		return;
	}
	if (HubEntries[i].bGameMenu) { ToggleGameMenu(); }
	else
	{
		// Abre sem fechar: se a janela ja esta aberta, o clique no menu nao a esconde
		UCanvasPanel* Target = nullptr;
		switch (HubEntries[i].Window)
		{
		case EMythRoWindow::Status: Target = StatusWindow; break;
		case EMythRoWindow::Items: Target = ItemsWindow; break;
		case EMythRoWindow::Equip: Target = EquipWindow; break;
		case EMythRoWindow::Skills: Target = SkillsWindow; break;
		case EMythRoWindow::Party: Target = PartyWindow; break;
		case EMythRoWindow::Quests: Target = QuestWindow; break;
		case EMythRoWindow::Minimap: Target = MinimapWindow; break;
		default: break;
		}
		if (!Target || !Target->IsVisible())
		{
			ToggleWindow(HubEntries[i].Window);
		}
	}
	if (!bMainMenuKeepOpen)
	{
		MainMenuPanel->SetVisibility(ESlateVisibility::Collapsed);
	}
}

bool UMythRoHUDWidget::HandleMainMenuClick(const FVector2D& ScreenPos)
{
	if (!MainMenuPanel)
	{
		return false;
	}
	if (IsUnder(MainMenuButtonBack, ScreenPos))
	{
		ToggleMainMenu();
		return true;
	}
	if (!MainMenuPanel->IsVisible())
	{
		return false;
	}
	if (IsUnder(MainMenuClose, ScreenPos))
	{
		MainMenuPanel->SetVisibility(ESlateVisibility::Collapsed);
		return true;
	}
	if (IsUnder(MainMenuPinBox, ScreenPos))
	{
		bMainMenuKeepOpen = !bMainMenuKeepOpen;
		MainMenuPinMark->SetVisibility(bMainMenuKeepOpen ? ESlateVisibility::HitTestInvisible : ESlateVisibility::Collapsed);
		return true;
	}
	for (int32 i = 0; i < MainMenuCells.Num() && i < UE_ARRAY_COUNT(HubEntries); ++i)
	{
		if (!IsUnder(MainMenuCells[i], ScreenPos))
		{
			continue;
		}
		MainMenuPick(i);
		return true;
	}
	return IsUnder(MainMenuPanel, ScreenPos); // clique no vazio do painel: nao anda o personagem
}

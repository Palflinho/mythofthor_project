// Configuracoes do jogo numa janela so, como o "Game settings" do cliente moderno: a mesma faixa de 5 abas (Padrao, Efeito,
// Controle, Graficos, Outros) em cima das tres janelas de opcoes (som e interface, opcoes do jogo, graficos); trocar de aba
// troca de janela no mesmo lugar.

#include "UI/MythRoHUDWidget.h"
#include "UI/MythRoUITextures.h"

#include "Components/CanvasPanel.h"
#include "Components/CanvasPanelSlot.h"
#include "Components/Image.h"
#include "Components/TextBlock.h"

namespace
{
	constexpr int32 StTabs = 5;
	const TCHAR* StNames[StTabs] = { TEXT("Padrão"), TEXT("Efeito"), TEXT("Controle"), TEXT("Gráficos"), TEXT("Outros") };
	const FLinearColor StBlue(0.12f, 0.3f, 0.65f);
}

void UMythRoHUDWidget::AddSettingsTabs(UCanvasPanel* Win)
{
	for (int32 t = 0; t < StTabs; ++t)
	{
		UImage* Box = AddRounded(Win, FLinearColor(0.8f, 0.83f, 0.9f, 1.f), FLinearColor(0.5f, 0.55f, 0.7f, 1.f), 6.f + t * 90.f, 19.f, 88.f, 18.f, 4.f, 0.f);
		Box->SetVisibility(ESlateVisibility::Visible);
		UTextBlock* Label = AddText(Win, 6.f + t * 90.f + 8.f, 21.f, 8, StBlue);
		Label->SetText(FText::FromString(StNames[t]));
		SettingsTabs.Add({ Win, Box, Label, t });
	}
}

int32 UMythRoHUDWidget::ActiveSettingsTab() const
{
	if (SoundWindow && SoundWindow->IsVisible()) { return 0; }
	if (GraphicsWindow && GraphicsWindow->IsVisible()) { return 3; }
	return OptTab == 0 ? 1 : (OptTab == 1 ? 2 : 4);
}

void UMythRoHUDWidget::UpdateSettingsTabs()
{
	const int32 Active = ActiveSettingsTab();
	for (const FSettingsTab& T : SettingsTabs)
	{
		const bool bOn = T.Index == Active;
		if (T.Box) { T.Box->SetColorAndOpacity(bOn ? FLinearColor::White : FLinearColor(0.8f, 0.83f, 0.9f, 1.f)); }
		if (T.Label) { T.Label->SetColorAndOpacity(FSlateColor(bOn ? FLinearColor(0.08f, 0.1f, 0.18f) : StBlue)); }
	}
}

bool UMythRoHUDWidget::HandleSettingsTab(const FVector2D& ScreenPos)
{
	for (const FSettingsTab& T : SettingsTabs)
	{
		if (!T.Window || !T.Window->IsVisible() || !IsUnder(T.Box, ScreenPos))
		{
			continue;
		}
		if (T.Index == 1 || T.Index == 2 || T.Index == 4)
		{
			ShowOptionsWindow(T.Index == 1 ? 0 : (T.Index == 2 ? 1 : 2));
			UpdateSettingsTabs();
			return true;
		}
		UCanvasPanel* Dest = T.Index == 0 ? SoundWindow.Get() : GraphicsWindow.Get();
		if (OptionsWindow) { OptionsWindow->SetVisibility(ESlateVisibility::Collapsed); }
		if (SoundWindow) { SoundWindow->SetVisibility(ESlateVisibility::Collapsed); }
		if (GraphicsWindow) { GraphicsWindow->SetVisibility(ESlateVisibility::Collapsed); }
		if (T.Index == 0) { RefreshSoundWindow(); } else { RefreshGraphicsWindow(); }
		Dest->SetVisibility(ESlateVisibility::SelfHitTestInvisible);
		OpenOrder.Remove(Dest);
		OpenOrder.Add(Dest);
		UpdateSettingsTabs();
		return true;
	}
	return false;
}

// Lista de emocoes (Alt+L) e lista de atalhos de emocao (Alt+M) do cliente LATAM (docs/ref/hud_latam/INDICE.md):
// grade de 5 x 6 baloes em 3 paginas (clicar manda /eN) e a tabela Alt+1..Alt+0 com a macro de cada um.

#include "UI/MythRoHUDWidget.h"

#include "Sprites/MythEmotion.h"

#include "Components/CanvasPanel.h"
#include "Components/Image.h"
#include "Components/TextBlock.h"
#include "Engine/Texture2D.h"

namespace
{
	const FLinearColor EmoText(0.12f, 0.12f, 0.12f);
	const FLinearColor EmoLink(0.12f, 0.3f, 0.65f);
	const FLinearColor EmoGray(0.6f, 0.6f, 0.6f);
	constexpr float EmoW = 252.f;
	constexpr float EmoCell = 46.f;
	constexpr float EmoGridX = 6.f;
	constexpr float EmoGridY = 24.f;

	// Macros padrao do cliente para Alt+1..Alt+0 (print "Shortcut List")
	const TCHAR* DefaultMacros[10] = { TEXT("/!"), TEXT("/?"), TEXT("/ho"), TEXT("/lv"), TEXT("/swt"), TEXT("/ic"), TEXT("/an"), TEXT("/ag"), TEXT("/$"), TEXT("/..") };
}

void UMythRoHUDWidget::BuildEmotionWindows()
{
	const float H = EmoGridY + 6 * EmoCell + 40.f;
	EmotionWindow = MakeRoPanel(TEXT("Emotion list"), FVector2D(EmoW, H), FVector2D(-300.f, 0.f));
	for (int32 i = 0; i < EmotionCellCount; ++i)
	{
		const float X = EmoGridX + (i % 5) * EmoCell;
		const float Y = EmoGridY + (i / 5) * EmoCell;
		UImage* Icon = AddImage(EmotionWindow, nullptr, X + 2.f, Y + 2.f, 42.f, 42.f);
		Icon->SetVisibility(ESlateVisibility::Collapsed);
		EmotionIcons.Add(Icon);
	}
	EmotionPrev = AddButtonHit(EmotionWindow, TEXT("back"), 22.f, H - 28.f);
	EmotionPageText = AddText(EmotionWindow, EmoW * 0.5f - 12.f, H - 24.f, 9, EmoText);
	EmotionNext = AddButtonHit(EmotionWindow, TEXT("next"), EmoW - 64.f, H - 28.f);

	MacroWindow = MakeRoPanel(TEXT("Shortcut list"), FVector2D(340.f, 17.f + 10 * 20.f + 38.f), FVector2D(-300.f, 0.f));
	for (int32 i = 0; i < 10; ++i)
	{
		const float Y = 22.f + i * 20.f;
		AddText(MacroWindow, 14.f, Y, 8, EmoText)->SetText(FText::FromString(FString::Printf(TEXT("Alt + %d"), (i + 1) % 10)));
		AddColor(MacroWindow, FLinearColor(0.93f, 0.93f, 0.93f, 1.f), 80.f, Y - 1.f, 250.f, 17.f);
		AddText(MacroWindow, 86.f, Y, 8, EmoText)->SetText(FText::FromString(DefaultMacros[i]));
	}
}

void UMythRoHUDWidget::RefreshEmotionList()
{
	if (!EmotionWindow || !EmotionWindow->IsVisible())
	{
		return;
	}
	const int32 Pages = (64 + EmotionCellCount - 1) / EmotionCellCount;
	EmotionPage = FMath::Clamp(EmotionPage, 0, Pages - 1);
	for (int32 i = 0; i < EmotionCellCount; ++i)
	{
		const int32 Order = EmotionPage * EmotionCellCount + i + 1;
		UTexture2D* Tex = Order <= 64 ? AMythEmotion::IconFor(AMythEmotion::TypeFromOrder(Order)) : nullptr;
		if (Tex)
		{
			EmotionIcons[i]->SetBrushFromTexture(Tex);
		}
		EmotionIcons[i]->SetVisibility(Tex ? ESlateVisibility::Visible : ESlateVisibility::Collapsed);
	}
	EmotionPageText->SetText(FText::FromString(FString::Printf(TEXT("%d / %d"), EmotionPage + 1, Pages)));
	EmotionPrev->SetColorAndOpacity(FSlateColor(EmotionPage > 0 ? EmoLink : EmoGray));
	EmotionNext->SetColorAndOpacity(FSlateColor(EmotionPage < Pages - 1 ? EmoLink : EmoGray));
}

bool UMythRoHUDWidget::HandleEmotionClick(const FVector2D& ScreenPos)
{
	if (!EmotionWindow || !EmotionWindow->IsVisible())
	{
		return false;
	}
	const int32 Pages = (64 + EmotionCellCount - 1) / EmotionCellCount;
	if (IsUnder(EmotionPrev, ScreenPos)) { --EmotionPage; RefreshEmotionList(); return true; }
	if (IsUnder(EmotionNext, ScreenPos) && EmotionPage < Pages - 1) { ++EmotionPage; RefreshEmotionList(); return true; }
	for (int32 i = 0; i < EmotionCellCount; ++i)
	{
		if (IsUnder(EmotionIcons[i], ScreenPos))
		{
			SubmitChat(FString::Printf(TEXT("/e%d"), EmotionPage * EmotionCellCount + i + 1));
			return true;
		}
	}
	return false;
}

// Janela de dialogo de NPC do UMythRoHUDWidget, como no cliente original: caixa branca com as falas (mes),
// botao "proximo" (next;), botao "fechar" (close;) e caixa de opcoes (select). Tudo vem do script do rAthena;
// o cliente so mostra e devolve CZ_REQ_NEXT_SCRIPT / CZ_CHOOSE_MENU / CZ_CLOSE_DIALOG.

#include "UI/MythRoHUDWidget.h"

#include "UI/MythRoUITextures.h"
#include "Network/RagnarokNetworkSubsystem.h"

#include "Blueprint/WidgetTree.h"
#include "Components/CanvasPanel.h"
#include "Components/CanvasPanelSlot.h"
#include "Components/Image.h"
#include "Components/TextBlock.h"
#include "Components/WrapBox.h"
#include "Components/WrapBoxSlot.h"
#include "Styling/CoreStyle.h"
#include "Brushes/SlateRoundedBoxBrush.h"

namespace
{
	const FLinearColor NpcBody(0.985f, 0.985f, 0.99f, 0.96f);
	const FLinearColor NpcEdge(0.47f, 0.52f, 0.62f, 1.f);
	const FLinearColor NpcRowHover(0.78f, 0.86f, 0.98f, 1.f);
	// UISayDialogWnd do cliente (docs/ref/06 3.9): 360 x 160 com cantos de raio 10. Os botoes (btn_next/close/ok/
	// cancel.bmp) ficam no tamanho nativo de 42 x 20 para nao borrar
	constexpr float NpcW = 360.f;
	constexpr float NpcH = 160.f;
	constexpr float NpcRadius = 10.f;
	constexpr float NpcBtnW = 42.f;
	constexpr float NpcBtnH = 20.f;
	constexpr int32 NpcFontSize = 9;
	constexpr float NpcRowH = 18.f;
	constexpr int32 NpcMaxMenuRows = 10;

	/** Caixa branca de cantos arredondados com borda, desenhada como no cliente (sem textura). */
	void MakeRounded(UImage* Img, float Scale)
	{
		if (Img)
		{
			Img->SetColorAndOpacity(FLinearColor::White);
			Img->SetBrush(FSlateRoundedBoxBrush(NpcBody, NpcRadius * Scale, NpcEdge, 1.f));
		}
	}

	FSlateBrush NpcBrush(UTexture2D* Tex, const FVector2D& Size)
	{
		FSlateBrush Brush;
		Brush.SetResourceObject(Tex);
		Brush.ImageSize = Size;
		Brush.DrawAs = ESlateBrushDrawType::Image;
		return Brush;
	}

	/** Divide uma fala nas cores do RO: "^RRGGBB" troca a cor do texto seguinte. */
	void NpcColorSegments(const FString& Text, TArray<TPair<FString, FLinearColor>>& Out)
	{
		FLinearColor Color(0.f, 0.f, 0.f);
		FString Cur;
		for (int32 i = 0; i < Text.Len(); ++i)
		{
			if (Text[i] == TEXT('^') && i + 6 < Text.Len())
			{
				const FString Hex = Text.Mid(i + 1, 6);
				bool bHex = true;
				for (TCHAR C : Hex)
				{
					bHex &= FChar::IsHexDigit(C);
				}
				if (bHex)
				{
					if (!Cur.IsEmpty())
					{
						Out.Add({ Cur, Color });
						Cur.Empty();
					}
					Color = FLinearColor(FColor::FromHex(Hex));
					i += 6;
					continue;
				}
			}
			Cur.AppendChar(Text[i]);
		}
		if (!Cur.IsEmpty())
		{
			Out.Add({ Cur, Color });
		}
	}
}

void UMythRoHUDWidget::BuildNpcDialog()
{
	// Caixa de fala centralizada na tela, um pouco acima do personagem, como o cliente original
	NpcWindow = MakeWindow(FVector2D(0.5f, 0.5f), FVector2D(0.5f, 0.5f), FVector2D(0.f, -170.f), FVector2D(NpcW, NpcH));
	MakeRounded(AddColor(NpcWindow, NpcBody, 0.f, 0.f, NpcW, NpcH), UIScale);
	NpcText = WidgetTree->ConstructWidget<UWrapBox>();
	NpcText->SetInnerSlotPadding(FVector2D(0.f, 1.f));
	NpcText->SetVisibility(ESlateVisibility::HitTestInvisible);
	if (UCanvasPanelSlot* S = NpcWindow->AddChildToCanvas(NpcText))
	{
		S->SetPosition(FVector2D(12.f, 10.f) * UIScale);
		S->SetSize(FVector2D(NpcW - 24.f, NpcH - NpcBtnH - 20.f) * UIScale);
	}
	NpcText->SetWrapSize((NpcW - 24.f) * UIScale);
	NpcText->SetExplicitWrapSize(true);
	NpcNextButton = AddImage(NpcWindow, FMythRoUITextures::Get(TEXT("btn_next.bmp")), NpcW - NpcBtnW - 8.f, NpcH - NpcBtnH - 6.f, NpcBtnW, NpcBtnH);
	NpcCloseBtn = AddImage(NpcWindow, FMythRoUITextures::Get(TEXT("btn_close.bmp")), NpcW - NpcBtnW - 8.f, NpcH - NpcBtnH - 6.f, NpcBtnW, NpcBtnH);
	NpcNextButton->SetVisibility(ESlateVisibility::Collapsed);
	NpcCloseBtn->SetVisibility(ESlateVisibility::Collapsed);
	NpcWindow->SetVisibility(ESlateVisibility::Collapsed);

	// Caixa de opcoes (select): logo abaixo da fala
	const float MenuH = 10.f + NpcMaxMenuRows * NpcRowH + NpcBtnH + 14.f;
	NpcMenuWindow = MakeWindow(FVector2D(0.5f, 0.5f), FVector2D(0.5f, 0.f), FVector2D(0.f, -170.f + NpcH * 0.5f * UIScale + 6.f), FVector2D(NpcW, MenuH));
	NpcMenuEdge = AddColor(NpcMenuWindow, NpcBody, 0.f, 0.f, NpcW, MenuH);
	MakeRounded(NpcMenuEdge, UIScale);
	NpcMenuBody = AddColor(NpcMenuWindow, FLinearColor::Transparent, 0.f, 0.f, 1.f, 1.f);
	for (int32 i = 0; i < NpcMaxMenuRows; ++i)
	{
		UImage* Row = AddColor(NpcMenuWindow, FLinearColor::Transparent, 6.f, 6.f + i * NpcRowH, NpcW - 12.f, NpcRowH);
		Row->SetVisibility(ESlateVisibility::Visible); // recebe o clique (nao passa para o jogo)
		NpcMenuRows.Add(Row);
		NpcMenuTexts.Add(AddText(NpcMenuWindow, 14.f, 7.f + i * NpcRowH, NpcFontSize, FLinearColor::Black));
	}
	NpcMenuOk = AddImage(NpcMenuWindow, FMythRoUITextures::Get(TEXT("btn_ok.bmp")), NpcW - 2.f * NpcBtnW - 14.f, MenuH - NpcBtnH - 6.f, NpcBtnW, NpcBtnH);
	NpcMenuCancel = AddImage(NpcMenuWindow, FMythRoUITextures::Get(TEXT("btn_cancel.bmp")), NpcW - NpcBtnW - 8.f, MenuH - NpcBtnH - 6.f, NpcBtnW, NpcBtnH);
	NpcMenuOk->SetVisibility(ESlateVisibility::Visible);
	NpcMenuCancel->SetVisibility(ESlateVisibility::Visible);
	NpcMenuWindow->SetVisibility(ESlateVisibility::Collapsed);
}

void UMythRoHUDWidget::NpcSay(int64 NpcID, const FString& Text)
{
	if (!NpcWindow || !NpcText)
	{
		return;
	}
	if (bNpcClearOnNextSay || NpcTalking != NpcID)
	{
		NpcText->ClearChildren(); // depois do "proximo" a caixa recomeca vazia
		bNpcClearOnNextSay = false;
	}
	NpcTalking = NpcID;
	NpcWindow->SetVisibility(ESlateVisibility::SelfHitTestInvisible);
	NpcNextButton->SetVisibility(ESlateVisibility::Collapsed);
	NpcCloseBtn->SetVisibility(ESlateVisibility::Collapsed);
	if (UCanvasPanelSlot* S = Cast<UCanvasPanelSlot>(NpcWindow->Slot))
	{
		S->SetZOrder(++TopZOrder);
	}

	// Cada linha (mes) ocupa a largura toda; as palavras quebram na largura da caixa e mantem a cor
	TArray<TPair<FString, FLinearColor>> Segments;
	NpcColorSegments(Text, Segments);
	const FSlateFontInfo Font = FMythRoUITextures::RoFont(FMath::RoundToInt(NpcFontSize * UIScale));
	for (const TPair<FString, FLinearColor>& Seg : Segments)
	{
		TArray<FString> Words;
		Seg.Key.ParseIntoArray(Words, TEXT(" "), true);
		for (const FString& W : Words)
		{
			UTextBlock* T = WidgetTree->ConstructWidget<UTextBlock>();
			T->SetFont(Font);
			T->SetColorAndOpacity(FSlateColor(Seg.Value));
			T->SetText(FText::FromString(W + TEXT(" ")));
			NpcText->AddChildToWrapBox(T);
		}
	}
	// Quebra de linha: um bloco vazio com a largura inteira
	UTextBlock* Break = WidgetTree->ConstructWidget<UTextBlock>();
	Break->SetFont(Font);
	if (UWrapBoxSlot* BS = NpcText->AddChildToWrapBox(Break))
	{
		BS->SetFillEmptySpace(true);
	}
}

void UMythRoHUDWidget::NpcWait(int64 NpcID)
{
	if (NpcWindow && NpcTalking == NpcID)
	{
		NpcNextButton->SetVisibility(ESlateVisibility::Visible);
		NpcCloseBtn->SetVisibility(ESlateVisibility::Collapsed);
	}
}

void UMythRoHUDWidget::NpcCloseButton(int64 NpcID)
{
	if (NpcWindow && NpcTalking == NpcID)
	{
		NpcWindow->SetVisibility(ESlateVisibility::SelfHitTestInvisible);
		NpcNextButton->SetVisibility(ESlateVisibility::Collapsed);
		NpcCloseBtn->SetVisibility(ESlateVisibility::Visible);
	}
}

void UMythRoHUDWidget::NpcMenu(int64 NpcID, const TArray<FString>& Options)
{
	if (!NpcMenuWindow)
	{
		return;
	}
	NpcTalking = NpcID;
	NpcMenuIndex.Reset();
	int32 Row = 0;
	for (int32 i = 0; i < Options.Num() && Row < NpcMenuRows.Num(); ++i)
	{
		if (Options[i].TrimStartAndEnd().IsEmpty())
		{
			continue; // item vazio: escondido, mas a numeracao enviada continua a original
		}
		TArray<TPair<FString, FLinearColor>> Segments;
		NpcColorSegments(Options[i], Segments);
		FString Plain;
		for (const TPair<FString, FLinearColor>& S : Segments)
		{
			Plain += S.Key;
		}
		NpcMenuTexts[Row]->SetText(FText::FromString(Plain));
		NpcMenuTexts[Row]->SetColorAndOpacity(FSlateColor(Segments.Num() > 0 ? Segments[0].Value : FLinearColor::Black));
		NpcMenuRows[Row]->SetVisibility(ESlateVisibility::Visible);
		NpcMenuIndex.Add(i + 1);
		++Row;
	}
	for (int32 r = Row; r < NpcMenuRows.Num(); ++r)
	{
		NpcMenuTexts[r]->SetText(FText::GetEmpty());
		NpcMenuRows[r]->SetVisibility(ESlateVisibility::Collapsed);
	}
	// Altura da caixa conforme o numero de opcoes; OK e Cancelar ficam embaixo
	const float MenuH = 10.f + FMath::Max(Row, 1) * NpcRowH + NpcBtnH + 14.f;
	auto Place = [this](UWidget* W, float X, float Y, float Wd, float Ht)
	{
		if (UCanvasPanelSlot* S = W ? Cast<UCanvasPanelSlot>(W->Slot) : nullptr)
		{
			S->SetPosition(FVector2D(X, Y) * UIScale);
			S->SetSize(FVector2D(Wd, Ht) * UIScale);
		}
	};
	if (UCanvasPanelSlot* S = Cast<UCanvasPanelSlot>(NpcMenuWindow->Slot))
	{
		S->SetSize(FVector2D(NpcW, MenuH) * UIScale);
	}
	Place(NpcMenuEdge, 0.f, 0.f, NpcW, MenuH);
	Place(NpcMenuOk, NpcW - 2.f * NpcBtnW - 14.f, MenuH - NpcBtnH - 6.f, NpcBtnW, NpcBtnH);
	Place(NpcMenuCancel, NpcW - NpcBtnW - 8.f, MenuH - NpcBtnH - 6.f, NpcBtnW, NpcBtnH);
	NpcMenuSelected = 0;
	RefreshNpcMenuHighlight();
	NpcMenuWindow->SetVisibility(ESlateVisibility::SelfHitTestInvisible);
	if (UCanvasPanelSlot* S = Cast<UCanvasPanelSlot>(NpcMenuWindow->Slot))
	{
		S->SetZOrder(++TopZOrder);
	}
}

void UMythRoHUDWidget::RefreshNpcMenuHighlight()
{
	for (int32 r = 0; r < NpcMenuRows.Num(); ++r)
	{
		NpcMenuRows[r]->SetColorAndOpacity(r == NpcMenuSelected && NpcMenuIndex.IsValidIndex(r) ? NpcRowHover : FLinearColor::Transparent);
	}
}

void UMythRoHUDWidget::HideNpcDialog()
{
	if (NpcWindow)
	{
		NpcWindow->SetVisibility(ESlateVisibility::Collapsed);
		NpcText->ClearChildren();
	}
	if (NpcMenuWindow)
	{
		NpcMenuWindow->SetVisibility(ESlateVisibility::Collapsed);
	}
	NpcTalking = 0;
	bNpcClearOnNextSay = false;
}

bool UMythRoHUDWidget::NpcDialogConfirm()
{
	URagnarokNetworkSubsystem* Net = GetNet();
	if (!Net || NpcTalking == 0)
	{
		return false;
	}
	if (NpcMenuWindow && NpcMenuWindow->IsVisible() && NpcMenuIndex.IsValidIndex(NpcMenuSelected))
	{
		Net->SelectNpcMenu(NpcTalking, NpcMenuIndex[NpcMenuSelected]);
		NpcMenuWindow->SetVisibility(ESlateVisibility::Collapsed);
		bNpcClearOnNextSay = true;
		return true;
	}
	if (NpcNextButton && NpcNextButton->IsVisible())
	{
		Net->NextNpcDialog(NpcTalking);
		NpcNextButton->SetVisibility(ESlateVisibility::Collapsed);
		bNpcClearOnNextSay = true;
		return true;
	}
	if (NpcCloseBtn && NpcCloseBtn->IsVisible())
	{
		Net->CloseNpcDialog(NpcTalking);
		HideNpcDialog();
		return true;
	}
	return false;
}

bool UMythRoHUDWidget::HandleNpcClick(const FVector2D& ScreenPos, bool bDouble)
{
	URagnarokNetworkSubsystem* Net = GetNet();
	if (!Net || NpcTalking == 0)
	{
		return false;
	}
	if (NpcMenuWindow && NpcMenuWindow->IsVisible())
	{
		for (int32 r = 0; r < NpcMenuIndex.Num(); ++r)
		{
			if (IsUnder(NpcMenuRows[r], ScreenPos))
			{
				NpcMenuSelected = r;
				RefreshNpcMenuHighlight();
				if (bDouble)
				{
					NpcDialogConfirm(); // duplo clique escolhe, como no cliente original
				}
				return true;
			}
		}
		if (IsUnder(NpcMenuOk, ScreenPos))
		{
			NpcDialogConfirm();
			return true;
		}
		if (IsUnder(NpcMenuCancel, ScreenPos))
		{
			Net->SelectNpcMenu(NpcTalking, 255); // 255 = cancelar (o script trata como fim)
			HideNpcDialog();
			return true;
		}
	}
	if ((IsUnder(NpcNextButton, ScreenPos) || IsUnder(NpcCloseBtn, ScreenPos)))
	{
		NpcDialogConfirm();
		return true;
	}
	return false;
}

bool UMythRoHUDWidget::IsNpcDialogOpen() const
{
	return NpcTalking != 0;
}

bool UMythRoHUDWidget::NpcMenuChoose(int32 Row)
{
	if (!NpcMenuWindow || !NpcMenuWindow->IsVisible() || !NpcMenuIndex.IsValidIndex(Row))
	{
		return false;
	}
	NpcMenuSelected = Row;
	RefreshNpcMenuHighlight();
	return NpcDialogConfirm();
}

bool UMythRoHUDWidget::NpcMenuMove(int32 Delta)
{
	if (!NpcMenuWindow || !NpcMenuWindow->IsVisible() || NpcMenuIndex.Num() == 0)
	{
		return false;
	}
	NpcMenuSelected = (NpcMenuSelected + Delta + NpcMenuIndex.Num()) % NpcMenuIndex.Num();
	RefreshNpcMenuHighlight();
	return true;
}

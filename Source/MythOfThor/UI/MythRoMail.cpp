// Correio (RODEX, Alt+R, docs/ref/hud_latam/INDICE.md "RODEX"), no formato do cliente moderno: janela de lista bg_rodex_list com o
// atualizar e o escrever no topo, cartas com o icone de estado, titulo e remetente, e a janela de leitura bg_rodex_read. Quem entrega,
// guarda e valida e o servidor (pacotes 0x0ac2, 0x0b63, 0x09ed, 0x0a12, 0x09f6, 0x09f2, 0x09f4, 0x09e7; pedidos 0x0ac0, 0x09ea,
// 0x0a08, 0x0a6e, 0x09f5, 0x09f1, 0x09f3). Anexar itens ou zeny ao escrever fica para depois.

#include "UI/MythRoHUDWidget.h"
#include "UI/MythRoUITextures.h"

#include "Network/RagnarokNetworkSubsystem.h"

#include "Blueprint/WidgetTree.h"
#include "Components/CanvasPanel.h"
#include "Components/CanvasPanelSlot.h"
#include "Components/EditableTextBox.h"
#include "Components/Image.h"
#include "Components/TextBlock.h"

namespace
{
	const FLinearColor MlText(0.12f, 0.12f, 0.12f);
	const FLinearColor MlBlue(0.12f, 0.3f, 0.65f);
	const FLinearColor MlGray(0.55f, 0.55f, 0.55f);
	constexpr float MlW = 309.f;
	constexpr float MlH = 416.f;
	constexpr float MlRowY = 124.f;
	constexpr float MlRowH = 49.f;
	constexpr int32 MlRows = 5;
	constexpr int32 MlBodyLines = 8;
	constexpr int32 MlLineChars = 40;
	const TCHAR* MlArt = TEXT("basic_interface/rodexsystem/renewal/");

	FString MlPath(const TCHAR* Name) { return FString(MlArt) + Name + TEXT(".bmp"); }

	FSlateBrush MlBrush(const FString& Path, const FVector2D& Size)
	{
		FSlateBrush B;
		B.SetResourceObject(FMythRoUITextures::Get(Path));
		B.ImageSize = Size;
		B.DrawAs = ESlateBrushDrawType::Image;
		return B;
	}

	FString MlShort(const FString& S, int32 Max) { return S.Len() > Max ? S.Left(Max - 1) + TEXT("…") : S; }
}

void UMythRoHUDWidget::BuildMailWindows()
{
	// Lista
	MailWindow = MakeWindow(FVector2D(0.5f, 0.5f), FVector2D(0.5f, 0.5f), FVector2D(-100.f * UIScale, -30.f * UIScale), FVector2D(MlW, MlH));
	MailWindow->SetVisibility(ESlateVisibility::Collapsed);
	UImage* Back = AddImage(MailWindow, FMythRoUITextures::Get(MlPath(TEXT("bg_rodex_list"))), 0.f, 0.f, MlW, MlH);
	Back->SetVisibility(ESlateVisibility::Visible);
	MailHeader = AddText(MailWindow, 252.f, 56.f, 7, MlBlue);
	MailRefresh = AddButtonEx(MailWindow, MlPath(TEXT("btn_refresh_out")), MlPath(TEXT("btn_refresh_over")), MlPath(TEXT("btn_refresh_press")), 250.f, 28.f);
	MailWrite = AddButtonEx(MailWindow, MlPath(TEXT("btn_write_out")), MlPath(TEXT("btn_write_over")), MlPath(TEXT("btn_write_press")), 276.f, 28.f);
	for (int32 i = 0; i < MlRows; ++i)
	{
		const float Y = MlRowY + i * MlRowH;
		UImage* Hit = AddColor(MailWindow, FLinearColor::Transparent, 8.f, Y, MlW - 16.f, MlRowH - 2.f);
		Hit->SetVisibility(ESlateVisibility::Visible);
		MailRowHits.Add(Hit);
		MailRowIcons.Add(AddImage(MailWindow, FMythRoUITextures::Get(MlPath(TEXT("icon_status_mail_received"))), 14.f, Y + 8.f, 24.f, 24.f));
		MailRows.Add(AddText(MailWindow, 46.f, Y + 6.f, 8, MlText, true));
		MailSenders.Add(AddText(MailWindow, 46.f, Y + 24.f, 7, MlBlue));
		MailRowGifts.Add(AddImage(MailWindow, FMythRoUITextures::Get(MlPath(TEXT("icon_item"))), 244.f, Y + 6.f, 28.f, 25.f));
	}
	MailPrev = AddButtonEx(MailWindow, MlPath(TEXT("btn_prev_out")), MlPath(TEXT("btn_prev_over")), MlPath(TEXT("btn_prev_press")), 254.f, 384.f);
	MailNext = AddButtonEx(MailWindow, MlPath(TEXT("btn_next_out")), MlPath(TEXT("btn_next_over")), MlPath(TEXT("btn_next_press")), 278.f, 384.f);
	MailDelete = AddTextButton(MailWindow, TEXT("Excluir"), 10.f, 386.f, 70.f);
	MailClose = AddButtonHit(MailWindow, TEXT("close"), 96.f, 386.f);
	MakeDraggable(MailWindow, MlW, 22.f);

	// Leitura
	MailReadWindow = MakeWindow(FVector2D(0.5f, 0.5f), FVector2D(0.5f, 0.5f), FVector2D(212.f * UIScale, -30.f * UIScale), FVector2D(300.f, 400.f));
	MailReadWindow->SetVisibility(ESlateVisibility::Collapsed);
	UImage* ReadBack = AddImage(MailReadWindow, FMythRoUITextures::Get(MlPath(TEXT("bg_rodex_read"))), 0.f, 0.f, 300.f, 400.f);
	ReadBack->SetVisibility(ESlateVisibility::Visible);
	MailFrom = AddText(MailReadWindow, 30.f, 46.f, 8, MlBlue, true);
	for (int32 i = 0; i < MlBodyLines; ++i)
	{
		MailBody.Add(AddText(MailReadWindow, 30.f, 130.f + i * 20.f, 8, MlText));
	}
	MailTakeZeny = AddText(MailReadWindow, 30.f, 330.f, 8, MlBlue);
	MailTakeItems = AddText(MailReadWindow, 150.f, 330.f, 8, MlBlue);
	MailReadClose = AddButtonHit(MailReadWindow, TEXT("close"), 230.f, 364.f);
	MakeDraggable(MailReadWindow, 300.f, 22.f);

	// Escrever
	MailWriteWindow = MakeRoPanel(TEXT("Escrever correio"), FVector2D(340.f, 17.f + 150.f), FVector2D(60.f, -60.f));
	AddText(MailWriteWindow, 12.f, 26.f, 8, MlText)->SetText(FText::FromString(TEXT("Para")));
	AddText(MailWriteWindow, 12.f, 56.f, 8, MlText)->SetText(FText::FromString(TEXT("Título")));
	AddText(MailWriteWindow, 12.f, 86.f, 8, MlText)->SetText(FText::FromString(TEXT("Texto")));
	for (int32 i = 0; i < 3; ++i)
	{
		UEditableTextBox* Box = WidgetTree->ConstructWidget<UEditableTextBox>();
		if (UCanvasPanelSlot* S = MailWriteWindow->AddChildToCanvas(Box))
		{
			S->SetPosition(FVector2D(60.f, 24.f + i * 30.f) * UIScale);
			S->SetSize(FVector2D(268.f, 20.f) * UIScale);
		}
		(i == 0 ? MailToBox : (i == 1 ? MailTitleBox : MailTextBox)) = Box;
	}
	MailSend = AddTextButton(MailWriteWindow, TEXT("Enviar"), 80.f, 17.f + 116.f, 80.f);
	MailCancel = AddButtonHit(MailWriteWindow, TEXT("cancel"), 190.f, 17.f + 116.f);
}

void UMythRoHUDWidget::OpenMailWindow()
{
	if (!MailWindow)
	{
		return;
	}
	const bool bOpen = MailWindow->GetVisibility() == ESlateVisibility::Collapsed;
	MailWindow->SetVisibility(bOpen ? ESlateVisibility::SelfHitTestInvisible : ESlateVisibility::Collapsed);
	if (!bOpen)
	{
		MailReadWindow->SetVisibility(ESlateVisibility::Collapsed);
		if (URagnarokNetworkSubsystem* CloseNet = GetNet()) { CloseNet->ClearOpenMail(); }
	}
	if (bOpen)
	{
		OpenOrder.Remove(MailWindow);
		OpenOrder.Add(MailWindow);
		MailPage = 0;
		if (URagnarokNetworkSubsystem* Net = GetNet())
		{
			Net->SendMailRequest(0x0ac0, 0, 0); // abre a caixa: o servidor manda a lista
		}
	}
	RefreshMail();
}

void UMythRoHUDWidget::RefreshMail()
{
	const URagnarokNetworkSubsystem* Net = GetNet();
	if (!MailWindow || !Net || !MailWindow->IsVisible())
	{
		return;
	}
	const TArray<FRoMail>& List = Net->GetMails();
	MailPage = FMath::Clamp(MailPage, 0, FMath::Max(0, (List.Num() - 1) / MlRows));
	MailHeader->SetText(FText::FromString(FString::Printf(TEXT("%d cartas"), List.Num())));
	for (int32 i = 0; i < MlRows; ++i)
	{
		const FRoMail* M = List.IsValidIndex(MailPage * MlRows + i) ? &List[MailPage * MlRows + i] : nullptr;
		MailRowHits[i]->SetVisibility(M ? ESlateVisibility::Visible : ESlateVisibility::Collapsed);
		MailRowIcons[i]->SetVisibility(M ? ESlateVisibility::HitTestInvisible : ESlateVisibility::Collapsed);
		MailRows[i]->SetVisibility(M ? ESlateVisibility::HitTestInvisible : ESlateVisibility::Collapsed);
		MailSenders[i]->SetVisibility(M ? ESlateVisibility::HitTestInvisible : ESlateVisibility::Collapsed);
		MailRowGifts[i]->SetVisibility(M && (M->Kind & 0x6) ? ESlateVisibility::HitTestInvisible : ESlateVisibility::Collapsed);
		if (!M)
		{
			continue;
		}
		MailRowIcons[i]->SetBrush(MlBrush(MlPath(M->bRead ? TEXT("icon_status_mail_read") : TEXT("icon_status_mail_received_new")), FVector2D(24.f, 24.f) * UIScale));
		MailRows[i]->SetText(FText::FromString(MlShort(M->Title, 26)));
		MailRows[i]->SetColorAndOpacity(FSlateColor(M->Id == Net->GetOpenMailId() ? MlBlue : MlText));
		MailSenders[i]->SetText(FText::FromString(M->Sender));
	}
	// Leitura
	const bool bReading = Net->GetOpenMailId() != 0;
	MailReadWindow->SetVisibility(bReading ? ESlateVisibility::SelfHitTestInvisible : ESlateVisibility::Collapsed);
	if (!bReading)
	{
		return;
	}
	OpenOrder.Remove(MailReadWindow);
	OpenOrder.Add(MailReadWindow);
	for (UTextBlock* L : MailBody) { L->SetText(FText::GetEmpty()); }
	MailFrom->SetText(FText::GetEmpty());
	MailTakeZeny->SetText(FText::GetEmpty());
	MailTakeItems->SetText(FText::GetEmpty());
	for (const FRoMail& M : List)
	{
		if (M.Id == Net->GetOpenMailId())
		{
			MailFrom->SetText(FText::FromString(FString::Printf(TEXT("%s  -  %s"), *M.Sender, *MlShort(M.Title, 22))));
		}
	}
	const FString& Text = Net->GetOpenMailText();
	for (int32 Line = 0, Pos = 0; Line < MlBodyLines && Pos < Text.Len(); ++Line)
	{
		MailBody[Line]->SetText(FText::FromString(Text.Mid(Pos, MlLineChars)));
		Pos += MlLineChars;
	}
	if (Net->GetOpenMailZeny() > 0)
	{
		MailTakeZeny->SetText(FText::FromString(FString::Printf(TEXT("[Pegar %s z]"), *FText::AsNumber(Net->GetOpenMailZeny()).ToString())));
	}
	if (Net->GetOpenMailItems() > 0)
	{
		MailTakeItems->SetText(FText::FromString(FString::Printf(TEXT("[Pegar %d itens]"), Net->GetOpenMailItems())));
	}
}

bool UMythRoHUDWidget::HandleMailClick(const FVector2D& ScreenPos)
{
	URagnarokNetworkSubsystem* Net = GetNet();
	if (!Net)
	{
		return false;
	}
	if (MailWriteWindow && MailWriteWindow->IsVisible())
	{
		if (IsUnder(MailSend, ScreenPos))
		{
			const FString To = MailToBox->GetText().ToString().TrimStartAndEnd();
			if (!To.IsEmpty())
			{
				Net->SendMailWrite(To, MailTitleBox->GetText().ToString(), MailTextBox->GetText().ToString());
				MailWriteWindow->SetVisibility(ESlateVisibility::Collapsed);
			}
			return true;
		}
		if (IsUnder(MailCancel, ScreenPos))
		{
			Net->SendMailBeginWrite(FString()); // cancela
			MailWriteWindow->SetVisibility(ESlateVisibility::Collapsed);
			return true;
		}
	}
	if (MailReadWindow && MailReadWindow->IsVisible() && IsUnder(MailReadWindow, ScreenPos))
	{
		const int64 Open = Net->GetOpenMailId();
		uint8 Tab = 0;
		for (const FRoMail& M : Net->GetMails()) { if (M.Id == Open) { Tab = M.Tab; } }
		if (IsUnder(MailReadClose, ScreenPos)) { MailReadWindow->SetVisibility(ESlateVisibility::Collapsed); Net->ClearOpenMail(); return true; }
		if (IsUnder(MailTakeZeny, ScreenPos) && !MailTakeZeny->GetText().IsEmpty()) { Net->SendMailRequest(0x09f1, Open, Tab); return true; }
		if (IsUnder(MailTakeItems, ScreenPos) && !MailTakeItems->GetText().IsEmpty()) { Net->SendMailRequest(0x09f3, Open, Tab); return true; }
		return true;
	}
	if (!MailWindow || !MailWindow->IsVisible() || !IsUnder(MailWindow, ScreenPos))
	{
		return false;
	}
	if (IsUnder(MailClose, ScreenPos)) { OpenMailWindow(); return true; }
	if (IsUnder(MailRefresh, ScreenPos)) { Net->SendMailRequest(0x0ac1, 0, 0); return true; }
	if (IsUnder(MailPrev, ScreenPos)) { --MailPage; RefreshMail(); return true; }
	if (IsUnder(MailNext, ScreenPos)) { ++MailPage; RefreshMail(); return true; }
	for (int32 i = 0; i < MlRows; ++i)
	{
		const int32 Index = MailPage * MlRows + i;
		if (Net->GetMails().IsValidIndex(Index) && IsUnder(MailRowHits[i], ScreenPos))
		{
			Net->SendMailRequest(0x09ea, Net->GetMails()[Index].Id, Net->GetMails()[Index].Tab);
			return true;
		}
	}
	if (IsUnder(MailDelete, ScreenPos) && Net->GetOpenMailId() != 0)
	{
		uint8 Tab = 0;
		for (const FRoMail& M : Net->GetMails()) { if (M.Id == Net->GetOpenMailId()) { Tab = M.Tab; } }
		Net->SendMailRequest(0x09f5, Net->GetOpenMailId(), Tab);
		return true;
	}
	if (IsUnder(MailWrite, ScreenPos))
	{
		Net->SendMailBeginWrite(MailToBox->GetText().ToString().TrimStartAndEnd().IsEmpty() ? FString(TEXT("-")) : MailToBox->GetText().ToString());
		MailWriteWindow->SetVisibility(ESlateVisibility::SelfHitTestInvisible);
		OpenOrder.Remove(MailWriteWindow);
		OpenOrder.Add(MailWriteWindow);
		return true;
	}
	return true;
}

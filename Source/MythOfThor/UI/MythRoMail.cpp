// Correio (RODEX, Alt+R, docs/ref/hud_latam/INDICE.md "RODEX"): caixa de entrada, leitura, pegar zeny e itens, excluir e escrever
// cartas de texto. Quem entrega, guarda e valida e o servidor (pacotes 0x0ac2, 0x09eb, 0x09ed, 0x0a12, 0x09f6, 0x09f2, 0x09f4,
// 0x09e7; pedidos 0x0ac0, 0x09ea, 0x0a08, 0x0a6e, 0x09f5, 0x09f1, 0x09f3). Anexar itens ou zeny ao escrever fica para depois.

#include "UI/MythRoHUDWidget.h"

#include "Network/RagnarokNetworkSubsystem.h"

#include "Blueprint/WidgetTree.h"
#include "Components/CanvasPanel.h"
#include "Components/CanvasPanelSlot.h"
#include "Components/EditableTextBox.h"
#include "Components/TextBlock.h"

namespace
{
	const FLinearColor MlText(0.12f, 0.12f, 0.12f);
	const FLinearColor MlLink(0.12f, 0.3f, 0.65f);
	const FLinearColor MlGray(0.55f, 0.55f, 0.55f);
	constexpr float MlW = 380.f;
	constexpr float MlRowH = 17.f;
	constexpr float MlListY = 44.f;
	constexpr int32 MlRows = 9;
	constexpr int32 MlBodyLines = 6;
	constexpr int32 MlLineChars = 56;
}

void UMythRoHUDWidget::BuildMailWindows()
{
	const float BodyY = MlListY + MlRows * MlRowH + 10.f;
	const float H = BodyY + 22.f + MlBodyLines * MlRowH + 34.f;
	MailWindow = MakeRoPanel(TEXT("Correio"), FVector2D(MlW, H), FVector2D(-100.f, -150.f));
	MailHeader = AddText(MailWindow, 10.f, 22.f, 8, MlGray);
	for (int32 i = 0; i < MlRows; ++i)
	{
		MailRows.Add(AddText(MailWindow, 12.f, MlListY + i * MlRowH, 8, MlText));
	}
	MailFrom = AddText(MailWindow, 12.f, BodyY, 8, MlGray);
	for (int32 i = 0; i < MlBodyLines; ++i)
	{
		MailBody.Add(AddText(MailWindow, 14.f, BodyY + 20.f + i * MlRowH, 8, MlText));
	}
	const float LinkY = H - 26.f;
	MailTakeZeny = AddText(MailWindow, 10.f, LinkY, 8, MlLink);
	MailTakeItems = AddText(MailWindow, 100.f, LinkY, 8, MlLink);
	MailDelete = AddButtonHit(MailWindow, TEXT("del_res"), 200.f, LinkY - 4.f);
	MailWrite = AddButtonHit(MailWindow, TEXT("write"), 280.f, LinkY - 4.f);

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
	if (bOpen)
	{
		OpenOrder.Remove(MailWindow);
		OpenOrder.Add(MailWindow);
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
	int32 Unread = 0;
	for (const FRoMail& M : List) { Unread += M.bRead ? 0 : 1; }
	MailHeader->SetText(FText::FromString(FString::Printf(TEXT("%d cartas, %d não lidas"), List.Num(), Unread)));
	for (int32 i = 0; i < MlRows; ++i)
	{
		if (!List.IsValidIndex(i))
		{
			MailRows[i]->SetText(FText::GetEmpty());
			continue;
		}
		const FRoMail& M = List[i];
		MailRows[i]->SetText(FText::FromString(FString::Printf(TEXT("%s%s  -  %s"), M.bRead ? TEXT("") : TEXT("● "), *M.Title, *M.Sender)));
		MailRows[i]->SetColorAndOpacity(FSlateColor(M.Id == Net->GetOpenMailId() ? MlLink : (M.bRead ? MlGray : MlText)));
	}
	for (UTextBlock* L : MailBody) { L->SetText(FText::GetEmpty()); }
	MailFrom->SetText(FText::GetEmpty());
	MailTakeZeny->SetText(FText::GetEmpty());
	MailTakeItems->SetText(FText::GetEmpty());
	if (Net->GetOpenMailId() != 0)
	{
		for (const FRoMail& M : List)
		{
			if (M.Id == Net->GetOpenMailId())
			{
				MailFrom->SetText(FText::FromString(FString::Printf(TEXT("De %s: %s"), *M.Sender, *M.Title)));
			}
		}
		// Quebra o texto em linhas de MlLineChars caracteres
		const FString& Text = Net->GetOpenMailText();
		for (int32 Line = 0, Pos = 0; Line < MlBodyLines && Pos < Text.Len(); ++Line)
		{
			const FString Chunk = Text.Mid(Pos, MlLineChars);
			MailBody[Line]->SetText(FText::FromString(Chunk));
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
	if (!MailWindow || !MailWindow->IsVisible())
	{
		return false;
	}
	for (int32 i = 0; i < MlRows && i < Net->GetMails().Num(); ++i)
	{
		if (IsUnder(MailRows[i], ScreenPos))
		{
			Net->SendMailRequest(0x09ea, Net->GetMails()[i].Id, Net->GetMails()[i].Tab);
			return true;
		}
	}
	const int64 Open = Net->GetOpenMailId();
	if (Open != 0)
	{
		uint8 Tab = 0;
		for (const FRoMail& M : Net->GetMails()) { if (M.Id == Open) { Tab = M.Tab; } }
		if (IsUnder(MailTakeZeny, ScreenPos) && !MailTakeZeny->GetText().IsEmpty()) { Net->SendMailRequest(0x09f1, Open, Tab); return true; }
		if (IsUnder(MailTakeItems, ScreenPos) && !MailTakeItems->GetText().IsEmpty()) { Net->SendMailRequest(0x09f3, Open, Tab); return true; }
		if (IsUnder(MailDelete, ScreenPos)) { Net->SendMailRequest(0x09f5, Open, Tab); return true; }
	}
	if (IsUnder(MailWrite, ScreenPos))
	{
		Net->SendMailBeginWrite(MailToBox->GetText().ToString().TrimStartAndEnd().IsEmpty() ? FString(TEXT("-")) : MailToBox->GetText().ToString());
		MailWriteWindow->SetVisibility(ESlateVisibility::SelfHitTestInvisible);
		OpenOrder.Remove(MailWriteWindow);
		OpenOrder.Add(MailWriteWindow);
		return true;
	}
	return false;
}

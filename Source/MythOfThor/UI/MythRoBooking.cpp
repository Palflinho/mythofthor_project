// Procura de Grupo (party booking): quem quer um grupo anuncia (nivel e classes), os outros procuram. Janela aberta pelas
// ferramentas "anunciar" e "procurar" da janela do grupo (Alt+Z). Quem aceita e a regra do servidor (rAthena party_booking_*);
// aqui so se pede e se mostra a lista. Clicar num anuncio abre o chat ja com o sussurro para o anunciante.

#include "UI/MythRoHUDWidget.h"

#include "Network/RagnarokNetworkSubsystem.h"
#include "Gameplay/MythStatsComponent.h"
#include "UI/MythRoUITextures.h"

#include "Blueprint/WidgetTree.h"
#include "Components/CanvasPanel.h"
#include "Components/CanvasPanelSlot.h"
#include "Components/EditableTextBox.h"
#include "Components/Image.h"
#include "Components/TextBlock.h"

namespace
{
	constexpr float PbW = 300.f;
	constexpr float PbRowH = 20.f;
	constexpr int32 PbRows = 9;
	constexpr float PbHeadY = 48.f;
	constexpr float PbListY = 62.f;
	constexpr float PbH = PbListY + PbRows * PbRowH + 24.f;
	const FLinearColor PbText(0.08f, 0.10f, 0.18f);
	const FLinearColor PbGray(0.45f, 0.45f, 0.45f);
}

void UMythRoHUDWidget::BuildBookingWindow()
{
	BookingWindow = MakeRoPanel(TEXT("Procura de Grupo"), FVector2D(PbW, PbH), FVector2D(-120.f, 10.f));
	AddText(BookingWindow, 8.f, 25.f, 8, PbText)->SetText(FText::FromString(TEXT("Nível")));
	BookingLevelBox = WidgetTree->ConstructWidget<UEditableTextBox>();
	BookingLevelBox->SetText(FText::FromString(TEXT("1")));
	if (UCanvasPanelSlot* S = BookingWindow->AddChildToCanvas(BookingLevelBox))
	{
		S->SetPosition(FVector2D(44.f, 21.f) * UIScale);
		S->SetSize(FVector2D(46.f, 20.f) * UIScale);
	}
	BookingSearchBtn = AddTextButton(BookingWindow, TEXT("Procurar"), 96.f, 21.f, 62.f);
	BookingRegisterBtn = AddTextButton(BookingWindow, TEXT("Anunciar"), 162.f, 21.f, 62.f);
	BookingDeleteBtn = AddTextButton(BookingWindow, TEXT("Remover"), 228.f, 21.f, 64.f);
	static const TCHAR* Heads[3] = { TEXT("Nome"), TEXT("Nível"), TEXT("Classes") };
	static const float HeadX[3] = { 10.f, 130.f, 172.f };
	for (int32 c = 0; c < 3; ++c)
	{
		AddText(BookingWindow, HeadX[c], PbHeadY, 8, PbGray)->SetText(FText::FromString(Heads[c]));
	}
	for (int32 i = 0; i < PbRows; ++i)
	{
		const float Y = PbListY + i * PbRowH;
		UImage* Back = AddColor(BookingWindow, FLinearColor::Transparent, 4.f, Y, PbW - 8.f, PbRowH - 1.f);
		Back->SetVisibility(ESlateVisibility::Visible);
		BookingRowBacks.Add(Back);
		BookingRowTexts.Add(AddText(BookingWindow, 10.f, Y + 4.f, 8, PbText));
		BookingRowTexts.Add(AddText(BookingWindow, 130.f, Y + 4.f, 8, PbText));
		BookingRowTexts.Add(AddText(BookingWindow, 172.f, Y + 4.f, 8, PbText));
	}
	BookingFooter = AddText(BookingWindow, 8.f, PbH - 20.f, 7, FLinearColor(0.12f, 0.27f, 0.55f));
	BookingFooter->SetText(FText::FromString(TEXT("Clique num anúncio para falar com o jogador.")));
}

void UMythRoHUDWidget::ToggleBookingWindow()
{
	if (!BookingWindow)
	{
		return;
	}
	ToggleWindow(EMythRoWindow::Booking);
	if (BookingWindow->IsVisible())
	{
		if (BookingLevelBox && Stats) { BookingLevelBox->SetText(FText::AsNumber(Stats->GetBaseLevel())); }
		if (URagnarokNetworkSubsystem* Net = GetNet()) { Net->SendPartyBookingSearch(0, 0, -1, 0, 9); }
	}
}

int32 UMythRoHUDWidget::BookingLevel() const
{
	return BookingLevelBox ? FMath::Clamp(FCString::Atoi(*BookingLevelBox->GetText().ToString()), 1, 999) : 1;
}

void UMythRoHUDWidget::UpdateBooking()
{
	URagnarokNetworkSubsystem* Net = GetNet();
	if (!BookingWindow || !Net || Net->GetBookingRevision() == BookingRevisionSeen)
	{
		return;
	}
	if (!BookingWindow->IsVisible())
	{
		return; // a resposta espera a janela abrir
	}
	BookingRevisionSeen = Net->GetBookingRevision();
	const int32 R = Net->GetBookingLastResult();
	if (R == 0) { AddChatLine(TEXT("Anúncio de grupo criado."), FLinearColor(0.7f, 1.f, 0.7f)); }
	else if (R == 1) { AddChatLine(TEXT("Não foi possível criar o anúncio."), FLinearColor(1.f, 0.6f, 0.5f)); }
	else if (R == 2) { AddChatLine(TEXT("Você já tem um anúncio de grupo."), FLinearColor(1.f, 0.85f, 0.4f)); }
	else if (R == 10) { AddChatLine(TEXT("Anúncio de grupo removido."), FLinearColor(0.7f, 1.f, 0.7f)); }
	else if (R == 12 || R == 13) { AddChatLine(TEXT("Você não tem anúncio de grupo."), FLinearColor(1.f, 0.85f, 0.4f)); }
	const TArray<FRoPartyBookingAd>& Ads = Net->GetBookingAds();
	for (int32 i = 0; i < PbRows; ++i)
	{
		const FRoPartyBookingAd* Ad = Ads.IsValidIndex(i) ? &Ads[i] : nullptr;
		BookingRowBacks[i]->SetVisibility(Ad ? ESlateVisibility::Visible : ESlateVisibility::Collapsed);
		FString Jobs;
		if (Ad)
		{
			for (int32 j = 0; j < Ad->Jobs.Num() && j < 2; ++j)
			{
				Jobs += (j ? TEXT(", ") : TEXT("")) + URagnarokNetworkSubsystem::GetJobNameFromClass(Ad->Jobs[j]);
			}
			if (Ad->Jobs.Num() > 2) { Jobs += FString::Printf(TEXT(" +%d"), Ad->Jobs.Num() - 2); }
			if (Jobs.IsEmpty()) { Jobs = TEXT("Qualquer"); }
		}
		BookingRowTexts[i * 3 + 0]->SetText(Ad ? FText::FromString(Ad->Name) : FText::GetEmpty());
		BookingRowTexts[i * 3 + 1]->SetText(Ad ? FText::AsNumber(Ad->Level) : FText::GetEmpty());
		BookingRowTexts[i * 3 + 2]->SetText(Ad ? FText::FromString(Jobs) : FText::GetEmpty());
	}
	if (BookingFooter && Ads.Num() == 0 && R == 20)
	{
		BookingFooter->SetText(FText::FromString(TEXT("Nenhum anúncio de grupo no momento.")));
	}
}

bool UMythRoHUDWidget::HandleBookingClick(const FVector2D& ScreenPos)
{
	URagnarokNetworkSubsystem* Net = GetNet();
	if (!BookingWindow || !BookingWindow->IsVisible() || !Net)
	{
		return false;
	}
	if (IsUnder(BookingSearchBtn, ScreenPos))
	{
		Net->SendPartyBookingSearch(0, 0, -1, 0, 9);
		return true;
	}
	if (IsUnder(BookingRegisterBtn, ScreenPos))
	{
		Net->SendPartyBookingRegister(BookingLevel(), 0, TArray<int32>());
		return true;
	}
	if (IsUnder(BookingDeleteBtn, ScreenPos))
	{
		Net->SendPartyBookingDelete();
		return true;
	}
	const TArray<FRoPartyBookingAd>& Ads = Net->GetBookingAds();
	for (int32 i = 0; i < BookingRowBacks.Num() && i < Ads.Num(); ++i)
	{
		if (IsUnder(BookingRowBacks[i], ScreenPos))
		{
			if (ChatInput)
			{
				ChatInput->SetText(FText::FromString(FString::Printf(TEXT("/w \"%s\" "), *Ads[i].Name)));
				FocusChat();
			}
			return true;
		}
	}
	return false;
}

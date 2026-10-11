// Banco (Ctrl+B, docs/ref/hud_latam/INDICE.md "Bank Window"): saldo guardado no servidor, deposito e saque de zeny.
// O limite, a conta e o resultado vem do servidor (pacotes 0x09b7, 0x09a6, 0x09a8, 0x09aa); aqui so se mostra e se pede.

#include "UI/MythRoHUDWidget.h"

#include "Network/RagnarokNetworkSubsystem.h"

#include "Blueprint/WidgetTree.h"
#include "Components/CanvasPanel.h"
#include "Components/CanvasPanelSlot.h"
#include "Components/EditableTextBox.h"
#include "Components/TextBlock.h"
#include "UI/MythRoUITextures.h"

namespace
{
	const FLinearColor BkText(0.12f, 0.12f, 0.12f);
	const FLinearColor BkLinkQ(0.12f, 0.3f, 0.65f);
	const FLinearColor BkLink(0.12f, 0.3f, 0.65f);
	const FLinearColor BkGray(0.55f, 0.55f, 0.55f);
}

void UMythRoHUDWidget::BuildBankWindow()
{
	// Janela do banco (print "Bank Window"): depositado e zeny atual em cima, a faixa cinza com o saco de zeny, a quantia, os botoes
	// Depositar e Sacar e os atalhos de valor (Max, 10M, 1M, 100K)
	BankWindow = MakeRoPanel(TEXT("Banco"), FVector2D(290.f, 17.f + 138.f), FVector2D(-280.f, 60.f));
	AddText(BankWindow, 14.f, 24.f, 8, BkText, true)->SetText(FText::FromString(TEXT("Depositado")));
	AddText(BankWindow, 14.f, 42.f, 8, BkText, true)->SetText(FText::FromString(TEXT("Zeny atual")));
	for (int32 i = 0; i < 2; ++i)
	{
		UTextBlock* Value = AddText(BankWindow, 140.f, 24.f + i * 18.f, 8, BkText);
		Value->SetJustification(ETextJustify::Right);
		if (UCanvasPanelSlot* S = Cast<UCanvasPanelSlot>(Value->Slot))
		{
			S->SetAutoSize(false);
			S->SetSize(FVector2D(136.f, 14.f) * UIScale);
		}
		(i == 0 ? BankBalanceText : BankZenyText) = Value;
	}
	AddColor(BankWindow, FLinearColor(0.88f, 0.9f, 0.95f, 1.f), 0.f, 17.f + 50.f, 290.f, 54.f);
	AddImage(BankWindow, FMythRoUITextures::Get(TEXT("basic_interface/zeny.bmp")), 16.f, 17.f + 60.f, 32.f, 32.f);
	BankAmountBox = WidgetTree->ConstructWidget<UEditableTextBox>();
	if (UCanvasPanelSlot* S = BankWindow->AddChildToCanvas(BankAmountBox))
	{
		S->SetPosition(FVector2D(56.f, 17.f + 66.f) * UIScale);
		S->SetSize(FVector2D(116.f, 20.f) * UIScale);
	}
	BankDepositLink = AddTextButton(BankWindow, TEXT("Depositar"), 188.f, 17.f + 54.f, 90.f);
	BankWithdrawLink = AddTextButton(BankWindow, TEXT("Sacar"), 188.f, 17.f + 80.f, 90.f);
	// Atalhos de valor: Max :: 10M :: 1M :: 100K
	const TCHAR* Quick[4] = { TEXT("Máx"), TEXT("10M"), TEXT("1M"), TEXT("100K") };
	for (int32 i = 0; i < 4; ++i)
	{
		UTextBlock* T = AddText(BankWindow, 52.f + i * 54.f, 17.f + 112.f, 8, BkLink);
		T->SetText(FText::FromString(Quick[i]));
		BankQuick.Add(T);
	}
}

void UMythRoHUDWidget::OpenBankWindow()
{
	if (!BankWindow)
	{
		return;
	}
	const bool bOpen = BankWindow->GetVisibility() == ESlateVisibility::Collapsed;
	BankWindow->SetVisibility(bOpen ? ESlateVisibility::SelfHitTestInvisible : ESlateVisibility::Collapsed);
	if (bOpen)
	{
		OpenOrder.Remove(BankWindow);
		OpenOrder.Add(BankWindow);
		if (URagnarokNetworkSubsystem* Net = GetNet())
		{
			Net->SendBankCommand(0x09b6, 0); // abrir: o servidor responde e pedimos o saldo
		}
	}
	else if (URagnarokNetworkSubsystem* Net = GetNet())
	{
		Net->SendBankCommand(0x09b8, 0);
	}
	RefreshBank();
}

void UMythRoHUDWidget::RefreshBank()
{
	const URagnarokNetworkSubsystem* Net = GetNet();
	if (!BankWindow || !Net || !BankWindow->IsVisible())
	{
		return;
	}
	BankBalanceText->SetText(FText::FromString(FString::Printf(TEXT("%s z"), *FText::AsNumber(Net->GetBankBalance()).ToString())));
	BankZenyText->SetText(FText::FromString(FString::Printf(TEXT("%s z"), *FText::AsNumber(Net->GetPlayerStats().Zeny).ToString())));
}

bool UMythRoHUDWidget::HandleBankClick(const FVector2D& ScreenPos)
{
	URagnarokNetworkSubsystem* Net = GetNet();
	if (!Net || !BankWindow || !BankWindow->IsVisible())
	{
		return false;
	}
	for (int32 i = 0; i < BankQuick.Num(); ++i)
	{
		if (IsUnder(BankQuick[i], ScreenPos))
		{
			const int64 Amount = i == 0 ? Net->GetPlayerStats().Zeny : (i == 1 ? 10000000 : (i == 2 ? 1000000 : 100000));
			BankAmountBox->SetText(FText::AsNumber(FMath::Min<int64>(Amount, MAX_int32)));
			return true;
		}
	}
	const bool bDeposit = IsUnder(BankDepositLink, ScreenPos);
	if (bDeposit || IsUnder(BankWithdrawLink, ScreenPos))
	{
		const int32 Amount = FCString::Atoi(*BankAmountBox->GetText().ToString().Replace(TEXT("."), TEXT("")).Replace(TEXT(","), TEXT("")));
		if (Amount > 0)
		{
			Net->SendBankCommand(bDeposit ? 0x09a7 : 0x09a9, Amount);
		}
		return true;
	}
	return false;
}

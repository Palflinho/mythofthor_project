// Banco (Ctrl+B, docs/ref/hud_latam/INDICE.md "Bank Window"): saldo guardado no servidor, deposito e saque de zeny.
// O limite, a conta e o resultado vem do servidor (pacotes 0x09b7, 0x09a6, 0x09a8, 0x09aa); aqui so se mostra e se pede.

#include "UI/MythRoHUDWidget.h"

#include "Network/RagnarokNetworkSubsystem.h"

#include "Blueprint/WidgetTree.h"
#include "Components/CanvasPanel.h"
#include "Components/CanvasPanelSlot.h"
#include "Components/EditableTextBox.h"
#include "Components/TextBlock.h"

namespace
{
	const FLinearColor BkText(0.12f, 0.12f, 0.12f);
	const FLinearColor BkLink(0.12f, 0.3f, 0.65f);
	const FLinearColor BkGray(0.55f, 0.55f, 0.55f);
}

void UMythRoHUDWidget::BuildBankWindow()
{
	BankWindow = MakeRoPanel(TEXT("Banco"), FVector2D(280.f, 17.f + 140.f), FVector2D(-280.f, 60.f));
	BankBalanceText = AddText(BankWindow, 14.f, 26.f, 9, BkText);
	BankZenyText = AddText(BankWindow, 14.f, 46.f, 9, BkGray);
	AddText(BankWindow, 14.f, 76.f, 9, BkText)->SetText(FText::FromString(TEXT("Quantia")));
	BankAmountBox = WidgetTree->ConstructWidget<UEditableTextBox>();
	if (UCanvasPanelSlot* S = BankWindow->AddChildToCanvas(BankAmountBox))
	{
		S->SetPosition(FVector2D(70.f, 72.f) * UIScale);
		S->SetSize(FVector2D(190.f, 20.f) * UIScale);
	}
	BankDepositLink = AddText(BankWindow, 30.f, 110.f, 10, BkLink);
	BankDepositLink->SetText(FText::FromString(TEXT("Depositar")));
	BankWithdrawLink = AddText(BankWindow, 130.f, 110.f, 10, BkLink);
	BankWithdrawLink->SetText(FText::FromString(TEXT("Sacar")));
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
	BankBalanceText->SetText(FText::FromString(FString::Printf(TEXT("Saldo no banco: %s z"), *FText::AsNumber(Net->GetBankBalance()).ToString())));
	BankZenyText->SetText(FText::FromString(FString::Printf(TEXT("Zeny com você: %s z"), *FText::AsNumber(Net->GetPlayerStats().Zeny).ToString())));
}

bool UMythRoHUDWidget::HandleBankClick(const FVector2D& ScreenPos)
{
	URagnarokNetworkSubsystem* Net = GetNet();
	if (!Net || !BankWindow || !BankWindow->IsVisible())
	{
		return false;
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

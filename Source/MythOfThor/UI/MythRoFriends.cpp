// Lista de amigos (Alt+H) e pedido de amizade (docs/ref/hud_latam/INDICE.md, print 20_amigos). A lista, quem esta online
// e as respostas vem do servidor (pacotes 0x0201, 0x0206, 0x0207, 0x0209, 0x020a); aqui so se mostra e se pedem as acoes.

#include "UI/MythRoHUDWidget.h"

#include "Network/RagnarokNetworkSubsystem.h"

#include "UI/MythRoUITextures.h"
#include "Components/CanvasPanel.h"
#include "Components/CanvasPanelSlot.h"
#include "Components/Image.h"
#include "Components/TextBlock.h"

namespace
{
	const FLinearColor FrText(0.12f, 0.12f, 0.12f);
	const FLinearColor FrLink(0.12f, 0.3f, 0.65f);
	const FLinearColor FrGray(0.55f, 0.55f, 0.55f);
	const FLinearColor FrGreen(0.1f, 0.55f, 0.2f);
	constexpr float FrW = 270.f;
	constexpr float FrRowH = 20.f;
	constexpr float FrListY = 40.f;
}

void UMythRoHUDWidget::BuildFriendsWindow()
{
	const float H = FrListY + FriendRowCount * FrRowH + 36.f;
	FriendsWindow = MakeRoPanel(TEXT("Amigos"), FVector2D(FrW, H), FVector2D(-330.f, 40.f));
	AddFriendPartySwitch(FriendsWindow, H, false);
	FriendsTitle = AddText(FriendsWindow, 10.f, 22.f, 8, FrGray);
	for (int32 i = 0; i < FriendRowCount; ++i)
	{
		const float Y = FrListY + i * FrRowH;
		FriendNames.Add(AddText(FriendsWindow, 12.f, Y, 8, FrText));
		FriendStates.Add(AddText(FriendsWindow, 150.f, Y, 7, FrGray));
		UTextBlock* Del = AddText(FriendsWindow, FrW - 62.f, Y, 7, FrLink);
		Del->SetText(FText::FromString(TEXT("Remover")));
		FriendDelete.Add(Del);
	}
	AddText(FriendsWindow, 12.f, H - 40.f, 7, FrGray)->SetText(FText::FromString(TEXT("Para adicionar: /friend Nome ou botão direito no jogador.")));

	FriendAskWindow = MakeRoPanel(TEXT("Pedido de amizade"), FVector2D(300.f, 92.f), FVector2D(0.f, -180.f));
	FriendAskText = AddText(FriendAskWindow, 10.f, 26.f, 9, FrText);
	FriendAcceptLink = AddButtonHit(FriendAskWindow, TEXT("agree"), 50.f, 58.f);
	FriendDeclineLink = AddButtonHit(FriendAskWindow, TEXT("disagree"), 170.f, 58.f);
}

void UMythRoHUDWidget::AddFriendPartySwitch(UCanvasPanel* Win, float H, bool bParty)
{
	// Radio "Amigos" / "Grupo" no pe da janela (print 19): o da janela atual aparece marcado
	UTexture2D* On = FMythRoUITextures::Get(TEXT("achievement_re/btn_radio_on.bmp"));
	UTexture2D* Off = FMythRoUITextures::Get(TEXT("achievement_re/btn_radio_off.bmp"));
	UImage* Friend = AddImage(Win, bParty ? Off : On, 10.f, H - 20.f, 12.f, 12.f);
	UImage* Party = AddImage(Win, bParty ? On : Off, 90.f, H - 20.f, 12.f, 12.f);
	AddText(Win, 26.f, H - 21.f, 7, FrText)->SetText(FText::FromString(TEXT("Amigos")));
	AddText(Win, 106.f, H - 21.f, 7, FrText)->SetText(FText::FromString(TEXT("Grupo")));
	Friend->SetVisibility(ESlateVisibility::Visible);
	Party->SetVisibility(ESlateVisibility::Visible);
	(bParty ? PartySwitchFriend : FriendsSwitchParty) = bParty ? Friend : Party;
}

bool UMythRoHUDWidget::HandleFriendPartySwitch(const FVector2D& ScreenPos)
{
	UCanvasPanel* From = nullptr;
	EMythRoWindow To = EMythRoWindow::Friends;
	if (PartyWindow && PartyWindow->IsVisible() && IsUnder(PartySwitchFriend, ScreenPos))
	{
		From = PartyWindow;
		To = EMythRoWindow::Friends;
	}
	else if (FriendsWindow && FriendsWindow->IsVisible() && IsUnder(FriendsSwitchParty, ScreenPos))
	{
		From = FriendsWindow;
		To = EMythRoWindow::Party;
	}
	if (!From)
	{
		return false;
	}
	UCanvasPanel* Dest = To == EMythRoWindow::Friends ? FriendsWindow.Get() : PartyWindow.Get();
	// A janela nova abre onde a antiga estava
	if (UCanvasPanelSlot* FromSlot = Cast<UCanvasPanelSlot>(From->Slot); FromSlot && Root)
	{
		const FVector2D Local = Root->GetCachedGeometry().AbsoluteToLocal(From->GetCachedGeometry().GetAbsolutePosition());
		if (UCanvasPanelSlot* DestSlot = Cast<UCanvasPanelSlot>(Dest->Slot))
		{
			DestSlot->SetAnchors(FAnchors(0.f, 0.f));
			DestSlot->SetAlignment(FVector2D::ZeroVector);
			DestSlot->SetPosition(Local);
		}
	}
	From->SetVisibility(ESlateVisibility::Collapsed);
	ToggleWindow(To);
	return true;
}

void UMythRoHUDWidget::RefreshFriends()
{
	const URagnarokNetworkSubsystem* Net = GetNet();
	if (!FriendsWindow || !Net || !FriendsWindow->IsVisible())
	{
		return;
	}
	const TArray<FRoFriend>& List = Net->GetFriends();
	int32 Online = 0;
	for (const FRoFriend& F : List) { Online += F.bOnline ? 1 : 0; }
	FriendsTitle->SetText(FText::FromString(FString::Printf(TEXT("%d amigos, %d online"), List.Num(), Online)));
	for (int32 i = 0; i < FriendRowCount; ++i)
	{
		const FRoFriend* F = List.IsValidIndex(i) ? &List[i] : nullptr;
		const ESlateVisibility Vis = F ? ESlateVisibility::Visible : ESlateVisibility::Collapsed;
		FriendNames[i]->SetVisibility(Vis);
		FriendStates[i]->SetVisibility(Vis);
		FriendDelete[i]->SetVisibility(Vis);
		if (!F)
		{
			continue;
		}
		FriendNames[i]->SetText(FText::FromString(F->Name));
		FriendNames[i]->SetColorAndOpacity(FSlateColor(F->bOnline ? FrText : FrGray));
		FriendStates[i]->SetText(FText::FromString(F->bOnline ? TEXT("online") : TEXT("offline")));
		FriendStates[i]->SetColorAndOpacity(FSlateColor(F->bOnline ? FrGreen : FrGray));
	}
}

void UMythRoHUDWidget::OnFriendRequestReceived(int32 AccountId, int32 CharId, const FString& Name)
{
	if (!FriendAskWindow)
	{
		return;
	}
	PendingFriendAid = AccountId;
	PendingFriendCid = CharId;
	FriendAskText->SetText(FText::FromString(FString::Printf(TEXT("%s quer ser seu amigo."), *Name)));
	FriendAskWindow->SetVisibility(ESlateVisibility::SelfHitTestInvisible);
	OpenOrder.Remove(FriendAskWindow);
	OpenOrder.Add(FriendAskWindow);
}

bool UMythRoHUDWidget::HandleFriendsClick(const FVector2D& ScreenPos)
{
	URagnarokNetworkSubsystem* Net = GetNet();
	if (!Net)
	{
		return false;
	}
	if (FriendAskWindow && FriendAskWindow->IsVisible() && (IsUnder(FriendAcceptLink, ScreenPos) || IsUnder(FriendDeclineLink, ScreenPos)))
	{
		Net->SendFriendReply(PendingFriendAid, PendingFriendCid, IsUnder(FriendAcceptLink, ScreenPos));
		FriendAskWindow->SetVisibility(ESlateVisibility::Collapsed);
		OpenOrder.Remove(FriendAskWindow);
		return true;
	}
	if (FriendsWindow && FriendsWindow->IsVisible())
	{
		for (int32 i = 0; i < FriendRowCount && i < Net->GetFriends().Num(); ++i)
		{
			if (IsUnder(FriendDelete[i], ScreenPos))
			{
				Net->SendFriendDelete(Net->GetFriends()[i].AccountId, Net->GetFriends()[i].CharId);
				return true;
			}
		}
	}
	return false;
}

void UMythRoHUDWidget::AnswerFriendRequest(bool bAccept)
{
	if (URagnarokNetworkSubsystem* Net = GetNet(); Net && PendingFriendAid != 0)
	{
		Net->SendFriendReply(PendingFriendAid, PendingFriendCid, bAccept);
	}
	if (FriendAskWindow)
	{
		FriendAskWindow->SetVisibility(ESlateVisibility::Collapsed);
		OpenOrder.Remove(FriendAskWindow);
	}
	PendingFriendAid = 0;
}

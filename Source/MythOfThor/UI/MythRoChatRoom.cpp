// Sala de chat (Alt+C, docs/ref/hud_latam/INDICE.md "Make a Room"): criar, entrar clicando no titulo sobre a cabeca de quem
// abriu a sala, lista de quem esta dentro e sair. Limite, senha e quem pode entrar sao do servidor (chat.cpp).

#include "UI/MythRoHUDWidget.h"

#include "Network/RagnarokNetworkSubsystem.h"

#include "Blueprint/WidgetTree.h"
#include "Components/CanvasPanel.h"
#include "Components/CanvasPanelSlot.h"
#include "Components/EditableTextBox.h"
#include "Components/TextBlock.h"

namespace
{
	const FLinearColor CrText(0.12f, 0.12f, 0.12f);
	const FLinearColor CrLink(0.12f, 0.3f, 0.65f);
	const FLinearColor CrGray(0.55f, 0.55f, 0.55f);
}

void UMythRoHUDWidget::BuildChatRoomWindows()
{
	// Criar
	ChatRoomCreateWindow = MakeRoPanel(TEXT("Criar sala"), FVector2D(340.f, 17.f + 160.f), FVector2D(-200.f, -100.f));
	AddText(ChatRoomCreateWindow, 14.f, 26.f, 9, CrText)->SetText(FText::FromString(TEXT("Título")));
	RoomTitleBox = WidgetTree->ConstructWidget<UEditableTextBox>();
	if (UCanvasPanelSlot* S = ChatRoomCreateWindow->AddChildToCanvas(RoomTitleBox))
	{
		S->SetPosition(FVector2D(70.f, 24.f) * UIScale);
		S->SetSize(FVector2D(256.f, 20.f) * UIScale);
	}
	AddText(ChatRoomCreateWindow, 14.f, 56.f, 9, CrText)->SetText(FText::FromString(TEXT("Limite")));
	RoomLimitLeft = AddText(ChatRoomCreateWindow, 70.f, 56.f, 10, CrLink);
	RoomLimitLeft->SetText(FText::FromString(TEXT("◀")));
	RoomLimitText = AddText(ChatRoomCreateWindow, 92.f, 56.f, 9, CrText);
	RoomLimitRight = AddText(ChatRoomCreateWindow, 140.f, 56.f, 10, CrLink);
	RoomLimitRight->SetText(FText::FromString(TEXT("▶")));
	RoomTypeLink = AddText(ChatRoomCreateWindow, 190.f, 56.f, 9, CrLink);
	AddText(ChatRoomCreateWindow, 14.f, 86.f, 9, CrText)->SetText(FText::FromString(TEXT("Senha")));
	RoomPassBox = WidgetTree->ConstructWidget<UEditableTextBox>();
	if (UCanvasPanelSlot* S = ChatRoomCreateWindow->AddChildToCanvas(RoomPassBox))
	{
		S->SetPosition(FVector2D(70.f, 84.f) * UIScale);
		S->SetSize(FVector2D(130.f, 20.f) * UIScale);
	}
	RoomOkLink = AddText(ChatRoomCreateWindow, 90.f, 132.f, 11, CrLink);
	RoomOkLink->SetText(FText::FromString(TEXT("Criar")));
	RoomCancelLink = AddText(ChatRoomCreateWindow, 200.f, 132.f, 11, CrLink);
	RoomCancelLink->SetText(FText::FromString(TEXT("Cancelar")));

	// Sala atual
	ChatRoomWindow = MakeRoPanel(TEXT("Sala de chat"), FVector2D(280.f, 17.f + 14 * 18.f + 52.f), FVector2D(-200.f, -60.f));
	RoomTitleText = AddText(ChatRoomWindow, 12.f, 24.f, 9, CrText);
	for (int32 i = 0; i < RoomRowCount; ++i)
	{
		RoomMemberRows.Add(AddText(ChatRoomWindow, 14.f, 46.f + i * 18.f, 8, CrText));
		UTextBlock* Kick = AddText(ChatRoomWindow, 210.f, 46.f + i * 18.f, 7, CrLink);
		Kick->SetText(FText::FromString(TEXT("Expulsar")));
		RoomKickLinks.Add(Kick);
	}
	RoomLeaveLink = AddText(ChatRoomWindow, 14.f, 17.f + 14 * 18.f + 30.f, 10, CrLink);
	RoomLeaveLink->SetText(FText::FromString(TEXT("Sair da sala")));

	// Senha para entrar
	ChatRoomEnterWindow = MakeRoPanel(TEXT("Sala privada"), FVector2D(260.f, 17.f + 84.f), FVector2D(0.f, -150.f));
	AddText(ChatRoomEnterWindow, 12.f, 26.f, 9, CrText)->SetText(FText::FromString(TEXT("Senha da sala")));
	RoomEnterPassBox = WidgetTree->ConstructWidget<UEditableTextBox>();
	if (UCanvasPanelSlot* S = ChatRoomEnterWindow->AddChildToCanvas(RoomEnterPassBox))
	{
		S->SetPosition(FVector2D(12.f, 44.f) * UIScale);
		S->SetSize(FVector2D(236.f, 20.f) * UIScale);
	}
	RoomEnterOkLink = AddText(ChatRoomEnterWindow, 60.f, 74.f, 10, CrLink);
	RoomEnterOkLink->SetText(FText::FromString(TEXT("Entrar")));
	RoomEnterCancelLink = AddText(ChatRoomEnterWindow, 150.f, 74.f, 10, CrLink);
	RoomEnterCancelLink->SetText(FText::FromString(TEXT("Cancelar")));
	RoomNewLimit = 10;
	RoomNewPublic = true;
}

void UMythRoHUDWidget::OpenChatRoomWindow()
{
	URagnarokNetworkSubsystem* Net = GetNet();
	UCanvasPanel* Target = Net && Net->IsInChatRoom() ? ChatRoomWindow.Get() : ChatRoomCreateWindow.Get();
	if (!Target)
	{
		return;
	}
	const bool bOpen = Target->GetVisibility() == ESlateVisibility::Collapsed;
	if (ChatRoomWindow) { ChatRoomWindow->SetVisibility(ESlateVisibility::Collapsed); }
	if (ChatRoomCreateWindow) { ChatRoomCreateWindow->SetVisibility(ESlateVisibility::Collapsed); }
	if (bOpen)
	{
		Target->SetVisibility(ESlateVisibility::SelfHitTestInvisible);
		OpenOrder.Remove(Target);
		OpenOrder.Add(Target);
	}
	RefreshChatRoom();
}

void UMythRoHUDWidget::AskChatRoomPassword(int32 ChatId)
{
	PendingRoomId = ChatId;
	if (RoomEnterPassBox) { RoomEnterPassBox->SetText(FText::GetEmpty()); }
	ChatRoomEnterWindow->SetVisibility(ESlateVisibility::SelfHitTestInvisible);
	OpenOrder.Remove(ChatRoomEnterWindow);
	OpenOrder.Add(ChatRoomEnterWindow);
}

void UMythRoHUDWidget::RefreshChatRoom()
{
	URagnarokNetworkSubsystem* Net = GetNet();
	if (!Net || !ChatRoomWindow)
	{
		return;
	}
	// Entrou numa sala: a janela dela abre; saiu: fecha
	if (Net->IsInChatRoom() && !ChatRoomWindow->IsVisible())
	{
		ChatRoomCreateWindow->SetVisibility(ESlateVisibility::Collapsed);
		ChatRoomWindow->SetVisibility(ESlateVisibility::SelfHitTestInvisible);
		OpenOrder.Remove(ChatRoomWindow);
		OpenOrder.Add(ChatRoomWindow);
	}
	else if (!Net->IsInChatRoom() && ChatRoomWindow->IsVisible())
	{
		ChatRoomWindow->SetVisibility(ESlateVisibility::Collapsed);
	}
	if (ChatRoomCreateWindow->IsVisible())
	{
		RoomLimitText->SetText(FText::FromString(FString::Printf(TEXT("%d pessoas"), RoomNewLimit)));
		RoomTypeLink->SetText(FText::FromString(RoomNewPublic ? TEXT("[ Pública ]") : TEXT("[ Privada ]")));
	}
	if (ChatRoomWindow->IsVisible())
	{
		RoomTitleText->SetText(FText::FromString(Net->GetChatRoomTitle()));
		const bool bOwner = Net->GetChatRoomMembers().Num() > 0 && Net->GetChatRoomMembers()[0].Equals(Net->GetSelfName(), ESearchCase::IgnoreCase);
		for (int32 i = 0; i < RoomRowCount; ++i)
		{
			const bool bHas = Net->GetChatRoomMembers().IsValidIndex(i);
			RoomMemberRows[i]->SetVisibility(bHas ? ESlateVisibility::Visible : ESlateVisibility::Collapsed);
			RoomKickLinks[i]->SetVisibility(bHas && bOwner && i > 0 ? ESlateVisibility::Visible : ESlateVisibility::Collapsed);
			if (bHas)
			{
				RoomMemberRows[i]->SetText(FText::FromString(FString::Printf(TEXT("%s%s"), *Net->GetChatRoomMembers()[i], i == 0 ? TEXT("  (dono)") : TEXT(""))));
			}
		}
	}
}

bool UMythRoHUDWidget::HandleChatRoomClick(const FVector2D& ScreenPos)
{
	URagnarokNetworkSubsystem* Net = GetNet();
	if (!Net)
	{
		return false;
	}
	if (ChatRoomEnterWindow && ChatRoomEnterWindow->IsVisible())
	{
		if (IsUnder(RoomEnterOkLink, ScreenPos))
		{
			Net->SendEnterChatRoom(PendingRoomId, RoomEnterPassBox->GetText().ToString());
			ChatRoomEnterWindow->SetVisibility(ESlateVisibility::Collapsed);
			return true;
		}
		if (IsUnder(RoomEnterCancelLink, ScreenPos))
		{
			ChatRoomEnterWindow->SetVisibility(ESlateVisibility::Collapsed);
			return true;
		}
	}
	if (ChatRoomCreateWindow && ChatRoomCreateWindow->IsVisible())
	{
		if (IsUnder(RoomLimitLeft, ScreenPos)) { RoomNewLimit = FMath::Max(2, RoomNewLimit - 1); }
		else if (IsUnder(RoomLimitRight, ScreenPos)) { RoomNewLimit = FMath::Min(20, RoomNewLimit + 1); }
		else if (IsUnder(RoomTypeLink, ScreenPos)) { RoomNewPublic = !RoomNewPublic; }
		else if (IsUnder(RoomOkLink, ScreenPos))
		{
			const FString Title = RoomTitleBox->GetText().ToString().TrimStartAndEnd();
			if (!Title.IsEmpty())
			{
				Net->SendCreateChatRoom(Title, RoomNewLimit, RoomNewPublic, RoomPassBox->GetText().ToString());
				ChatRoomCreateWindow->SetVisibility(ESlateVisibility::Collapsed);
			}
			return true;
		}
		else if (IsUnder(RoomCancelLink, ScreenPos))
		{
			ChatRoomCreateWindow->SetVisibility(ESlateVisibility::Collapsed);
			return true;
		}
		else
		{
			return false;
		}
		RefreshChatRoom();
		return true;
	}
	if (ChatRoomWindow && ChatRoomWindow->IsVisible())
	{
		if (IsUnder(RoomLeaveLink, ScreenPos)) { Net->SendLeaveChatRoom(); return true; }
		for (int32 i = 1; i < RoomRowCount && i < Net->GetChatRoomMembers().Num(); ++i)
		{
			if (RoomKickLinks[i]->IsVisible() && IsUnder(RoomKickLinks[i], ScreenPos))
			{
				Net->SendKickFromChatRoom(Net->GetChatRoomMembers()[i]);
				return true;
			}
		}
	}
	return false;
}

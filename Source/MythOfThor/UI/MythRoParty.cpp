// Grupo (clif_party_*, docs/ref/17 com os ids conferidos no rAthena): janela Alt+Z com os membros, barra de PV, mapa e
// lider, e o dialogo de convite. Quem pode convidar, expulsar, dividir XP e itens e decisao do servidor; aqui so se
// mostra o que ele manda e se pedem as acoes (/organize, /invite, /leave, expulsar).

#include "UI/MythRoHUDWidget.h"

#include "Network/RagnarokNetworkSubsystem.h"

#include "Components/CanvasPanel.h"
#include "Components/CanvasPanelSlot.h"
#include "Components/Image.h"
#include "Components/TextBlock.h"

namespace
{
	const FLinearColor PartyText(0.12f, 0.12f, 0.12f);
	const FLinearColor PartyLink(0.12f, 0.3f, 0.65f);
	const FLinearColor PartyGray(0.45f, 0.45f, 0.45f);
	const FLinearColor PartyRed(0.7f, 0.1f, 0.1f);
	constexpr float PartyW = 280.f;
	constexpr float PartyRowH = 34.f;
	constexpr float PartyListY = 40.f;
	constexpr float PartyBarW = 150.f;
}

void UMythRoHUDWidget::BuildPartyWindow()
{
	const float H = PartyListY + PartyRowCount * PartyRowH + 34.f;
	PartyWindow = MakeRoPanel(TEXT("Grupo"), FVector2D(PartyW, H), FVector2D(330.f, -60.f));
	AddFriendPartySwitch(PartyWindow, H, true);
	PartyNameText = AddText(PartyWindow, 8.f, 22.f, 9, PartyText);
	for (int32 i = 0; i < PartyRowCount; ++i)
	{
		const float Y = PartyListY + i * PartyRowH;
		PartyRowNames.Add(AddText(PartyWindow, 8.f, Y, 8, PartyText));
		PartyRowInfo.Add(AddText(PartyWindow, 8.f, Y + 12.f, 7, PartyGray));
		PartyBarBg.Add(AddColor(PartyWindow, FLinearColor(0.25f, 0.25f, 0.25f, 1.f), 8.f, Y + 24.f, PartyBarW, 5.f));
		PartyBarFill.Add(AddColor(PartyWindow, FLinearColor(0.2f, 0.75f, 0.2f, 1.f), 8.f, Y + 24.f, PartyBarW, 5.f));
		UTextBlock* Expel = AddText(PartyWindow, PartyW - 74.f, Y + 6.f, 8, PartyLink);
		Expel->SetText(FText::FromString(TEXT("Expulsar")));
		PartyExpelLinks.Add(Expel);
	}
	PartyLeaveLink = AddButtonHit(PartyWindow, TEXT("exit"), 160.f, H - 28.f);

	// Convite recebido
	PartyInviteWindow = MakeRoPanel(TEXT("Convite de grupo"), FVector2D(300.f, 92.f), FVector2D(0.f, -120.f));
	PartyInviteText = AddText(PartyInviteWindow, 10.f, 26.f, 9, PartyText);
	PartyAcceptLink = AddButtonHit(PartyInviteWindow, TEXT("agree"), 50.f, 58.f);
	PartyDeclineLink = AddButtonHit(PartyInviteWindow, TEXT("disagree"), 170.f, 58.f);
}

void UMythRoHUDWidget::RefreshParty()
{
	const URagnarokNetworkSubsystem* Net = GetNet();
	if (!PartyWindow || !Net || !PartyWindow->IsVisible())
	{
		return;
	}
	const TArray<FRoPartyMember>& Members = Net->GetPartyMembers();
	const int64 SelfId = Net->GetSelfID();
	bool bSelfLeader = false;
	for (const FRoPartyMember& M : Members)
	{
		bSelfLeader |= M.bLeader && M.AccountId == SelfId;
	}
	PartyNameText->SetText(FText::FromString(Net->IsInParty() ? Net->GetPartyName() : FString(TEXT("Você não está em um grupo. Use /organize \"nome\"."))));
	for (int32 i = 0; i < PartyRowCount; ++i)
	{
		const FRoPartyMember* M = Members.IsValidIndex(i) ? &Members[i] : nullptr;
		const ESlateVisibility Vis = M ? ESlateVisibility::HitTestInvisible : ESlateVisibility::Collapsed;
		PartyRowNames[i]->SetVisibility(Vis);
		PartyRowInfo[i]->SetVisibility(Vis);
		PartyBarBg[i]->SetVisibility(Vis);
		PartyBarFill[i]->SetVisibility(Vis);
		PartyExpelLinks[i]->SetVisibility(M && bSelfLeader && M->AccountId != SelfId ? ESlateVisibility::Visible : ESlateVisibility::Collapsed);
		if (!M)
		{
			continue;
		}
		const bool bSelf = M->AccountId == SelfId;
		int64 Hp = M->Hp, MaxHp = M->MaxHp;
		if (bSelf)
		{
			Hp = Net->GetPlayerStats().HP;
			MaxHp = Net->GetPlayerStats().MaxHP;
		}
		const bool bDead = M->bDead || (bSelf && Hp <= 0 && MaxHp > 0);
		PartyRowNames[i]->SetText(FText::FromString(M->Name));
		PartyRowNames[i]->SetColorAndOpacity(FSlateColor(M->bOffline ? PartyGray : (bDead ? PartyRed : PartyText)));
		FString Info = M->Map;
		Info.RemoveFromEnd(TEXT(".gat"));
		if (M->ClassId > 0 || M->Level > 0)
		{
			Info = FString::Printf(TEXT("Nv %d %s  -  %s"), M->Level, *URagnarokNetworkSubsystem::GetJobNameFromClass(M->ClassId), *Info);
		}
		if (M->bLeader) { Info += TEXT("  (líder)"); }
		if (M->bOffline) { Info += TEXT("  (desconectado)"); }
		else if (bDead) { Info += TEXT("  (morto)"); }
		PartyRowInfo[i]->SetText(FText::FromString(Info));
		const float Fraction = (MaxHp > 0 && !M->bOffline) ? FMath::Clamp(static_cast<float>(Hp) / static_cast<float>(MaxHp), 0.f, 1.f) : 0.f;
		if (UCanvasPanelSlot* S = Cast<UCanvasPanelSlot>(PartyBarFill[i]->Slot))
		{
			S->SetSize(FVector2D(PartyBarW * Fraction, 5.f));
		}
		PartyBarFill[i]->SetColorAndOpacity(Fraction < 0.25f ? FLinearColor(0.85f, 0.2f, 0.15f) : FLinearColor(0.2f, 0.75f, 0.2f));
	}
	PartyLeaveLink->SetVisibility(Net->IsInParty() ? ESlateVisibility::Visible : ESlateVisibility::Collapsed);
}

void UMythRoHUDWidget::ShowPartyInvite(int32 PartyId, const FString& PartyName)
{
	if (!PartyInviteWindow)
	{
		return;
	}
	PendingPartyId = PartyId;
	PartyInviteText->SetText(FText::FromString(FString::Printf(TEXT("Você foi convidado para o grupo \"%s\"."), *PartyName)));
	PartyInviteWindow->SetVisibility(ESlateVisibility::SelfHitTestInvisible);
	OpenOrder.Remove(PartyInviteWindow);
	OpenOrder.Add(PartyInviteWindow);
	if (UCanvasPanelSlot* S = Cast<UCanvasPanelSlot>(PartyInviteWindow->Slot))
	{
		S->SetZOrder(++TopZOrder);
	}
}

void UMythRoHUDWidget::AnswerPartyInvite(bool bAccept)
{
	if (PartyInviteWindow)
	{
		PartyInviteWindow->SetVisibility(ESlateVisibility::Collapsed);
		OpenOrder.Remove(PartyInviteWindow);
	}
	if (URagnarokNetworkSubsystem* Net = GetNet(); Net && PendingPartyId != 0)
	{
		Net->SendPartyInviteReply(PendingPartyId, bAccept);
	}
	PendingPartyId = 0;
}

bool UMythRoHUDWidget::HandlePartyClick(const FVector2D& ScreenPos)
{
	URagnarokNetworkSubsystem* Net = GetNet();
	if (!Net)
	{
		return false;
	}
	if (PartyInviteWindow && PartyInviteWindow->IsVisible())
	{
		if (IsUnder(PartyAcceptLink, ScreenPos)) { AnswerPartyInvite(true); return true; }
		if (IsUnder(PartyDeclineLink, ScreenPos)) { AnswerPartyInvite(false); return true; }
	}
	if (!PartyWindow || !PartyWindow->IsVisible())
	{
		return false;
	}
	if (IsUnder(PartyLeaveLink, ScreenPos) && PartyLeaveLink->IsVisible())
	{
		Net->SendLeaveParty();
		return true;
	}
	const TArray<FRoPartyMember>& Members = Net->GetPartyMembers();
	for (int32 i = 0; i < PartyRowCount && i < Members.Num(); ++i)
	{
		if (PartyExpelLinks[i]->IsVisible() && IsUnder(PartyExpelLinks[i], ScreenPos))
		{
			Net->SendPartyExpel(Members[i].AccountId, Members[i].Name);
			return true;
		}
	}
	return false;
}

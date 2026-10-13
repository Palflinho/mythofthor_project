// Grupo (clif_party_*, docs/ref/17 com os ids conferidos no rAthena): janela Alt+Z com os membros, barra de PV, mapa e
// lider, e o dialogo de convite. Quem pode convidar, expulsar, dividir XP e itens e decisao do servidor; aqui so se
// mostra o que ele manda e se pedem as acoes (/organize, /invite, /leave, expulsar).

#include "UI/MythRoHUDWidget.h"
#include "UI/MythRoUITextures.h"

#include "Network/RagnarokNetworkSubsystem.h"

#include "Components/CanvasPanel.h"
#include "Components/CanvasPanelSlot.h"
#include "Components/EditableTextBox.h"
#include "Components/Image.h"
#include "Components/TextBlock.h"

namespace
{
	const FLinearColor PartyText(0.12f, 0.12f, 0.12f);
	const FLinearColor PartyLink(0.12f, 0.3f, 0.65f);
	const FLinearColor PartyGray(0.45f, 0.45f, 0.45f);
	const FLinearColor PartyRed(0.7f, 0.1f, 0.1f);
	// Texto escuro sobre o cartao claro do membro
	const FLinearColor PartyCardText(0.08f, 0.08f, 0.08f);
	const FLinearColor PartyCardGray(0.35f, 0.35f, 0.38f);
	const FLinearColor PartyCardRed(0.7f, 0.1f, 0.1f);
	const FLinearColor PartyCardLink(0.12f, 0.3f, 0.65f);
	constexpr float PartyW = 286.f; // roBrowser PartyFriends V1 / goro: 286 de largura
	constexpr float PartyRowH = 38.f; // cartao de membro de 38 (roBrowser info-v2)
	constexpr float PartyListY = 40.f;
	constexpr float PartyBarW = 150.f;
	constexpr float PartyToolH = 22.f; // faixa da barra de ferramentas acima de Amigos/Grupo
	FString PartyBtn(const TCHAR* Name) { return FString::Printf(TEXT("basic_interface/mesbtn_%s.bmp"), Name); }
}

void UMythRoHUDWidget::BuildPartyWindow()
{
	const float H = PartyListY + PartyRowCount * PartyRowH + 34.f + PartyToolH;
	PartyWindow = MakeRoPanel(TEXT("Party"), FVector2D(PartyW, H), FVector2D(330.f, -60.f));
	AddFriendPartySwitch(PartyWindow, H, true);
	PartyNameText = AddText(PartyWindow, 8.f, 22.f, 9, PartyText);
	for (int32 i = 0; i < PartyRowCount; ++i)
	{
		const float Y = PartyListY + i * PartyRowH;
		PartyCards.Add(AddColor(PartyWindow, FLinearColor(0.95f, 0.96f, 0.98f, 1.f), 2.f, Y, PartyW - 4.f, PartyRowH - 3.f)); // cartao claro, sem a moldura escura do bg_partymember
		PartyJobIcons.Add(AddImage(PartyWindow, nullptr, 8.f, Y + 7.f, 24.f, 24.f));
		PartyCrowns.Add(AddImage(PartyWindow, FMythRoUITextures::Get(TEXT("renewalparty/ico_partycrown.bmp")), 8.f, Y - 6.f));
		PartyRowNames.Add(AddText(PartyWindow,  40.f, Y + 3.f, 8, PartyText));
		PartyRowInfo.Add(AddText(PartyWindow, 40.f, Y + 15.f, 7, PartyCardGray));
		PartyBarBg.Add(AddColor(PartyWindow, FLinearColor(0.25f, 0.25f, 0.25f, 1.f), 40.f, Y + 28.f, PartyBarW, 5.f));
		PartyBarFill.Add(AddColor(PartyWindow, FLinearColor(0.2f, 0.75f, 0.2f, 1.f), 40.f, Y + 28.f, PartyBarW, 5.f));
		UTextBlock* Expel = AddText(PartyWindow, PartyW - 74.f, Y + 12.f, 8, PartyCardLink);
		Expel->SetText(FText::FromString(TEXT("Expel")));
		PartyExpelLinks.Add(Expel);
	}
	// Barra de ferramentas (print 19): faixa clara mesbtn_left/mid/right e botoes de 26 x 20 colados
	const float ToolY = H - 26.f - PartyToolH;
	AddImage(PartyWindow, FMythRoUITextures::Get(PartyBtn(TEXT("left"))), 4.f, ToolY, 7.f, 20.f);
	AddImage(PartyWindow, FMythRoUITextures::Get(PartyBtn(TEXT("mid"))), 11.f, ToolY, PartyW - 22.f, 20.f);
	AddImage(PartyWindow, FMythRoUITextures::Get(PartyBtn(TEXT("right"))), PartyW - 11.f, ToolY, 7.f, 20.f);
	auto Tool = [this, ToolY](const TCHAR* Name, float X) -> UImage*
	{
		UImage* Img = AddButtonEx(PartyWindow, PartyBtn(Name), PartyBtn(Name), PartyBtn(Name), X, ToolY);
		return Img;
	};
	PartyToolBooking = Tool(TEXT("partymaster_01"), 10.f);
	PartyToolSearch = Tool(TEXT("04"), 36.f);
	PartyToolLock = Tool(TEXT("07"), 62.f);
	PartyToolCreate = Tool(TEXT("08"), 88.f);
	PartyToolInvite = Tool(TEXT("010"), 88.f);
	PartyToolLeave = Tool(TEXT("09"), 114.f);

	// Convite recebido
	PartyInviteWindow = MakeRoPanel(TEXT("Party invitation"), FVector2D(300.f, 92.f), FVector2D(0.f, -120.f));
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
	PartyNameText->SetText(FText::FromString(Net->IsInParty() ? Net->GetPartyName() : FString(TEXT("You are not in a party. Use /organize \"name\"."))));
	for (int32 i = 0; i < PartyRowCount; ++i)
	{
		const FRoPartyMember* M = Members.IsValidIndex(i) ? &Members[i] : nullptr;
		const ESlateVisibility Vis = M ? ESlateVisibility::HitTestInvisible : ESlateVisibility::Collapsed;
		PartyRowNames[i]->SetVisibility(Vis);
		PartyCards[i]->SetVisibility(Vis);
		PartyJobIcons[i]->SetVisibility(Vis);
		PartyCrowns[i]->SetVisibility(M && M->bLeader ? ESlateVisibility::HitTestInvisible : ESlateVisibility::Collapsed);
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
		{
			// icone da classe (renewalparty/icon_jobs_<id>.bmp; a versao _die quando morto)
			UTexture2D* JobIcon = FMythRoUITextures::Get(FString::Printf(TEXT("renewalparty/icon_jobs_%d%s.bmp"), M->ClassId, bDead ? TEXT("_die") : TEXT("")));
			if (JobIcon) { PartyJobIcons[i]->SetBrushFromTexture(JobIcon, false); PartyJobIcons[i]->SetDesiredSizeOverride(FVector2D(24.f, 24.f) * UIScale); }
			PartyJobIcons[i]->SetVisibility(JobIcon ? ESlateVisibility::HitTestInvisible : ESlateVisibility::Collapsed);
		}
		PartyRowNames[i]->SetColorAndOpacity(FSlateColor(M->bOffline ? PartyCardGray : (bDead ? PartyCardRed : PartyCardText)));
		FString Info = M->Map;
		Info.RemoveFromEnd(TEXT(".gat"));
		if (M->ClassId > 0 || M->Level > 0)
		{
			Info = FString::Printf(TEXT("Nv %d %s  -  %s"), M->Level, *URagnarokNetworkSubsystem::GetJobNameFromClass(M->ClassId), *Info);
		}
		if (M->bLeader) { Info += TEXT("  (leader)"); }
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
	// Fora de um grupo: criar; num grupo: convidar e sair
	const bool bInParty = Net->IsInParty();
	PartyToolCreate->SetVisibility(bInParty ? ESlateVisibility::Collapsed : ESlateVisibility::Visible);
	PartyToolInvite->SetVisibility(bInParty ? ESlateVisibility::Visible : ESlateVisibility::Collapsed);
	PartyToolLeave->SetVisibility(bInParty ? ESlateVisibility::Visible : ESlateVisibility::Collapsed);
}

void UMythRoHUDWidget::ShowPartyInvite(int32 PartyId, const FString& PartyName)
{
	if (!PartyInviteWindow)
	{
		return;
	}
	PendingPartyId = PartyId;
	PartyInviteText->SetText(FText::FromString(FString::Printf(TEXT("You were invited to the party \"%s\"."), *PartyName)));
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
	if (IsUnder(PartyToolLeave, ScreenPos))
	{
		Net->SendLeaveParty();
		return true;
	}
	auto PrefillChat = [this](const TCHAR* Command)
	{
		// Como o cliente: o comando fica pronto no chat para completar com o nome
		if (ChatInput)
		{
			ChatInput->SetText(FText::FromString(Command));
			FocusChat();
		}
	};
	if (IsUnder(PartyToolCreate, ScreenPos))
	{
		PrefillChat(TEXT("/organize "));
		return true;
	}
	if (IsUnder(PartyToolInvite, ScreenPos))
	{
		PrefillChat(TEXT("/invite "));
		return true;
	}
	if (IsUnder(PartyToolLock, ScreenPos))
	{
		// Cadeado: trava a janela no lugar (mesbtn_06 fechado / 07 aberto)
		const bool bLock = !LockedWindows.Contains(PartyWindow);
		if (bLock) { LockedWindows.Add(PartyWindow); } else { LockedWindows.Remove(PartyWindow); }
		for (FRoButton& B : RoButtons)
		{
			if (B.Image.Get() == PartyToolLock)
			{
				UTexture2D* Tex = FMythRoUITextures::Get(PartyBtn(bLock ? TEXT("06") : TEXT("07")));
				B.Normal = B.Hover = B.Pressed = Tex;
				B.Shown = -1;
			}
		}
		return true;
	}
	if (IsUnder(PartyToolBooking, ScreenPos) || IsUnder(PartyToolSearch, ScreenPos))
	{
		ToggleBookingWindow();
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

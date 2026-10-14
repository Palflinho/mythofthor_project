// Cla (guilda) do UMythRoHUDWidget: janela Alt+G no formato do cliente (roBrowserLegacy Guild, 400 de largura, abas de 64x23
// com os nomes da msgstringtable LATAM: Cla, Membros, Cargos, Habil., Expulsos, Avisos) e o convite recebido.
//  - Cla: nome, nivel, lider, membros (online/maximo), media de nivel, territorios, EXP, aliancas e inimigos.
//  - Membros: nome, cargo, classe, nivel e EXP doada; online em verde; clique escolhe (o lider pode expulsar).
//  - Avisos: assunto e texto (o lider edita e manda com OK).
// Cargos, habilidades e expulsos ainda nao tem tela. Criar e desfazer vao pelos comandos do cliente (/guild, /breakguild).
// Pacotes em Network/RagnarokNetworkGuild.cpp; quem pode o que e decisao do servidor.

#include "UI/MythRoHUDWidget.h"
#include "Core/MythRoClientTables.h"
#include "UI/MythRoUITextures.h"

#include "Network/RagnarokNetworkSubsystem.h"

#include "Blueprint/WidgetTree.h"
#include "Components/CanvasPanel.h"
#include "Components/CanvasPanelSlot.h"
#include "Components/EditableTextBox.h"
#include "Components/Image.h"
#include "Components/TextBlock.h"
#include "HAL/PlatformTime.h"

namespace
{
	const FLinearColor GuildText(0.1f, 0.1f, 0.1f);
	const FLinearColor GuildGray(0.45f, 0.45f, 0.45f);
	const FLinearColor GuildTabBand(0.706f, 0.706f, 0.706f);  // #b4b4b4
	const FLinearColor GuildTabOff(0.784f, 0.784f, 0.784f);   // #c8c8c8
	const FLinearColor GuildOnline(0.894f, 1.f, 0.886f);      // #e4ffe2
	const FLinearColor GuildPick(0.439f, 0.624f, 0.929f);     // #709fed
	constexpr float GuildW = 400.f;
	constexpr float GuildH = 320.f;
	constexpr float GuildTop = 44.f;   // o conteudo das abas comeca abaixo da faixa
	constexpr float GuildRowH = 20.f;
	constexpr int32 GuildInfoCount = 9;

	FString GuildMsg(int32 Index)
	{
		return FMythRoClientTables::Message(Index).TrimStartAndEnd();
	}

	/** As abas que ja tem tela: Cla, Membros e Avisos. */
	bool GuildTabReady(int32 Tab) { return Tab >= 0 && Tab <= 5; }
}

void UMythRoHUDWidget::BuildGuildWindow()
{
	GuildWindow = MakeRoPanel(GuildMsg(258), FVector2D(GuildW, GuildH), FVector2D(60.f, -40.f));
	AddColor(GuildWindow, GuildTabBand, 2.f, 18.f, GuildW - 4.f, 24.f);
	for (int32 t = 0; t < GuildTabCount; ++t)
	{
		UImage* Bg = AddColor(GuildWindow, GuildTabOff, 3.f + t * 66.f, 19.f, 64.f, 23.f);
		Bg->SetVisibility(ESlateVisibility::Visible);
		GuildTabBg.Add(Bg);
		UTextBlock* T = AddText(GuildWindow, 7.f + t * 66.f, 24.f, 8, GuildText);
		T->SetText(FText::FromString(GuildMsg(340 + t)));
		GuildTabText.Add(T);
	}

	// Aba Cla: coluna da esquerda (x 8) e da direita (x 200), linhas de 16 como no roBrowser (um pouco mais altas aqui)
	for (int32 i = 0; i < GuildInfoCount; ++i)
	{
		const bool bRight = i >= 6;
		const float X = bRight ? 210.f : 10.f;
		const float Y = GuildTop + 8.f + (bRight ? (i - 6) * 74.f : i * 18.f);
		GuildInfoLines.Add(AddText(GuildWindow, X, Y, 8, GuildText));
	}
	// caixas de aliancas e inimigos (vazias por enquanto: o servidor manda no ZC_MYGUILD_BASIC_INFO)
	GuildAllyBoxes.Add(AddColor(GuildWindow, GuildTabOff, 210.f, GuildTop + 100.f, 168.f, 48.f));
	GuildAllyBoxes.Add(AddColor(GuildWindow, GuildTabOff, 210.f, GuildTop + 174.f, 168.f, 48.f));
	GuildAllyTexts.Add(AddText(GuildWindow, 214.f, GuildTop + 102.f, 7, GuildText));
	GuildAllyTexts.Add(AddText(GuildWindow, 214.f, GuildTop + 176.f, 7, GuildText));
	GuildNoneText = AddText(GuildWindow, 10.f, GuildTop + 10.f, 9, GuildText);
	GuildNoneText->SetText(FText::FromString(GuildMsg(2075)));
	GuildCreateBtn = AddTextButton(GuildWindow, GuildMsg(2076), 10.f, GuildTop + 34.f, 90.f);

	// Aba Membros
	static const float Cols[5] = { 30.f, 136.f, 214.f, 300.f, 336.f };
	static const int32 HeaderMsgs[5] = { 328, 342, 0, 329, 335 };
	for (int32 c = 0; c < 5; ++c)
	{
		UTextBlock* H = AddText(GuildWindow, Cols[c], GuildTop + 2.f, 8, GuildGray);
		H->SetText(FText::FromString(c == 2 ? FString(TEXT("Class")) : GuildMsg(HeaderMsgs[c])));
		GuildHeader.Add(H);
	}
	for (int32 i = 0; i < GuildRows; ++i)
	{
		const float Y = GuildTop + 20.f + i * GuildRowH;
		UImage* Bg = AddColor(GuildWindow, GuildOnline, 4.f, Y, GuildW - 8.f, GuildRowH - 1.f);
		Bg->SetVisibility(ESlateVisibility::Visible);
		GuildRowBg.Add(Bg);
		GuildRowIcon.Add(AddImage(GuildWindow, nullptr, 8.f, Y + 1.f, 18.f, 18.f));
		GuildRowName.Add(AddText(GuildWindow, Cols[0], Y + 3.f, 8, GuildText));
		GuildRowPos.Add(AddText(GuildWindow, Cols[1], Y + 3.f, 8, GuildText));
		GuildRowJob.Add(AddText(GuildWindow, Cols[2], Y + 3.f, 8, GuildText));
		GuildRowLevel.Add(AddText(GuildWindow, Cols[3], Y + 3.f, 8, GuildText));
		GuildRowExp.Add(AddText(GuildWindow, Cols[4], Y + 3.f, 8, GuildText));
	}

	// Aba Avisos (roBrowser: assunto em 50,11 de 331; texto em 9,52)
	auto Box = [this](float X, float Y, float W) -> UEditableTextBox*
	{
		UEditableTextBox* B = WidgetTree->ConstructWidget<UEditableTextBox>();
		if (UCanvasPanelSlot* S = GuildWindow->AddChildToCanvas(B))
		{
			S->SetPosition(FVector2D(X, Y) * UIScale);
			S->SetSize(FVector2D(W, 20.f) * UIScale);
		}
		return B;
	};
	GuildNoticeLabel1 = AddText(GuildWindow, 10.f, GuildTop + 13.f, 8, GuildText);
	GuildNoticeLabel1->SetText(FText::FromString(TEXT("Title")));
	GuildSubjectBox = Box(60.f, GuildTop + 10.f, 328.f);
	GuildNoticeLabel2 = AddText(GuildWindow, 10.f, GuildTop + 40.f, 8, GuildText);
	GuildNoticeLabel2->SetText(FText::FromString(TEXT("Content")));
	GuildNoticeBox = Box(10.f, GuildTop + 56.f, 378.f);

	// Rodape
	const float FootY = GuildH - 26.f;
	GuildFooterText = AddText(GuildWindow, 10.f, FootY + 4.f, 8, GuildGray);
	GuildOkBtn = AddTextButton(GuildWindow, TEXT("OK"), GuildW - 60.f, FootY, 50.f);
	GuildExpelBtn = AddTextButton(GuildWindow, GuildMsg(509), GuildW - 180.f, FootY, 76.f);
	GuildLeaveBtn = AddTextButton(GuildWindow, GuildMsg(508), GuildW - 98.f, FootY, 88.f);
	GuildDisbandBtn = AddTextButton(GuildWindow, GuildMsg(2088), GuildW - 98.f, FootY, 88.f);

	// Convite recebido
	GuildInviteWindow = MakeRoPanel(GuildMsg(258), FVector2D(320.f, 104.f), FVector2D(0.f, -150.f));
	GuildInviteText = AddText(GuildInviteWindow, 10.f, 24.f, 9, GuildText);
	GuildInviteText->SetAutoWrapText(true);
	if (UCanvasPanelSlot* S = Cast<UCanvasPanelSlot>(GuildInviteText->Slot))
	{
		S->SetAutoSize(false);
		S->SetSize(FVector2D(300.f, 44.f) * UIScale);
	}
	GuildAcceptLink = AddButtonHit(GuildInviteWindow, TEXT("agree"), 60.f, 72.f);
	GuildDeclineLink = AddButtonHit(GuildInviteWindow, TEXT("disagree"), 180.f, 72.f);
}

void UMythRoHUDWidget::ToggleGuildWindow()
{
	if (!GuildWindow)
	{
		return;
	}
	if (GuildWindow->IsVisible())
	{
		GuildWindow->SetVisibility(ESlateVisibility::Collapsed);
		OpenOrder.Remove(GuildWindow);
		return;
	}
	GuildWindow->SetVisibility(ESlateVisibility::SelfHitTestInvisible);
	OpenOrder.Remove(GuildWindow);
	OpenOrder.Add(GuildWindow);
	if (UCanvasPanelSlot* S = Cast<UCanvasPanelSlot>(GuildWindow->Slot))
	{
		S->SetZOrder(++TopZOrder);
	}
	bGuildNoticeFilled = false;
	GuildSelected = -1;
	GuildConfirm = 0;
	if (URagnarokNetworkSubsystem* Net = GetNet())
	{
		// Como o cliente ao abrir: informacoes basicas e a lista de membros (com os nomes dos cargos)
		Net->SendGuildMenuRequest(0);
		Net->SendGuildMenuRequest(1);
	}
	RefreshGuild();
}

void UMythRoHUDWidget::SetGuildTab(int32 Tab)
{
	GuildTab = FMath::Clamp(Tab, 0, GuildTabCount - 1);
	GuildConfirm = 0;
	if (URagnarokNetworkSubsystem* Net = GetNet(); Net && Net->IsInGuild() && GuildTab <= 4)
	{
		Net->SendGuildMenuRequest(GuildTab); // 0 dados, 1 membros, 2 cargos, 3 habilidades, 4 expulsos
	}
	RefreshGuild();
}

void UMythRoHUDWidget::OnGuildEvent(uint8 Event)
{
	if (Event == 1 && GuildInviteWindow)
	{
		const URagnarokNetworkSubsystem* Net = GetNet();
		const FString Name = Net ? Net->GetGuildInviteName() : FString();
		GuildInviteText->SetText(FText::FromString(FString::Printf(TEXT("[%s]\n%s"), *Name, *GuildMsg(377))));
		GuildInviteWindow->SetVisibility(ESlateVisibility::SelfHitTestInvisible);
		OpenOrder.Remove(GuildInviteWindow);
		OpenOrder.Add(GuildInviteWindow);
		if (UCanvasPanelSlot* S = Cast<UCanvasPanelSlot>(GuildInviteWindow->Slot))
		{
			S->SetZOrder(++TopZOrder);
		}
		return;
	}
	if (Event == 2)
	{
		GuildSelected = -1;
		bGuildNoticeFilled = false;
	}
	RefreshGuild();
}

void UMythRoHUDWidget::AnswerGuildInvite(bool bAccept)
{
	if (GuildInviteWindow)
	{
		GuildInviteWindow->SetVisibility(ESlateVisibility::Collapsed);
		OpenOrder.Remove(GuildInviteWindow);
	}
	if (URagnarokNetworkSubsystem* Net = GetNet(); Net && Net->GetGuildInviteId() != 0)
	{
		Net->SendGuildInviteReply(Net->GetGuildInviteId(), bAccept);
	}
}

void UMythRoHUDWidget::RefreshGuild()
{
	const URagnarokNetworkSubsystem* Net = GetNet();
	if (!GuildWindow || !Net || !GuildWindow->IsVisible())
	{
		return;
	}
	const bool bIn = Net->IsInGuild();
	if (!bIn)
	{
		GuildTab = 0;
	}
	if (GuildConfirm != 0 && FPlatformTime::Seconds() > GuildConfirmUntil)
	{
		GuildConfirm = 0;
	}
	for (int32 t = 0; t < GuildTabCount; ++t)
	{
		GuildTabBg[t]->SetColorAndOpacity(t == GuildTab ? FLinearColor::White : GuildTabOff);
		GuildTabText[t]->SetColorAndOpacity(FSlateColor(GuildTabReady(t) && (bIn || t == 0) ? GuildText : GuildGray));
	}
	auto Show = [](UWidget* W, bool b) { if (W) { W->SetVisibility(b ? ESlateVisibility::HitTestInvisible : ESlateVisibility::Collapsed); } };

	// Aba Cla
	const bool bInfo = GuildTab == 0;
	const FRoGuildInfo& G = Net->GetGuildInfo();
	const FString Name = G.Name.IsEmpty() ? Net->GetGuildName() : G.Name;
	const FString Lines[GuildInfoCount] = {
		FString::Printf(TEXT("%s: %s"), *GuildMsg(328), *Name),
		FString::Printf(TEXT("%s: %d"), *GuildMsg(329), G.Level),
		FString::Printf(TEXT("%s: %s"), *GuildMsg(330), *G.Master),
		FString::Printf(TEXT("%s: %d / %d"), *GuildMsg(331), G.Online, G.MaxMembers),
		FString::Printf(TEXT("%s: %d"), *GuildMsg(332), G.AvgLevel),
		FString::Printf(TEXT("%s: %s"), *GuildMsg(333), *G.Land),
		FString::Printf(TEXT("%s: %lld / %lld"), *GuildMsg(335), G.Exp, G.MaxExp),
		GuildMsg(338),
		GuildMsg(339),
	};
	for (int32 i = 0; i < GuildInfoCount; ++i)
	{
		GuildInfoLines[i]->SetText(FText::FromString(Lines[i]));
		Show(GuildInfoLines[i], bInfo && bIn);
	}
	Show(GuildNoneText, bInfo && !bIn);
	for (UImage* B : GuildAllyBoxes) { Show(B, bInfo && bIn); }
	for (int32 k = 0; k < GuildAllyTexts.Num(); ++k)
	{
		FString Lines2;
		int32 Count = 0;
		for (const FRoGuildRelation& Rel : Net->GetGuildRelations())
		{
			if (Rel.Relation == k && Count++ < 3)
			{
				Lines2 += Rel.Name + LINE_TERMINATOR;
			}
		}
		GuildAllyTexts[k]->SetText(FText::FromString(Lines2.TrimEnd()));
		Show(GuildAllyTexts[k], bInfo && bIn);
	}
	SetTextButtonShown(GuildCreateBtn, bInfo && !bIn);

	// Aba Membros: online primeiro (como a opcao "Membros online primeiro" do cliente), depois pelo nome
	const bool bMembers = GuildTab == 1 && bIn;
	TArray<FRoGuildMember> Members = Net->GetGuildMembers();
	Members.StableSort([](const FRoGuildMember& A, const FRoGuildMember& B) { return A.bOnline != B.bOnline ? A.bOnline : A.Position < B.Position; });

	// Abas em lista (Membros, Cargos, Habil., Expulsos): cada uma preenche as mesmas 11 linhas de 5 colunas
	struct FGuildRowData
	{
		FString C[5];
		bool bOn = true;
		int32 ClassId = -1;
		bool bUpgrade = false;
	};
	const bool bList = bIn && GuildTab >= 1 && GuildTab <= 4;
	TArray<FGuildRowData> Data;
	FString Heads[5];
	if (bList && GuildTab == 1)
	{
		Heads[0] = GuildMsg(328); Heads[1] = GuildMsg(342); Heads[2] = TEXT("Class"); Heads[3] = GuildMsg(329); Heads[4] = GuildMsg(335);
		for (const FRoGuildMember& M : Members)
		{
			FGuildRowData R;
			R.C[0] = M.Name;
			R.C[1] = Net->GetGuildPositionName(M.Position);
			R.C[2] = URagnarokNetworkSubsystem::GetJobNameFromClass(M.ClassId);
			R.C[3] = FString::FromInt(M.Level);
			R.C[4] = FText::AsNumber(M.Exp).ToString();
			R.bOn = M.bOnline;
			R.ClassId = M.ClassId;
			Data.Add(R);
		}
	}
	else if (bList && GuildTab == 2)
	{
		Heads[0] = GuildMsg(340 + 2); Heads[1] = TEXT("Rights"); Heads[2] = TEXT("Tax");
		Heads[0] = TEXT("Position");
		for (const FRoGuildPosition& P : Net->GetGuildPositionInfos())
		{
			FGuildRowData R;
			R.C[0] = FString::Printf(TEXT("%d. %s"), P.Id + 1, *Net->GetGuildPositionName(P.Id));
			FString Rights;
			if (P.Rights & 0x01) { Rights += TEXT("Invite"); }
			if (P.Rights & 0x10) { Rights += TEXT("Expel"); }
			R.C[1] = Rights.IsEmpty() ? FString(TEXT("-")) : Rights.TrimEnd();
			R.C[2] = FString::Printf(TEXT("%d%%"), P.PayRate);
			Data.Add(R);
		}
	}
	else if (bList && GuildTab == 3)
	{
		Heads[0] = TEXT("Skill"); Heads[1] = TEXT("Level"); Heads[2] = TEXT("SP"); Heads[3] = TEXT("Range"); Heads[4] = Net->IsGuildMaster() ? TEXT("Raise") : TEXT("");
		for (const FRoGuildSkill& S : Net->GetGuildSkills())
		{
			FGuildRowData R;
			const FString Pt = FMythRoClientTables::SkillName(S.Id);
			R.C[0] = Pt.IsEmpty() ? S.Name : Pt;
			R.C[1] = FString::FromInt(S.Level);
			R.C[2] = S.Level > 0 ? FString::FromInt(S.Sp) : FString(TEXT("-"));
			R.C[3] = S.Level > 0 ? FString::FromInt(S.Range) : FString(TEXT("-"));
			R.bUpgrade = S.bUpgradable && Net->GetGuildSkillPoints() > 0;
			R.C[4] = R.bUpgrade ? FString(TEXT("[+]")) : FString();
			R.bOn = S.Level > 0;
			Data.Add(R);
		}
	}
	else if (bList && GuildTab == 4)
	{
		Heads[0] = GuildMsg(328); Heads[1] = TEXT("Reason");
		for (const FRoGuildBan& B : Net->GetGuildBans())
		{
			FGuildRowData R;
			R.C[0] = B.Name;
			R.C[1] = B.Reason;
			Data.Add(R);
		}
	}
	GuildScroll = FMath::Clamp(GuildScroll, 0, FMath::Max(0, Data.Num() - GuildRows));
	// Posicao das colunas de cada aba (as mesmas 5 colunas de texto servem para as quatro listas)
	static const float ColX[5][5] = { { 30.f, 136.f, 214.f, 300.f, 336.f }, { 30.f, 136.f, 214.f, 300.f, 336.f }, { 30.f, 170.f, 300.f, 0.f, 0.f }, { 30.f, 190.f, 250.f, 300.f, 352.f }, { 30.f, 150.f, 0.f, 0.f, 0.f } };
	const float* Xs = ColX[FMath::Clamp(GuildTab, 0, 4)];
	auto PlaceX = [this](UWidget* W, float X)
	{
		if (UCanvasPanelSlot* Slot = W ? Cast<UCanvasPanelSlot>(W->Slot) : nullptr)
		{
			Slot->SetPosition(FVector2D(X * UIScale, Slot->GetPosition().Y));
		}
	};
	for (int32 c = 0; c < GuildHeader.Num() && c < 5; ++c) { PlaceX(GuildHeader[c], Xs[c]); }
	for (int32 i = 0; i < GuildRows; ++i)
	{
		UWidget* RowCells[5] = { GuildRowName[i], GuildRowPos[i], GuildRowJob[i], GuildRowLevel[i], GuildRowExp[i] };
		for (int32 c = 0; c < 5; ++c) { PlaceX(RowCells[c], Xs[c]); }
	}
	for (int32 c = 0; c < GuildHeader.Num() && c < 5; ++c)
	{
		GuildHeader[c]->SetText(FText::FromString(Heads[c]));
		Show(GuildHeader[c], bList && !Heads[c].IsEmpty());
	}
	for (int32 i = 0; i < GuildRows; ++i)
	{
		const int32 Index = GuildScroll + i;
		const FGuildRowData* M = bList && Data.IsValidIndex(Index) ? &Data[Index] : nullptr;
		GuildRowBg[i]->SetVisibility(M ? ESlateVisibility::Visible : ESlateVisibility::Collapsed);
		UTextBlock* Cells[5] = { GuildRowName[i], GuildRowPos[i], GuildRowJob[i], GuildRowLevel[i], GuildRowExp[i] };
		for (int32 c = 0; c < 5; ++c)
		{
			Show(Cells[c], M != nullptr);
		}
		Show(GuildRowIcon[i], false);
		if (!M)
		{
			continue;
		}
		GuildRowBg[i]->SetColorAndOpacity(GuildTab == 1 && Index == GuildSelected ? GuildPick : (GuildTab == 1 ? (M->bOn ? GuildOnline : FLinearColor::White) : FLinearColor::White));
		if (M->ClassId >= 0)
		{
			if (UTexture2D* Icon = FMythRoUITextures::Get(FString::Printf(TEXT("renewalparty/icon_jobs_%d.bmp"), M->ClassId)))
			{
				GuildRowIcon[i]->SetBrushFromTexture(Icon, false);
				GuildRowIcon[i]->SetDesiredSizeOverride(FVector2D(18.f, 18.f) * UIScale);
				Show(GuildRowIcon[i], true);
			}
		}
		const FLinearColor Color = M->bOn ? GuildText : GuildGray;
		for (int32 c = 0; c < 5; ++c)
		{
			Cells[c]->SetText(FText::FromString(M->C[c]));
			Cells[c]->SetColorAndOpacity(FSlateColor(c == 4 && M->bUpgrade ? FLinearColor(0.1f, 0.45f, 0.1f) : Color));
		}
	}

	// Aba Avisos: o lider edita; os outros so leem
	const bool bNotice = GuildTab == 5 && bIn;
	Show(GuildNoticeLabel1, bNotice);
	Show(GuildNoticeLabel2, bNotice);
	for (UEditableTextBox* B : { GuildSubjectBox.Get(), GuildNoticeBox.Get() })
	{
		B->SetVisibility(bNotice ? ESlateVisibility::Visible : ESlateVisibility::Collapsed);
		B->SetIsReadOnly(!Net->IsGuildMaster());
	}
	if (bNotice && !bGuildNoticeFilled)
	{
		bGuildNoticeFilled = true;
		GuildSubjectBox->SetText(FText::FromString(Net->GetGuildNoticeSubject()));
		GuildNoticeBox->SetText(FText::FromString(Net->GetGuildNoticeText()));
	}

	// Rodape
	const FRoGuildMember* Picked = bMembers && Members.IsValidIndex(GuildSelected) ? &Members[GuildSelected] : nullptr;
	const bool bCanExpel = Picked && Net->IsGuildMaster() && Picked->AccountId != static_cast<int32>(Net->GetSelfID());
	SetTextButtonShown(GuildOkBtn, bNotice && Net->IsGuildMaster());
	SetTextButtonShown(GuildExpelBtn, bCanExpel);
	SetTextButtonShown(GuildLeaveBtn, bIn && !Net->IsGuildMaster() && !bNotice);
	SetTextButtonShown(GuildDisbandBtn, bIn && Net->IsGuildMaster() && !bNotice);
	FString Footer;
	if (GuildConfirm == 1) { Footer = TEXT("Click again to leave the guild."); }
	else if (GuildConfirm == 2 && Picked) { Footer = FString::Printf(TEXT("Click again to expel %s."), *Picked->Name); }
	else if (bMembers)
	{
		int32 On = 0;
		for (const FRoGuildMember& M : Members) { On += M.bOnline ? 1 : 0; }
		Footer = FString::Printf(TEXT("%s: %d / %d"), *GuildMsg(331), On, Members.Num());
	}
	else if (bList && GuildTab == 3)
	{
		Footer = FString::Printf(TEXT("Guild skill points: %d"), Net->GetGuildSkillPoints());
	}
	GuildFooterText->SetText(FText::FromString(Footer));
}

bool UMythRoHUDWidget::HandleGuildClick(const FVector2D& ScreenPos)
{
	URagnarokNetworkSubsystem* Net = GetNet();
	if (!Net)
	{
		return false;
	}
	if (GuildInviteWindow && GuildInviteWindow->IsVisible())
	{
		if (IsUnder(GuildAcceptLink, ScreenPos)) { AnswerGuildInvite(true); return true; }
		if (IsUnder(GuildDeclineLink, ScreenPos)) { AnswerGuildInvite(false); return true; }
	}
	if (!GuildWindow || !GuildWindow->IsVisible())
	{
		return false;
	}
	for (int32 t = 0; t < GuildTabCount; ++t)
	{
		if (!IsUnder(GuildTabBg[t], ScreenPos))
		{
			continue;
		}
		if (GuildTabReady(t) && (Net->IsInGuild() || t == 0))
		{
			SetGuildTab(t);
			if (t <= 4) { Net->SendGuildMenuRequest(t); } // 0 dados, 1 membros, 2 cargos, 3 habilidades, 4 expulsos
		}
		else if (Net->IsInGuild())
		{
			AddChatLine(TEXT("This tab does not exist yet in Myth of Thor."), FLinearColor(1.f, 0.85f, 0.4f));
		}
		return true;
	}
	auto PrefillChat = [this](const TCHAR* Command)
	{
		if (ChatInput)
		{
			ChatInput->SetText(FText::FromString(Command));
			FocusChat();
		}
	};
	if (IsUnder(GuildCreateBtn, ScreenPos))
	{
		PrefillChat(TEXT("/guild "));
		return true;
	}
	if (IsUnder(GuildDisbandBtn, ScreenPos))
	{
		PrefillChat(TEXT("/breakguild "));
		return true;
	}
	const double Now = FPlatformTime::Seconds();
	if (IsUnder(GuildLeaveBtn, ScreenPos))
	{
		if (GuildConfirm == 1 && Now <= GuildConfirmUntil)
		{
			GuildConfirm = 0;
			Net->SendLeaveGuild(FString());
		}
		else
		{
			GuildConfirm = 1;
			GuildConfirmUntil = Now + 4.0;
		}
		RefreshGuild();
		return true;
	}
	TArray<FRoGuildMember> Members = Net->GetGuildMembers();
	Members.StableSort([](const FRoGuildMember& A, const FRoGuildMember& B) { return A.bOnline != B.bOnline ? A.bOnline : A.Position < B.Position; });
	if (IsUnder(GuildExpelBtn, ScreenPos) && Members.IsValidIndex(GuildSelected))
	{
		if (GuildConfirm == 2 && Now <= GuildConfirmUntil)
		{
			GuildConfirm = 0;
			Net->SendGuildExpel(Members[GuildSelected].AccountId, Members[GuildSelected].CharId, FString());
		}
		else
		{
			GuildConfirm = 2;
			GuildConfirmUntil = Now + 4.0;
		}
		RefreshGuild();
		return true;
	}
	if (IsUnder(GuildOkBtn, ScreenPos))
	{
		Net->SendGuildNotice(GuildSubjectBox->GetText().ToString(), GuildNoticeBox->GetText().ToString());
		return true;
	}
	for (int32 i = 0; i < GuildRows; ++i)
	{
		if (GuildTab == 3 && Net->IsGuildMaster() && Net->GetGuildSkillPoints() > 0 && IsUnder(GuildRowBg[i], ScreenPos) && GuildRowExp[i]->IsVisible() && IsUnder(GuildRowExp[i], ScreenPos))
		{
			const int32 SkIndex = GuildScroll + i;
			if (Net->GetGuildSkills().IsValidIndex(SkIndex) && Net->GetGuildSkills()[SkIndex].bUpgradable)
			{
				Net->SendSkillUp(Net->GetGuildSkills()[SkIndex].Id); // o servidor decide
				GuildSkillRefreshAt = FPlatformTime::Seconds() + 1.0; // e a lista nova e pedida logo depois
			}
			return true;
		}
		if (GuildTab == 1 && IsUnder(GuildRowBg[i], ScreenPos))
		{
			GuildSelected = GuildScroll + i;
			GuildConfirm = 0;
			RefreshGuild();
			return true;
		}
	}
	return false;
}

bool UMythRoHUDWidget::HandleGuildWheel(const FVector2D& ScreenPos, float Delta)
{
	if (!GuildWindow || !GuildWindow->IsVisible() || GuildTab < 1 || GuildTab > 4 || !IsUnder(GuildWindow, ScreenPos))
	{
		return false;
	}
	GuildScroll = FMath::Max(0, GuildScroll + (Delta > 0.f ? -1 : 1));
	RefreshGuild();
	return true;
}

// Registro de quests (Alt+U, docs/ref/hud_latam/INDICE.md "Quests"), no formato do cliente moderno: janela bg_quest1..3 com as abas
// Em andamento / Informacoes / Inativas, cartoes bg_questlist com icone, titulo, objetivo, marca de rastreio e o olho que abre os
// detalhes. Estado, contagens e quando uma quest entra ou sai sao do servidor (pacotes 0x09f8, 0x09f9, 0x09fa, 0x02b4, 0x02b7);
// o titulo vem de Config/quests.json.

#include "UI/MythRoHUDWidget.h"
#include "UI/MythQuestTitles.h"
#include "UI/MythRoUITextures.h"

#include "Network/RagnarokNetworkSubsystem.h"

#include "Components/CanvasPanel.h"
#include "Components/CanvasPanelSlot.h"
#include "Components/Image.h"
#include "Components/TextBlock.h"
#include "Dom/JsonObject.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "Serialization/JsonReader.h"
#include "Serialization/JsonSerializer.h"

namespace
{
	const FLinearColor QsText(0.12f, 0.12f, 0.12f);
	const FLinearColor QsTitle(0.1f, 0.25f, 0.6f);
	const FLinearColor QsGray(0.55f, 0.55f, 0.55f);
	const FLinearColor QsGreen(0.1f, 0.55f, 0.2f);
	constexpr float QsW = 381.f;
	constexpr float QsH = 466.f;
	constexpr float QsCardX = 51.f;
	constexpr float QsCardY = 36.f;
	constexpr float QsCardH = 46.f;
	constexpr int32 QsRows = 8;
	constexpr int32 QsDetailLines = 9;

	FSlateBrush RoQuestBrush(const FString& Path, const FVector2D& Size)
	{
		FSlateBrush B;
		B.SetResourceObject(FMythRoUITextures::Get(Path));
		B.ImageSize = Size;
		B.DrawAs = ESlateBrushDrawType::Image;
		return B;
	}

	FString Short(const FString& S, int32 Max)
	{
		return S.Len() > Max ? S.Left(Max - 1) + TEXT("…") : S;
	}
}

namespace MythQuests
{
	bool& TrackerVisible()
	{
		static bool bVisible = true;
		return bVisible;
	}

	FString TitleFor(int32 QuestId)
	{
		static TMap<int32, FString> Titles;
		static bool bLoaded = false;
		if (!bLoaded)
		{
			bLoaded = true;
			FString Text;
			TSharedPtr<FJsonObject> Root;
			if (FFileHelper::LoadFileToString(Text, *FPaths::Combine(FPaths::ProjectConfigDir(), TEXT("quests.json")))
				&& FJsonSerializer::Deserialize(TJsonReaderFactory<>::Create(Text), Root) && Root.IsValid())
			{
				for (const TPair<FString, TSharedPtr<FJsonValue>>& Pair : Root->Values)
				{
					Titles.Add(FCString::Atoi(*Pair.Key), Pair.Value->AsString());
				}
			}
		}
		const FString* Found = Titles.Find(QuestId);
		return Found ? *Found : FString::Printf(TEXT("Mission %d"), QuestId);
	}
}

void UMythRoHUDWidget::BuildQuestWindow()
{
	QuestWindow = MakeWindow(FVector2D(0.5f, 0.5f), FVector2D(0.5f, 0.5f), FVector2D(-260.f * UIScale, -40.f * UIScale), FVector2D(QsW, QsH));
	QuestWindow->SetVisibility(ESlateVisibility::Collapsed);
	QuestBack = AddImage(QuestWindow, FMythRoUITextures::Get(TEXT("renew_questui/bg_quest1.bmp")), 0.f, 0.f, QsW, QsH);
	QuestBack->SetVisibility(ESlateVisibility::Visible); // o clique na janela nao anda o personagem
	QuestHeader = AddText(QuestWindow, 62.f, 2.f, 8, FLinearColor::White, true);
	QuestHeader->SetText(FText::FromString(TEXT("Missions")));
	for (int32 i = 0; i < QsRows; ++i)
	{
		const float Y = QsCardY + i * QsCardH;
		UImage* Card = AddImage(QuestWindow, FMythRoUITextures::Get(TEXT("renew_questui/bg_questlist.bmp")), QsCardX, Y, 330.f, 46.f);
		Card->SetVisibility(ESlateVisibility::Visible);
		QuestCards.Add(Card);
		QuestIcons.Add(AddImage(QuestWindow, FMythRoUITextures::Get(TEXT("renew_questui/ico_nq.bmp")), QsCardX + 6.f, Y + 8.f, 26.f, 29.f));
		QuestTitles.Add(AddText(QuestWindow, QsCardX + 40.f, Y + 5.f, 8, QsTitle));
		QuestObjectives.Add(AddText(QuestWindow, QsCardX + 40.f, Y + 24.f, 7, QsText));
		UImage* Check = AddImage(QuestWindow, FMythRoUITextures::Get(TEXT("renew_questui/bt_check_on.bmp")), QsCardX + 310.f, Y + 5.f, 14.f, 14.f);
		Check->SetVisibility(ESlateVisibility::Visible);
		QuestChecks.Add(Check);
		QuestEyes.Add(AddButton(QuestWindow, TEXT("view"), QsCardX + 284.f, Y + 22.f));
	}
	QuestEmpty = AddText(QuestWindow, QsCardX + 70.f, QsCardY + 40.f, 8, QsGray);
	QuestViewBox = AddImage(QuestWindow, FMythRoUITextures::Get(TEXT("renew_questui/checkbox_on.bmp")), 56.f, QsH - 21.f, 10.f, 10.f);
	QuestViewBox->SetVisibility(ESlateVisibility::Visible);
	AddText(QuestWindow, 70.f, QsH - 23.f, 7, QsText)->SetText(FText::FromString(TEXT("Show missions on screen")));
	QuestClose = AddButtonHit(QuestWindow, TEXT("close"), QsW - 52.f, QsH - 26.f);
	MakeDraggable(QuestWindow, QsW, 18.f);

	// Detalhes: janela ao lado com o titulo e os objetivos (o "olho" do cartao)
	QuestDetailWindow = MakeRoPanel(TEXT("Mission"), FVector2D(300.f, 17.f + 14.f + QsDetailLines * 18.f + 36.f), FVector2D(160.f, -40.f));
	for (int32 i = 0; i < QsDetailLines; ++i)
	{
		QuestDetail.Add(AddText(QuestDetailWindow, 14.f, 28.f + i * 18.f, 8, QsText));
	}
}

void UMythRoHUDWidget::RefreshQuests()
{
	const URagnarokNetworkSubsystem* Net = GetNet();
	if (!QuestWindow || !Net || !QuestWindow->IsVisible())
	{
		return;
	}
	if (QuestBack)
	{
		QuestBack->SetBrush(RoQuestBrush(FString::Printf(TEXT("renew_questui/bg_quest%d.bmp"), QuestTab + 1), FVector2D(QsW, QsH) * UIScale));
	}
	// Em andamento: as que aparecem no rastreador; Inativas: as escondidas; Informacoes: so o texto de ajuda
	TArray<const FRoQuest*> Shown;
	for (const FRoQuest& Q : Net->GetQuests())
	{
		if ((QuestTab == 0 && Q.bActive) || (QuestTab == 2 && !Q.bActive))
		{
			Shown.Add(&Q);
		}
	}
	const int32 MaxScroll = FMath::Max(0, Shown.Num() - QsRows);
	QuestScroll = FMath::Clamp(QuestScroll, 0, MaxScroll);
	for (int32 i = 0; i < QsRows; ++i)
	{
		const FRoQuest* Q = Shown.IsValidIndex(QuestScroll + i) ? Shown[QuestScroll + i] : nullptr;
		const ESlateVisibility Vis = Q ? ESlateVisibility::Visible : ESlateVisibility::Collapsed;
		QuestCards[i]->SetVisibility(Vis);
		QuestIcons[i]->SetVisibility(Q ? ESlateVisibility::HitTestInvisible : ESlateVisibility::Collapsed);
		QuestTitles[i]->SetVisibility(Q ? ESlateVisibility::HitTestInvisible : ESlateVisibility::Collapsed);
		QuestObjectives[i]->SetVisibility(Q ? ESlateVisibility::HitTestInvisible : ESlateVisibility::Collapsed);
		QuestChecks[i]->SetVisibility(Vis);
		QuestEyes[i]->SetVisibility(Vis);
		if (!Q)
		{
			continue;
		}
		QuestTitles[i]->SetText(FText::FromString(Short(MythQuests::TitleFor(Q->Id), 40)));
		FString Objective = Q->Objectives.IsEmpty() ? FString(TEXT("Talk to the NPC to continue.")) : FString();
		for (const FRoQuestObjective& O : Q->Objectives)
		{
			Objective += (Objective.IsEmpty() ? TEXT("") : TEXT(", ")) + FString::Printf(TEXT("%s %d/%d"), *O.Name, O.Count, O.Total);
		}
		QuestObjectives[i]->SetText(FText::FromString(Short(Objective, 52)));
		QuestChecks[i]->SetBrush(RoQuestBrush(Q->bActive ? TEXT("renew_questui/bt_check_on.bmp") : TEXT("renew_questui/bt_check_off.bmp"), FVector2D(14.f, 14.f) * UIScale));
	}
	QuestEmpty->SetText(FText::FromString(Shown.IsEmpty() ? (QuestTab == 1 ? TEXT("Mission information appears here.") : TEXT("No missions in this tab.")) : TEXT("")));
	QuestViewBox->SetBrush(RoQuestBrush(MythQuests::TrackerVisible() ? TEXT("renew_questui/checkbox_on.bmp") : TEXT("renew_questui/checkbox_off.bmp"), FVector2D(10.f, 10.f) * UIScale));
	// Detalhes da selecionada
	if (QuestDetailWindow->IsVisible())
	{
		for (UTextBlock* Line : QuestDetail) { Line->SetText(FText::GetEmpty()); }
		const FRoQuest* Sel = nullptr;
		for (const FRoQuest& Q : Net->GetQuests()) { if (Q.Id == QuestSelectedId) { Sel = &Q; } }
		if (Sel)
		{
			QuestDetail[0]->SetText(FText::FromString(Short(MythQuests::TitleFor(Sel->Id), 44)));
			QuestDetail[0]->SetColorAndOpacity(FSlateColor(QsTitle));
			for (int32 j = 0; j < Sel->Objectives.Num() && j + 2 < QuestDetail.Num(); ++j)
			{
				const FRoQuestObjective& O = Sel->Objectives[j];
				QuestDetail[j + 2]->SetText(FText::FromString(FString::Printf(TEXT("Derrotar %s  %d/%d"), *O.Name, O.Count, O.Total)));
				QuestDetail[j + 2]->SetColorAndOpacity(FSlateColor(O.Total > 0 && O.Count >= O.Total ? QsGreen : QsText));
			}
			if (Sel->Objectives.IsEmpty())
			{
				QuestDetail[2]->SetText(FText::FromString(TEXT("Talk to the NPC to continue.")));
			}
		}
	}
}

bool UMythRoHUDWidget::HandleQuestClick(const FVector2D& ScreenPos)
{
	URagnarokNetworkSubsystem* Net = GetNet();
	if (!Net || !QuestWindow || !QuestWindow->IsVisible())
	{
		return false;
	}
	if (QuestDetailWindow && QuestDetailWindow->IsVisible() && IsUnder(QuestDetailWindow, ScreenPos))
	{
		return false; // a janela de detalhes tem o fechar dela
	}
	if (!IsUnder(QuestWindow, ScreenPos))
	{
		return false;
	}
	if (IsUnder(QuestClose, ScreenPos))
	{
		QuestWindow->SetVisibility(ESlateVisibility::Collapsed);
		QuestDetailWindow->SetVisibility(ESlateVisibility::Collapsed);
		return true;
	}
	if (IsUnder(QuestViewBox, ScreenPos))
	{
		MythQuests::TrackerVisible() = !MythQuests::TrackerVisible();
		RefreshQuests();
		return true;
	}
	// Abas: faixa vertical a esquerda (Em andamento, Informacoes, Inativas)
	const FGeometry& Geo = QuestWindow->GetCachedGeometry();
	const FVector2D Local = Geo.AbsoluteToLocal(ScreenPos) / UIScale;
	if (Local.X < 48.f && Local.Y > 28.f && Local.Y < 412.f)
	{
		QuestTab = Local.Y < 156.f ? 0 : (Local.Y < 284.f ? 1 : 2);
		QuestScroll = 0;
		RefreshQuests();
		return true;
	}
	TArray<const FRoQuest*> Shown;
	for (const FRoQuest& Q : Net->GetQuests())
	{
		if ((QuestTab == 0 && Q.bActive) || (QuestTab == 2 && !Q.bActive))
		{
			Shown.Add(&Q);
		}
	}
	for (int32 i = 0; i < QsRows; ++i)
	{
		const FRoQuest* Q = Shown.IsValidIndex(QuestScroll + i) ? Shown[QuestScroll + i] : nullptr;
		if (!Q)
		{
			continue;
		}
		if (IsUnder(QuestChecks[i], ScreenPos))
		{
			Net->SendQuestActive(Q->Id, !Q->bActive);
			return true;
		}
		if (IsUnder(QuestEyes[i], ScreenPos) || IsUnder(QuestCards[i], ScreenPos))
		{
			QuestSelectedId = Q->Id;
			QuestDetailWindow->SetVisibility(ESlateVisibility::SelfHitTestInvisible);
			OpenOrder.Remove(QuestDetailWindow);
			OpenOrder.Add(QuestDetailWindow);
			RefreshQuests();
			return true;
		}
	}
	return true;
}

bool UMythRoHUDWidget::HandleQuestWheel(const FVector2D& ScreenPos, float Delta)
{
	if (!QuestWindow || !QuestWindow->IsVisible() || !IsUnder(QuestWindow, ScreenPos))
	{
		return false;
	}
	QuestScroll += Delta > 0.f ? -1 : 1;
	RefreshQuests();
	return true;
}

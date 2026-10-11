// Registro de quests (Alt+U, docs/ref/hud_latam/INDICE.md "Quests"): lista com titulo e olho para mostrar no rastreador, detalhe
// dos objetivos de caca. Estado, contagens e quando uma quest entra ou sai sao do servidor (pacotes 0x09f8, 0x09f9, 0x09fa,
// 0x02b4, 0x02b7); o titulo vem de Config/quests.json.

#include "UI/MythRoHUDWidget.h"
#include "UI/MythQuestTitles.h"

#include "Network/RagnarokNetworkSubsystem.h"

#include "Components/CanvasPanel.h"
#include "Components/TextBlock.h"
#include "Dom/JsonObject.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "Serialization/JsonReader.h"
#include "Serialization/JsonSerializer.h"

namespace
{
	const FLinearColor QsText(0.12f, 0.12f, 0.12f);
	const FLinearColor QsLink(0.12f, 0.3f, 0.65f);
	const FLinearColor QsGray(0.55f, 0.55f, 0.55f);
	const FLinearColor QsGreen(0.1f, 0.55f, 0.2f);
	constexpr float QsW = 320.f;
	constexpr float QsRowH = 18.f;
	constexpr float QsListY = 40.f;
	constexpr int32 QsRows = 10;
}

namespace MythQuests
{
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
		return Found ? *Found : FString::Printf(TEXT("Missão %d"), QuestId);
	}
}

void UMythRoHUDWidget::BuildQuestWindow()
{
	const float H = QsListY + QsRows * QsRowH + 14.f + 6 * 16.f + 12.f;
	QuestWindow = MakeRoPanel(TEXT("Missões"), FVector2D(QsW, H), FVector2D(-300.f, -120.f));
	QuestHeader = AddText(QuestWindow, 10.f, 22.f, 8, QsGray);
	for (int32 i = 0; i < QsRows; ++i)
	{
		const float Y = QsListY + i * QsRowH;
		QuestTitles.Add(AddText(QuestWindow, 12.f, Y, 8, QsText));
		QuestEyes.Add(AddText(QuestWindow, QsW - 84.f, Y, 7, QsLink));
	}
	for (int32 i = 0; i < 6; ++i)
	{
		QuestDetail.Add(AddText(QuestWindow, 14.f, QsListY + QsRows * QsRowH + 14.f + i * 16.f, 8, QsText));
	}
}

void UMythRoHUDWidget::RefreshQuests()
{
	const URagnarokNetworkSubsystem* Net = GetNet();
	if (!QuestWindow || !Net || !QuestWindow->IsVisible())
	{
		return;
	}
	const TArray<FRoQuest>& List = Net->GetQuests();
	int32 Active = 0;
	for (const FRoQuest& Q : List) { Active += Q.bActive ? 1 : 0; }
	QuestHeader->SetText(FText::FromString(FString::Printf(TEXT("%d missões, %d no rastreador"), List.Num(), Active)));
	for (int32 i = 0; i < QsRows; ++i)
	{
		const FRoQuest* Q = List.IsValidIndex(i) ? &List[i] : nullptr;
		const ESlateVisibility Vis = Q ? ESlateVisibility::Visible : ESlateVisibility::Collapsed;
		QuestTitles[i]->SetVisibility(Vis);
		QuestEyes[i]->SetVisibility(Vis);
		if (!Q)
		{
			continue;
		}
		QuestTitles[i]->SetText(FText::FromString(MythQuests::TitleFor(Q->Id)));
		QuestTitles[i]->SetColorAndOpacity(FSlateColor(i == QuestSelected ? QsLink : QsText));
		QuestEyes[i]->SetText(FText::FromString(Q->bActive ? TEXT("[Ocultar]") : TEXT("[Rastrear]")));
	}
	for (UTextBlock* Line : QuestDetail) { Line->SetText(FText::GetEmpty()); }
	if (List.IsValidIndex(QuestSelected))
	{
		const FRoQuest& Q = List[QuestSelected];
		QuestDetail[0]->SetText(FText::FromString(MythQuests::TitleFor(Q.Id)));
		QuestDetail[0]->SetColorAndOpacity(FSlateColor(QsLink));
		for (int32 j = 0; j < Q.Objectives.Num() && j + 1 < QuestDetail.Num(); ++j)
		{
			const FRoQuestObjective& O = Q.Objectives[j];
			QuestDetail[j + 1]->SetText(FText::FromString(FString::Printf(TEXT("%s  %d/%d"), *O.Name, O.Count, O.Total)));
			QuestDetail[j + 1]->SetColorAndOpacity(FSlateColor(O.Total > 0 && O.Count >= O.Total ? QsGreen : QsText));
		}
		if (Q.Objectives.IsEmpty())
		{
			QuestDetail[1]->SetText(FText::FromString(TEXT("Fale com o NPC para continuar.")));
			QuestDetail[1]->SetColorAndOpacity(FSlateColor(QsGray));
		}
	}
	else
	{
		QuestDetail[0]->SetText(FText::FromString(TEXT("Clique numa missão para ver os objetivos.")));
		QuestDetail[0]->SetColorAndOpacity(FSlateColor(QsGray));
	}
}

bool UMythRoHUDWidget::HandleQuestClick(const FVector2D& ScreenPos)
{
	URagnarokNetworkSubsystem* Net = GetNet();
	if (!Net || !QuestWindow || !QuestWindow->IsVisible())
	{
		return false;
	}
	for (int32 i = 0; i < QsRows && i < Net->GetQuests().Num(); ++i)
	{
		if (IsUnder(QuestEyes[i], ScreenPos))
		{
			Net->SendQuestActive(Net->GetQuests()[i].Id, !Net->GetQuests()[i].bActive);
			return true;
		}
		if (IsUnder(QuestTitles[i], ScreenPos))
		{
			QuestSelected = i;
			RefreshQuests();
			return true;
		}
	}
	return false;
}

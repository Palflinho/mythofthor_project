// Conquistas (Alt+Y, docs/ref/hud_latam/INDICE.md "Achievement Quests"): lista as conquistas que o servidor
// manda, com progresso, pontos e o botao de receber o premio. Quem completa, pontua e premia e o servidor (pacotes 0x0a23,
// 0x0a24, 0x0a26, pedido 0x0a25); nomes e metas vem de Config/achievements.json (tools/gen_achievements.py).

#include "UI/MythRoHUDWidget.h"

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
	const FLinearColor AcText(0.12f, 0.12f, 0.12f);
	const FLinearColor AcLink(0.12f, 0.3f, 0.65f);
	const FLinearColor AcGray(0.55f, 0.55f, 0.55f);
	const FLinearColor AcGreen(0.1f, 0.55f, 0.2f);
	constexpr float AcW = 400.f;
	constexpr float AcRowH = 20.f;
	constexpr float AcListY = 44.f;

	struct FAchInfo { FString Name; int32 Score = 0; int32 Total = 0; };

	const FAchInfo* AchInfo(int32 Id)
	{
		static TMap<int32, FAchInfo> Table;
		static bool bLoaded = false;
		if (!bLoaded)
		{
			bLoaded = true;
			FString Text;
			TSharedPtr<FJsonObject> Root;
			if (FFileHelper::LoadFileToString(Text, *FPaths::Combine(FPaths::ProjectConfigDir(), TEXT("achievements.json")))
				&& FJsonSerializer::Deserialize(TJsonReaderFactory<>::Create(Text), Root) && Root.IsValid())
			{
				for (const TPair<FString, TSharedPtr<FJsonValue>>& Pair : Root->Values)
				{
					const TSharedPtr<FJsonObject>* Obj = nullptr;
					if (!Pair.Value->TryGetObject(Obj))
					{
						continue;
					}
					FAchInfo Info;
					Info.Name = (*Obj)->GetStringField(TEXT("name"));
					Info.Score = static_cast<int32>((*Obj)->GetNumberField(TEXT("score")));
					for (const TSharedPtr<FJsonValue>& T : (*Obj)->GetArrayField(TEXT("targets")))
					{
						Info.Total += static_cast<int32>(T->AsNumber());
					}
					Table.Add(FCString::Atoi(*Pair.Key), Info);
				}
			}
		}
		return Table.Find(Id);
	}
}

void UMythRoHUDWidget::BuildAchievementWindow()
{
	const float H = AcListY + AchRowCount * AcRowH + 40.f;
	AchievementWindow = MakeRoPanel(TEXT("Conquistas"), FVector2D(AcW, H), FVector2D(-120.f, -140.f));
	AchHeader = AddText(AchievementWindow, 10.f, 22.f, 8, AcGray);
	for (int32 i = 0; i < AchRowCount; ++i)
	{
		const float Y = AcListY + i * AcRowH;
		AchNames.Add(AddText(AchievementWindow, 12.f, Y, 8, AcText));
		AchProgress.Add(AddText(AchievementWindow, 250.f, Y, 7, AcGray));
		AchClaim.Add(AddText(AchievementWindow, AcW - 74.f, Y, 7, AcLink));
	}
	AchPrev = AddButtonHit(AchievementWindow, TEXT("back"), 12.f, H - 30.f);
	AchPage = AddText(AchievementWindow, AcW * 0.5f - 24.f, H - 24.f, 8, AcGray);
	AchNext = AddButtonHit(AchievementWindow, TEXT("next"), AcW - 56.f, H - 30.f);
}

void UMythRoHUDWidget::RefreshAchievements()
{
	const URagnarokNetworkSubsystem* Net = GetNet();
	if (!AchievementWindow || !Net || !AchievementWindow->IsVisible())
	{
		return;
	}
	const TArray<FRoAchievement>& List = Net->GetAchievements();
	int32 Done = 0;
	for (const FRoAchievement& A : List) { Done += A.bCompleted ? 1 : 0; }
	const int32 Pages = FMath::Max(1, FMath::DivideAndRoundUp(List.Num(), AchRowCount));
	AchPageIndex = FMath::Clamp(AchPageIndex, 0, Pages - 1);
	AchHeader->SetText(FText::FromString(FString::Printf(TEXT("Pontos: %d    Rank: %d    Completas: %d de %d"), Net->GetAchievementPoints(), Net->GetAchievementRank(), Done, List.Num())));
	AchPage->SetText(FText::FromString(FString::Printf(TEXT("%d / %d"), AchPageIndex + 1, Pages)));
	for (int32 i = 0; i < AchRowCount; ++i)
	{
		const FRoAchievement* A = List.IsValidIndex(AchPageIndex * AchRowCount + i) ? &List[AchPageIndex * AchRowCount + i] : nullptr;
		const ESlateVisibility Vis = A ? ESlateVisibility::Visible : ESlateVisibility::Collapsed;
		AchNames[i]->SetVisibility(Vis);
		AchProgress[i]->SetVisibility(Vis);
		AchClaim[i]->SetVisibility(Vis);
		if (!A)
		{
			continue;
		}
		const FAchInfo* Info = AchInfo(A->Id);
		AchNames[i]->SetText(FText::FromString(FString::Printf(TEXT("%s%s"), A->bCompleted ? TEXT("✔ ") : TEXT(""), Info ? *Info->Name : *FString::Printf(TEXT("Conquista %d"), A->Id))));
		AchNames[i]->SetColorAndOpacity(FSlateColor(A->bCompleted ? AcGreen : AcText));
		int32 Count = 0;
		for (int32 v : A->Objective) { Count += v; }
		AchProgress[i]->SetText(FText::FromString(A->bCompleted ? FString::Printf(TEXT("%d pts"), Info ? Info->Score : 0)
			: (Info && Info->Total > 0 ? FString::Printf(TEXT("%d / %d"), Count, Info->Total) : FString())));
		AchClaim[i]->SetText(FText::FromString(A->bCompleted && !A->bRewardTaken ? TEXT("[Receber]") : TEXT("")));
	}
}

void UMythRoHUDWidget::OpenAchievementWindow()
{
	if (!AchievementWindow)
	{
		return;
	}
	const bool bOpen = AchievementWindow->GetVisibility() == ESlateVisibility::Collapsed;
	AchievementWindow->SetVisibility(bOpen ? ESlateVisibility::SelfHitTestInvisible : ESlateVisibility::Collapsed);
	if (bOpen)
	{
		OpenOrder.Remove(AchievementWindow);
		OpenOrder.Add(AchievementWindow);
	}
	RefreshAchievements();
}

bool UMythRoHUDWidget::HandleAchievementClick(const FVector2D& ScreenPos)
{
	URagnarokNetworkSubsystem* Net = GetNet();
	if (!Net || !AchievementWindow || !AchievementWindow->IsVisible())
	{
		return false;
	}
	if (IsUnder(AchPrev, ScreenPos)) { --AchPageIndex; RefreshAchievements(); return true; }
	if (IsUnder(AchNext, ScreenPos)) { ++AchPageIndex; RefreshAchievements(); return true; }
	for (int32 i = 0; i < AchRowCount; ++i)
	{
		const int32 Index = AchPageIndex * AchRowCount + i;
		if (Net->GetAchievements().IsValidIndex(Index) && IsUnder(AchClaim[i], ScreenPos) && !AchClaim[i]->GetText().IsEmpty())
		{
			Net->SendAchievementReward(Net->GetAchievements()[Index].Id);
			return true;
		}
	}
	return false;
}

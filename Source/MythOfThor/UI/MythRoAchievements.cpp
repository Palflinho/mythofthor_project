// Conquistas (Alt+Y, docs/ref/hud_latam/INDICE.md "Achievement Quests"), no formato do cliente moderno: cabecalho bg_upper com a
// pontuacao, abas a esquerda (Visao geral, Basico, Aventura, Batalha, Missao, Masmorra, Grandes Conquistas), lista de cartoes
// list_out/list_complete_out e o painel de detalhes com o carimbo e o botao de receber o premio. Quem completa, pontua e premia
// e o servidor (pacotes 0x0a23, 0x0a24, 0x0a26, pedido 0x0a25); nomes e metas vem de Config/achievements.json
// (tools/gen_achievements.py).

#include "UI/MythRoHUDWidget.h"
#include "UI/MythRoUITextures.h"
#include "Core/MythRoClientTables.h"

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
	const FLinearColor AcText(0.12f, 0.12f, 0.12f);
	const FLinearColor AcTitle(0.1f, 0.25f, 0.55f);
	const FLinearColor AcRed(0.65f, 0.15f, 0.2f);
	constexpr float AcW = 750.f;
	constexpr float AcH = 451.f;
	constexpr float AcTabW = 102.f;
	constexpr float AcListX = 102.f;
	constexpr float AcListY = 46.f;
	constexpr float AcCardH = 81.f;
	constexpr int32 AcCards = 5;
	constexpr float AcDetailX = 411.f;
	constexpr int32 AcTabs = 7;

	struct FAchReward { int32 Item = 0; int32 Amount = 0; int32 Title = 0; };
	struct FAchInfo { FString Name; FString Group; int32 Score = 0; int32 Total = 0; TArray<FAchReward> Rewards; };

	const TCHAR* AcTabNames[AcTabs] = { TEXT("Overview"), TEXT("Basic"), TEXT("Aventura"), TEXT("Batalha"), TEXT("Mission"), TEXT("Masmorra"), TEXT("Grandes") };

	/** Aba (1 a 6) de um grupo do banco do servidor. */
	int32 AcTabOfGroup(const FString& Group)
	{
		if (Group == TEXT("Adventure")) { return 2; }
		if (Group == TEXT("Battle") || Group == TEXT("Taming")) { return 3; }
		if (Group == TEXT("Goal_Achieve")) { return 6; }
		return 1; // o resto (niveis, classes, chat, itens, zeny, amigos...) e o basico
	}

	const FAchInfo* AcInfo(int32 Id)
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
					Info.Group = (*Obj)->GetStringField(TEXT("group"));
					Info.Score = static_cast<int32>((*Obj)->GetNumberField(TEXT("score")));
					for (const TSharedPtr<FJsonValue>& T : (*Obj)->GetArrayField(TEXT("targets")))
					{
						Info.Total += static_cast<int32>(T->AsNumber());
					}
					const TArray<TSharedPtr<FJsonValue>>* Rw = nullptr;
					if ((*Obj)->TryGetArrayField(TEXT("rewards"), Rw))
					{
						for (const TSharedPtr<FJsonValue>& RV : *Rw)
						{
							const TSharedPtr<FJsonObject>* RO = nullptr;
							if (RV->TryGetObject(RO) && RO)
							{
								FAchReward Reward;
								Reward.Item = static_cast<int32>((*RO)->GetNumberField(TEXT("item")));
								Reward.Amount = static_cast<int32>((*RO)->GetNumberField(TEXT("n")));
								Reward.Title = static_cast<int32>((*RO)->GetNumberField(TEXT("title")));
								Info.Rewards.Add(Reward);
							}
						}
					}
					Table.Add(FCString::Atoi(*Pair.Key), Info);
				}
			}
		}
		return Table.Find(Id);
	}

	FSlateBrush AcBrush(const TCHAR* Path, const FVector2D& Size)
	{
		FSlateBrush B;
		B.SetResourceObject(FMythRoUITextures::Get(Path));
		B.ImageSize = Size;
		B.DrawAs = ESlateBrushDrawType::Image;
		return B;
	}

	FString AcShort(const FString& S, int32 Max) { return S.Len() > Max ? S.Left(Max - 1) + TEXT("…") : S; }
}

void UMythRoHUDWidget::BuildAchievementWindow()
{
	AchievementWindow = MakeWindow(FVector2D(0.5f, 0.5f), FVector2D(0.5f, 0.5f), FVector2D(-120.f * UIScale, -20.f * UIScale), FVector2D(AcW, AcH));
	AchievementWindow->SetVisibility(ESlateVisibility::Collapsed);
	UImage* Back = AddColor(AchievementWindow, FLinearColor::White, 0.f, 0.f, AcW, AcH);
	Back->SetVisibility(ESlateVisibility::Visible);
	AddImage(AchievementWindow, FMythRoUITextures::Get(TEXT("achievement_re/bg_upper.bmp")), 0.f, 0.f, AcW, 46.f);
	AddImage(AchievementWindow, FMythRoUITextures::Get(TEXT("achievement_re/bg_tab.bmp")), 0.f, AcListY, AcTabW, 405.f);
	AddImage(AchievementWindow, FMythRoUITextures::Get(TEXT("basic_interface/sys_close_off.bmp")), AcW - 16.f, 4.f, 11.f, 11.f);
	UTextBlock* AchTitle = nullptr;
	AddTitleBar(AchievementWindow, AcW, TEXT("Achievements"), &AchTitle);
	if (AchTitle) { AchTitle->SetColorAndOpacity(FSlateColor(FLinearColor::Black)); AchTitle->SetShadowColorAndOpacity(FLinearColor::Transparent); }
	AchHeader = AchTitle;
	AddImage(AchievementWindow, FMythRoUITextures::Get(TEXT("basic_interface/sys_close_off.bmp")), AcW - 15.f, 3.f, 11.f, 11.f);
	AchPoints = AddText(AchievementWindow, AcW * 0.5f - 20.f, 18.f, 11, AcTitle);
	AchRank = AddText(AchievementWindow, 17.f, 21.f, 10, AcTitle, true);
	for (int32 t = 0; t < AcTabs; ++t)
	{
		UImage* Tab = AddImage(AchievementWindow, FMythRoUITextures::Get(TEXT("achievement_re/tab_sub_out.bmp")), 0.f, AcListY + 6.f + t * 36.f, 100.f, 26.f);
		Tab->SetVisibility(ESlateVisibility::Visible);
		AchTabBoxes.Add(Tab);
		UTextBlock* Label = AddText(AchievementWindow, 10.f, AcListY + 11.f + t * 36.f, 8, AcTitle);
		Label->SetText(FText::FromString(AcTabNames[t]));
		AchTabTexts.Add(Label);
	}
	for (int32 i = 0; i < AcCards; ++i)
	{
		const float Y = AcListY + i * AcCardH;
		UImage* Card = AddImage(AchievementWindow, FMythRoUITextures::Get(TEXT("achievement_re/list_out.bmp")), AcListX, Y, 309.f, AcCardH);
		Card->SetVisibility(ESlateVisibility::Visible);
		AchCards.Add(Card);
		AchNames.Add(AddText(AchievementWindow, AcListX + 50.f, Y + 12.f, 8, AcTitle, true));
		AchProgress.Add(AddText(AchievementWindow, AcListX + 12.f, Y + 52.f, 7, AcText));
		AchScores.Add(AddText(AchievementWindow, AcListX + 270.f, Y + 12.f, 9, AcTitle, true));
		AchClaim.Add(AddText(AchievementWindow, AcListX + 200.f, Y + 52.f, 7, AcRed));
	}
	// Detalhes (a direita) e carimbo de concluida
	AddImage(AchievementWindow, FMythRoUITextures::Get(TEXT("achievement_re/bg_detail.bmp")), AcDetailX, AcListY, 328.f, 405.f);
	AchDetailTitle = AddText(AchievementWindow, AcDetailX + 12.f, AcListY + 12.f, 9, AcTitle, true);
	AchStamp = AddImage(AchievementWindow, FMythRoUITextures::Get(TEXT("achievement_re/detail_stamp.bmp")), AcDetailX + 250.f, AcListY + 6.f, 64.f, 54.f);
	for (int32 i = 0; i < 10; ++i)
	{
		AchDetailLines.Add(AddText(AchievementWindow, AcDetailX + 22.f, AcListY + 70.f + i * 18.f, 8, AcText));
	}
	AchReceive = AddButtonEx(AchievementWindow, TEXT("achievement_re/btn_receive_out.bmp"), TEXT("achievement_re/btn_receive_over.bmp"), TEXT("achievement_re/btn_receive_press.bmp"), AcDetailX + 90.f, AcListY + 340.f);
	AchReceiveLabel = AddText(AchievementWindow, AcDetailX + 120.f, AcListY + 343.f, 8, AcText);
	AchReceiveLabel->SetText(FText::FromString(TEXT("Claim reward")));
	// Visao geral: contagens por aba
	for (int32 i = 0; i < 6; ++i)
	{
		AchOverview.Add(AddText(AchievementWindow, AcListX + 30.f + (i % 2) * 300.f, AcListY + 100.f + (i / 2) * 70.f, 9, AcTitle));
		AchOverviewBars.Add(AddColor(AchievementWindow, FLinearColor(0.45f, 0.55f, 0.8f), AcListX + 30.f + (i % 2) * 300.f, AcListY + 122.f + (i / 2) * 70.f, 1.f, 6.f));
	}
	MakeDraggable(AchievementWindow, AcW, 18.f);
}

void UMythRoHUDWidget::RefreshAchievements()
{
	const URagnarokNetworkSubsystem* Net = GetNet();
	if (!AchievementWindow || !Net || !AchievementWindow->IsVisible())
	{
		return;
	}
	const TArray<FRoAchievement>& List = Net->GetAchievements();
	AchPoints->SetText(FText::AsNumber(Net->GetAchievementPoints()));
	AchRank->SetText(FText::AsNumber(Net->GetAchievementRank()));
	for (int32 t = 0; t < AcTabs; ++t)
	{
		AchTabBoxes[t]->SetBrush(AcBrush(t == AchTab ? TEXT("achievement_re/tab_sub_press.bmp") : TEXT("achievement_re/tab_sub_out.bmp"), FVector2D(100.f, 26.f) * UIScale));
		AchTabTexts[t]->SetColorAndOpacity(FSlateColor(t == AchTab ? FLinearColor::White : AcTitle));
	}
	// Contagens por aba (concluidas / total conhecido no servidor)
	int32 Done[AcTabs] = {}, Total[AcTabs] = {};
	TArray<const FRoAchievement*> Shown;
	for (const FRoAchievement& A : List)
	{
		const FAchInfo* Info = AcInfo(A.Id);
		const int32 Tab = Info ? AcTabOfGroup(Info->Group) : 1;
		++Total[Tab];
		Done[Tab] += A.bCompleted ? 1 : 0;
		if (Tab == AchTab)
		{
			Shown.Add(&A);
		}
	}
	const bool bOverview = AchTab == 0;
	for (int32 i = 0; i < 6; ++i)
	{
		const int32 Tab = i + 1;
		AchOverview[i]->SetVisibility(bOverview ? ESlateVisibility::HitTestInvisible : ESlateVisibility::Collapsed);
		AchOverviewBars[i]->SetVisibility(bOverview ? ESlateVisibility::HitTestInvisible : ESlateVisibility::Collapsed);
		AchOverview[i]->SetText(FText::FromString(FString::Printf(TEXT("%s   %d/%d"), AcTabNames[Tab], Done[Tab], Total[Tab])));
		if (UCanvasPanelSlot* S = Cast<UCanvasPanelSlot>(AchOverviewBars[i]->Slot))
		{
			S->SetSize(FVector2D(FMath::Max(1.f, 250.f * (Total[Tab] > 0 ? static_cast<float>(Done[Tab]) / Total[Tab] : 0.f)), 6.f) * UIScale);
		}
	}
	const int32 MaxScroll = FMath::Max(0, Shown.Num() - AcCards);
	AchScroll = FMath::Clamp(AchScroll, 0, MaxScroll);
	for (int32 i = 0; i < AcCards; ++i)
	{
		const FRoAchievement* A = !bOverview && Shown.IsValidIndex(AchScroll + i) ? Shown[AchScroll + i] : nullptr;
		const ESlateVisibility Vis = A ? ESlateVisibility::Visible : ESlateVisibility::Collapsed;
		const ESlateVisibility TextVis = A ? ESlateVisibility::HitTestInvisible : ESlateVisibility::Collapsed;
		AchCards[i]->SetVisibility(Vis);
		AchNames[i]->SetVisibility(TextVis);
		AchProgress[i]->SetVisibility(TextVis);
		AchScores[i]->SetVisibility(TextVis);
		AchClaim[i]->SetVisibility(TextVis);
		if (!A)
		{
			continue;
		}
		const FAchInfo* Info = AcInfo(A->Id);
		AchCards[i]->SetBrush(AcBrush(A->bCompleted ? TEXT("achievement_re/list_complete_out.bmp") : (A->Id == AchSelectedId ? TEXT("achievement_re/list_press.bmp") : TEXT("achievement_re/list_out.bmp")), FVector2D(309.f, AcCardH) * UIScale));
		AchNames[i]->SetText(FText::FromString(AcShort(Info ? Info->Name : FString::Printf(TEXT("Achievement %d"), A->Id), 28)));
		int32 Count = 0;
		for (int32 v : A->Objective) { Count += v; }
		AchProgress[i]->SetText(FText::FromString(A->bCompleted ? FString(TEXT("Completed")) : (Info && Info->Total > 0 ? FString::Printf(TEXT("Progresso %d / %d"), Count, Info->Total) : FString(TEXT("Em andamento")))));
		AchScores[i]->SetText(FText::AsNumber(Info ? Info->Score : 0));
		AchClaim[i]->SetText(FText::FromString(A->bCompleted && !A->bRewardTaken ? TEXT("Reward available") : TEXT("")));
	}
	// Detalhes da selecionada
	const FRoAchievement* Sel = nullptr;
	for (const FRoAchievement& A : List) { if (A.Id == AchSelectedId) { Sel = &A; } }
	const bool bDetail = !bOverview && Sel;
	AchDetailTitle->SetVisibility(bDetail ? ESlateVisibility::HitTestInvisible : ESlateVisibility::Collapsed);
	AchStamp->SetVisibility(bDetail && Sel->bCompleted ? ESlateVisibility::HitTestInvisible : ESlateVisibility::Collapsed);
	const bool bClaim = bDetail && Sel->bCompleted && !Sel->bRewardTaken;
	AchReceive->SetVisibility(bClaim ? ESlateVisibility::Visible : ESlateVisibility::Collapsed);
	AchReceiveLabel->SetVisibility(bClaim ? ESlateVisibility::HitTestInvisible : ESlateVisibility::Collapsed);
	for (UTextBlock* L : AchDetailLines) { L->SetText(FText::GetEmpty()); }
	if (bDetail)
	{
		const FAchInfo* Info = AcInfo(Sel->Id);
		AchDetailTitle->SetText(FText::FromString(AcShort(Info ? Info->Name : FString(), 34)));
		AchDetailLines[0]->SetText(FText::FromString(FString::Printf(TEXT("Points: %d"), Info ? Info->Score : 0)));
		for (int32 j = 0; j < 4 && Info; ++j)
		{
			if (Sel->Objective[j] > 0)
			{
				AchDetailLines[j + 1]->SetText(FText::FromString(FString::Printf(TEXT("• Objetivo %d: %d"), j + 1, Sel->Objective[j])));
			}
		}
		// Premios (do achievement_db do rAthena; quem entrega e o servidor ao clicar em Receber)
		int32 Line = 6;
		AchDetailLines[Line++]->SetText(FText::FromString(Info && Info->Rewards.Num() > 0 ? TEXT("Rewards:") : TEXT("No item reward.")));
		for (int32 r = 0; Info && r < Info->Rewards.Num() && Line < AchDetailLines.Num(); ++r)
		{
			const FAchReward& Rw = Info->Rewards[r];
			FString Text;
			if (Rw.Item > 0)
			{
				const FString ItemName = FMythRoClientTables::ItemName(Rw.Item);
				Text = FString::Printf(TEXT("  %dx %s"), FMath::Max(1, Rw.Amount), ItemName.IsEmpty() ? *FString::Printf(TEXT("Item #%d"), Rw.Item) : *ItemName);
			}
			else if (Rw.Title > 0)
			{
				Text = FString::Printf(TEXT("  Title #%d"), Rw.Title);
			}
			if (!Text.IsEmpty()) { AchDetailLines[Line++]->SetText(FText::FromString(Text)); }
		}
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
	if (!Net || !AchievementWindow || !AchievementWindow->IsVisible() || !IsUnder(AchievementWindow, ScreenPos))
	{
		return false;
	}
	const FVector2D Local = AchievementWindow->GetCachedGeometry().AbsoluteToLocal(ScreenPos) / UIScale;
	if (Local.X > AcW - 22.f && Local.Y < 20.f)
	{
		AchievementWindow->SetVisibility(ESlateVisibility::Collapsed);
		return true;
	}
	for (int32 t = 0; t < AcTabs; ++t)
	{
		if (IsUnder(AchTabBoxes[t], ScreenPos))
		{
			AchTab = t;
			AchScroll = 0;
			RefreshAchievements();
			return true;
		}
	}
	if (AchReceive->IsVisible() && IsUnder(AchReceive, ScreenPos))
	{
		Net->SendAchievementReward(AchSelectedId);
		return true;
	}
	TArray<const FRoAchievement*> Shown;
	for (const FRoAchievement& A : Net->GetAchievements())
	{
		const FAchInfo* Info = AcInfo(A.Id);
		if ((Info ? AcTabOfGroup(Info->Group) : 1) == AchTab)
		{
			Shown.Add(&A);
		}
	}
	for (int32 i = 0; i < AcCards; ++i)
	{
		if (Shown.IsValidIndex(AchScroll + i) && IsUnder(AchCards[i], ScreenPos))
		{
			AchSelectedId = Shown[AchScroll + i]->Id;
			RefreshAchievements();
			return true;
		}
	}
	return true;
}

bool UMythRoHUDWidget::HandleAchievementWheel(const FVector2D& ScreenPos, float Delta)
{
	if (!AchievementWindow || !AchievementWindow->IsVisible() || !IsUnder(AchievementWindow, ScreenPos))
	{
		return false;
	}
	AchScroll += Delta > 0.f ? -1 : 1;
	RefreshAchievements();
	return true;
}

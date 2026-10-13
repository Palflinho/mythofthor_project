// Mimir Codex (Alt+K): the monster bestiary. The server (server/npc/custom/myth/mimir_codex_npc.txt) counts the kills of each
// species per character and sends them as hidden "~MIMIR~" chat lines (URagnarokNetworkSubsystem::ConsumeMimirLine).
// Static data (name, level, HP, race, size, element, drop table) comes from Config/mob_codex.json (tools/gen_mob_codex.py).
// Knowledge level per species: 1 = 1 kill (basic info), 2 = 50 kills (HP and EXP; HP also shows on the target bar),
// 3 = 250 kills (full drop table and 1 Knowledge Point).

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
	const FLinearColor MmText(0.12f, 0.12f, 0.12f);
	const FLinearColor MmTitle(0.1f, 0.25f, 0.55f);
	const FLinearColor MmGreen(0.1f, 0.5f, 0.25f);
	const FLinearColor MmGray(0.5f, 0.5f, 0.55f);
	constexpr float MmW = 560.f;
	constexpr float MmH = 560.f;
	constexpr float MmListX = 10.f;
	constexpr float MmListY = 62.f;
	constexpr float MmRowH = 20.f;
	constexpr float MmListW = 210.f;
	constexpr float MmDetailX = 232.f;

	struct FMimirMob
	{
		FString Name, Element, Race, Size;
		int32 Level = 1;
		int32 Hp = 0;
		int32 Exp = 0;
		bool bBoss = false;
		TArray<TPair<int32, int32>> Drops; // item id, rate (1 = 0.01%)
		TArray<TPair<FString, int32>> Spawns; // map id, monsters spawned there
	};

	const TMap<int32, FMimirMob>& MimirTable()
	{
		static TMap<int32, FMimirMob> Table;
		static bool bLoaded = false;
		if (!bLoaded)
		{
			bLoaded = true;
			FString Text;
			TSharedPtr<FJsonObject> Root;
			if (FFileHelper::LoadFileToString(Text, *FPaths::Combine(FPaths::ProjectConfigDir(), TEXT("mob_codex.json")))
				&& FJsonSerializer::Deserialize(TJsonReaderFactory<>::Create(Text), Root) && Root.IsValid())
			{
				for (const TPair<FString, TSharedPtr<FJsonValue>>& Pair : Root->Values)
				{
					const TSharedPtr<FJsonObject>* Obj = nullptr;
					if (!Pair.Value->TryGetObject(Obj))
					{
						continue;
					}
					FMimirMob M;
					M.Name = (*Obj)->GetStringField(TEXT("n"));
					M.Level = static_cast<int32>((*Obj)->GetNumberField(TEXT("l")));
					M.Hp = static_cast<int32>((*Obj)->GetNumberField(TEXT("h")));
					M.Exp = static_cast<int32>((*Obj)->GetNumberField(TEXT("x")));
					M.Element = (*Obj)->GetStringField(TEXT("e"));
					M.Race = (*Obj)->GetStringField(TEXT("r"));
					M.Size = (*Obj)->GetStringField(TEXT("s"));
					M.bBoss = (*Obj)->GetNumberField(TEXT("m")) > 0.5;
					for (const TSharedPtr<FJsonValue>& D : (*Obj)->GetArrayField(TEXT("d")))
					{
						const TArray<TSharedPtr<FJsonValue>>& P = D->AsArray();
						if (P.Num() == 2)
						{
							M.Drops.Emplace(static_cast<int32>(P[0]->AsNumber()), static_cast<int32>(P[1]->AsNumber()));
						}
					}
					const TArray<TSharedPtr<FJsonValue>>* Sp = nullptr;
					if ((*Obj)->TryGetArrayField(TEXT("p"), Sp))
					{
						for (const TSharedPtr<FJsonValue>& S : *Sp)
						{
							const TArray<TSharedPtr<FJsonValue>>& P = S->AsArray();
							if (P.Num() == 2)
							{
								M.Spawns.Emplace(P[0]->AsString(), static_cast<int32>(P[1]->AsNumber()));
							}
						}
					}
					Table.Add(FCString::Atoi(*Pair.Key), MoveTemp(M));
				}
			}
			UE_LOG(LogTemp, Log, TEXT("MythRoMimir: %d monsters from Config/mob_codex.json"), Table.Num());
		}
		return Table;
	}

	/** Map display name from Config/maps.json (falls back to the map id). */
	FString MmMapName(const FString& MapId)
	{
		static TMap<FString, FString> Names;
		static bool bLoaded = false;
		if (!bLoaded)
		{
			bLoaded = true;
			FString Text;
			TSharedPtr<FJsonObject> Root;
			if (FFileHelper::LoadFileToString(Text, *FPaths::Combine(FPaths::ProjectConfigDir(), TEXT("maps.json"))) && FJsonSerializer::Deserialize(TJsonReaderFactory<>::Create(Text), Root) && Root.IsValid())
			{
				for (const TPair<FString, TSharedPtr<FJsonValue>>& Pair : Root->Values)
				{
					const TSharedPtr<FJsonObject>* Obj = nullptr;
					FString N;
					if (Pair.Value->TryGetObject(Obj) && (*Obj)->TryGetStringField(TEXT("name"), N))
					{
						N.TrimStartAndEndInline();
						Names.Add(Pair.Key, N);
					}
				}
			}
		}
		const FString* F = Names.Find(MapId);
		return F ? *F : MapId;
	}

	FString MmShort(const FString& S, int32 Max) { return S.Len() > Max ? S.Left(Max - 1) + TEXT("…") : S; }

	/** Kills still needed for the next knowledge level, or 0 at the top. */
	int32 MmNextAt(int32 Kills) { return Kills < 1 ? 1 : (Kills < 50 ? 50 : (Kills < 250 ? 250 : 0)); }
}

void UMythRoHUDWidget::BuildMimirWindow()
{
	MimirWindow = MakeWindow(FVector2D(0.5f, 0.5f), FVector2D(0.5f, 0.5f), FVector2D(-60.f * UIScale, -10.f * UIScale), FVector2D(MmW, MmH));
	MimirWindow->SetVisibility(ESlateVisibility::Collapsed);
	UImage* Back = AddColor(MimirWindow, FLinearColor(0.96f, 0.96f, 0.98f, 1.f), 0.f, 0.f, MmW, MmH);
	Back->SetVisibility(ESlateVisibility::Visible);
	UTextBlock* TitleText = nullptr;
	AddTitleBar(MimirWindow, MmW, TEXT("Mimir Codex"), &TitleText);
	if (TitleText) { TitleText->SetColorAndOpacity(FSlateColor(FLinearColor::Black)); TitleText->SetShadowColorAndOpacity(FLinearColor::Transparent); }
	AddImage(MimirWindow, FMythRoUITextures::Get(TEXT("basic_interface/sys_close_off.bmp")), MmW - 16.f, 4.f, 11.f, 11.f);
	MimirHeader = AddText(MimirWindow, 12.f, 26.f, 9, MmTitle, true);
	MimirNext = AddText(MimirWindow, 12.f, 42.f, 8, MmGray);
	AddColor(MimirWindow, FLinearColor(0.82f, 0.85f, 0.92f, 1.f), MmListX - 2.f, MmListY - 4.f, MmListW + 4.f, MimirRows * MmRowH + 8.f);
	for (int32 i = 0; i < MimirRows; ++i)
	{
		UImage* Row = AddColor(MimirWindow, FLinearColor(1.f, 1.f, 1.f, 0.f), MmListX, MmListY + i * MmRowH, MmListW, MmRowH - 1.f);
		Row->SetVisibility(ESlateVisibility::Visible);
		MimirRowBack.Add(Row);
		MimirRowText.Add(AddText(MimirWindow, MmListX + 6.f, MmListY + 3.f + i * MmRowH, 8, MmText));
	}
	MimirDetailTitle = AddText(MimirWindow, MmDetailX, 62.f, 11, MmTitle, true);
	for (int32 i = 0; i < 28; ++i)
	{
		MimirDetail.Add(AddText(MimirWindow, MmDetailX, 88.f + i * 16.f, 8, MmText));
	}
	MimirEmpty = AddText(MimirWindow, MmDetailX, 90.f, 8, MmGray);
	MimirEmpty->SetText(FText::FromString(TEXT("Defeat monsters to fill the Codex.\nSpeak to the Mimir Guardian in Prontera\nto claim milestone rewards.")));
	MakeDraggable(MimirWindow, MmW, 18.f);
}

void UMythRoHUDWidget::OpenMimirWindow()
{
	if (!MimirWindow)
	{
		return;
	}
	const bool bOpen = MimirWindow->GetVisibility() == ESlateVisibility::Collapsed;
	MimirWindow->SetVisibility(bOpen ? ESlateVisibility::SelfHitTestInvisible : ESlateVisibility::Collapsed);
	if (bOpen)
	{
		OpenOrder.Remove(MimirWindow);
		OpenOrder.Add(MimirWindow);
	}
	RefreshMimir();
}

void UMythRoHUDWidget::RefreshMimir()
{
	const URagnarokNetworkSubsystem* Net = GetNet();
	if (!MimirWindow || !Net || !MimirWindow->IsVisible())
	{
		return;
	}
	const TMap<int32, FMimirMob>& Table = MimirTable();
	TArray<int32> Ids;
	for (const TPair<int32, int32>& K : Net->GetMimirKills())
	{
		if (K.Value > 0 && Table.Contains(K.Key))
		{
			Ids.Add(K.Key);
		}
	}
	Ids.Sort();
	const int32 Points = Net->GetMimirPoints();
	MimirHeader->SetText(FText::FromString(FString::Printf(TEXT("Entries: %d      Knowledge Points: %d"), Ids.Num(), Points)));
	const int32 Marks[4] = { 10, 30, 60, 100 };
	FString NextText = TEXT("All milestones reached.");
	for (int32 Mark : Marks)
	{
		if (Points < Mark)
		{
			NextText = FString::Printf(TEXT("Next milestone: %d points (claim at the Mimir Guardian, Prontera)"), Mark);
			break;
		}
	}
	MimirNext->SetText(FText::FromString(NextText));
	MimirScroll = FMath::Clamp(MimirScroll, 0, FMath::Max(0, Ids.Num() - MimirRows));
	if (!Ids.Contains(MimirSelected) && Ids.Num() > 0)
	{
		MimirSelected = Ids[0];
	}
	for (int32 i = 0; i < MimirRows; ++i)
	{
		const bool bHas = Ids.IsValidIndex(MimirScroll + i);
		MimirRowBack[i]->SetVisibility(bHas ? ESlateVisibility::Visible : ESlateVisibility::Collapsed);
		MimirRowText[i]->SetVisibility(bHas ? ESlateVisibility::HitTestInvisible : ESlateVisibility::Collapsed);
		if (!bHas)
		{
			continue;
		}
		const int32 Id = Ids[MimirScroll + i];
		const FMimirMob& M = Table[Id];
		MimirRowBack[i]->SetColorAndOpacity(Id == MimirSelected ? FLinearColor(0.55f, 0.68f, 0.95f, 0.8f) : FLinearColor(1.f, 1.f, 1.f, 0.f));
		const int32 Tier = Net->GetMimirTier(Id);
		MimirRowText[i]->SetText(FText::FromString(FString::Printf(TEXT("%s  Lv %d   %s"), *MmShort(M.Name, 18), M.Level, *FString::ChrN(Tier, TEXT('*')))));
	}
	for (UTextBlock* L : MimirDetail) { L->SetText(FText::GetEmpty()); }
	const bool bSel = Ids.Contains(MimirSelected);
	MimirEmpty->SetVisibility(bSel ? ESlateVisibility::Collapsed : ESlateVisibility::HitTestInvisible);
	MimirDetailTitle->SetVisibility(bSel ? ESlateVisibility::HitTestInvisible : ESlateVisibility::Collapsed);
	if (!bSel)
	{
		return;
	}
	const FMimirMob& M = Table[MimirSelected];
	const int32 Kills = Net->GetMimirKillCount(MimirSelected);
	const int32 Tier = Net->GetMimirTier(MimirSelected);
	const int32 Next = MmNextAt(Kills);
	MimirDetailTitle->SetText(FText::FromString(FString::Printf(TEXT("%s%s"), M.bBoss ? TEXT("[Boss] ") : TEXT(""), *M.Name)));
	int32 L = 0;
	MimirDetail[L++]->SetText(FText::FromString(FString::Printf(TEXT("Level %d     Knowledge level %d / 3"), M.Level, Tier)));
	MimirDetail[L++]->SetText(FText::FromString(FString::Printf(TEXT("Race: %s     Size: %s"), *M.Race, *M.Size)));
	MimirDetail[L++]->SetText(FText::FromString(FString::Printf(TEXT("Element: %s"), *M.Element)));
	MimirDetail[L++]->SetText(FText::FromString(Next > 0 ? FString::Printf(TEXT("Defeated: %d   (next level at %d)"), Kills, Next) : FString::Printf(TEXT("Defeated: %d   (mastered)"), Kills)));
	++L;
	if (Tier >= 2)
	{
		MimirDetail[L++]->SetText(FText::FromString(FString::Printf(TEXT("HP: %d      Base EXP: %d"), M.Hp, M.Exp)));
		++L;
		MimirDetail[L++]->SetText(FText::FromString(M.Spawns.Num() > 0 ? TEXT("Found in (monsters per map):") : TEXT("Found in: no regular spawn")));
		for (int32 s = 0; s < M.Spawns.Num() && s < 8; ++s)
		{
			MimirDetail[L++]->SetText(FText::FromString(FString::Printf(TEXT("  %s: %d"), *MmShort(MmMapName(M.Spawns[s].Key), 40), M.Spawns[s].Value)));
		}
		if (M.Spawns.Num() > 8)
		{
			MimirDetail[L++]->SetText(FText::FromString(FString::Printf(TEXT("  ...and %d more maps"), M.Spawns.Num() - 8)));
		}
	}
	else
	{
		MimirDetail[L++]->SetText(FText::FromString(TEXT("HP / EXP / maps: ???  (50 kills)")));
	}
	++L;
	if (Tier >= 3)
	{
		MimirDetail[L++]->SetText(FText::FromString(TEXT("Drops:")));
		for (const TPair<int32, int32>& D : M.Drops)
		{
			if (L >= MimirDetail.Num())
			{
				break;
			}
			const FString ItemName = FMythRoClientTables::ItemName(D.Key);
			MimirDetail[L++]->SetText(FText::FromString(FString::Printf(TEXT("  %s   %.2f%%"), ItemName.IsEmpty() ? *FString::Printf(TEXT("Item #%d"), D.Key) : *MmShort(ItemName, 32), D.Value / 100.f)));
		}
		if (M.Drops.Num() == 0)
		{
			MimirDetail[L++]->SetText(FText::FromString(TEXT("  (no drops)")));
		}
	}
	else
	{
		MimirDetail[L++]->SetText(FText::FromString(TEXT("Drops: ???  (250 kills)")));
	}
}

bool UMythRoHUDWidget::HandleMimirClick(const FVector2D& ScreenPos)
{
	const URagnarokNetworkSubsystem* Net = GetNet();
	if (!Net || !MimirWindow || !MimirWindow->IsVisible() || !IsUnder(MimirWindow, ScreenPos))
	{
		return false;
	}
	const FVector2D Local = MimirWindow->GetCachedGeometry().AbsoluteToLocal(ScreenPos) / UIScale;
	if (Local.X > MmW - 22.f && Local.Y < 20.f)
	{
		MimirWindow->SetVisibility(ESlateVisibility::Collapsed);
		return true;
	}
	const TMap<int32, FMimirMob>& Table = MimirTable();
	TArray<int32> Ids;
	for (const TPair<int32, int32>& K : Net->GetMimirKills())
	{
		if (K.Value > 0 && Table.Contains(K.Key)) { Ids.Add(K.Key); }
	}
	Ids.Sort();
	for (int32 i = 0; i < MimirRows; ++i)
	{
		if (Ids.IsValidIndex(MimirScroll + i) && IsUnder(MimirRowBack[i], ScreenPos))
		{
			MimirSelected = Ids[MimirScroll + i];
			RefreshMimir();
			return true;
		}
	}
	return true;
}

bool UMythRoHUDWidget::HandleMimirWheel(const FVector2D& ScreenPos, float Delta)
{
	if (!MimirWindow || !MimirWindow->IsVisible() || !IsUnder(MimirWindow, ScreenPos))
	{
		return false;
	}
	MimirScroll += Delta > 0.f ? -2 : 2;
	RefreshMimir();
	return true;
}

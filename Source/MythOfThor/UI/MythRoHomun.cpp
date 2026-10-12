// Homunculo do UMythRoHUDWidget: "Informacao do Homunculus" (msg 936) no formato do HomunInformations do roBrowserLegacy:
// retrato, nome (renomear uma vez), nivel, fome (valor/100), lealdade em palavras (mesmos limites do mascote: 672, 673, 669,
// 674, 675), barras de HP/SP/EXP, Atq/Matq/Precisao/Critico/Def/Mdef/Esquiva/ASPD (200 - amotion/10), Alimentar (1113),
// Aguardar (1114, como Alt+T), Autoalimentar (3279) e a lista "Habilidades do Homunculus" (937): clique no nome usa a
// habilidade, [+] distribui ponto. Pacotes em Network/RagnarokNetworkHomun.cpp; IA basica em Player/MythPlayerHomun.cpp.

#include "UI/MythRoHUDWidget.h"
#include "UI/MythRoClientTables.h"
#include "UI/MythRoUITextures.h"

#include "Network/RagnarokNetworkSubsystem.h"
#include "Player/MythPlayerController.h"
#include "Sprites/MythMonster.h"
#include "Sprites/MythRoSpriteComponent.h"

#include "Blueprint/WidgetTree.h"
#include "Components/CanvasPanel.h"
#include "Components/CanvasPanelSlot.h"
#include "Components/EditableTextBox.h"
#include "Components/Image.h"
#include "Components/TextBlock.h"
#include "EngineUtils.h"

namespace
{
	const FLinearColor HomText(0.1f, 0.1f, 0.1f);
	const FLinearColor HomGray(0.45f, 0.45f, 0.45f);
	const FLinearColor HomLink(0.12f, 0.3f, 0.65f);
	constexpr float HomW = 360.f;
	constexpr float HomH = 392.f;
	constexpr float HomBarX = 114.f;
	constexpr float HomBarW = 238.f;
	constexpr float HomSkillY = 256.f;
	constexpr int32 HomLineCount = 11; // nivel, fome, lealdade + 8 atributos

	FString HomMsg(int32 Index) { return FMythRoClientTables::Message(Index).TrimStartAndEnd(); }
}

void UMythRoHUDWidget::BuildHomunWindow()
{
	HomunWindow = MakeRoPanel(HomMsg(936), FVector2D(HomW, HomH), FVector2D(-240.f, 20.f));
	AddColor(HomunWindow, FLinearColor(0.93f, 0.94f, 0.96f, 1.f), 8.f, 24.f, 96.f, 96.f);
	HomunPortrait = AddImage(HomunWindow, nullptr, 8.f, 24.f, 96.f, 96.f);
	AddText(HomunWindow, 114.f, 27.f, 8, HomText)->SetText(FText::FromString(HomMsg(328)));
	HomunNameBox = WidgetTree->ConstructWidget<UEditableTextBox>();
	if (UCanvasPanelSlot* S = HomunWindow->AddChildToCanvas(HomunNameBox))
	{
		S->SetPosition(FVector2D(156.f, 24.f) * UIScale);
		S->SetSize(FVector2D(120.f, 20.f) * UIScale);
	}
	HomunRenameBtn = AddTextButton(HomunWindow, TEXT("Renomear"), 282.f, 24.f, 70.f);
	for (int32 i = 0; i < HomLineCount; ++i)
	{
		const bool bStat = i >= 3;
		const int32 s = i - 3;
		const float X = bStat ? (s < 4 ? 8.f : 184.f) : 114.f;
		const float Y = bStat ? 160.f + (s % 4) * 17.f : 48.f + i * 17.f;
		HomunLines.Add(AddText(HomunWindow, X, Y, 8, HomText));
	}
	for (int32 b = 0; b < 3; ++b)
	{
		const float Y = 102.f + b * 18.f;
		AddColor(HomunWindow, FLinearColor(0.25f, 0.25f, 0.28f, 1.f), HomBarX, Y, HomBarW, 14.f)->SetVisibility(ESlateVisibility::HitTestInvisible);
		const FLinearColor Fill = b == 0 ? FLinearColor(0.2f, 0.7f, 0.25f) : (b == 1 ? FLinearColor(0.25f, 0.45f, 0.85f) : FLinearColor(0.85f, 0.65f, 0.2f));
		HomunBarFill.Add(AddColor(HomunWindow, Fill, HomBarX, Y, HomBarW, 14.f));
		HomunBarText.Add(AddText(HomunWindow, HomBarX + 4.f, Y + 1.f, 7, FLinearColor::White, true));
	}
	HomunSkillHeader = AddText(HomunWindow, 8.f, HomSkillY - 18.f, 8, HomGray);
	HomunSkillHeader->SetText(FText::FromString(HomMsg(937)));
	for (int32 i = 0; i < HomunSkillRows; ++i)
	{
		const float Y = HomSkillY + i * 20.f;
		HomunSkillIcons.Add(AddImage(HomunWindow, nullptr, 8.f, Y, 18.f, 18.f));
		UTextBlock* Name = AddText(HomunWindow, 32.f, Y + 2.f, 8, HomLink);
		Name->SetVisibility(ESlateVisibility::Visible);
		HomunSkillNames.Add(Name);
		UTextBlock* Up = AddText(HomunWindow, HomW - 30.f, Y + 2.f, 9, HomLink);
		Up->SetText(FText::FromString(TEXT("[+]")));
		Up->SetVisibility(ESlateVisibility::Visible);
		HomunSkillUps.Add(Up);
	}
	const float FootY = HomH - 28.f;
	HomunFeedBtn = AddTextButton(HomunWindow, HomMsg(1113), 8.f, FootY, 70.f);
	HomunWaitBtn = AddTextButton(HomunWindow, HomMsg(1114), 82.f, FootY, 70.f);
	HomunAutoFeed = AddText(HomunWindow, 164.f, FootY + 4.f, 8, HomText);
	HomunAutoFeed->SetVisibility(ESlateVisibility::Visible);
}

void UMythRoHUDWidget::ToggleHomunWindow()
{
	URagnarokNetworkSubsystem* Net = GetNet();
	if (!HomunWindow || !Net)
	{
		return;
	}
	if (HomunWindow->IsVisible())
	{
		HomunWindow->SetVisibility(ESlateVisibility::Collapsed);
		OpenOrder.Remove(HomunWindow);
		return;
	}
	if (!Net->HasHomun())
	{
		return;
	}
	HomunWindow->SetVisibility(ESlateVisibility::SelfHitTestInvisible);
	OpenOrder.Remove(HomunWindow);
	OpenOrder.Add(HomunWindow);
	if (UCanvasPanelSlot* S = Cast<UCanvasPanelSlot>(HomunWindow->Slot))
	{
		S->SetZOrder(++TopZOrder);
	}
	HomunShownName.Reset();
	Net->SendHomunMenu(0); // informacoes atuais
	RefreshHomun();
}

void UMythRoHUDWidget::OnHomunEvent(uint8 Event)
{
	if (Event == 2 && HomunWindow)
	{
		HomunWindow->SetVisibility(ESlateVisibility::Collapsed);
		OpenOrder.Remove(HomunWindow);
		HomunPortraitKey.Reset();
		return;
	}
	RefreshHomun();
}

void UMythRoHUDWidget::RefreshHomun()
{
	const URagnarokNetworkSubsystem* Net = GetNet();
	if (!HomunWindow || !Net || !HomunWindow->IsVisible())
	{
		return;
	}
	const FRoHomunInfo& H = Net->GetHomunInfo();
	// Retrato: primeiro quadro do sprite do homunculo no mundo (data/sprite/homun/)
	for (TActorIterator<AMythMonster> It(GetWorld()); It; ++It)
	{
		if (It->NetID == Net->GetHomunId() && It->Sprite && !It->Sprite->MonsterSpriteName.IsEmpty())
		{
			const FString Key = It->Sprite->MonsterSpriteFolder / It->Sprite->MonsterSpriteName;
			if (Key != HomunPortraitKey)
			{
				if (UTexture2D* Tex = FMythRoUITextures::GetSpriteFrame(Key, 0))
				{
					HomunPortraitKey = Key;
					HomunPortrait->SetBrushFromTexture(Tex, false);
					const float Scale = FMath::Min(1.f, 90.f / FMath::Max(Tex->GetSizeX(), Tex->GetSizeY()));
					const FVector2D Size(Tex->GetSizeX() * Scale, Tex->GetSizeY() * Scale);
					HomunPortrait->SetDesiredSizeOverride(Size * UIScale);
					if (UCanvasPanelSlot* S = Cast<UCanvasPanelSlot>(HomunPortrait->Slot))
					{
						S->SetPosition(FVector2D(8.f + (96.f - Size.X) * 0.5f, 24.f + (96.f - Size.Y) * 0.5f) * UIScale);
						S->SetSize(Size * UIScale);
					}
				}
			}
			break;
		}
	}
	if (!H.Name.IsEmpty() && H.Name != HomunShownName)
	{
		HomunShownName = H.Name;
		HomunNameBox->SetText(FText::FromString(H.Name));
	}
	HomunNameBox->SetIsReadOnly(H.bRenamed);
	SetTextButtonShown(HomunRenameBtn, !H.bRenamed);
	const int32 Intimacy = H.Intimacy;
	const FString Loyalty = HomMsg(Intimacy < 100 ? 672 : Intimacy < 250 ? 673 : Intimacy < 600 ? 669 : Intimacy < 900 ? 674 : 675);
	const int32 Aspd = H.Amotion > 0 ? 200 - H.Amotion / 10 : 0;
	const FString Lines[HomLineCount] = {
		FString::Printf(TEXT("%s: %d"), *HomMsg(329), H.Level),
		FString::Printf(TEXT("%s: %d/100"), *HomMsg(585), H.Hunger),
		FString::Printf(TEXT("%s: %s"), *HomMsg(586), *Loyalty),
		FString::Printf(TEXT("Atq: %d"), H.Atk),
		FString::Printf(TEXT("Matq: %d"), H.Matk),
		FString::Printf(TEXT("Precisão: %d"), H.Hit),
		FString::Printf(TEXT("Crítico: %d"), H.Crit),
		FString::Printf(TEXT("Def: %d"), H.Def),
		FString::Printf(TEXT("Mdef: %d"), H.Mdef),
		FString::Printf(TEXT("Esquiva: %d"), H.Flee),
		FString::Printf(TEXT("ASPD: %d"), Aspd),
	};
	for (int32 i = 0; i < HomLineCount; ++i)
	{
		HomunLines[i]->SetText(FText::FromString(Lines[i]));
	}
	const int64 Cur[3] = { H.Hp, H.Sp, H.Exp };
	const int64 Max[3] = { H.MaxHp, H.MaxSp, H.ExpNext };
	static const TCHAR* Labels[3] = { TEXT("HP"), TEXT("SP"), TEXT("EXP") };
	for (int32 b = 0; b < 3; ++b)
	{
		const float Frac = Max[b] > 0 ? FMath::Clamp(static_cast<float>(Cur[b]) / static_cast<float>(Max[b]), 0.f, 1.f) : 0.f;
		if (UCanvasPanelSlot* S = Cast<UCanvasPanelSlot>(HomunBarFill[b]->Slot))
		{
			S->SetSize(FVector2D(HomBarW * Frac, 14.f) * UIScale);
		}
		HomunBarText[b]->SetText(FText::FromString(FString::Printf(TEXT("%s %lld / %lld"), Labels[b], Cur[b], Max[b])));
	}
	// Habilidades
	const TArray<FRoHomunSkill>& Skills = Net->GetHomunSkills();
	HomunSkillHeader->SetText(FText::FromString(FString::Printf(TEXT("%s  (pontos: %d)"), *HomMsg(937), H.SkillPoints)));
	for (int32 i = 0; i < HomunSkillRows; ++i)
	{
		const FRoHomunSkill* S = Skills.IsValidIndex(i) ? &Skills[i] : nullptr;
		HomunSkillNames[i]->SetVisibility(S ? ESlateVisibility::Visible : ESlateVisibility::Collapsed);
		HomunSkillUps[i]->SetVisibility(S && S->bUpgradable && H.SkillPoints > 0 ? ESlateVisibility::Visible : ESlateVisibility::Collapsed);
		UTexture2D* Icon = S ? FMythRoClientTables::SkillIcon(S->Id) : nullptr;
		HomunSkillIcons[i]->SetVisibility(Icon ? ESlateVisibility::HitTestInvisible : ESlateVisibility::Collapsed);
		if (!S)
		{
			continue;
		}
		if (Icon)
		{
			HomunSkillIcons[i]->SetBrushFromTexture(Icon, false);
			HomunSkillIcons[i]->SetDesiredSizeOverride(FVector2D(18.f, 18.f) * UIScale);
		}
		FString Name = FMythRoClientTables::SkillName(S->Id);
		if (Name.IsEmpty())
		{
			Name = FString::Printf(TEXT("Habilidade %d"), S->Id);
		}
		const bool bPassive = S->Inf == 0;
		HomunSkillNames[i]->SetText(FText::FromString(S->Level > 0
			? FString::Printf(TEXT("%s  Nv %d%s"), *Name, S->Level, bPassive ? TEXT("  (passiva)") : *FString::Printf(TEXT("  (%d SP)"), S->Sp))
			: FString::Printf(TEXT("%s  (não aprendida)"), *Name)));
		HomunSkillNames[i]->SetColorAndOpacity(FSlateColor(S->Level > 0 && !bPassive ? HomLink : HomGray));
	}
	SetTextButtonShown(HomunFeedBtn, true);
	HomunAutoFeed->SetText(FText::FromString(FString::Printf(TEXT("[%s] %s"), Net->IsHomunAutoFeed() ? TEXT("x") : TEXT("  "),
		*HomMsg(3279).Replace(TEXT("\\n"), TEXT(" ")))));
}

bool UMythRoHUDWidget::HandleHomunClick(const FVector2D& ScreenPos)
{
	URagnarokNetworkSubsystem* Net = GetNet();
	if (!Net || !HomunWindow || !HomunWindow->IsVisible())
	{
		return false;
	}
	if (IsUnder(HomunFeedBtn, ScreenPos))
	{
		Net->SendHomunMenu(1);
		return true;
	}
	if (IsUnder(HomunWaitBtn, ScreenPos))
	{
		if (AMythPlayerController* PC = Cast<AMythPlayerController>(GetOwningPlayer()))
		{
			PC->OnHotkeyHomunStandby();
		}
		return true;
	}
	if (IsUnder(HomunAutoFeed, ScreenPos))
	{
		Net->SendConfig(3, !Net->IsHomunAutoFeed());
		return true;
	}
	if (IsUnder(HomunRenameBtn, ScreenPos))
	{
		const FString Name = HomunNameBox->GetText().ToString().TrimStartAndEnd();
		if (!Name.IsEmpty() && Name != Net->GetHomunInfo().Name)
		{
			Net->SendHomunRename(Name);
		}
		return true;
	}
	const TArray<FRoHomunSkill>& Skills = Net->GetHomunSkills();
	for (int32 i = 0; i < HomunSkillRows && i < Skills.Num(); ++i)
	{
		const FRoHomunSkill& S = Skills[i];
		if (IsUnder(HomunSkillUps[i], ScreenPos))
		{
			Net->SendSkillUp(S.Id);
			return true;
		}
		if (IsUnder(HomunSkillNames[i], ScreenPos) && S.Level > 0 && S.Inf != 0)
		{
			// Alvo: em si (inf 4) o proprio homunculo; apoio (16) o dono; ofensiva (1) o alvo do dono
			int64 Target = Net->GetHomunId();
			if (S.Inf & 16)
			{
				Target = Net->GetSelfID();
			}
			else if (S.Inf & 1)
			{
				const AMythPlayerController* PC = Cast<AMythPlayerController>(GetOwningPlayer());
				const AMythMonster* Focus = PC ? PC->GetFocusedTarget() : nullptr;
				if (!Focus)
				{
					AddChatLine(HomMsg(3286), FLinearColor(1.f, 0.85f, 0.4f));
					return true;
				}
				Target = Focus->NetID;
			}
			Net->UseSkill(S.Id, S.Level, Target);
			return true;
		}
	}
	return false;
}

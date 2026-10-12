#include "UI/MythRoHUDWidget.h"
#include "UI/MythRoUITextures.h"
#include "Gameplay/MythStatsComponent.h"
#include "Network/RagnarokNetworkSubsystem.h"
#include "Sprites/MythRoSpriteComponent.h"
#include "Components/CanvasPanel.h"
#include "Components/CanvasPanelSlot.h"
#include "Components/Image.h"
#include "Components/TextBlock.h"
#include "GameFramework/Pawn.h"

// HUD propria do Myth of Thor (toque pessoal sobre o cliente 1:1): painel do personagem no canto superior esquerdo
// (avatar da classe, nome, barras de HP e SP com "atual / max" e %) e as barras de EXP de Base e Classe embaixo,
// com divisoes a cada 10%, dica ao passar o mouse e o botao de level up quando ha pontos para distribuir.

namespace
{
	constexpr float MhPanelW = 236.f;
	constexpr float MhPanelH = 56.f;
	constexpr float MhBarX = 76.f;
	constexpr float MhBarW = 118.f;
	constexpr float MhBarH = 10.f;
	constexpr float MhExpHalfW = 330.f;
	constexpr float MhExpGap = 12.f;
	constexpr float MhExpBarY = 26.f;
	constexpr float MhExpBarH = 12.f;

	const FLinearColor MhText(0.95f, 0.96f, 1.f);
	const FLinearColor MhBarBack(0.02f, 0.03f, 0.06f, 0.85f);

	void SetBox(UImage* Img, float X, float Y, float W, float H, float Scale)
	{
		if (UCanvasPanelSlot* S = Cast<UCanvasPanelSlot>(Img->Slot))
		{
			S->SetPosition(FVector2D(X, Y) * Scale);
			S->SetSize(FVector2D(W, H) * Scale);
		}
	}

	void OutlineText(UTextBlock* T)
	{
		FSlateFontInfo F = T->GetFont();
		F.OutlineSettings.OutlineSize = 1;
		F.OutlineSettings.OutlineColor = FLinearColor(0.f, 0.f, 0.f, 0.9f);
		T->SetFont(F);
	}
}

void UMythRoHUDWidget::BuildModernHud()
{
	// --- Painel do personagem (canto superior esquerdo)
	ModernPanel = MakeWindow(FVector2D(0.f, 0.f), FVector2D(0.f, 0.f), FVector2D(6.f * UIScale, 6.f * UIScale), FVector2D(MhPanelW, MhPanelH));
	AddRounded(ModernPanel, FLinearColor(0.05f, 0.06f, 0.11f, 0.78f), FLinearColor(0.5f, 0.55f, 0.7f, 0.7f), 0.f, 0.f, MhPanelW, MhPanelH, 8.f, 8.f)->SetVisibility(ESlateVisibility::Visible);
	AddRounded(ModernPanel, FLinearColor(0.13f, 0.15f, 0.25f, 0.95f), FLinearColor(0.7f, 0.75f, 0.9f, 1.f), 4.f, 4.f, 36.f, 36.f, 18.f, 18.f);
	ModernAvatar = AddImage(ModernPanel, nullptr, 6.f, 6.f, 32.f, 32.f);
	ModernAvatar->SetVisibility(ESlateVisibility::Collapsed);
	ModernClassIcon = AddImage(ModernPanel, nullptr, 46.f, 15.f, 24.f, 24.f); // simbolo da classe, o mesmo da janela do grupo
	ModernClassIcon->SetVisibility(ESlateVisibility::Collapsed);

	ModernName = AddText(ModernPanel, MhBarX, 3.f, 8, MhText, true);
	ModernJob = AddText(ModernPanel, MhBarX + MhBarW - 10.f, 4.f, 6, FLinearColor(0.7f, 0.78f, 1.f));
	ModernJob->SetJustification(ETextJustify::Right);

	auto MakeBar = [this](float Y, const FLinearColor& Fill, const TCHAR* Label, TObjectPtr<UImage>& OutFill, TObjectPtr<UTextBlock>& OutText, TObjectPtr<UTextBlock>& OutPct)
	{
		AddColor(ModernPanel, MhBarBack, MhBarX, Y, MhBarW, MhBarH);
		OutFill = AddColor(ModernPanel, Fill, MhBarX, Y, MhBarW, MhBarH);
		UTextBlock* Tag = AddText(ModernPanel, MhBarX + 3.f, Y - 1.f, 6, MhText);
		Tag->SetText(FText::FromString(Label));
		OutlineText(Tag);
		OutText = AddText(ModernPanel, MhBarX + 20.f, Y - 1.f, 6, MhText);
		OutlineText(OutText);
		OutPct = AddText(ModernPanel, MhBarX + MhBarW + 5.f, Y - 1.f, 6, MhText);
	};
	MakeBar(17.f, FLinearColor(0.2f, 0.78f, 0.28f), TEXT("HP"), ModernHPFill, ModernHPText, ModernHPPct);
	MakeBar(29.f, FLinearColor(0.2f, 0.45f, 0.95f), TEXT("SP"), ModernSPFill, ModernSPText, ModernSPPct);
	ModernWeightZeny = AddText(ModernPanel, MhBarX, 42.f, 6, FLinearColor(0.85f, 0.9f, 1.f));

	// --- Barras de EXP embaixo (Base a esquerda, Classe a direita), com o botao de level up acima do rotulo
	ModernExpStrip = MakeWindow(FVector2D(0.5f, 1.f), FVector2D(0.5f, 1.f), FVector2D(106.f, 0.f), FVector2D(2.f * MhExpHalfW + MhExpGap, 40.f));
	for (int32 Side = 0; Side < 2; ++Side)
	{
		const float X0 = Side * (MhExpHalfW + MhExpGap);
		AddColor(ModernExpStrip, MhBarBack, X0, MhExpBarY, MhExpHalfW, MhExpBarH);
		UImage* Fill = AddColor(ModernExpStrip, Side == 0 ? FLinearColor(0.95f, 0.75f, 0.2f) : FLinearColor(0.35f, 0.8f, 0.95f), X0, MhExpBarY, 1.f, MhExpBarH);
		for (int32 t = 1; t < 10; ++t) // divisoes a cada 10%
		{
			AddColor(ModernExpStrip, FLinearColor(1.f, 1.f, 1.f, 0.35f), X0 + MhExpHalfW * t / 10.f, MhExpBarY, 1.f, MhExpBarH);
		}
		UTextBlock* Label = AddText(ModernExpStrip, X0 + 4.f, MhExpBarY - 1.f, 7, MhText);
		OutlineText(Label);
		UImage* Hover = AddColor(ModernExpStrip, FLinearColor::Transparent, X0, MhExpBarY, MhExpHalfW, MhExpBarH);
		Hover->SetVisibility(ESlateVisibility::Visible); // precisa ser clicavel para mostrar a dica ao passar o mouse
		UImage* LvUp = AddImage(ModernExpStrip, FMythRoUITextures::Get(TEXT("basic_interface/lv_up_on.bmp")), X0 + 2.f, 2.f, 22.f, 22.f);
		LvUp->SetVisibility(ESlateVisibility::Collapsed);
		if (Side == 0) { ModernBaseFill = Fill; ModernBaseLabel = Label; ModernBaseHover = Hover; ModernBaseLvUp = LvUp; }
		else { ModernJobFill = Fill; ModernJobLabel = Label; ModernJobHover = Hover; ModernJobLvUp = LvUp; }
	}
	UpdateModernHud();
}

void UMythRoHUDWidget::UpdateModernHud()
{
	if (!ModernPanel || !Stats)
	{
		return;
	}
	const URagnarokNetworkSubsystem* Net = GetNet();
	const bool bOnline = Net && Net->GetNetworkState() == ERagnarokNetworkState::ConnectedMap;
	const FRagnarokPlayerStats* NetStats = bOnline ? &Net->GetPlayerStats() : nullptr;

	ModernName->SetText(FText::FromString(Stats->GetCharName()));
	ModernJob->SetText(FText::FromString(FString::Printf(TEXT("%s Nv.%d"), NetStats ? *NetStats->JobName : TEXT("Aprendiz"), Stats->GetBaseLevel())));

	auto Bar = [this](UImage* Fill, UTextBlock* Text, UTextBlock* Pct, float Y, int32 Cur, int32 Max, const FLinearColor& Color)
	{
		const float Frac = Max > 0 ? FMath::Clamp(static_cast<float>(Cur) / Max, 0.f, 1.f) : 0.f;
		SetBox(Fill, MhBarX, Y, MhBarW * Frac, MhBarH, UIScale);
		Fill->SetColorAndOpacity(Color);
		Text->SetText(FText::FromString(FString::Printf(TEXT("%d / %d"), Cur, Max)));
		Pct->SetText(FText::FromString(FString::Printf(TEXT("%d%%"), FMath::RoundToInt(Frac * 100.f))));
	};
	const float HPFrac = static_cast<float>(Stats->GetHP()) / FMath::Max(1, Stats->GetMaxHP());
	const FLinearColor HPColor = HPFrac > 0.5f ? FLinearColor(0.2f, 0.78f, 0.28f) : (HPFrac > 0.25f ? FLinearColor(0.95f, 0.75f, 0.15f) : FLinearColor(0.9f, 0.18f, 0.18f));
	Bar(ModernHPFill, ModernHPText, ModernHPPct, 17.f, Stats->GetHP(), Stats->GetMaxHP(), HPColor);
	Bar(ModernSPFill, ModernSPText, ModernSPPct, 29.f, Stats->GetSP(), Stats->GetMaxSP(), FLinearColor(0.2f, 0.45f, 0.95f));

	if (NetStats && NetStats->MaxWeight > 0)
	{
		const int32 WeightPct = FMath::RoundToInt(100.f * NetStats->Weight / NetStats->MaxWeight);
		ModernWeightZeny->SetText(FText::FromString(FString::Printf(TEXT("Peso %d%%   Zeny %s"), WeightPct, *FText::AsNumber(NetStats->Zeny).ToString())));
		ModernWeightZeny->SetColorAndOpacity(FSlateColor(WeightPct >= 50 ? FLinearColor(1.f, 0.45f, 0.4f) : FLinearColor(0.85f, 0.9f, 1.f)));
	}
	else
	{
		ModernWeightZeny->SetText(FText::FromString(TEXT("Peso 0%   Zeny 0")));
	}

	// Avatar: retrato do sprite do jogador (refeito quando o visual muda)
	const APawn* Pawn = GetOwningPlayerPawn();
	const UMythRoSpriteComponent* Sprite = Pawn ? Pawn->FindComponentByClass<UMythRoSpriteComponent>() : nullptr;
	if (Sprite && Sprite->GetLoadVersion() != ModernAvatarVersion)
	{
		ModernAvatarVersion = Sprite->GetLoadVersion();
		ModernAvatarTex = Sprite->RenderPortrait(32, 32, 34);
		if (ModernAvatarTex)
		{
			FSlateBrush Brush;
			Brush.SetResourceObject(ModernAvatarTex);
			Brush.ImageSize = FVector2D(32.f, 32.f) * UIScale;
			ModernAvatar->SetBrush(Brush);
		}
	}
	ModernAvatar->SetVisibility(ModernAvatarTex ? ESlateVisibility::HitTestInvisible : ESlateVisibility::Collapsed);

	if (NetStats)
	{
		UTexture2D* JobIcon = FMythRoUITextures::Get(FString::Printf(TEXT("renewalparty/icon_jobs_%d.bmp"), NetStats->JobClass));
		if (JobIcon)
		{
			FSlateBrush IconBrush;
			IconBrush.SetResourceObject(JobIcon);
			IconBrush.ImageSize = FVector2D(24.f, 24.f) * UIScale;
			ModernClassIcon->SetBrush(IconBrush);
		}
		ModernClassIcon->SetVisibility(JobIcon ? ESlateVisibility::HitTestInvisible : ESlateVisibility::Collapsed);
	}

	// EXP de Base e de Classe
	const float BaseFrac = Stats->GetExpFraction();
	const float JobFrac = NetStats && NetStats->NextJobExp > 0 ? static_cast<float>(static_cast<double>(NetStats->JobExp) / NetStats->NextJobExp) : 0.f;
	SetBox(ModernBaseFill, 0.f, MhExpBarY, MhExpHalfW * BaseFrac, MhExpBarH, UIScale);
	SetBox(ModernJobFill, MhExpHalfW + MhExpGap, MhExpBarY, MhExpHalfW * JobFrac, MhExpBarH, UIScale);
	ModernBaseLabel->SetText(FText::FromString(FString::Printf(TEXT("Base %d"), Stats->GetBaseLevel())));
	ModernJobLabel->SetText(FText::FromString(FString::Printf(TEXT("Job %d"), NetStats ? NetStats->JobLevel : 1)));
	ModernBaseHover->SetToolTipText(FText::FromString(FString::Printf(TEXT("EXP de Base: %.2f%%"), BaseFrac * 100.f)));
	ModernJobHover->SetToolTipText(FText::FromString(FString::Printf(TEXT("EXP de Classe: %.2f%%"), JobFrac * 100.f)));

	// Botoes de level up: so com pontos pendentes (Base: atributos; Classe: habilidades)
	ModernBaseLvUp->SetVisibility(NetStats && NetStats->StatusPoints > 0 ? ESlateVisibility::Visible : ESlateVisibility::Collapsed);
	ModernJobLvUp->SetVisibility(NetStats && NetStats->SkillPoints > 0 ? ESlateVisibility::Visible : ESlateVisibility::Collapsed);
	if (NetStats)
	{
		ModernBaseLvUp->SetToolTipText(FText::FromString(FString::Printf(TEXT("%d pontos de atributo para distribuir"), NetStats->StatusPoints)));
		ModernJobLvUp->SetToolTipText(FText::FromString(FString::Printf(TEXT("%d pontos de habilidade para distribuir"), NetStats->SkillPoints)));
	}
}

bool UMythRoHUDWidget::HandleModernHudClick(const FVector2D& ScreenPos)
{
	if (ModernBaseLvUp && ModernBaseLvUp->IsVisible() && IsUnder(ModernBaseLvUp, ScreenPos))
	{
		if (!StatusWindow || !StatusWindow->IsVisible()) { ToggleWindow(EMythRoWindow::Status); }
		return true;
	}
	if (ModernJobLvUp && ModernJobLvUp->IsVisible() && IsUnder(ModernJobLvUp, ScreenPos))
	{
		if (!SkillsWindow || !SkillsWindow->IsVisible()) { ToggleWindow(EMythRoWindow::Skills); }
		return true;
	}
	return false;
}

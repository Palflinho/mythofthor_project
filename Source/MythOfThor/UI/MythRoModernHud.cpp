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
	constexpr float MhPanelW = 214.f;
	constexpr float MhTitleH = 17.f;  // barra de titulo azul das janelas
	constexpr float MhPanelH = 56.f + MhTitleH;
	constexpr float MhY1 = 17.f + MhTitleH;  // barra de HP
	constexpr float MhY2 = 29.f + MhTitleH;  // barra de SP
	constexpr float MhBarX = 54.f;
	constexpr float MhBarW = 118.f;
	constexpr float MhBarH = 10.f;
	constexpr float MhExpHalfW = 330.f;
	constexpr float MhExpGap = 12.f;
	constexpr float MhExpBarY = 26.f;
	constexpr float MhExpBarH = 12.f;

	const FLinearColor MhBody(0.965f, 0.965f, 0.98f, 1.f);        // fundo claro das janelas
	const FLinearColor MhInk(0.1f, 0.12f, 0.2f);                  // texto escuro
	const FLinearColor MhText(1.f, 1.f, 1.f);                     // texto sobre as barras coloridas (com contorno)
	const FLinearColor MhBarBack(0.78f, 0.82f, 0.9f, 1.f);

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
	AddRounded(ModernPanel, MhBody, FLinearColor::Transparent, 0.f, 9.f, MhPanelW, MhPanelH - 9.f, 0.f, 7.f)->SetVisibility(ESlateVisibility::Visible);
	AddTitleBar(ModernPanel, MhPanelW, TEXT("Personagem"));
	AddRounded(ModernPanel, FLinearColor(0.86f, 0.9f, 0.98f, 1.f), FLinearColor(0.45f, 0.55f, 0.8f, 1.f), 4.f, MhTitleH + 4.f, 36.f, 36.f, 18.f, 18.f);
	ModernClassIcon = AddImage(ModernPanel, nullptr, 8.f, MhTitleH + 8.f, 28.f, 28.f); // simbolo da classe (o mesmo da janela do grupo)
	ModernClassIcon->SetVisibility(ESlateVisibility::Collapsed);

	// nome do personagem no cabecalho; o nome da CLASSE fica embaixo do simbolo dela
	ModernName = AddText(ModernPanel, MhBarX, MhTitleH + 2.f, 8, MhInk, true);
	ModernClassName = AddText(ModernPanel, 1.f, MhTitleH + 42.f, 6, FLinearColor(0.12f, 0.27f, 0.55f));
	ModernClassName->SetJustification(ETextJustify::Center);
	if (UCanvasPanelSlot* NS = Cast<UCanvasPanelSlot>(ModernClassName->Slot)) { NS->SetAutoSize(false); NS->SetSize(FVector2D(50.f, 11.f) * UIScale); }
	ModernJob = AddText(ModernPanel, MhBarX + MhBarW - 10.f, MhTitleH + 3.f, 6, FLinearColor(0.12f, 0.27f, 0.55f));
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
		OutPct = AddText(ModernPanel, MhBarX + MhBarW + 5.f, Y - 1.f, 6, MhInk);
	};
	MakeBar(MhY1, FLinearColor(0.2f, 0.78f, 0.28f), TEXT("HP"), ModernHPFill, ModernHPText, ModernHPPct);
	MakeBar(MhY2, FLinearColor(0.2f, 0.45f, 0.95f), TEXT("SP"), ModernSPFill, ModernSPText, ModernSPPct);
	ModernWeightZeny = AddText(ModernPanel, MhBarX, MhTitleH + 42.f, 6, MhInk);

	// --- Barras de EXP embaixo (Base a esquerda, Classe a direita), com o botao de level up acima do rotulo
	ModernExpStrip = MakeWindow(FVector2D(0.5f, 1.f), FVector2D(0.5f, 1.f), FVector2D(106.f, 0.f), FVector2D(2.f * MhExpHalfW + MhExpGap, 40.f));
	for (int32 Side = 0; Side < 2; ++Side)
	{
		const float X0 = Side * (MhExpHalfW + MhExpGap);
		AddRounded(ModernExpStrip, MhBody, FLinearColor::Transparent, X0 - 3.f, MhExpBarY - 3.f, MhExpHalfW + 6.f, MhExpBarH + 6.f, 4.f, 4.f);
		AddColor(ModernExpStrip, MhBarBack, X0, MhExpBarY, MhExpHalfW, MhExpBarH);
		UImage* Fill = AddColor(ModernExpStrip, Side == 0 ? FLinearColor(0.95f, 0.75f, 0.2f) : FLinearColor(0.35f, 0.8f, 0.95f), X0, MhExpBarY, 1.f, MhExpBarH);
		for (int32 t = 1; t < 10; ++t) // divisoes a cada 10%
		{
			AddColor(ModernExpStrip, FLinearColor(0.1f, 0.15f, 0.3f, 0.3f), X0 + MhExpHalfW * t / 10.f, MhExpBarY, 1.f, MhExpBarH);
		}
		UTextBlock* Label = AddText(ModernExpStrip, Side == 0 ? X0 + 4.f : X0 + MhExpHalfW - 74.f, MhExpBarY - 1.f, 7, MhInk);
		if (Side == 1)
		{
			Label->SetJustification(ETextJustify::Right);
			if (UCanvasPanelSlot* LS = Cast<UCanvasPanelSlot>(Label->Slot)) { LS->SetAutoSize(false); LS->SetSize(FVector2D(70.f, 12.f) * UIScale); }
		}
		UImage* Hover = AddColor(ModernExpStrip, FLinearColor::Transparent, X0, MhExpBarY, MhExpHalfW, MhExpBarH);
		Hover->SetVisibility(ESlateVisibility::Visible); // precisa ser clicavel para mostrar a dica ao passar o mouse
		// o botao de level up fica acima do rotulo (Base: canto esquerdo; Classe: canto direito, espelhado)
		UImage* LvUp = AddImage(ModernExpStrip, FMythRoUITextures::Get(TEXT("basic_interface/lv_up_on.bmp")), Side == 0 ? X0 + 2.f : X0 + MhExpHalfW - 24.f, 2.f, 22.f, 22.f);
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
	{
		const FString ClassName = NetStats ? NetStats->JobName : FString(TEXT("Aprendiz"));
		ModernClassName->SetText(FText::FromString(ClassName.Len() > 11 ? ClassName.Left(10) + TEXT("…") : ClassName));
	}
	ModernJob->SetText(FText::FromString(FString::Printf(TEXT("Nv. %d"), Stats->GetBaseLevel()))); // a classe fica embaixo do simbolo

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
	Bar(ModernHPFill, ModernHPText, ModernHPPct, MhY1, Stats->GetHP(), Stats->GetMaxHP(), HPColor);
	Bar(ModernSPFill, ModernSPText, ModernSPPct, MhY2, Stats->GetSP(), Stats->GetMaxSP(), FLinearColor(0.2f, 0.45f, 0.95f));

	if (NetStats && NetStats->MaxWeight > 0)
	{
		const int32 WeightPct = FMath::RoundToInt(100.f * NetStats->Weight / NetStats->MaxWeight);
		ModernWeightZeny->SetText(FText::FromString(FString::Printf(TEXT("Peso %d%%   Zeny %s"), WeightPct, *FText::AsNumber(NetStats->Zeny).ToString())));
		ModernWeightZeny->SetColorAndOpacity(FSlateColor(WeightPct >= 50 ? FLinearColor(0.8f, 0.15f, 0.1f) : MhInk));
	}
	else
	{
		ModernWeightZeny->SetText(FText::FromString(TEXT("Peso 0%   Zeny 0")));
	}

	if (NetStats)
	{
		UTexture2D* JobIcon = FMythRoUITextures::Get(FString::Printf(TEXT("renewalparty/icon_jobs_%d.bmp"), NetStats->JobClass));
		if (JobIcon)
		{
			FSlateBrush IconBrush;
			IconBrush.SetResourceObject(JobIcon);
			IconBrush.ImageSize = FVector2D(28.f, 28.f) * UIScale;
			ModernClassIcon->SetBrush(IconBrush);
		}
		ModernClassIcon->SetVisibility(JobIcon ? ESlateVisibility::HitTestInvisible : ESlateVisibility::Collapsed);
	}

	// EXP de Base e de Classe
	const float BaseFrac = Stats->GetExpFraction();
	const float JobFrac = NetStats && NetStats->NextJobExp > 0 ? static_cast<float>(static_cast<double>(NetStats->JobExp) / NetStats->NextJobExp) : 0.f;
	SetBox(ModernBaseFill, 0.f, MhExpBarY, MhExpHalfW * BaseFrac, MhExpBarH, UIScale);
	SetBox(ModernJobFill, MhExpHalfW + MhExpGap + MhExpHalfW * (1.f - JobFrac), MhExpBarY, MhExpHalfW * JobFrac, MhExpBarH, UIScale); // cresce da direita para o centro
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

#include "UI/MythClassicHUDWidget.h"

#include "Gameplay/AutoBattleComponent.h"
#include "Gameplay/MythStatsComponent.h"

#include "Blueprint/WidgetTree.h"
#include "Components/Border.h"
#include "Components/Button.h"
#include "Components/ButtonSlot.h"
#include "Components/CanvasPanel.h"
#include "Components/CanvasPanelSlot.h"
#include "Components/HorizontalBox.h"
#include "Components/HorizontalBoxSlot.h"
#include "Components/Overlay.h"
#include "Components/OverlaySlot.h"
#include "Components/ProgressBar.h"
#include "Components/SizeBox.h"
#include "Components/Spacer.h"
#include "Components/TextBlock.h"
#include "Components/VerticalBox.h"
#include "Components/VerticalBoxSlot.h"
#include "GameFramework/Pawn.h"
#include "Styling/CoreStyle.h"

namespace
{
	const FLinearColor ColHP(0.20f, 0.85f, 0.25f);
	const FLinearColor ColSP(0.20f, 0.45f, 1.00f);
	const FLinearColor ColExp(0.95f, 0.75f, 0.15f);
	const FLinearColor ColPanel(0.02f, 0.03f, 0.08f, 0.65f);
	const FLinearColor ColBarBack(0.f, 0.f, 0.f, 0.7f);

	FString FormatClock(float Seconds)
	{
		const int32 Total = FMath::Max(0, FMath::CeilToInt(Seconds));
		return FString::Printf(TEXT("%02d:%02d:%02d"), Total / 3600, (Total / 60) % 60, Total % 60);
	}
}

void UMythClassicHUDWidget::NativeOnInitialized()
{
	Super::NativeOnInitialized();
	BuildLayout();
}

void UMythClassicHUDWidget::BuildLayout()
{
	if (!WidgetTree)
	{
		return;
	}

	auto MakeText = [this](int32 Size, const FLinearColor& Color, const TCHAR* Font = TEXT("Bold"))
	{
		UTextBlock* T = WidgetTree->ConstructWidget<UTextBlock>();
		T->SetFont(FCoreStyle::GetDefaultFontStyle(Font, Size));
		T->SetColorAndOpacity(FSlateColor(Color));
		T->SetShadowOffset(FVector2D(1.f, 1.f));
		T->SetShadowColorAndOpacity(FLinearColor(0.f, 0.f, 0.f, 0.9f));
		return T;
	};

	// Barra com fundo escuro e texto centralizado por cima: devolve o Overlay e entrega Bar/Text.
	auto MakeBar = [this, &MakeText](const FLinearColor& Fill, float W, float H, UProgressBar*& OutBar, UTextBlock*& OutText) -> USizeBox*
	{
		USizeBox* Box = WidgetTree->ConstructWidget<USizeBox>();
		Box->SetWidthOverride(W);
		Box->SetHeightOverride(H);
		UOverlay* Ov = WidgetTree->ConstructWidget<UOverlay>();
		Box->AddChild(Ov);

		OutBar = WidgetTree->ConstructWidget<UProgressBar>();
		OutBar->SetFillColorAndOpacity(Fill);
		OutBar->SetPercent(1.f);
		if (UOverlaySlot* S = Ov->AddChildToOverlay(OutBar))
		{
			S->SetHorizontalAlignment(HAlign_Fill);
			S->SetVerticalAlignment(VAlign_Fill);
		}

		OutText = MakeText(9, FLinearColor::White);
		if (UOverlaySlot* S = Ov->AddChildToOverlay(OutText))
		{
			S->SetHorizontalAlignment(HAlign_Center);
			S->SetVerticalAlignment(VAlign_Center);
		}
		return Box;
	};

	UCanvasPanel* Root = WidgetTree->ConstructWidget<UCanvasPanel>();
	Root->SetVisibility(ESlateVisibility::SelfHitTestInvisible); // so o botao captura cliques
	WidgetTree->RootWidget = Root;

	// ------------------------------------------------ Topo-esquerda: nome, nivel, HP e SP
	{
		UBorder* Panel = WidgetTree->ConstructWidget<UBorder>();
		Panel->SetBrushColor(ColPanel);
		Panel->SetPadding(FMargin(10.f, 6.f));
		UVerticalBox* VB = WidgetTree->ConstructWidget<UVerticalBox>();
		Panel->SetContent(VB);

		NameText = MakeText(11, FLinearColor::White);
		VB->AddChildToVerticalBox(NameText);

		UProgressBar* Bar; UTextBlock* Txt;
		USizeBox* HPBox = MakeBar(ColHP, 220.f, 14.f, Bar, Txt);
		HPBar = Bar; HPText = Txt;
		if (UVerticalBoxSlot* S = VB->AddChildToVerticalBox(HPBox)) { S->SetPadding(FMargin(0.f, 4.f, 0.f, 2.f)); }

		USizeBox* SPBox = MakeBar(ColSP, 220.f, 14.f, Bar, Txt);
		SPBar = Bar; SPText = Txt;
		VB->AddChildToVerticalBox(SPBox);

		if (UCanvasPanelSlot* S = Root->AddChildToCanvas(Panel))
		{
			S->SetAnchors(FAnchors(0.f, 0.f));
			S->SetPosition(FVector2D(16.f, 16.f));
			S->SetAutoSize(true);
		}
	}

	// ------------------------------------------------ Rodape: EXP com 10 marcadores de 10%
	{
		UOverlay* Ov = WidgetTree->ConstructWidget<UOverlay>();

		ExpBar = WidgetTree->ConstructWidget<UProgressBar>();
		ExpBar->SetFillColorAndOpacity(ColExp);
		ExpBar->SetPercent(0.f);
		if (UOverlaySlot* S = Ov->AddChildToOverlay(ExpBar))
		{
			S->SetHorizontalAlignment(HAlign_Fill);
			S->SetVerticalAlignment(VAlign_Fill);
		}

		UHorizontalBox* Marks = WidgetTree->ConstructWidget<UHorizontalBox>();
		for (int32 i = 0; i < 10; ++i)
		{
			USpacer* Cell = WidgetTree->ConstructWidget<USpacer>();
			if (UHorizontalBoxSlot* S = Marks->AddChildToHorizontalBox(Cell))
			{
				S->SetSize(FSlateChildSize(ESlateSizeRule::Fill));
			}
			if (i < 9)
			{
				USizeBox* Sep = WidgetTree->ConstructWidget<USizeBox>();
				Sep->SetWidthOverride(2.f);
				UBorder* Line = WidgetTree->ConstructWidget<UBorder>();
				Line->SetBrushColor(FLinearColor(0.f, 0.f, 0.f, 0.75f));
				Sep->AddChild(Line);
				Marks->AddChildToHorizontalBox(Sep);
			}
		}
		if (UOverlaySlot* S = Ov->AddChildToOverlay(Marks))
		{
			S->SetHorizontalAlignment(HAlign_Fill);
			S->SetVerticalAlignment(VAlign_Fill);
		}

		ExpText = MakeText(9, FLinearColor::White);
		if (UOverlaySlot* S = Ov->AddChildToOverlay(ExpText))
		{
			S->SetHorizontalAlignment(HAlign_Center);
			S->SetVerticalAlignment(VAlign_Center);
		}

		if (UCanvasPanelSlot* S = Root->AddChildToCanvas(Ov))
		{
			S->SetAnchors(FAnchors(0.f, 1.f, 1.f, 1.f)); // estica na largura, colado no rodape
			S->SetOffsets(FMargin(0.f, -16.f, 0.f, 16.f));
		}
	}

	// ------------------------------------------------ Baixo-direita: Auto-Battle
	{
		UBorder* Panel = WidgetTree->ConstructWidget<UBorder>();
		Panel->SetBrushColor(ColPanel);
		Panel->SetPadding(FMargin(10.f, 8.f));
		UVerticalBox* VB = WidgetTree->ConstructWidget<UVerticalBox>();
		Panel->SetContent(VB);

		AutoButton = WidgetTree->ConstructWidget<UButton>();
		AutoButtonText = MakeText(11, FLinearColor::White);
		AutoButton->AddChild(AutoButtonText);
		AutoButton->OnClicked.AddDynamic(this, &UMythClassicHUDWidget::OnAutoButtonClicked);
		if (UVerticalBoxSlot* S = VB->AddChildToVerticalBox(AutoButton)) { S->SetPadding(FMargin(0.f, 0.f, 0.f, 6.f)); }

		UProgressBar* Bar; UTextBlock* Txt;
		USizeBox* StamBox = MakeBar(FLinearColor(0.95f, 0.55f, 0.1f), 220.f, 14.f, Bar, Txt);
		StaminaBar = Bar; StaminaText = Txt;
		VB->AddChildToVerticalBox(StamBox);

		WeightText = MakeText(9, FLinearColor::White, TEXT("Regular"));
		if (UVerticalBoxSlot* S = VB->AddChildToVerticalBox(WeightText)) { S->SetPadding(FMargin(0.f, 4.f, 0.f, 0.f)); }

		MessageText = MakeText(9, FLinearColor(1.f, 0.85f, 0.3f), TEXT("Regular"));
		MessageText->SetAutoWrapText(true);
		MessageText->SetVisibility(ESlateVisibility::Collapsed);
		VB->AddChildToVerticalBox(MessageText);

		if (UCanvasPanelSlot* S = Root->AddChildToCanvas(Panel))
		{
			S->SetAnchors(FAnchors(1.f, 1.f));
			S->SetAlignment(FVector2D(1.f, 1.f));
			S->SetPosition(FVector2D(-16.f, -36.f));
			S->SetAutoSize(true);
		}
	}

	RefreshStats();
	RefreshAutoAll();
}

void UMythClassicHUDWidget::BindToPawn(APawn* Pawn)
{
	if (Stats)
	{
		Stats->OnStatsChanged.RemoveDynamic(this, &UMythClassicHUDWidget::RefreshStats);
	}
	if (AutoBattle)
	{
		AutoBattle->OnAutoBattleStateChanged.RemoveDynamic(this, &UMythClassicHUDWidget::RefreshAutoState);
		AutoBattle->OnAutoBattleTimeUpdated.RemoveDynamic(this, &UMythClassicHUDWidget::RefreshStamina);
		AutoBattle->OnAutoBattleError.RemoveDynamic(this, &UMythClassicHUDWidget::ShowMessage);
		AutoBattle->OnInventoryWeightChanged.RemoveDynamic(this, &UMythClassicHUDWidget::RefreshWeight);
	}

	Stats = Pawn ? Pawn->FindComponentByClass<UMythStatsComponent>() : nullptr;
	AutoBattle = Pawn ? Pawn->FindComponentByClass<UAutoBattleComponent>() : nullptr;

	if (Stats)
	{
		Stats->OnStatsChanged.AddDynamic(this, &UMythClassicHUDWidget::RefreshStats);
	}
	if (AutoBattle)
	{
		AutoBattle->OnAutoBattleStateChanged.AddDynamic(this, &UMythClassicHUDWidget::RefreshAutoState);
		AutoBattle->OnAutoBattleTimeUpdated.AddDynamic(this, &UMythClassicHUDWidget::RefreshStamina);
		AutoBattle->OnAutoBattleError.AddDynamic(this, &UMythClassicHUDWidget::ShowMessage);
		AutoBattle->OnInventoryWeightChanged.AddDynamic(this, &UMythClassicHUDWidget::RefreshWeight);
	}

	RefreshStats();
	RefreshAutoAll();
}

void UMythClassicHUDWidget::NativeTick(const FGeometry& MyGeometry, float InDeltaTime)
{
	Super::NativeTick(MyGeometry, InDeltaTime);

	if (MessageTimeLeft > 0.f)
	{
		MessageTimeLeft -= InDeltaTime;
		if (MessageTimeLeft <= 0.f && MessageText)
		{
			MessageText->SetVisibility(ESlateVisibility::Collapsed);
		}
	}
}

void UMythClassicHUDWidget::RefreshStats()
{
	if (!NameText)
	{
		return;
	}
	if (!Stats)
	{
		NameText->SetText(FText::GetEmpty());
		return;
	}

	NameText->SetText(FText::FromString(FString::Printf(TEXT("%s   Lv. %d"), *Stats->GetCharName(), Stats->GetBaseLevel())));

	HPBar->SetPercent(static_cast<float>(Stats->GetHP()) / FMath::Max(1, Stats->GetMaxHP()));
	HPText->SetText(FText::FromString(FString::Printf(TEXT("HP  %d / %d"), Stats->GetHP(), Stats->GetMaxHP())));

	SPBar->SetPercent(static_cast<float>(Stats->GetSP()) / FMath::Max(1, Stats->GetMaxSP()));
	SPText->SetText(FText::FromString(FString::Printf(TEXT("SP  %d / %d"), Stats->GetSP(), Stats->GetMaxSP())));

	const float Exp = Stats->GetExpFraction();
	ExpBar->SetPercent(Exp);
	ExpText->SetText(FText::FromString(FString::Printf(TEXT("Base EXP  %.2f%%"), Exp * 100.f)));
}

void UMythClassicHUDWidget::RefreshAutoAll()
{
	if (!AutoButton)
	{
		return;
	}
	if (!AutoBattle)
	{
		AutoButton->SetIsEnabled(false);
		AutoButtonText->SetText(FText::FromString(TEXT("AUTO-BATTLE: --")));
		return;
	}
	LastShownSecond = -1;
	RefreshAutoState(AutoBattle->IsAutoBattleActive());
	RefreshStamina(AutoBattle->GetRemainingStaminaSeconds());
	RefreshWeight(AutoBattle->GetInventoryWeightPercent(), AutoBattle->IsOverweight());
}

void UMythClassicHUDWidget::RefreshAutoState(bool bIsActive)
{
	if (!AutoButton || !AutoBattle)
	{
		return;
	}
	AutoButtonText->SetText(FText::FromString(bIsActive ? TEXT("AUTO-BATTLE: ON") : TEXT("AUTO-BATTLE: OFF")));
	AutoButtonText->SetColorAndOpacity(FSlateColor(bIsActive ? ColHP : FLinearColor::White));

	// Sempre da para desligar; para ligar, nao pode estar caido, sem vigor ou com sobrepeso.
	const bool bCanStart = !AutoBattle->IsPlayerDowned() && !AutoBattle->IsOverweight() && AutoBattle->GetRemainingStaminaSeconds() > 0.f;
	AutoButton->SetIsEnabled(bIsActive || bCanStart);
}

void UMythClassicHUDWidget::RefreshStamina(float RemainingSeconds)
{
	if (!StaminaBar)
	{
		return;
	}
	const int32 Sec = FMath::CeilToInt(RemainingSeconds);
	if (Sec == LastShownSecond)
	{
		return; // o componente emite todo frame; so atualiza o texto quando o segundo muda
	}
	LastShownSecond = Sec;
	StaminaBar->SetPercent(RemainingSeconds / UAutoBattleComponent::MAX_DAILY_STAMINA_SECONDS);
	StaminaText->SetText(FText::FromString(FString::Printf(TEXT("Vigor  %s / 03:00:00"), *FormatClock(RemainingSeconds))));
	if (Sec <= 0 && AutoBattle)
	{
		RefreshAutoState(AutoBattle->IsAutoBattleActive());
	}
}

void UMythClassicHUDWidget::RefreshWeight(float WeightPercent, bool bOverweight)
{
	if (!WeightText)
	{
		return;
	}
	if (bOverweight)
	{
		WeightText->SetText(FText::FromString(FString::Printf(TEXT("SOBREPESO %.0f%% - Auto-Battle bloqueado"), WeightPercent)));
		WeightText->SetColorAndOpacity(FSlateColor(FLinearColor(1.f, 0.25f, 0.25f)));
	}
	else
	{
		WeightText->SetText(FText::FromString(FString::Printf(TEXT("Peso %.0f%%  (bloqueia em %.0f%%)"), WeightPercent, UAutoBattleComponent::OVERWEIGHT_THRESHOLD_PERCENT)));
		WeightText->SetColorAndOpacity(FSlateColor(FLinearColor::White));
	}
	if (AutoBattle)
	{
		RefreshAutoState(AutoBattle->IsAutoBattleActive());
	}
}

void UMythClassicHUDWidget::ShowMessage(const FString& Reason)
{
	if (MessageText)
	{
		MessageText->SetText(FText::FromString(Reason));
		MessageText->SetVisibility(ESlateVisibility::HitTestInvisible);
		MessageTimeLeft = 5.f;
	}
}

void UMythClassicHUDWidget::OnAutoButtonClicked()
{
	if (AutoBattle)
	{
		AutoBattle->ToggleAutoBattle(!AutoBattle->IsAutoBattleActive());
	}
}

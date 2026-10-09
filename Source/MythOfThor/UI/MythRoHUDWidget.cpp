#include "UI/MythRoHUDWidget.h"

#include "UI/MythRoUITextures.h"
#include "Gameplay/AutoBattleComponent.h"
#include "Gameplay/MythStatsComponent.h"
#include "World/MythMapInfo.h"
#include "Network/RagnarokNetworkSubsystem.h"
#include "Player/MythPlayerController.h"

#include "Blueprint/WidgetTree.h"
#include "Components/Button.h"
#include "Components/CanvasPanel.h"
#include "Components/CanvasPanelSlot.h"
#include "Components/EditableTextBox.h"
#include "Components/Image.h"
#include "Components/TextBlock.h"
#include "Components/VerticalBox.h"
#include "Components/VerticalBoxSlot.h"
#include "Framework/Application/SlateApplication.h"
#include "GameFramework/Pawn.h"
#include "Styling/CoreStyle.h"

namespace
{
	// Cores do cliente original
	const FLinearColor ColText(0.08f, 0.10f, 0.18f);        // texto escuro das janelas claras
	const FLinearColor ColLabel(0.12f, 0.27f, 0.55f);       // rotulos azulados
	const FLinearColor ColTitle(1.f, 1.f, 1.f);
	const FLinearColor ColWindow(0.965f, 0.965f, 0.975f, 0.94f);
	const FLinearColor ColWindowEdge(0.62f, 0.64f, 0.70f, 0.95f);
	const FLinearColor ColChatBack(0.f, 0.f, 0.f, 0.38f);
	const FLinearColor ColSystem(0.55f, 0.95f, 0.55f);
	const FLinearColor ColError(1.f, 0.45f, 0.45f);
	constexpr int32 MaxChatLines = 7;

	FString Basic(const TCHAR* Name) { return FString::Printf(TEXT("basic_interface/%s.bmp"), Name); }

	FString RoHudFormatClock(float Seconds)
	{
		const int32 Total = FMath::Max(0, FMath::CeilToInt(Seconds));
		return FString::Printf(TEXT("%02d:%02d:%02d"), Total / 3600, (Total / 60) % 60, Total % 60);
	}

	FSlateBrush TextureBrush(UTexture2D* Tex, const FVector2D& Size)
	{
		FSlateBrush Brush;
		Brush.SetResourceObject(Tex);
		Brush.ImageSize = Size;
		Brush.DrawAs = ESlateBrushDrawType::Image;
		return Brush;
	}
}

// ------------------------------------------------------------------------------------------------ medidor

void FMythRoGauge::SetFraction(float Fraction)
{
	if (!Mid)
	{
		return;
	}
	const float Fill = Width * FMath::Clamp(Fraction, 0.f, 1.f);
	const bool bShow = Fill >= 1.f;
	const float CapW = FMath::Min(4.f, Fill * 0.5f);
	auto Place = [this](UImage* Img, float PX, float W)
	{
		if (UCanvasPanelSlot* S = Img ? Cast<UCanvasPanelSlot>(Img->Slot) : nullptr)
		{
			S->SetPosition(FVector2D(PX, Y) * Scale);
			S->SetSize(FVector2D(FMath::Max(W, 0.f), Height) * Scale);
		}
	};
	Place(Left, X, CapW);
	Place(Mid, X + CapW, Fill - 2.f * CapW);
	Place(Right, X + Fill - CapW, CapW);
	for (UImage* Img : { Left.Get(), Mid.Get(), Right.Get() })
	{
		if (Img)
		{
			Img->SetVisibility(bShow ? ESlateVisibility::HitTestInvisible : ESlateVisibility::Collapsed);
		}
	}
}

// ------------------------------------------------------------------------------------------------ construcao

void UMythRoHUDWidget::NativeOnInitialized()
{
	Super::NativeOnInitialized();
	BuildLayout();
}

UCanvasPanel* UMythRoHUDWidget::MakeWindow(const FVector2D& Anchor, const FVector2D& Alignment, const FVector2D& Offset, const FVector2D& Size)
{
	UCanvasPanel* Win = WidgetTree->ConstructWidget<UCanvasPanel>();
	Win->SetVisibility(ESlateVisibility::SelfHitTestInvisible);
	if (UCanvasPanelSlot* S = Root->AddChildToCanvas(Win))
	{
		S->SetAnchors(FAnchors(Anchor.X, Anchor.Y));
		S->SetAlignment(Alignment);
		S->SetPosition(Offset);
		S->SetSize(Size * UIScale);
	}
	return Win;
}

UImage* UMythRoHUDWidget::AddImage(UCanvasPanel* Parent, UTexture2D* Tex, float X, float Y, float W, float H)
{
	UImage* Img = WidgetTree->ConstructWidget<UImage>();
	Img->SetVisibility(ESlateVisibility::HitTestInvisible);
	const float TW = W >= 0.f ? W : (Tex ? Tex->GetSizeX() : 1.f);
	const float TH = H >= 0.f ? H : (Tex ? Tex->GetSizeY() : 1.f);
	if (Tex)
	{
		Img->SetBrush(TextureBrush(Tex, FVector2D(TW, TH) * UIScale));
	}
	if (UCanvasPanelSlot* S = Parent->AddChildToCanvas(Img))
	{
		S->SetPosition(FVector2D(X, Y) * UIScale);
		S->SetSize(FVector2D(TW, TH) * UIScale);
	}
	return Img;
}

UImage* UMythRoHUDWidget::AddColor(UCanvasPanel* Parent, const FLinearColor& Color, float X, float Y, float W, float H)
{
	UImage* Img = WidgetTree->ConstructWidget<UImage>();
	Img->SetVisibility(ESlateVisibility::HitTestInvisible);
	Img->SetColorAndOpacity(Color);
	if (UCanvasPanelSlot* S = Parent->AddChildToCanvas(Img))
	{
		S->SetPosition(FVector2D(X, Y) * UIScale);
		S->SetSize(FVector2D(W, H) * UIScale);
	}
	return Img;
}

UTextBlock* UMythRoHUDWidget::AddText(UCanvasPanel* Parent, float X, float Y, int32 Size, const FLinearColor& Color, bool bShadow)
{
	UTextBlock* T = WidgetTree->ConstructWidget<UTextBlock>();
	T->SetFont(FCoreStyle::GetDefaultFontStyle("Regular", FMath::RoundToInt(Size * UIScale)));
	T->SetColorAndOpacity(FSlateColor(Color));
	if (bShadow)
	{
		T->SetShadowOffset(FVector2D(1.f, 1.f));
		T->SetShadowColorAndOpacity(FLinearColor(0.f, 0.f, 0.f, 0.85f));
	}
	if (UCanvasPanelSlot* S = Parent->AddChildToCanvas(T))
	{
		S->SetPosition(FVector2D(X, Y) * UIScale);
		S->SetAutoSize(true);
	}
	return T;
}

void UMythRoHUDWidget::AddTitleBar(UCanvasPanel* Parent, float Width, const FString& Title, UTextBlock** OutText)
{
	// Barra de titulo classica: titlebar_left + titlebar_mid (esticado) + titlebar_right (17 px de altura)
	AddImage(Parent, FMythRoUITextures::Get(Basic(TEXT("titlebar_left"))), 0.f, 0.f, 12.f, 17.f);
	AddImage(Parent, FMythRoUITextures::Get(Basic(TEXT("titlebar_mid"))), 12.f, 0.f, Width - 24.f, 17.f);
	AddImage(Parent, FMythRoUITextures::Get(Basic(TEXT("titlebar_right"))), Width - 12.f, 0.f, 12.f, 17.f);
	UTextBlock* T = AddText(Parent, 8.f, 2.f, 8, ColTitle, true);
	T->SetText(FText::FromString(Title));
	if (OutText)
	{
		*OutText = T;
	}
}

FMythRoGauge UMythRoHUDWidget::MakeGauge(UCanvasPanel* Parent, const TCHAR* Color, float X, float Y, float W, bool bBackground)
{
	// Com fundo: moldura gze_bg (9 px) e preenchimento 1 px para dentro. Sem fundo: a moldura ja faz parte
	// da arte da janela (ex.: HP/SP do basewin_bg) e o preenchimento vai exatamente em (X, Y, W, 7).
	FMythRoGauge G;
	G.X = bBackground ? X + 1.f : X;
	G.Y = bBackground ? Y + 0.5f : Y;
	G.Width = bBackground ? W - 2.f : W;
	G.Height = bBackground ? 8.f : 7.f;
	G.Scale = UIScale;
	if (bBackground)
	{
		AddImage(Parent, FMythRoUITextures::Get(Basic(TEXT("gze_bg_left"))), X, Y, 2.f, 9.f);
		AddImage(Parent, FMythRoUITextures::Get(Basic(TEXT("gze_bg_mid"))), X + 2.f, Y, W - 6.f, 9.f);
		AddImage(Parent, FMythRoUITextures::Get(Basic(TEXT("gze_bg_right"))), X + W - 4.f, Y, 4.f, 9.f);
	}
	G.Left = AddImage(Parent, FMythRoUITextures::Get(Basic(*FString::Printf(TEXT("gze%s_left"), Color))), G.X, G.Y, 4.f, G.Height);
	G.Mid = AddImage(Parent, FMythRoUITextures::Get(Basic(*FString::Printf(TEXT("gze%s_mid"), Color))), G.X + 4.f, G.Y, G.Width - 8.f, G.Height);
	G.Right = AddImage(Parent, FMythRoUITextures::Get(Basic(*FString::Printf(TEXT("gze%s_right"), Color))), G.X + G.Width - 4.f, G.Y, 4.f, G.Height);
	G.SetFraction(1.f);
	return G;
}

UButton* UMythRoHUDWidget::AddRoButton(UCanvasPanel* Parent, const TCHAR* BaseName, float X, float Y)
{
	UTexture2D* Off = FMythRoUITextures::Get(Basic(*FString::Printf(TEXT("btn_%s_off"), BaseName)));
	UTexture2D* On = FMythRoUITextures::Get(Basic(*FString::Printf(TEXT("btn_%s_on"), BaseName)));
	const FVector2D Size = FVector2D(30.f, 20.f) * UIScale;
	FButtonStyle Style;
	Style.SetNormal(TextureBrush(Off, Size));
	Style.SetHovered(TextureBrush(On ? On : Off, Size));
	Style.SetPressed(TextureBrush(On ? On : Off, Size));
	Style.SetNormalPadding(FMargin(0.f));
	Style.SetPressedPadding(FMargin(0.f));
	UButton* B = WidgetTree->ConstructWidget<UButton>();
	B->SetStyle(Style);
	if (UCanvasPanelSlot* S = Parent->AddChildToCanvas(B))
	{
		S->SetPosition(FVector2D(X, Y) * UIScale);
		S->SetSize(Size);
	}
	return B;
}

void UMythRoHUDWidget::BuildLayout()
{
	if (!WidgetTree)
	{
		return;
	}
	Root = WidgetTree->ConstructWidget<UCanvasPanel>();
	Root->SetVisibility(ESlateVisibility::SelfHitTestInvisible); // so janelas/botoes capturam o mouse
	WidgetTree->RootWidget = Root;

	BuildBasicInfo();
	BuildMinimap();
	BuildHotbar();
	BuildChat();
	BuildAutoBattle();
	BuildStatusWindow();
	BuildItemsWindow();
	BuildEquipWindow();
	BuildSkillsWindow();
	BuildNpcDialog();
	AddChatLine(TEXT("Bem-vindo a Rune-Midgard."), ColSystem);
}

void UMythRoHUDWidget::BuildBasicInfo()
{
	// Janela "Informacoes basicas" (basewin_bg, 280x120) no topo-esquerda
	UCanvasPanel* Win = MakeWindow(FVector2D(0.f, 0.f), FVector2D(0.f, 0.f), FVector2D(8.f, 8.f), FVector2D(280.f, 120.f));
	BasicWindow = Win;
	AddImage(Win, FMythRoUITextures::Get(Basic(TEXT("basewin_bg"))), 0.f, 0.f);

	NameText = AddText(Win, 8.f, 1.f, 8, ColTitle, true);      // nome na barra de titulo
	JobText = AddText(Win, 10.f, 20.f, 8, ColText);
	BaseLvText = AddText(Win, 10.f, 34.f, 8, ColText);
	JobLvText = AddText(Win, 10.f, 48.f, 8, ColText);

	// HP e SP nas molduras da direita (85x9)
	UTextBlock* HPLabel = AddText(Win, 94.f, 20.f, 8, ColLabel);
	HPLabel->SetText(FText::FromString(TEXT("HP")));
	HPGauge = MakeGauge(Win, TEXT("red"), 112.f, 23.f, 83.f, false); // dentro da moldura x 111-196, y 22-30
	HPText = AddText(Win, 128.f, 31.f, 7, ColText);
	UTextBlock* SPLabel = AddText(Win, 94.f, 41.f, 8, ColLabel);
	SPLabel->SetText(FText::FromString(TEXT("SP")));
	SPGauge = MakeGauge(Win, TEXT("blue"), 112.f, 44.f, 83.f, false); // moldura y 43-51
	SPText = AddText(Win, 128.f, 52.f, 7, ColText);

	// Caixa cinza: EXP de base e de classe
	UTextBlock* BaseExpLabel = AddText(Win, 10.f, 73.f, 7, ColLabel); // caixa cinza y 72-96
	BaseExpLabel->SetText(FText::FromString(TEXT("Base")));
	BaseExpGauge = MakeGauge(Win, TEXT("blue"), 44.f, 75.f, 148.f);
	UTextBlock* JobExpLabel = AddText(Win, 10.f, 84.f, 7, ColLabel);
	JobExpLabel->SetText(FText::FromString(TEXT("Classe")));
	JobExpGauge = MakeGauge(Win, TEXT("blue"), 44.f, 86.f, 148.f);

	// Faixa de baixo: peso e zeny
	WeightZenyText = AddText(Win, 8.f, 102.f, 7, ColText);

	// Coluna direita: os 6 botoes classicos (2 colunas x 3 linhas)
	UButton* B = AddRoButton(Win, TEXT("status"), 206.f, 22.f); B->OnClicked.AddDynamic(this, &UMythRoHUDWidget::OnStatusClicked);
	B = AddRoButton(Win, TEXT("items"), 242.f, 22.f);  B->OnClicked.AddDynamic(this, &UMythRoHUDWidget::OnItemsClicked);
	B = AddRoButton(Win, TEXT("equip"), 206.f, 46.f);  B->OnClicked.AddDynamic(this, &UMythRoHUDWidget::OnEquipClicked);
	B = AddRoButton(Win, TEXT("skill"), 242.f, 46.f);  B->OnClicked.AddDynamic(this, &UMythRoHUDWidget::OnSkillClicked);
	B = AddRoButton(Win, TEXT("map"), 206.f, 70.f);    B->OnClicked.AddDynamic(this, &UMythRoHUDWidget::OnMapClicked);
	B = AddRoButton(Win, TEXT("option"), 242.f, 70.f); B->OnClicked.AddDynamic(this, &UMythRoHUDWidget::OnOptionClicked);
	MakeDraggable(Win, 280.f, 17.f);
}

void UMythRoHUDWidget::BuildMinimap()
{
	// Janela do minimapa (topo-direita): titulo com o nome do mapa, imagem 128x128 e coordenadas
	const float W = 136.f, H = 17.f + 132.f + 16.f;
	MinimapWindow = MakeWindow(FVector2D(1.f, 0.f), FVector2D(1.f, 0.f), FVector2D(-8.f, 8.f), FVector2D(W, H));
	AddColor(MinimapWindow, ColWindowEdge, 0.f, 16.f, W, H - 16.f);
	AddColor(MinimapWindow, ColWindow, 1.f, 17.f, W - 2.f, H - 18.f);
	UTextBlock* Title = nullptr;
	AddTitleBar(MinimapWindow, W, TEXT("Mapa"), &Title);
	MapTitleText = Title;
	MinimapImage = AddImage(MinimapWindow, nullptr, 4.f, 19.f, 128.f, 128.f);
	MinimapImage->SetColorAndOpacity(FLinearColor::Transparent);
	PlayerDot = AddColor(MinimapWindow, FLinearColor(1.f, 0.85f, 0.1f), 0.f, 0.f, 4.f, 4.f);
	PlayerDot->SetVisibility(ESlateVisibility::Collapsed);
	CoordsText = AddText(MinimapWindow, 6.f, 149.f, 7, ColText);
	MakeDraggable(MinimapWindow, W, 17.f);
}

void UMythRoHUDWidget::BuildHotbar()
{
	// Barra de atalhos (shortitem_bg, 280x34) no baixo-centro, com F1..F9 em cada espaco
	UCanvasPanel* Win = MakeWindow(FVector2D(0.5f, 1.f), FVector2D(0.5f, 1.f), FVector2D(0.f, -8.f), FVector2D(280.f, 34.f));
	HotbarWindow = Win;
	AddImage(Win, FMythRoUITextures::Get(Basic(TEXT("shortitem_bg"))), 0.f, 0.f);
	for (int32 i = 0; i < 9; ++i)
	{
		// Espaco do atalho: area de soltar (recebe o mouse), icone 24x24 e quantidade/nivel no canto
		UImage* HotSlot = AddColor(Win, FLinearColor::Transparent, 4.f + 29.f * i, 4.f, 28.f, 28.f);
		HotSlot->SetVisibility(ESlateVisibility::Visible);
		HotbarSlots.Add(HotSlot);
		UImage* Icon = AddImage(Win, nullptr, 6.f + 29.f * i, 6.f, 24.f, 24.f);
		Icon->SetVisibility(ESlateVisibility::Collapsed);
		HotbarIcons.Add(Icon);
		UTextBlock* Count = AddText(Win, 20.f + 29.f * i, 21.f, 6, ColText);
		Count->SetShadowOffset(FVector2D(1.f, 1.f));
		Count->SetShadowColorAndOpacity(FLinearColor(1.f, 1.f, 1.f, 0.9f));
		HotbarCounts.Add(Count);
		UTextBlock* T = AddText(Win, 6.f + 29.f * i, 3.f, 6, ColLabel);
		T->SetText(FText::FromString(FString::Printf(TEXT("F%d"), i + 1)));
	}
	// So a borda esquerda arrasta a barra: o resto recebe os atalhos arrastados
	MakeDraggable(Win, 4.f, 34.f);
}

void UMythRoHUDWidget::BuildChat()
{
	// Chat (baixo-esquerda): abas originais, area translucida com as linhas e o campo de digitacao
	const float W = 320.f, H = 21.f + 98.f + 20.f;
	UCanvasPanel* Win = MakeWindow(FVector2D(0.f, 1.f), FVector2D(0.f, 1.f), FVector2D(8.f, -8.f), FVector2D(W, H));
	AddImage(Win, FMythRoUITextures::Get(Basic(TEXT("chat_tab_0"))), 0.f, 0.f);
	AddColor(Win, ColChatBack, 0.f, 21.f, W, 98.f);

	ChatLines = WidgetTree->ConstructWidget<UVerticalBox>();
	ChatLines->SetVisibility(ESlateVisibility::HitTestInvisible);
	if (UCanvasPanelSlot* S = Win->AddChildToCanvas(ChatLines))
	{
		S->SetPosition(FVector2D(6.f, 24.f) * UIScale);
		S->SetSize(FVector2D(W - 12.f, 92.f) * UIScale);
	}

	ChatInput = WidgetTree->ConstructWidget<UEditableTextBox>();
	ChatInput->SetHintText(FText::FromString(TEXT("Enter para falar  |  @comando")));
	ChatInput->OnTextCommitted.AddDynamic(this, &UMythRoHUDWidget::OnChatCommitted);
	if (UCanvasPanelSlot* S = Win->AddChildToCanvas(ChatInput))
	{
		S->SetPosition(FVector2D(0.f, 120.f) * UIScale);
		S->SetSize(FVector2D(W, 19.f) * UIScale);
	}
	MakeDraggable(Win, W, 21.f); // pela faixa das abas
}

void UMythRoHUDWidget::BuildAutoBattle()
{
	// Janela do Auto-Battle (baixo-direita): botao liga/desliga, vigor de 3h e peso
	const float W = 170.f, H = 17.f + 52.f;
	UCanvasPanel* Win = MakeWindow(FVector2D(1.f, 1.f), FVector2D(1.f, 1.f), FVector2D(-8.f, -8.f), FVector2D(W, H));
	AddColor(Win, ColWindowEdge, 0.f, 16.f, W, H - 16.f);
	AddColor(Win, ColWindow, 1.f, 17.f, W - 2.f, H - 18.f);
	AddTitleBar(Win, W, TEXT("Auto-Battle"));

	AutoButton = WidgetTree->ConstructWidget<UButton>();
	AutoButton->OnClicked.AddDynamic(this, &UMythRoHUDWidget::OnAutoButtonClicked);
	AutoButtonText = WidgetTree->ConstructWidget<UTextBlock>();
	AutoButtonText->SetFont(FCoreStyle::GetDefaultFontStyle("Bold", FMath::RoundToInt(7 * UIScale)));
	AutoButtonText->SetColorAndOpacity(FSlateColor(ColText));
	AutoButton->AddChild(AutoButtonText);
	if (UCanvasPanelSlot* S = Win->AddChildToCanvas(AutoButton))
	{
		S->SetPosition(FVector2D(6.f, 21.f) * UIScale);
		S->SetSize(FVector2D(60.f, 18.f) * UIScale);
	}
	StaminaGauge = MakeGauge(Win, TEXT("blue"), 72.f, 26.f, 92.f);
	StaminaText = AddText(Win, 72.f, 36.f, 6, ColText);
	UTextBlock* Hint = AddText(Win, 6.f, 52.f, 6, ColLabel);
	Hint->SetText(FText::FromString(TEXT("Vigor diario: 3h  |  trava com peso >= 90%")));
	MakeDraggable(Win, W, 17.f);
}

// ------------------------------------------------------------------------------------------------ dados

void UMythRoHUDWidget::BindToPawn(APawn* Pawn)
{
	if (Stats)
	{
		Stats->OnStatsChanged.RemoveAll(this);
	}
	if (AutoBattle)
	{
		AutoBattle->OnAutoBattleStateChanged.RemoveAll(this);
		AutoBattle->OnAutoBattleTimeUpdated.RemoveAll(this);
		AutoBattle->OnAutoBattleError.RemoveAll(this);
		AutoBattle->OnInventoryWeightChanged.RemoveAll(this);
	}
	BoundPawn = Pawn;
	Stats = Pawn ? Pawn->FindComponentByClass<UMythStatsComponent>() : nullptr;
	AutoBattle = Pawn ? Pawn->FindComponentByClass<UAutoBattleComponent>() : nullptr;
	if (Stats)
	{
		Stats->OnStatsChanged.AddDynamic(this, &UMythRoHUDWidget::RefreshStats);
	}
	if (AutoBattle)
	{
		AutoBattle->OnAutoBattleStateChanged.AddDynamic(this, &UMythRoHUDWidget::RefreshAutoState);
		AutoBattle->OnAutoBattleTimeUpdated.AddDynamic(this, &UMythRoHUDWidget::RefreshStamina);
		AutoBattle->OnAutoBattleError.AddDynamic(this, &UMythRoHUDWidget::ShowError);
		AutoBattle->OnInventoryWeightChanged.AddDynamic(this, &UMythRoHUDWidget::RefreshWeight);
	}
	RefreshStats();
	RefreshAutoAll();
}

void UMythRoHUDWidget::RefreshStats()
{
	if (!NameText || !Stats)
	{
		return;
	}
	NameText->SetText(FText::FromString(Stats->GetCharName()));
	// Classe/nivel de classe/zeny ainda nao vem do servidor: valores do Aprendiz ate a rede ser ligada.
	JobText->SetText(FText::FromString(TEXT("Aprendiz")));
	BaseLvText->SetText(FText::FromString(FString::Printf(TEXT("Base Lv. %d"), Stats->GetBaseLevel())));
	JobLvText->SetText(FText::FromString(TEXT("Job Lv. 1")));

	HPGauge.SetFraction(static_cast<float>(Stats->GetHP()) / FMath::Max(1, Stats->GetMaxHP()));
	HPText->SetText(FText::FromString(FString::Printf(TEXT("%d / %d"), Stats->GetHP(), Stats->GetMaxHP())));
	SPGauge.SetFraction(static_cast<float>(Stats->GetSP()) / FMath::Max(1, Stats->GetMaxSP()));
	SPText->SetText(FText::FromString(FString::Printf(TEXT("%d / %d"), Stats->GetSP(), Stats->GetMaxSP())));
	BaseExpGauge.SetFraction(Stats->GetExpFraction());
	JobExpGauge.SetFraction(0.f);
	WeightZenyText->SetText(FText::FromString(FString::Printf(TEXT("Peso : %.0f%%      Zeny : 0"), WeightPercent)));
}

void UMythRoHUDWidget::RefreshAutoAll()
{
	if (!AutoButton)
	{
		return;
	}
	if (!AutoBattle)
	{
		AutoButton->SetIsEnabled(false);
		AutoButtonText->SetText(FText::FromString(TEXT("--")));
		return;
	}
	LastShownSecond = -1;
	RefreshAutoState(AutoBattle->IsAutoBattleActive());
	RefreshStamina(AutoBattle->GetRemainingStaminaSeconds());
	RefreshWeight(AutoBattle->GetInventoryWeightPercent(), AutoBattle->IsOverweight());
}

void UMythRoHUDWidget::RefreshAutoState(bool bIsActive)
{
	if (!AutoButton || !AutoBattle)
	{
		return;
	}
	AutoButtonText->SetText(FText::FromString(bIsActive ? TEXT("  Ligado") : TEXT("  Desligado")));
	AutoButtonText->SetColorAndOpacity(FSlateColor(bIsActive ? FLinearColor(0.1f, 0.55f, 0.15f) : ColText));
	// Sempre da para desligar; para ligar, nao pode estar caido, sem vigor ou com sobrepeso.
	const bool bCanStart = !AutoBattle->IsPlayerDowned() && !AutoBattle->IsOverweight() && AutoBattle->GetRemainingStaminaSeconds() > 0.f;
	AutoButton->SetIsEnabled(bIsActive || bCanStart);
}

void UMythRoHUDWidget::RefreshStamina(float RemainingSeconds)
{
	const int32 Sec = FMath::CeilToInt(RemainingSeconds);
	if (Sec == LastShownSecond || !StaminaText)
	{
		return; // o componente emite todo frame; so atualiza quando o segundo muda
	}
	LastShownSecond = Sec;
	StaminaGauge.SetFraction(RemainingSeconds / UAutoBattleComponent::MAX_DAILY_STAMINA_SECONDS);
	StaminaText->SetText(FText::FromString(FString::Printf(TEXT("%s / 03:00:00"), *RoHudFormatClock(RemainingSeconds))));
	if (Sec <= 0 && AutoBattle)
	{
		RefreshAutoState(AutoBattle->IsAutoBattleActive());
	}
}

void UMythRoHUDWidget::RefreshWeight(float InWeightPercent, bool bOverweight)
{
	WeightPercent = InWeightPercent;
	RefreshStats();
	if (bOverweight)
	{
		AddChatLine(FString::Printf(TEXT("Sobrepeso (%.0f%%): Auto-Battle bloqueado."), InWeightPercent), ColError);
	}
	if (AutoBattle)
	{
		RefreshAutoState(AutoBattle->IsAutoBattleActive());
	}
}

void UMythRoHUDWidget::ShowError(const FString& Reason)
{
	AddChatLine(Reason, ColError);
}

void UMythRoHUDWidget::OnAutoButtonClicked()
{
	if (AutoBattle)
	{
		AutoBattle->ToggleAutoBattle(!AutoBattle->IsAutoBattleActive());
	}
}

// ------------------------------------------------------------------------------------------------ chat

void UMythRoHUDWidget::AddChatLine(const FString& Text, FLinearColor Color)
{
	if (!ChatLines)
	{
		return;
	}
	while (ChatLines->GetChildrenCount() >= MaxChatLines)
	{
		ChatLines->RemoveChildAt(0);
	}
	UTextBlock* T = WidgetTree->ConstructWidget<UTextBlock>();
	T->SetFont(FCoreStyle::GetDefaultFontStyle("Regular", FMath::RoundToInt(7 * UIScale)));
	T->SetColorAndOpacity(FSlateColor(Color));
	T->SetShadowOffset(FVector2D(1.f, 1.f));
	T->SetShadowColorAndOpacity(FLinearColor(0.f, 0.f, 0.f, 0.9f));
	T->SetAutoWrapText(true);
	T->SetText(FText::FromString(Text));
	ChatLines->AddChildToVerticalBox(T);
}

void UMythRoHUDWidget::FocusChat()
{
	if (ChatInput)
	{
		ChatInput->SetKeyboardFocus();
	}
}

bool UMythRoHUDWidget::IsChatFocused() const
{
	return ChatInput && ChatInput->HasKeyboardFocus();
}

void UMythRoHUDWidget::OnChatCommitted(const FText& Text, ETextCommit::Type CommitMethod)
{
	if (CommitMethod != ETextCommit::OnEnter)
	{
		return;
	}
	const FString Msg = Text.ToString().TrimStartAndEnd();
	ChatInput->SetText(FText::GetEmpty());
	if (FSlateApplication::IsInitialized())
	{
		FSlateApplication::Get().SetAllUserFocusToGameViewport(); // devolve o teclado ao jogo (WASD)
	}
	if (Msg.IsEmpty())
	{
		return;
	}
	// Conectado: tudo vai para o servidor (inclusive os comandos @ de GM, que o rAthena interpreta no chat).
	// O proprio servidor devolve a fala (ZC_NOTIFY_PLAYERCHAT), que chega pelo OnChatMessage.
	if (URagnarokNetworkSubsystem* Net = GetNet())
	{
		if (Net->GetNetworkState() == ERagnarokNetworkState::ConnectedMap)
		{
			Net->SendGlobalMessage(Msg);
			return;
		}
	}
	if (Msg.StartsWith(TEXT("@")))
	{
		// Comandos de GM sao do servidor (regra: o cliente nao reimplementa): so funcionam com a rede ligada.
		AddChatLine(FString::Printf(TEXT("%s  (comandos @ precisam da conexao com o servidor)"), *Msg), ColError);
		return;
	}
	const FString Name = Stats ? Stats->GetCharName() : TEXT("Voce");
	AddChatLine(FString::Printf(TEXT("%s : %s"), *Name, *Msg), FLinearColor::White);
}

// ------------------------------------------------------------------------------------------------ botoes

void UMythRoHUDWidget::OnStatusClicked() { ToggleWindow(EMythRoWindow::Status); }
void UMythRoHUDWidget::OnItemsClicked()  { ToggleWindow(EMythRoWindow::Items); }
void UMythRoHUDWidget::OnEquipClicked()  { ToggleWindow(EMythRoWindow::Equip); }
void UMythRoHUDWidget::OnSkillClicked()  { ToggleWindow(EMythRoWindow::Skills); }
void UMythRoHUDWidget::OnOptionClicked() { AddChatLine(TEXT("Configuracoes: em construcao."), ColSystem); }

void UMythRoHUDWidget::OnMapClicked()
{
	if (MinimapWindow)
	{
		const bool bVisible = MinimapWindow->GetVisibility() != ESlateVisibility::Collapsed;
		MinimapWindow->SetVisibility(bVisible ? ESlateVisibility::Collapsed : ESlateVisibility::SelfHitTestInvisible);
	}
}

// ------------------------------------------------------------------------------------------------ arrastar

void UMythRoHUDWidget::MakeDraggable(UCanvasPanel* Window, float Width, float HandleHeight)
{
	if (!Window)
	{
		return;
	}
	// Alca invisivel mas "clicavel" (Visible): o clique nao vai para o jogo e chega aqui pelo NativeOnMouseButtonDown.
	UImage* Handle = WidgetTree->ConstructWidget<UImage>();
	Handle->SetColorAndOpacity(FLinearColor::Transparent);
	Handle->SetVisibility(ESlateVisibility::Visible);
	if (UCanvasPanelSlot* S = Window->AddChildToCanvas(Handle))
	{
		S->SetPosition(FVector2D::ZeroVector);
		S->SetSize(FVector2D(Width, HandleHeight) * UIScale);
	}
	DragWindows.Add({ Window, HandleHeight });
}

FReply UMythRoHUDWidget::NativeOnMouseButtonDown(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent)
{
	if (InMouseEvent.GetEffectingButton() != EKeys::LeftMouseButton || !Root)
	{
		return Super::NativeOnMouseButtonDown(InGeometry, InMouseEvent);
	}
	const FVector2D Screen = InMouseEvent.GetScreenSpacePosition();
	if (HandleNpcClick(Screen, false))
	{
		return FReply::Handled();
	}
	if (HandleWindowClick(Screen))
	{
		return FReply::Handled();
	}
	if (BeginHotDrag(Screen))
	{
		return FReply::Handled().CaptureMouse(TakeWidget()); // arrastando skill/item/atalho
	}
	for (int32 i = DragWindows.Num() - 1; i >= 0; --i)
	{
		UCanvasPanel* Win = DragWindows[i].Window.Get();
		if (!Win || Win->GetVisibility() == ESlateVisibility::Collapsed)
		{
			continue;
		}
		const FGeometry& WinGeo = Win->GetCachedGeometry();
		const FVector2D Local = WinGeo.AbsoluteToLocal(Screen);
		if (Local.X < 0.f || Local.Y < 0.f || Local.X > WinGeo.GetLocalSize().X || Local.Y > DragWindows[i].HandleHeight * UIScale)
		{
			continue;
		}
		// Passa a janela para ancora no topo-esquerda (a posicao vira simples) e traz para a frente.
		UCanvasPanelSlot* S = Cast<UCanvasPanelSlot>(Win->Slot);
		if (!S)
		{
			continue;
		}
		const FGeometry& RootGeo = Root->GetCachedGeometry();
		const FVector2D TopLeft = RootGeo.AbsoluteToLocal(WinGeo.GetAbsolutePosition());
		const FVector2D Size = S->GetSize();
		S->SetAnchors(FAnchors(0.f, 0.f));
		S->SetAlignment(FVector2D::ZeroVector);
		S->SetPosition(TopLeft);
		S->SetSize(Size);
		S->SetZOrder(++TopZOrder);
		Dragging = Win;
		DragOffset = RootGeo.AbsoluteToLocal(Screen) - TopLeft;
		return FReply::Handled().CaptureMouse(TakeWidget());
	}
	return Super::NativeOnMouseButtonDown(InGeometry, InMouseEvent);
}

FReply UMythRoHUDWidget::NativeOnMouseMove(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent)
{
	if (bHotDragging)
	{
		UpdateHotDrag(InMouseEvent.GetScreenSpacePosition());
		return FReply::Handled();
	}
	if (UCanvasPanel* Win = Dragging.Get())
	{
		if (UCanvasPanelSlot* S = Cast<UCanvasPanelSlot>(Win->Slot))
		{
			const FVector2D Mouse = Root->GetCachedGeometry().AbsoluteToLocal(InMouseEvent.GetScreenSpacePosition());
			// Mantem pelo menos a barra de titulo dentro da tela
			const FVector2D Limit = Root->GetCachedGeometry().GetLocalSize();
			FVector2D Pos = Mouse - DragOffset;
			Pos.X = FMath::Clamp(Pos.X, -S->GetSize().X + 40.f, Limit.X - 40.f);
			Pos.Y = FMath::Clamp(Pos.Y, 0.f, Limit.Y - 20.f);
			S->SetPosition(Pos);
		}
		return FReply::Handled();
	}
	return Super::NativeOnMouseMove(InGeometry, InMouseEvent);
}

FReply UMythRoHUDWidget::NativeOnMouseButtonUp(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent)
{
	if (bHotDragging)
	{
		const FVector2D Screen = InMouseEvent.GetScreenSpacePosition();
		const int32 From = DragFromHotbar;
		if (From != INDEX_NONE && HotbarSlotAt(Screen) == From)
		{
			// clique simples num atalho (sem arrastar): usa, como no cliente original
			bHotDragging = false;
			if (DragIcon)
			{
				DragIcon->SetVisibility(ESlateVisibility::Collapsed);
			}
			if (AMythPlayerController* PC = Cast<AMythPlayerController>(GetOwningPlayer()))
			{
				PC->UseHotkey(From);
			}
			return FReply::Handled().ReleaseMouseCapture();
		}
		EndHotDrag(Screen);
		return FReply::Handled().ReleaseMouseCapture();
	}
	if (Dragging.IsValid())
	{
		Dragging = nullptr;
		return FReply::Handled().ReleaseMouseCapture();
	}
	return Super::NativeOnMouseButtonUp(InGeometry, InMouseEvent);
}

// ------------------------------------------------------------------------------------------------ minimapa

void UMythRoHUDWidget::NativeTick(const FGeometry& MyGeometry, float InDeltaTime)
{
	Super::NativeTick(MyGeometry, InDeltaTime);
	UpdateMinimap();
	WindowRefreshTimer -= InDeltaTime;
	if (WindowRefreshTimer <= 0.f)
	{
		WindowRefreshTimer = 0.25f;
		RefreshWindows();
	}
}

void UMythRoHUDWidget::UpdateMinimap()
{
	const AMythMapInfo* Info = AMythMapInfo::Find(GetWorld());
	if (!Info || !MinimapImage)
	{
		return;
	}
	if (LoadedMinimap != Info->MapName)
	{
		LoadedMinimap = Info->MapName;
		MapTitleText->SetText(FText::FromString(Info->MapName));
		if (UTexture2D* Tex = FMythRoUITextures::Get(FString::Printf(TEXT("map/%s.bmp"), *Info->MapName)))
		{
			MinimapImage->SetBrush(TextureBrush(Tex, FVector2D(128.f, 128.f) * UIScale));
			MinimapImage->SetColorAndOpacity(FLinearColor::White);
		}
	}
	const APawn* Pawn = BoundPawn.Get();
	if (!Pawn)
	{
		return;
	}
	const FIntPoint Cell = Info->WorldToCell(Pawn->GetActorLocation());
	CoordsText->SetText(FText::FromString(FString::Printf(TEXT("%s  %d, %d"), *Info->MapName, Cell.X, Cell.Y)));

	// O .bmp do minimapa encaixa o mapa inteiro no lado maior e centraliza no outro (como o cliente original).
	const FIntPoint Size = Info->MapSizeCells;
	if (Size.X <= 0 || Size.Y <= 0)
	{
		PlayerDot->SetVisibility(ESlateVisibility::Collapsed);
		return;
	}
	const float MapPx = 128.f;
	const float K = MapPx / FMath::Max(Size.X, Size.Y);
	const float OffX = (MapPx - Size.X * K) * 0.5f;
	const float OffY = (MapPx - Size.Y * K) * 0.5f;
	const float Px = 4.f + OffX + (Cell.X + 0.5f) * K;
	const float Py = 19.f + MapPx - (OffY + (Cell.Y + 0.5f) * K); // celula Y cresce para cima; a imagem, para baixo
	if (UCanvasPanelSlot* S = Cast<UCanvasPanelSlot>(PlayerDot->Slot))
	{
		S->SetPosition(FVector2D(Px - 2.f, Py - 2.f) * UIScale);
	}
	PlayerDot->SetVisibility(ESlateVisibility::HitTestInvisible);
}

#include "UI/MythRoHUDWidget.h"

#include "UI/MythRoUITextures.h"
#include "Gameplay/AutoBattleComponent.h"
#include "Gameplay/MythStatsComponent.h"
#include "World/MythMapInfo.h"
#include "Network/RagnarokNetworkSubsystem.h"
#include "Player/MythPlayerController.h"

#include "Blueprint/WidgetTree.h"
#include "Components/Button.h"
#include "Engine/Texture2D.h"
#include "Components/CanvasPanel.h"
#include "Components/CanvasPanelSlot.h"
#include "Components/EditableTextBox.h"
#include "Components/Image.h"
#include "Components/TextBlock.h"
#include "Components/VerticalBox.h"
#include "Components/VerticalBoxSlot.h"
#include "Framework/Application/SlateApplication.h"
#include "Brushes/SlateRoundedBoxBrush.h"
#include "Styling/SlateTypes.h"
#include "GameFramework/Pawn.h"
#include "Styling/CoreStyle.h"

namespace
{
	// Cores do cliente original
	const FLinearColor ColText(0.08f, 0.10f, 0.18f);        // texto escuro das janelas claras
	const FLinearColor ColLabel(0.12f, 0.27f, 0.55f);       // rotulos azulados
	const FLinearColor ColTitle(1.f, 1.f, 1.f);
	const FLinearColor ColWindow(0.965f, 0.965f, 0.98f, 0.9f);
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
	// Escala da interface escolhida no menu Esc (Som e interface); vale ao abrir o jogo
	float SavedScale = 0.f;
	if (GConfig->GetFloat(TEXT("Myth.UI"), TEXT("Scale"), SavedScale, GGameUserSettingsIni) && SavedScale >= 0.8f && SavedScale <= 2.5f)
	{
		UIScale = SavedScale;
	}
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

UImage* UMythRoHUDWidget::AddButton(UCanvasPanel* Parent, const TCHAR* Name, float X, float Y)
{
	return AddButtonEx(Parent, FString::Printf(TEXT("btn_%s.bmp"), Name), FString::Printf(TEXT("btn_%s_a.bmp"), Name), FString::Printf(TEXT("btn_%s_b.bmp"), Name), X, Y);
}

UImage* UMythRoHUDWidget::AddButtonEx(UCanvasPanel* Parent, const FString& Normal, const FString& Hover, const FString& Pressed, float X, float Y)
{
	UTexture2D* NormalTex = FMythRoUITextures::Get(Normal);
	UImage* Img = AddImage(Parent, NormalTex, X, Y);
	Img->SetVisibility(ESlateVisibility::Visible);
	FRoButton B;
	B.Image = Img;
	B.Normal = NormalTex;
	B.Hover = FMythRoUITextures::Get(Hover);
	B.Pressed = FMythRoUITextures::Get(Pressed);
	RoButtons.Add(B);
	return Img;
}

UTextBlock* UMythRoHUDWidget::AddButtonHit(UCanvasPanel* Parent, const TCHAR* Name, float X, float Y)
{
	UImage* Img = AddButton(Parent, Name, X, Y);
	const FVector2D Size = Img->GetBrush().ImageSize; // ja com a escala da interface
	UTextBlock* Hit = AddText(Parent, X, Y, 6, FLinearColor::Transparent);
	if (UCanvasPanelSlot* S = Cast<UCanvasPanelSlot>(Hit->Slot))
	{
		S->SetAutoSize(false);
		S->SetSize(Size);
	}
	return Hit;
}

UTextBlock* UMythRoHUDWidget::AddTextButton(UCanvasPanel* Parent, const FString& Label, float X, float Y, float W)
{
	// Moldura de 3 partes do cliente (btn_out_left/mid/right, over e press) com o texto centralizado por cima
	constexpr float H = 20.f;
	auto Part = [](const TCHAR* State, const TCHAR* Side) { return FString::Printf(TEXT("basic_interface/btn_%s_%s.bmp"), State, Side); };
	UImage* L = AddImage(Parent, FMythRoUITextures::Get(Part(TEXT("out"), TEXT("left"))), X, Y, 6.f, H);
	UImage* M = AddImage(Parent, FMythRoUITextures::Get(Part(TEXT("out"), TEXT("mid"))), X + 6.f, Y, W - 12.f, H);
	UImage* R = AddImage(Parent, FMythRoUITextures::Get(Part(TEXT("out"), TEXT("right"))), X + W - 6.f, Y, 6.f, H);
	M->SetVisibility(ESlateVisibility::Visible);
	FRoButton B;
	B.Image = M;
	B.Left = L;
	B.Right = R;
	B.Part = TEXT("text");
	B.Normal = FMythRoUITextures::Get(Part(TEXT("out"), TEXT("mid")));
	B.Hover = FMythRoUITextures::Get(Part(TEXT("over"), TEXT("mid")));
	B.Pressed = FMythRoUITextures::Get(Part(TEXT("press"), TEXT("mid")));
	UTextBlock* T = AddText(Parent, X, Y + 4.f, 7, FLinearColor(0.1f, 0.1f, 0.15f));
	B.Label = T;
	RoButtons.Add(B);
	T->SetText(FText::FromString(Label));
	T->SetJustification(ETextJustify::Center);
	if (UCanvasPanelSlot* S = Cast<UCanvasPanelSlot>(T->Slot))
	{
		S->SetAutoSize(false);
		S->SetSize(FVector2D(W, H - 4.f) * UIScale);
	}
	return T;
}

void UMythRoHUDWidget::SetTextButtonShown(UTextBlock* Label, bool bShow)
{
	if (!Label)
	{
		return;
	}
	Label->SetVisibility(bShow ? ESlateVisibility::HitTestInvisible : ESlateVisibility::Collapsed);
	for (FRoButton& B : RoButtons)
	{
		if (B.Label.Get() != Label)
		{
			continue;
		}
		if (UImage* M = B.Image.Get()) { M->SetVisibility(bShow ? ESlateVisibility::Visible : ESlateVisibility::Collapsed); }
		if (UImage* L = B.Left.Get()) { L->SetVisibility(bShow ? ESlateVisibility::HitTestInvisible : ESlateVisibility::Collapsed); }
		if (UImage* R = B.Right.Get()) { R->SetVisibility(bShow ? ESlateVisibility::HitTestInvisible : ESlateVisibility::Collapsed); }
	}
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

namespace
{
	/** Retangulo arredondado 32x32 de 9 fatias: miolo branco, borda de 1 px a 62% e cantos antialiasados (raio 7). */
	UTexture2D* RoundedPanelTexture()
	{
		static TObjectPtr<UTexture2D> Tex;
		if (Tex)
		{
			return Tex;
		}
		constexpr int32 N = 32;
		constexpr float R = 7.f;
		TArray<FColor> Px;
		Px.SetNumUninitialized(N * N);
		for (int32 y = 0; y < N; ++y)
		{
			for (int32 x = 0; x < N; ++x)
			{
				// distancia ao retangulo arredondado (negativa dentro)
				const float Qx = FMath::Abs(x + 0.5f - N * 0.5f) - (N * 0.5f - R);
				const float Qy = FMath::Abs(y + 0.5f - N * 0.5f) - (N * 0.5f - R);
				const float D = FVector2D(FMath::Max(Qx, 0.f), FMath::Max(Qy, 0.f)).Size() + FMath::Min(FMath::Max(Qx, Qy), 0.f) - R;
				const float A = FMath::Clamp(0.5f - D, 0.f, 1.f);
				const float Edge = FMath::Clamp(1.2f + D, 0.f, 1.f); // 1 perto da borda
				const uint8 V = static_cast<uint8>(255.f * FMath::Lerp(1.f, 0.62f, Edge));
				Px[y * N + x] = FColor(V, V, V, static_cast<uint8>(255.f * A));
			}
		}
		UTexture2D* T = UTexture2D::CreateTransient(N, N, PF_B8G8R8A8);
		T->Filter = TF_Bilinear;
		T->SRGB = true;
		T->NeverStream = true;
		T->AddToRoot();
		FTexture2DMipMap& Mip = T->GetPlatformData()->Mips[0];
		FMemory::Memcpy(Mip.BulkData.Lock(LOCK_READ_WRITE), Px.GetData(), Px.Num() * sizeof(FColor));
		Mip.BulkData.Unlock();
		T->UpdateResource();
		Tex = T;
		return T;
	}
}

UImage* UMythRoHUDWidget::AddRounded(UCanvasPanel* Parent, const FLinearColor& Fill, const FLinearColor& Border, float X, float Y, float W, float H, float RadiusTop, float RadiusBottom)
{
	UImage* Img = WidgetTree->ConstructWidget<UImage>();
	FSlateBrush Brush;
	Brush.SetResourceObject(RoundedPanelTexture());
	Brush.DrawAs = ESlateBrushDrawType::Box;
	Brush.Margin = FMargin(0.3f);
	Brush.ImageSize = FVector2D(32.f, 32.f);
	Img->SetBrush(Brush);
	// A borda vem gravada na textura (mais escura que o miolo): o miolo leva a cor Fill, a borda a cor Fill x 0,62
	const float Mix = Border.A > 0.f ? 1.f : 1.f;
	(void)Mix;
	Img->SetColorAndOpacity(Fill);
	Img->SetVisibility(ESlateVisibility::HitTestInvisible);
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
	T->SetFont(FMythRoUITextures::RoFont(FMath::RoundToInt(Size * UIScale)));
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
	AddImage(Parent, FMythRoUITextures::Get(Basic(TEXT("sys_base_off"))), 4.f, 3.f, 11.f, 11.f); // o icone redondo da janela, como no cliente moderno
	UTextBlock* T = AddText(Parent, 18.f, 2.f, 8, ColTitle, true);
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
	BuildItemInfoWindow();
	BuildGameMenu();
	BuildShopWindows();
	BuildStorageWindow();
	BuildCartWindow();
	BuildSelectListWindow();
	BuildVendingWindows();
	BuildWorldMapWindow();
	BuildPartyWindow();
	BuildGuildWindow();
	BuildPetWindow();
	BuildHomunWindow();
	BuildOptionsWindow();
	BuildEmotionWindows();
	BuildTradeWindows();
	BuildFriendsWindow();
	BuildChatRoomWindows();
	BuildQuestWindow();
	BuildShortcutWindow();
	BuildBankWindow();
	BuildAchievementWindow();
	BuildEquipSwitchWindow();
	BuildNaviWindow();
	BuildMailWindows();
	BuildEntryScreens();
	BuildNpcDialog();
	StyleEditBoxes();
	AddChatLine(TEXT("Bem-vindo a Rune-Midgard."), ColSystem);
}

void UMythRoHUDWidget::BuildBasicInfo()
{
	// "Basic Information" do cliente LATAM (docs/ref/12, print 17): 232 de largura; nome, classe, HP/SP com "atual /
	// max" e %, nivel de base/classe com a barra de EXP, peso/zeny, a barra de recolher e a grade de icones do menu
	constexpr float W = 232.f;
	UCanvasPanel* Win = MakeWindow(FVector2D(0.f, 0.f), FVector2D(0.f, 0.f), FVector2D(0.f, 0.f), FVector2D(W, BasicFullH));
	BasicWindow = Win;
	BasicBody = AddRounded(Win, ColWindow, ColWindowEdge, 0.f, 8.f, W, BasicInfoH - 8.f, 0.f, 7.f);
	BasicBody->SetVisibility(ESlateVisibility::Visible); // o clique na janela nao atravessa para o jogo
	AddTitleBar(Win, W, TEXT("Informações Básicas"));
	BasicMiniButton = AddButtonEx(Win, TEXT("basic_interface/sys_mini_off.bmp"), TEXT("basic_interface/sys_mini_on.bmp"), TEXT("basic_interface/sys_mini_on.bmp"), W - 15.f, 3.f);
	BasicDetailStart = Win->GetChildrenCount(); // daqui para baixo: detalhes que somem na versao recolhida

	NameText = AddText(Win, 6.f, 19.f, 8, ColText, true);
	JobText = AddText(Win, 6.f, 32.f, 8, ColText);

	AddText(Win, 6.f, 48.f, 8, ColLabel)->SetText(FText::FromString(TEXT("HP")));
	HPGauge = MakeGauge(Win, TEXT("red"), 24.f, 50.f, 166.f, false);
	HPText = AddText(Win, 72.f, 47.f, 7, FLinearColor(0.05f, 0.05f, 0.1f));
	HPPctText = AddText(Win, 196.f, 48.f, 7, ColText);
	AddText(Win, 6.f, 62.f, 8, ColLabel)->SetText(FText::FromString(TEXT("SP")));
	SPGauge = MakeGauge(Win, TEXT("blue"), 24.f, 64.f, 166.f, false);
	SPText = AddText(Win, 72.f, 61.f, 7, FLinearColor(0.05f, 0.05f, 0.1f));
	for (UTextBlock* Num : { HPText.Get(), SPText.Get() })
	{
		FSlateFontInfo NumFont = Num->GetFont(); // numeros sobre a barra: contorno branco para ler sobre o vermelho e o azul
		NumFont.OutlineSettings.OutlineSize = 1;
		NumFont.OutlineSettings.OutlineColor = FLinearColor::White;
		Num->SetFont(NumFont);
	}
	SPPctText = AddText(Win, 196.f, 62.f, 7, ColText);

	// Fundo cinza claro das linhas de nivel, como no print
	AddColor(Win, FLinearColor(0.9f, 0.92f, 0.96f, 1.f), 2.f, 78.f, W - 4.f, 28.f);
	BaseLvText = AddText(Win, 8.f, 79.f, 7, ColLabel);
	BaseExpGauge = MakeGauge(Win, TEXT("blue"), 70.f, 82.f, 110.f);
	BaseExpPctText = AddText(Win, 186.f, 79.f, 7, ColText);
	JobLvText = AddText(Win, 8.f, 92.f, 7, ColLabel);
	JobExpGauge = MakeGauge(Win, TEXT("blue"), 70.f, 95.f, 110.f);
	JobExpPctText = AddText(Win, 186.f, 92.f, 7, ColText);

	WeightZenyText = AddText(Win, 8.f, 109.f, 7, ColText);

	// Barra de recolher (bt_menu_normal 219x9) e a grade do menu (bg_menu 220x228, icones 32x34 em 5 colunas)
	MenuToggleBar = AddImage(Win, FMythRoUITextures::Get(TEXT("menu_icon/bt_menu_normal.bmp")), 6.f, 124.f, 219.f, 9.f);
	MenuToggleBar->SetVisibility(ESlateVisibility::Visible);
	// A grade do menu fica sobre o bg_menu.tga do cliente (preto a 50%), fora do quadro branco das informacoes (print 03)
	MenuBack = AddImage(Win, FMythRoUITextures::Get(TEXT("menu_icon/bg_menu.tga")), 6.f, 135.f, 220.f, 228.f);
	static const TCHAR* Icons[] = {
		TEXT("status"), TEXT("equip"), TEXT("item"), TEXT("skill"), TEXT("party"),
		TEXT("guild"), TEXT("quest"), TEXT("map"), TEXT("navigation"), TEXT("option"),
		TEXT("sns"), TEXT("rec"), TEXT("mail"), TEXT("achievement"), TEXT("adventureguide"),
		TEXT("shop"), TEXT("keyboard"), TEXT("attendance"), TEXT("bank"), TEXT("repute"),
		TEXT("tip"),
	};
	for (int32 i = 0; i < UE_ARRAY_COUNT(Icons); ++i)
	{
		const float X = 6.f + 8.f + (i % 5) * 41.f;
		const float Y = 135.f + 8.f + (i / 5) * 43.f;
		UImage* Icon = AddImage(Win, FMythRoUITextures::Get(FString::Printf(TEXT("menu_icon/bt_%s.bmp"), Icons[i])), X, Y, 32.f, 34.f);
		Icon->SetVisibility(ESlateVisibility::Visible);
		MenuIcons.Add(Icon);
		MenuIconNames.Add(Icons[i]);
	}
	// Versao recolhida (print 21): duas linhas resumidas
	BasicMini1 = AddText(Win, 6.f, 19.f, 7, ColText);
	BasicMini2 = AddText(Win, 6.f, 31.f, 7, ColText);
	BasicMini1->SetVisibility(ESlateVisibility::Collapsed);
	BasicMini2->SetVisibility(ESlateVisibility::Collapsed);
	MakeDraggable(Win, W, 16.f);
}

void UMythRoHUDWidget::SetBasicMenuOpen(bool bOpen)
{
	// A seta alterna entre a janela completa (informacoes + grade do menu) e a versao recolhida de 2 linhas do
	// cliente LATAM ("Nv. 100 / classe / Nv. 16 / Exp. 60 %" e "HP. x / y | SP. x / y")
	if (!BasicWindow)
	{
		return;
	}
	bBasicMenuOpen = bOpen;
	// Os filhos criados depois da barra de titulo sao os detalhes; guarda a visibilidade original deles uma vez
	if (BasicSavedVisibility.Num() == 0)
	{
		for (int32 i = BasicDetailStart; i < BasicWindow->GetChildrenCount(); ++i)
		{
			UWidget* Child = BasicWindow->GetChildAt(i);
			BasicSavedVisibility.Add(Child, Child->GetVisibility());
		}
	}
	for (const TPair<TWeakObjectPtr<UWidget>, ESlateVisibility>& Pair : BasicSavedVisibility)
	{
		UWidget* Child = Pair.Key.Get();
		if (!Child || Child == MenuToggleBar || Child == BasicMini1 || Child == BasicMini2)
		{
			continue;
		}
		Child->SetVisibility(bOpen ? Pair.Value : ESlateVisibility::Collapsed);
	}
	if (BasicMini1) { BasicMini1->SetVisibility(bOpen ? ESlateVisibility::Collapsed : ESlateVisibility::HitTestInvisible); }
	if (BasicMini2) { BasicMini2->SetVisibility(bOpen ? ESlateVisibility::Collapsed : ESlateVisibility::HitTestInvisible); }
	if (UCanvasPanelSlot* S = MenuToggleBar ? Cast<UCanvasPanelSlot>(MenuToggleBar->Slot) : nullptr)
	{
		S->SetPosition(FVector2D(6.f, bOpen ? 124.f : 44.f) * UIScale);
	}
	if (MenuToggleBar)
	{
		MenuToggleBar->SetBrush(TextureBrush(FMythRoUITextures::Get(bOpen ? TEXT("menu_icon/bt_menu_normal.bmp") : TEXT("menu_icon/bt_menu_close_normal.bmp")), FVector2D(219.f, 9.f) * UIScale));
	}
	const float H = bOpen ? BasicFullH : 56.f;
	if (UCanvasPanelSlot* S = BasicWindow ? Cast<UCanvasPanelSlot>(BasicWindow->Slot) : nullptr)
	{
		S->SetSize(FVector2D(232.f, H) * UIScale);
	}
	if (UCanvasPanelSlot* S = BasicBody ? Cast<UCanvasPanelSlot>(BasicBody->Slot) : nullptr)
	{
		S->SetSize(FVector2D(232.f, bOpen ? BasicInfoH - 8.f : H - 16.f) * UIScale);
	}
}

bool UMythRoHUDWidget::HandleBasicMenuClick(const FVector2D& ScreenPos)
{
	if (!BasicWindow || !BasicWindow->IsVisible())
	{
		return false;
	}
	if (IsUnder(MenuToggleBar, ScreenPos) || IsUnder(BasicMiniButton, ScreenPos))
	{
		SetBasicMenuOpen(!bBasicMenuOpen);
		return true;
	}
	for (int32 i = 0; bBasicMenuOpen && i < MenuIcons.Num(); ++i)
	{
		if (!IsUnder(MenuIcons[i], ScreenPos))
		{
			continue;
		}
		const FString& Name = MenuIconNames[i];
		if (Name == TEXT("status")) { ToggleWindow(EMythRoWindow::Status); }
		else if (Name == TEXT("equip")) { ToggleWindow(EMythRoWindow::Equip); }
		else if (Name == TEXT("item")) { ToggleWindow(EMythRoWindow::Items); }
		else if (Name == TEXT("skill")) { ToggleWindow(EMythRoWindow::Skills); }
		else if (Name == TEXT("map")) { ToggleWindow(EMythRoWindow::Minimap); }
		else if (Name == TEXT("option")) { ToggleGameMenu(); }
		else if (Name == TEXT("party")) { ToggleWindow(EMythRoWindow::Party); }
		else if (Name == TEXT("guild")) { ToggleWindow(EMythRoWindow::Guild); }
		else if (Name == TEXT("quest")) { ToggleWindow(EMythRoWindow::Quests); }
		else if (Name == TEXT("navigation")) { ToggleWindow(EMythRoWindow::Navi); }
		else if (Name == TEXT("sns")) { ToggleWindow(EMythRoWindow::Friends); }
		else if (Name == TEXT("mail")) { ToggleWindow(EMythRoWindow::Mail); }
		else if (Name == TEXT("achievement")) { ToggleWindow(EMythRoWindow::Achievements); }
		else if (Name == TEXT("keyboard")) { ToggleWindow(EMythRoWindow::Shortcuts); }
		else if (Name == TEXT("bank")) { ToggleWindow(EMythRoWindow::Bank); }
		else { AddChatLine(TEXT("Esta janela ainda não existe no Myth of Thor."), ColSystem); }
		return true;
	}
	return false;
}

void UMythRoHUDWidget::BuildMinimap()
{
	// Minimapa do LATAM (print 22): a imagem do mapa (com zoom, recortada em 128x128) e, embaixo, a faixa com as coordenadas e os
	// botoes de aproximar (+), afastar (-) e recolher. O ponto do jogador e vermelho
	const float W = 128.f, H = 128.f + 16.f;
	MinimapWindow = MakeWindow(FVector2D(1.f, 0.f), FVector2D(1.f, 0.f), FVector2D(-4.f, 4.f), FVector2D(W, H));
	MinimapView = WidgetTree->ConstructWidget<UCanvasPanel>();
	MinimapView->SetClipping(EWidgetClipping::ClipToBounds);
	MinimapView->SetVisibility(ESlateVisibility::Visible); // o clique no minimapa nao anda o personagem
	if (UCanvasPanelSlot* S = MinimapWindow->AddChildToCanvas(MinimapView))
	{
		S->SetPosition(FVector2D::ZeroVector);
		S->SetSize(FVector2D(W, 128.f) * UIScale);
	}
	MinimapImage = AddImage(MinimapView, nullptr, 0.f, 0.f, 128.f, 128.f);
	MinimapImage->SetColorAndOpacity(FLinearColor::Transparent);
	PlayerDot = AddColor(MinimapView, FLinearColor(0.95f, 0.15f, 0.1f), 0.f, 0.f, 5.f, 5.f);
	PlayerDot->SetVisibility(ESlateVisibility::Collapsed);
	CoordsText = AddText(MinimapWindow, 4.f, 131.f, 7, FLinearColor::White, true);
	MinimapZoomIn = AddButtonEx(MinimapWindow, TEXT("minimap/i_plus_1.bmp"), TEXT("minimap/i_plus_2.bmp"), TEXT("minimap/i_plus_3.bmp"), 82.f, 130.f);
	MinimapZoomOut = AddButtonEx(MinimapWindow, TEXT("minimap/i_minus_1.bmp"), TEXT("minimap/i_minus_2.bmp"), TEXT("minimap/i_minus_3.bmp"), 98.f, 130.f);
	MinimapFold = AddButtonEx(MinimapWindow, TEXT("minimap/i_mini_1.bmp"), TEXT("minimap/i_mini_2.bmp"), TEXT("minimap/i_mini_3.bmp"), 114.f, 130.f);
	MapTitleText = AddText(MinimapWindow, 0.f, 0.f, 7, FLinearColor::White);
	MapTitleText->SetVisibility(ESlateVisibility::Collapsed); // o LATAM nao escreve o nome no minimapa
}

bool UMythRoHUDWidget::HandleMinimapClick(const FVector2D& ScreenPos)
{
	if (!MinimapWindow || !MinimapWindow->IsVisible())
	{
		return false;
	}
	if (IsUnder(MinimapZoomIn, ScreenPos)) { MinimapZoom = FMath::Min(3.f, MinimapZoom + 0.5f); return true; }
	if (IsUnder(MinimapZoomOut, ScreenPos)) { MinimapZoom = FMath::Max(1.f, MinimapZoom - 0.5f); return true; }
	if (IsUnder(MinimapFold, ScreenPos))
	{
		bMinimapFolded = !bMinimapFolded;
		MinimapView->SetVisibility(bMinimapFolded ? ESlateVisibility::Collapsed : ESlateVisibility::Visible);
		return true;
	}
	return false;
}

void UMythRoHUDWidget::BuildHotbar()
{
	// Barra de atalhos do LATAM (print 03): no topo, colada a direita das informacoes basicas. Duas linhas de 9 espacos
	// (moldura shortcut_frame 290x34), o numero da linha a esquerda (azul a ativa) e as setas para subir e descer
	constexpr float RowH = 34.f;
	UCanvasPanel* Win = MakeWindow(FVector2D(0.f, 0.f), FVector2D(0.f, 0.f), FVector2D(234.f * UIScale, 2.f), FVector2D(306.f, RowH * 2.f));
	HotbarWindow = Win;
	for (int32 r = 0; r < 2; ++r)
	{
		AddImage(Win, FMythRoUITextures::Get(TEXT("shortcut/shortcut_frame.bmp")), 0.f, r * RowH, 290.f, RowH);
		UImage* Box = AddColor(Win, FLinearColor(0.93f, 0.85f, 0.9f, 1.f), 4.f, r * RowH + 9.f, 8.f, 14.f);
		HotbarRowBoxes.Add(Box);
		UTextBlock* Num = AddText(Win, 5.f, r * RowH + 10.f, 6, FLinearColor(0.4f, 0.3f, 0.4f));
		HotbarRowTexts.Add(Num);
	}
	for (int32 i = 0; i < 18; ++i)
	{
		// Espaco do atalho: area de soltar (recebe o mouse), icone 24x24 e quantidade/nivel no canto
		const float X = 12.f + 30.6f * (i % 9);
		const float Y = (i / 9) * RowH;
		UImage* HotSlot = AddColor(Win, FLinearColor::Transparent, X, Y + 3.f, 28.f, 28.f);
		HotSlot->SetVisibility(ESlateVisibility::Visible);
		HotbarSlots.Add(HotSlot);
		UImage* Icon = AddImage(Win, nullptr, X + 2.f, Y + 5.f, 24.f, 24.f);
		Icon->SetVisibility(ESlateVisibility::Collapsed);
		HotbarIcons.Add(Icon);
		UTextBlock* Count = AddText(Win, X + 15.f, Y + 20.f, 6, ColText);
		Count->SetShadowOffset(FVector2D(1.f, 1.f));
		Count->SetShadowColorAndOpacity(FLinearColor(1.f, 1.f, 1.f, 0.9f));
		HotbarCounts.Add(Count);
	}
	HotbarUp = AddButtonEx(Win, TEXT("shortcut/btn_minus_normal.bmp"), TEXT("shortcut/btn_minus_over.bmp"), TEXT("shortcut/btn_minus_press.bmp"), 292.f, 3.f);
	HotbarDown = AddButtonEx(Win, TEXT("shortcut/btn_plus_normal.bmp"), TEXT("shortcut/btn_plus_over.bmp"), TEXT("shortcut/btn_plus_press.bmp"), 292.f, RowH + 3.f);
	// So a borda esquerda arrasta a barra: o resto recebe os atalhos arrastados
	MakeDraggable(Win, 4.f, RowH * 2.f);
}

void UMythRoHUDWidget::BuildChat()
{
	// Chat (baixo-esquerda, print 18): abas "Regular Message" (branca, ativa) e "Battle Message" (escura), corpo translucido
	// com as linhas, barra de rolagem a direita e o campo de digitacao
	const float W = 440.f, H = 21.f + 98.f + 20.f; // largura do chat do LATAM (~60% da tela no print)
	UCanvasPanel* Win = MakeWindow(FVector2D(0.f, 1.f), FVector2D(0.f, 1.f), FVector2D(8.f, -8.f), FVector2D(W, H));
	ChatWindow = Win;
	const TCHAR* TabNames[2] = { TEXT("Geral"), TEXT("Batalha") };
	for (int32 t = 0; t < 2; ++t)
	{
		UImage* Box = AddRounded(Win, FLinearColor::White, FLinearColor(0.3f, 0.3f, 0.3f, 1.f), 76.f * t, 3.f, 76.f, 18.f, 4.f, 0.f);
		Box->SetVisibility(ESlateVisibility::Visible);
		ChatTabBoxes.Add(Box);
		UTextBlock* Text = AddText(Win, 76.f * t + 10.f, 5.f, 8, FLinearColor::Black);
		Text->SetText(FText::FromString(TabNames[t]));
		ChatTabTexts.Add(Text);
	}
	AddRounded(Win, ColChatBack, FLinearColor(1.f, 1.f, 1.f, 0.35f), 0.f, 21.f, W, 98.f, 0.f, 0.f);

	ChatLines = WidgetTree->ConstructWidget<UVerticalBox>();
	ChatLines->SetVisibility(ESlateVisibility::HitTestInvisible);
	if (UCanvasPanelSlot* S = Win->AddChildToCanvas(ChatLines))
	{
		S->SetPosition(FVector2D(6.f, 24.f) * UIScale);
		S->SetSize(FVector2D(W - 28.f, 92.f) * UIScale);
	}
	// Barra de rolagem: trilho preto, setas do cliente e o cursor branco
	AddColor(Win, FLinearColor(0.f, 0.f, 0.f, 0.85f), W - 14.f, 22.f, 12.f, 96.f);
	ChatUpArrow = AddImage(Win, FMythRoUITextures::Get(TEXT("basic_interface/dialscr_up.bmp")), W - 14.f, 22.f);
	ChatUpArrow->SetVisibility(ESlateVisibility::Visible);
	ChatDownArrow = AddImage(Win, FMythRoUITextures::Get(TEXT("basic_interface/dialscr_down.bmp")), W - 14.f, 106.f);
	ChatDownArrow->SetVisibility(ESlateVisibility::Visible);
	ChatThumb = AddColor(Win, FLinearColor(0.8f, 0.82f, 0.9f, 1.f), W - 13.f, 34.f, 10.f, 14.f);

	ChatInput = WidgetTree->ConstructWidget<UEditableTextBox>();
	ChatInput->SetHintText(FText::FromString(TEXT("Enter para falar  |  @comando")));
	ChatInput->OnTextCommitted.AddDynamic(this, &UMythRoHUDWidget::OnChatCommitted);
	if (UCanvasPanelSlot* S = Win->AddChildToCanvas(ChatInput))
	{
		S->SetPosition(FVector2D(0.f, 120.f) * UIScale);
		S->SetSize(FVector2D(W, 19.f) * UIScale);
	}
	MakeDraggable(Win, W, 21.f); // pela faixa das abas
	RebuildChatView();
}

void UMythRoHUDWidget::RebuildChatView()
{
	if (!ChatLines)
	{
		return;
	}
	ChatLines->ClearChildren();
	TArray<const FChatEntry*> Shown;
	for (const FChatEntry& E : ChatLog)
	{
		if (ChatTab == 0 || E.bBattle)
		{
			Shown.Add(&E);
		}
	}
	ChatScroll = FMath::Clamp(ChatScroll, 0, FMath::Max(0, Shown.Num() - MaxChatLines));
	const int32 End = Shown.Num() - ChatScroll;
	for (int32 i = FMath::Max(0, End - MaxChatLines); i < End; ++i)
	{
		UTextBlock* T = WidgetTree->ConstructWidget<UTextBlock>();
		T->SetFont(FMythRoUITextures::RoFont(FMath::RoundToInt(7 * UIScale)));
		T->SetColorAndOpacity(FSlateColor(Shown[i]->Color));
		T->SetShadowOffset(FVector2D(1.f, 1.f));
		T->SetShadowColorAndOpacity(FLinearColor(0.f, 0.f, 0.f, 0.9f));
		T->SetAutoWrapText(true);
		T->SetText(FText::FromString(Shown[i]->Text));
		ChatLines->AddChildToVerticalBox(T);
	}
	for (int32 t = 0; t < ChatTabBoxes.Num(); ++t)
	{
		const bool bActive = t == ChatTab;
		ChatTabBoxes[t]->SetColorAndOpacity(bActive ? FLinearColor::White : FLinearColor(0.02f, 0.02f, 0.02f, 0.97f));
		ChatTabTexts[t]->SetColorAndOpacity(FSlateColor(bActive ? FLinearColor::Black : FLinearColor::White));
	}
	// Cursor da barra: posicao proporcional ao historico (embaixo = mais novo)
	if (UCanvasPanelSlot* S = ChatThumb ? Cast<UCanvasPanelSlot>(ChatThumb->Slot) : nullptr)
	{
		const float Range = FMath::Max(1, Shown.Num() - MaxChatLines);
		const float Frac = Shown.Num() > MaxChatLines ? 1.f - ChatScroll / Range : 1.f;
		S->SetPosition(FVector2D(440.f - 13.f, 34.f + Frac * (106.f - 14.f - 34.f)) * UIScale);
	}
}

bool UMythRoHUDWidget::HandleChatClick(const FVector2D& ScreenPos)
{
	if (!ChatWindow || !ChatWindow->IsVisible())
	{
		return false;
	}
	for (int32 t = 0; t < ChatTabBoxes.Num(); ++t)
	{
		if (IsUnder(ChatTabBoxes[t], ScreenPos))
		{
			ChatTab = t;
			ChatScroll = 0;
			RebuildChatView();
			return true;
		}
	}
	if (IsUnder(ChatUpArrow, ScreenPos)) { ++ChatScroll; RebuildChatView(); return true; }
	if (IsUnder(ChatDownArrow, ScreenPos)) { --ChatScroll; RebuildChatView(); return true; }
	return false;
}

bool UMythRoHUDWidget::HandleChatWheel(const FVector2D& ScreenPos, float Delta)
{
	if (!ChatWindow || !ChatWindow->IsVisible() || !IsUnder(ChatWindow, ScreenPos))
	{
		return false;
	}
	ChatScroll += Delta > 0.f ? 2 : -2;
	RebuildChatView();
	return true;
}

void UMythRoHUDWidget::BuildAutoBattle()
{
	// Janela do Auto-Battle (baixo-direita): botao liga/desliga, vigor de 3h e peso
	const float W = 170.f, H = 17.f + 52.f;
	UCanvasPanel* Win = MakeWindow(FVector2D(1.f, 1.f), FVector2D(1.f, 1.f), FVector2D(-8.f, -8.f), FVector2D(W, H));
	AddRounded(Win, ColWindow, ColWindowEdge, 0.f, 8.f, W, H - 8.f, 0.f, 7.f);
	AddTitleBar(Win, W, TEXT("Auto-Battle"));

	AutoButton = WidgetTree->ConstructWidget<UButton>();
	AutoButton->OnClicked.AddDynamic(this, &UMythRoHUDWidget::OnAutoButtonClicked);
	AutoButtonText = WidgetTree->ConstructWidget<UTextBlock>();
	AutoButtonText->SetFont(FMythRoUITextures::RoFont(FMath::RoundToInt(7 * UIScale), true));
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
	// O cliente do RO nao tem esta janela na tela: fica escondida ate a HUD nova dar um lugar para ela
	Win->SetVisibility(ESlateVisibility::Collapsed);
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
	// Classe, nivel de classe, exp de classe e zeny vem do servidor; offline ficam os valores do Aprendiz.
	const URagnarokNetworkSubsystem* Net = GetNet();
	const bool bOnline = Net && Net->GetNetworkState() == ERagnarokNetworkState::ConnectedMap;
	const FRagnarokPlayerStats* NetStats = bOnline ? &Net->GetPlayerStats() : nullptr;
	JobText->SetText(FText::FromString(NetStats ? NetStats->JobName : FString(TEXT("Aprendiz"))));
	BaseLvText->SetText(FText::FromString(FString::Printf(TEXT("Base Lv. %d"), Stats->GetBaseLevel())));
	JobLvText->SetText(FText::FromString(FString::Printf(TEXT("Job Lv. %d"), NetStats ? NetStats->JobLevel : 1)));

	const float HPFrac = static_cast<float>(Stats->GetHP()) / FMath::Max(1, Stats->GetMaxHP());
	const float SPFrac = static_cast<float>(Stats->GetSP()) / FMath::Max(1, Stats->GetMaxSP());
	HPGauge.SetFraction(HPFrac);
	HPText->SetText(FText::FromString(FString::Printf(TEXT("%d  /  %d"), Stats->GetHP(), Stats->GetMaxHP())));
	SPGauge.SetFraction(SPFrac);
	SPText->SetText(FText::FromString(FString::Printf(TEXT("%d  /  %d"), Stats->GetSP(), Stats->GetMaxSP())));
	if (BasicMini1 && BasicMini2)
	{
		BasicMini1->SetText(FText::FromString(FString::Printf(TEXT("Nv. %d / %s / Nv. %d / Exp. %d %%"), Stats->GetBaseLevel(),
			NetStats ? *NetStats->JobName : TEXT("Aprendiz"), NetStats ? NetStats->JobLevel : 1, FMath::RoundToInt(Stats->GetExpFraction() * 100.f))));
		BasicMini2->SetText(FText::FromString(FString::Printf(TEXT("HP. %d / %d  |  SP. %d / %d"), Stats->GetHP(), Stats->GetMaxHP(), Stats->GetSP(), Stats->GetMaxSP())));
	}
	if (HPPctText) { HPPctText->SetText(FText::FromString(FString::Printf(TEXT("%d%%"), FMath::RoundToInt(HPFrac * 100.f)))); }
	if (SPPctText) { SPPctText->SetText(FText::FromString(FString::Printf(TEXT("%d%%"), FMath::RoundToInt(SPFrac * 100.f)))); }
	BaseExpGauge.SetFraction(Stats->GetExpFraction());
	const float JobFrac = NetStats && NetStats->NextJobExp > 0 ? static_cast<float>(static_cast<double>(NetStats->JobExp) / NetStats->NextJobExp) : 0.f;
	JobExpGauge.SetFraction(JobFrac);
	if (BaseExpPctText) { BaseExpPctText->SetText(FText::FromString(FString::Printf(TEXT("%.2f%%"), Stats->GetExpFraction() * 100.f))); }
	if (JobExpPctText) { JobExpPctText->SetText(FText::FromString(FString::Printf(TEXT("%.2f%%"), JobFrac * 100.f))); }
	// Como o cliente: "Peso : atual / max   Zeny : 29.743" (o rAthena manda o peso em decimos)
	const FString Zeny = FText::AsNumber(NetStats ? NetStats->Zeny : 0LL).ToString();
	if (NetStats && NetStats->MaxWeight > 0)
	{
		WeightZenyText->SetText(FText::FromString(FString::Printf(TEXT("Peso : %d / %d     Zeny : %s"), NetStats->Weight / 10, NetStats->MaxWeight / 10, *Zeny)));
		// Como o cliente: passou de 50% o peso fica vermelho (o servidor decide o efeito: regeneracao parada, 90% sem combate)
		WeightZenyText->SetColorAndOpacity(FSlateColor(NetStats->Weight * 2 >= NetStats->MaxWeight ? FLinearColor(0.85f, 0.1f, 0.1f) : ColText));
	}
	else
	{
		WeightZenyText->SetText(FText::FromString(FString::Printf(TEXT("Peso : %.0f%%     Zeny : %s"), WeightPercent, *Zeny)));
	}
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
	// Mensagens de combate (experiencia, drops, dano) vao tambem para a aba Batalha
	const FString Lower = Text.ToLower();
	FChatEntry Entry;
	Entry.Text = Text;
	Entry.Color = Color;
	Entry.bBattle = Lower.Contains(TEXT("exp")) || Lower.Contains(TEXT("dano")) || Lower.Contains(TEXT("drop")) || Lower.Contains(TEXT("ganhou")) || Lower.Contains(TEXT("derrot"));
	ChatLog.Add(Entry);
	if (ChatLog.Num() > 300)
	{
		ChatLog.RemoveAt(0);
	}
	if (ChatScroll > 0 && (ChatTab == 0 || Entry.bBattle))
	{
		++ChatScroll; // lendo o historico: a tela nao pula quando chega linha nova
	}
	RebuildChatView();
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
	SubmitChat(Msg);
}

void UMythRoHUDWidget::SubmitChat(const FString& Msg)
{
	if (Msg.IsEmpty())
	{
		return;
	}
	ChatHistory.Remove(Msg);
	ChatHistory.Add(Msg);
	if (ChatHistory.Num() > 20)
	{
		ChatHistory.RemoveAt(0);
	}
	ChatHistoryPos = INDEX_NONE;
	if (HandleChatCommand(Msg))
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
	if (EntryState != 0)
	{
		if (InMouseEvent.GetEffectingButton() == EKeys::LeftMouseButton)
		{
			HandleEntryClick(InMouseEvent.GetScreenSpacePosition());
		}
		return FReply::Handled();
	}
	// Ctrl + botao direito num item da mochila: poe ou tira dos favoritos (aba com a estrela)
	if (InMouseEvent.GetEffectingButton() == EKeys::RightMouseButton && InMouseEvent.IsControlDown() && ItemsWindow && ItemsWindow->IsVisible())
	{
		if (URagnarokNetworkSubsystem* FavNet = GetNet())
		{
			for (int32 c = 0; c < ItemCells.Num(); ++c)
			{
				if (ItemCellIndex[c] >= 0 && IsUnder(ItemCells[c], InMouseEvent.GetScreenSpacePosition()))
				{
					for (const FRagnarokInventoryItem& It : FavNet->GetInventoryItems())
					{
						if (It.Index == ItemCellIndex[c])
						{
							FavNet->SendItemFavorite(It.Index, It.bFavorite);
							return FReply::Handled();
						}
					}
				}
			}
		}
	}
	if (InMouseEvent.GetEffectingButton() == EKeys::RightMouseButton && Root && HandleItemInfoClick(InMouseEvent.GetScreenSpacePosition()))
	{
		return FReply::Handled(); // botao direito num item: descricao
	}
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
		if (!Win || Win->GetVisibility() == ESlateVisibility::Collapsed || LockedWindows.Contains(Win))
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
	// Clique em qualquer lugar de uma janela aberta fica na interface, como no cliente original
	for (const FDragWindow& D : DragWindows)
	{
		if (D.Window.IsValid() && D.Window->GetVisibility() != ESlateVisibility::Collapsed && IsUnder(D.Window.Get(), Screen))
		{
			return FReply::Handled();
		}
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
	TickNaviAuto(InDeltaTime);
	TickWorldMap();
	if (EquipWindow && EquipWindow->IsVisible())
	{
		UpdateEquipMount(); // municao e falcao/montaria mudam sem avisar a janela
		UpdateEquipPreview(); // o sprite do jogador recarrega depois do pacote de visual (ZC_SPRITE_CHANGE)
	}
	UpdateMinimap();
	UpdateButtons();
	WindowRefreshTimer -= InDeltaTime;
	if (WindowRefreshTimer <= 0.f)
	{
		WindowRefreshTimer = 0.25f;
		RefreshWindows();
	}
}

void UMythRoHUDWidget::StyleEditBoxes()
{
	// Todas as caixas de texto usam a fonte da interface no mesmo tamanho dos rotulos, fundo branco com borda fina e o texto escuro
	FTextBlockStyle TextStyle;
	TextStyle.SetFont(FMythRoUITextures::RoFont(FMath::RoundToInt(8 * UIScale)));
	TextStyle.SetColorAndOpacity(FSlateColor(FLinearColor(0.08f, 0.1f, 0.18f)));
	const FSlateRoundedBoxBrush Normal(FLinearColor(1.f, 1.f, 1.f, 0.97f), 2.f, FLinearColor(0.55f, 0.6f, 0.75f, 1.f), 1.f);
	const FSlateRoundedBoxBrush Focused(FLinearColor(1.f, 1.f, 1.f, 1.f), 2.f, FLinearColor(0.3f, 0.5f, 0.9f, 1.f), 1.f);
	FEditableTextBoxStyle Style;
	Style.SetTextStyle(TextStyle);
	Style.SetBackgroundImageNormal(Normal);
	Style.SetBackgroundImageHovered(Normal);
	Style.SetBackgroundImageFocused(Focused);
	Style.SetBackgroundImageReadOnly(Normal);
	Style.SetForegroundColor(FSlateColor(FLinearColor(0.08f, 0.1f, 0.18f)));
	Style.SetFocusedForegroundColor(FSlateColor(FLinearColor(0.08f, 0.1f, 0.18f)));
	Style.SetPadding(FMargin(5.f, 1.f));
	WidgetTree->ForEachWidget([&Style](UWidget* Widget)
	{
		if (UEditableTextBox* Box = Cast<UEditableTextBox>(Widget))
		{
			Box->WidgetStyle = Style;
			Box->SynchronizeProperties();
		}
	});
}

void UMythRoHUDWidget::UpdateButtons()
{
	if (!FSlateApplication::IsInitialized())
	{
		return;
	}
	const FVector2D CursorPos = FSlateApplication::Get().GetCursorPos();
	const bool bDown = FSlateApplication::Get().GetPressedMouseButtons().Contains(EKeys::LeftMouseButton);
	for (FRoButton& B : RoButtons)
	{
		UImage* Img = B.Image.Get();
		if (!Img || !Img->IsVisible() || !Img->GetCachedGeometry().GetLocalSize().X)
		{
			continue;
		}
		const bool bOver = IsUnder(Img, CursorPos);
		const int32 Want = bOver ? (bDown ? 2 : 1) : 0;
		if (Want == B.Shown)
		{
			continue;
		}
		UTexture2D* Tex = Want == 2 ? B.Pressed.Get() : (Want == 1 ? B.Hover.Get() : B.Normal.Get());
		if (!Tex)
		{
			Tex = B.Normal.Get();
		}
		if (Tex && B.Part == TEXT("text"))
		{
			// botao de texto: a imagem do meio mantem o tamanho (so troca a textura); as pontas seguem o estado
			const TCHAR* State = Want == 2 ? TEXT("press") : (Want == 1 ? TEXT("over") : TEXT("out"));
			FSlateBrush Brush = Img->GetBrush();
			Brush.SetResourceObject(Tex);
			Img->SetBrush(Brush);
			if (UImage* Lf = B.Left.Get()) { FSlateBrush Bl = Lf->GetBrush(); Bl.SetResourceObject(FMythRoUITextures::Get(FString::Printf(TEXT("basic_interface/btn_%s_left.bmp"), State))); Lf->SetBrush(Bl); }
			if (UImage* Rt = B.Right.Get()) { FSlateBrush Br = Rt->GetBrush(); Br.SetResourceObject(FMythRoUITextures::Get(FString::Printf(TEXT("basic_interface/btn_%s_right.bmp"), State))); Rt->SetBrush(Br); }
		}
		else if (Tex)
		{
			// So troca a textura: botoes desenhados como caixa (9 fatias, ex. Compartilhar/Procurar da Navegacao) mantem as margens
			FSlateBrush Brush = Img->GetBrush();
			Brush.SetResourceObject(Tex);
			Brush.ImageSize = FVector2D(Tex->GetSizeX(), Tex->GetSizeY()) * UIScale;
			Img->SetBrush(Brush);
		}
		B.Shown = Want;
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
			MinimapImage->SetColorAndOpacity(FLinearColor::White);
		}
	}
	const APawn* Pawn = BoundPawn.Get();
	if (!Pawn)
	{
		return;
	}
	const FIntPoint Cell = Info->WorldToCell(Pawn->GetActorLocation());
	CoordsText->SetText(FText::FromString(FString::Printf(TEXT("%d  %d"), Cell.X, Cell.Y))); // como o LATAM: "214 346"

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
	const float Px0 = OffX + (Cell.X + 0.5f) * K;
	const float Py0 = MapPx - (OffY + (Cell.Y + 0.5f) * K); // celula Y cresce para cima; a imagem, para baixo
	// Zoom: a imagem cresce e e deslocada para o jogador ficar no centro (sem sair do recorte de 128 x 128)
	const float Z = MinimapZoom;
	const float ImgSize = MapPx * Z;
	const float ImgX = FMath::Clamp(MapPx * 0.5f - Px0 * Z, MapPx - ImgSize, 0.f);
	const float ImgY = FMath::Clamp(MapPx * 0.5f - Py0 * Z, MapPx - ImgSize, 0.f);
	if (ImgSize != MinimapShownSize || LoadedMinimapBrush != Info->MapName)
	{
		MinimapShownSize = ImgSize;
		LoadedMinimapBrush = Info->MapName;
		if (UTexture2D* Tex = FMythRoUITextures::Get(FString::Printf(TEXT("map/%s.bmp"), *Info->MapName)))
		{
			MinimapImage->SetBrush(TextureBrush(Tex, FVector2D(ImgSize, ImgSize) * UIScale));
		}
	}
	if (UCanvasPanelSlot* S = Cast<UCanvasPanelSlot>(MinimapImage->Slot))
	{
		S->SetPosition(FVector2D(ImgX, ImgY) * UIScale);
		S->SetSize(FVector2D(ImgSize, ImgSize) * UIScale);
	}
	if (UCanvasPanelSlot* S = Cast<UCanvasPanelSlot>(PlayerDot->Slot))
	{
		S->SetPosition(FVector2D(ImgX + Px0 * Z - 2.f, ImgY + Py0 * Z - 2.f) * UIScale);
	}
	PlayerDot->SetVisibility(ESlateVisibility::HitTestInvisible);
}

// Janelas classicas do cliente original (Atributos, Itens, Equipamento, Habilidades) do UMythRoHUDWidget.
// Regra do projeto: tudo o que e mostrado aqui vem do rAthena (URagnarokNetworkSubsystem); o cliente so desenha
// e envia pedidos (CZ_STATUS_CHANGE, CZ_USE_ITEM, CZ_REQ_WEAR_EQUIP, CZ_REQ_TAKEOFF_EQUIP).

#include "UI/MythRoHUDWidget.h"

#include "UI/MythRoClientTables.h"
#include "UI/MythRoSkillTree.h"
#include "UI/MythRoUITextures.h"
#include "Network/RagnarokNetworkSubsystem.h"
#include "Player/MythPlayerController.h"
#include "Sprites/MythRoSpriteComponent.h"

#include "Blueprint/WidgetTree.h"
#include "Components/CanvasPanel.h"
#include "Components/CanvasPanelSlot.h"
#include "Components/Image.h"
#include "Components/TextBlock.h"
#include "Engine/GameInstance.h"
#include "Dom/JsonObject.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "Serialization/JsonReader.h"
#include "Serialization/JsonSerializer.h"

namespace
{
	const FLinearColor RoWinText(0.08f, 0.10f, 0.18f);
	const FLinearColor RoWinLabel(0.12f, 0.27f, 0.55f);
	const FLinearColor RoWinBody(0.965f, 0.965f, 0.98f, 1.0f);
	const FLinearColor RoWinFaint(0.45f, 0.48f, 0.55f);

	FString RoWinArt(const TCHAR* Name) { return FString::Printf(TEXT("basic_interface/%s.bmp"), Name); }

	FSlateBrush RoWinBrush(UTexture2D* Tex, const FVector2D& Size)
	{
		FSlateBrush Brush;
		Brush.SetResourceObject(Tex);
		Brush.ImageSize = Size;
		Brush.DrawAs = ESlateBrushDrawType::Image;
		return Brush;
	}

	// Ordem dos atributos no CZ_STATUS_CHANGE do rAthena (SP_STR = 13 ... SP_LUK = 18)
	const TCHAR* RoStatNames[6] = { TEXT("Str"), TEXT("Agi"), TEXT("Vit"), TEXT("Int"), TEXT("Dex"), TEXT("Luk") };
	constexpr int32 RoSpStr = 13;

	// Encaixes da janela de equipamento (arte equipwin_bg): coluna esquerda e direita, mascara EQP_* do rAthena
	struct FRoEquipSlot { uint32 Mask; bool bRight; float CenterY; };
	const FRoEquipSlot RoEquipSlots[10] = {
		{ 0x0100, false, 17.5f },  // cabeca (topo)
		{ 0x0200, false, 42.5f },  // cabeca (meio)
		{ 0x0002, false, 69.f },   // mao direita
		{ 0x0004, false, 95.f },   // capa
		{ 0x0008, false, 121.f },  // acessorio (esq.)
		{ 0x0001, true, 17.5f },   // cabeca (baixo)
		{ 0x0010, true, 42.5f },   // vestimenta
		{ 0x0020, true, 69.f },    // mao esquerda
		{ 0x0040, true, 95.f },    // calcados
		{ 0x0080, true, 121.f },   // acessorio (dir.)
	};

	// Abas da janela de itens pelo tipo do item (IT_* do rAthena): 0 item, 1 equip, 2 etc
	int32 RoItemTab(int32 Type)
	{
		switch (Type)
		{
		case 0: case 2: case 11: case 18: return 0;          // cura, usavel, consumo com atraso, cash
		case 4: case 5: case 7: case 8: case 12: return 1;   // armadura, arma, ovo, acessorio de pet, sombra
		default: return 2;                                   // etc, carta, municao (como no cliente: a municao fica em Diversos)
		}
	}

	constexpr int32 RoItemCols = 7;
	constexpr int32 RoItemRows = 4;
	constexpr int32 RoSkillRows = 7;
	// Arvore de habilidades: grade de 7 colunas, celula de 40 x 46 (moldura 32, icone 24, nivel embaixo)
	constexpr float RoTreeLeft = 32.f; // 24 de coluna de abas verticais (1a/2a/...) + 8 de margem, como no print 13
	constexpr float RoTreeTop = 24.f;
	constexpr float RoTreeCellW = 68.f; // print 13 do LATAM: 7 colunas a cada ~68
	constexpr float RoTreeCellH = 56.f;
	constexpr int32 RoTreeRows = 9;
	// Janela de equipamento: topo da arte (titulo 17 + abas 17), rodape e largura da previa do personagem
	constexpr float EqTop = 34.f;
	constexpr float EqFootH = 22.f;
	constexpr float EqPreviewW = 120.f;
	// Personagem da janela de Habilidades: 80 x 100, subindo da barra de baixo
	constexpr float SkillPortraitLift = 66.f;
}

namespace
{
	/** Config/titles.json (tools/gen_titles.py): nomes dos titulos (tabela do cliente) e qual conquista da cada um (servidor). */
	struct FRoTitleTables
	{
		TMap<int32, FString> Names;
		TMap<int32, int32> Achievements;
	};

	const FRoTitleTables& RoTitles()
	{
		static FRoTitleTables T;
		static bool bLoaded = false;
		if (!bLoaded)
		{
			bLoaded = true;
			FString Text;
			TSharedPtr<FJsonObject> Root;
			if (FFileHelper::LoadFileToString(Text, *FPaths::Combine(FPaths::ProjectConfigDir(), TEXT("titles.json")))
				&& FJsonSerializer::Deserialize(TJsonReaderFactory<>::Create(Text), Root) && Root.IsValid())
			{
				const TSharedPtr<FJsonObject>* Obj = nullptr;
				if (Root->TryGetObjectField(TEXT("titles"), Obj))
				{
					for (const auto& P : (*Obj)->Values) { T.Names.Add(FCString::Atoi(*P.Key), P.Value->AsString()); }
				}
				if (Root->TryGetObjectField(TEXT("achievements"), Obj))
				{
					for (const auto& P : (*Obj)->Values) { T.Achievements.Add(FCString::Atoi(*P.Key), static_cast<int32>(P.Value->AsNumber())); }
				}
			}
		}
		return T;
	}
}

// Aba da mochila de um item (print 04): 0 consumiveis, 1 equipamentos, 2 diversos e 3 favoritos (o servidor guarda a marca)
static int32 RoItemTabOf(const FRagnarokInventoryItem& Item)
{
	return Item.bFavorite ? 3 : RoItemTab(Item.Type);
}

URagnarokNetworkSubsystem* UMythRoHUDWidget::GetNet() const
{
	const UGameInstance* GI = GetGameInstance();
	return GI ? GI->GetSubsystem<URagnarokNetworkSubsystem>() : nullptr;
}

namespace
{
	/** Janelas minimizadas (o botao da barra de titulo): so a barra de titulo recebe clique. */
	TSet<const UWidget*>& MinimizedWindows()
	{
		static TSet<const UWidget*> Set;
		return Set;
	}
}

bool UMythRoHUDWidget::IsUnder(const UWidget* Widget, const FVector2D& ScreenPos)
{
	if (!Widget || !Widget->IsVisible())
	{
		return false;
	}
	if (MinimizedWindows().Num() > 0)
	{
		// Dentro de uma janela minimizada so a barra de titulo existe: o resto (cortado pela moldura) nao recebe clique
		for (const UWidget* P = Widget->GetParent(); P; P = P->GetParent())
		{
			if (MinimizedWindows().Contains(P))
			{
				const FGeometry& PGeo = P->GetCachedGeometry();
				if (PGeo.AbsoluteToLocal(ScreenPos).Y > PGeo.GetLocalSize().Y)
				{
					return false;
				}
			}
		}
	}
	const FGeometry& Geo = Widget->GetCachedGeometry();
	const FVector2D Local = Geo.AbsoluteToLocal(ScreenPos);
	return Local.X >= 0.f && Local.Y >= 0.f && Local.X <= Geo.GetLocalSize().X && Local.Y <= Geo.GetLocalSize().Y;
}

UCanvasPanel* UMythRoHUDWidget::MakeRoPanel(const FString& Title, const FVector2D& Size, const FVector2D& Offset)
{
	UCanvasPanel* Win = MakeWindow(FVector2D(0.5f, 0.5f), FVector2D(0.5f, 0.5f), Offset, Size);
	// Fundo "Visible": o clique na janela chega ao HUD e nao atravessa para o jogo (nao anda nem ataca)
	AddRounded(Win, RoWinBody, FLinearColor::Transparent, 0.f, 9.f, Size.X, Size.Y - 9.f, 0.f, 7.f)->SetVisibility(ESlateVisibility::Visible);
	AddTitleBar(Win, Size.X, Title);
	MakeDraggable(Win, Size.X, 17.f);
	// Botao de fechar (sys_close) por cima da alca de arrastar; o clique e testado antes do arrasto
	UImage* Close = AddImage(Win, FMythRoUITextures::Get(RoWinArt(TEXT("sys_close_off"))), Size.X - 15.f, 3.f, 11.f, 11.f);
	CloseButtons.Add(Close, Win);
	FRoMinimize Mini;
	Mini.Win = Win;
	Mini.Button = AddButtonEx(Win, TEXT("basic_interface/sys_mini_off.bmp"), TEXT("basic_interface/sys_mini_on.bmp"), TEXT("basic_interface/sys_mini_on.bmp"), Size.X - 28.f, 3.f);
	Mini.FullSize = Size;
	Win->SetClipping(EWidgetClipping::ClipToBounds);
	Minimizables.Add(Mini);
	Win->SetVisibility(ESlateVisibility::Collapsed);
	return Win;
}

// ------------------------------------------------------------------------------------------------ construcao

void UMythRoHUDWidget::BuildStatusWindow()
{
	// Janela de atributos (Alt+A, print 12): arte w_statwin_bg (280 x 124) do cliente moderno; seis linhas Str..Luk com o valor
	// "base+bonus", a seta e o custo do ponto; ao lado Atk/Def, Matk/Mdef, Hit/Flee, Critical/Aspd, pontos e clã
	StatusWindow = MakeRoPanel(TEXT("Status"), FVector2D(280.f, 17.f + 124.f), FVector2D(-340.f, -170.f));
	AddImage(StatusWindow, FMythRoUITextures::Get(TEXT("statuswnd/w_statwin_bg.bmp")), 0.f, 17.f);
	UTexture2D* Arrow = FMythRoUITextures::Get(RoWinArt(TEXT("arw_right_on")));
	auto Label = [this](float X, float Y, const TCHAR* Text)
	{
		UTextBlock* T = AddText(StatusWindow, X, Y, 8, RoWinLabel);
		T->SetFont(FMythRoUITextures::RoFont(FMath::RoundToInt(8 * UIScale), true));
		T->SetText(FText::FromString(Text));
	};
	auto Box = [this](float X, float Y, float W, ETextJustify::Type J) -> UTextBlock*
	{
		UTextBlock* T = AddText(StatusWindow, X, Y, 7, RoWinText);
		T->SetJustification(J);
		if (UCanvasPanelSlot* S = Cast<UCanvasPanelSlot>(T->Slot))
		{
			S->SetAutoSize(false);
			S->SetSize(FVector2D(W, 14.f) * UIScale);
		}
		return T;
	};
	for (int32 i = 0; i < 6; ++i)
	{
		const float Y = 17.f + 5.f + 16.f * i;
		Label(8.f, Y, ToCStr(FString(RoStatNames[i])));
		StatTexts.Add(Box(38.f, Y, 38.f, ETextJustify::Left));
		StatUpArrows.Add(AddImage(StatusWindow, Arrow, 76.f, Y + 2.f, 8.f, 9.f));
		StatCostTexts.Add(Box(84.f, Y, 16.f, ETextJustify::Right));
	}
	// Derivados (duas colunas) e as duas linhas longas de baixo, sobre as linhas da arte
	const TCHAR* Left[4] = { TEXT("Atk"), TEXT("Matk"), TEXT("Hit"), TEXT("Critical") };
	const TCHAR* Right[4] = { TEXT("Def"), TEXT("Mdef"), TEXT("Flee"), TEXT("Aspd") };
	for (int32 i = 0; i < 4; ++i)
	{
		const float Y = 17.f + 5.f + 16.f * i;
		Label(110.f, Y, Left[i]);
		DerivedTexts.Add(Box(110.f, Y, 78.f, ETextJustify::Right));
		Label(196.f, Y, Right[i]);
		DerivedTexts.Add(Box(196.f, Y, 78.f, ETextJustify::Right));
	}
	Label(110.f, 17.f + 5.f + 64.f, TEXT("Pontos de Atributo"));
	DerivedTexts.Add(Box(110.f, 17.f + 5.f + 64.f, 164.f, ETextJustify::Right));
	Label(110.f, 17.f + 5.f + 80.f, TEXT("Clã"));
	DerivedTexts.Add(Box(110.f, 17.f + 5.f + 80.f, 164.f, ETextJustify::Right));
}

void UMythRoHUDWidget::BuildItemsWindow()
{
	// Inventario (Alt+E, prints 04-07): faixa de 4 abas com icone (tab_pri_0N) a esquerda, grade de 7 x 4 celulas 32x32
	// (itemwin_mid) e o rodape com a contagem de itens (48/100), o cadeado de descarte e a busca
	const float W = 24.f + RoItemCols * 32.f + 6.f;
	const float H = 17.f + RoItemRows * 32.f + 6.f + 24.f;
	ItemsWindow = MakeRoPanel(TEXT("Itens"), FVector2D(W, H), FVector2D(340.f, -170.f));
	ItemTabImage = AddImage(ItemsWindow, FMythRoUITextures::Get(RoWinArt(TEXT("tab_pri_01"))), 0.f, 19.f, 21.f, 134.f);
	UTexture2D* CellTex = FMythRoUITextures::Get(RoWinArt(TEXT("itemwin_mid")));
	for (int32 i = 0; i < RoItemCols * RoItemRows; ++i)
	{
		const float X = 24.f + (i % RoItemCols) * 32.f;
		const float Y = 21.f + (i / RoItemCols) * 32.f;
		UImage* Cell = AddImage(ItemsWindow, CellTex, X, Y, 32.f, 32.f);
		Cell->SetVisibility(ESlateVisibility::Visible); // recebe hover/clique (nao passa para o jogo)
		ItemCells.Add(Cell);
		UImage* Icon = AddImage(ItemsWindow, nullptr, X + 4.f, Y + 4.f, 24.f, 24.f);
		Icon->SetVisibility(ESlateVisibility::Collapsed);
		ItemIcons.Add(Icon);
		UTextBlock* Amount = AddText(ItemsWindow, X + 17.f, Y + 20.f, 6, RoWinText);
		Amount->SetShadowOffset(FVector2D(1.f, 1.f));
		Amount->SetShadowColorAndOpacity(FLinearColor(1.f, 1.f, 1.f, 0.9f));
		ItemAmounts.Add(Amount);
		ItemCellIndex.Add(-1);
	}
	// Rodape: livro + "n/100", cadeado, lupa de comparacao e o botao de busca
	AddImage(ItemsWindow, FMythRoUITextures::Get(TEXT("inventory/icon_num.bmp")), 26.f, H - 20.f);
	ItemCountText = AddText(ItemsWindow, 40.f, H - 21.f, 7, RoWinText);
	AddImage(ItemsWindow, FMythRoUITextures::Get(TEXT("inventory/item_drop_lock_off.bmp")), 82.f, H - 19.f);
	AddImage(ItemsWindow, FMythRoUITextures::Get(TEXT("inventory/item_compare_off.bmp")), 102.f, H - 19.f);
	ItemInfoText = AddText(ItemsWindow, 122.f, H - 21.f, 7, RoWinFaint);
	AddButtonEx(ItemsWindow, TEXT("inventory/btn_search_new_out.bmp"), TEXT("inventory/btn_search_new_over.bmp"), TEXT("inventory/btn_search_new_press.bmp"), W - 32.f, H - 22.f);
}

void UMythRoHUDWidget::BuildEquipWindow()
{
	// Equipamento (Alt+Q), print 08 do LATAM e EquipmentV4 do roBrowser: 280 de largura, 4 abas de 70 (Geral, Especial, Titulo,
	// Fonte de dano), arte equipwin_bg (280 x 130) com os 10 encaixes em linhas de 26, o personagem no centro e o rodape de 22 com
	// "Mostrar equip." e os botoes de atributos, troca rapida e tirar tudo
	EquipWindow = MakeRoPanel(TEXT("Equipamento"), FVector2D(280.f, EqTop + 130.f + EqFootH), FVector2D(-340.f, 120.f));
	const TCHAR* TabNames[4] = { TEXT("Geral"), TEXT("Especial"), TEXT("Título"), TEXT("Fonte de dano") };
	for (int32 t = 0; t < 4; ++t)
	{
		UImage* Box = AddRounded(EquipWindow, FLinearColor::White, FLinearColor(0.62f, 0.66f, 0.76f, 1.f), 1.f + t * 70.f, 18.f, 69.f, EqTop - 18.f, 4.f, 0.f);
		Box->SetVisibility(ESlateVisibility::Visible);
		EquipTabBoxes.Add(Box);
		UTextBlock* Label = AddText(EquipWindow, 1.f + t * 70.f, 19.f, 8, RoWinText);
		Label->SetText(FText::FromString(TabNames[t]));
		Label->SetJustification(ETextJustify::Center);
		if (UCanvasPanelSlot* LS = Cast<UCanvasPanelSlot>(Label->Slot)) { LS->SetAutoSize(false); LS->SetSize(FVector2D(69.f, 14.f) * UIScale); }
		EquipTabTexts.Add(Label);
	}
	EquipBack = AddImage(EquipWindow, FMythRoUITextures::Get(RoWinArt(TEXT("equipwin_bg"))), 0.f, EqTop);
	// Personagem no centro (feito por RenderPortrait): por baixo dos nomes, os pes sobre a sombra desenhada na arte
	EquipPreview = AddImage(EquipWindow, nullptr, 140.f - EqPreviewW * 0.5f, EqTop, EqPreviewW, 130.f);
	EquipPreview->SetVisibility(ESlateVisibility::Collapsed);
	for (const FRoEquipSlot& S : RoEquipSlots)
	{
		const float Y = EqTop + S.CenterY;
		// O rotulo da arte ("cabeça", "mão dir"...) continua visivel, apagado, por baixo do nome (como no print)
		UImage* Back = AddColor(EquipWindow, RoWinBody, 0.f, 0.f, 1.f, 1.f);
		Back->SetVisibility(ESlateVisibility::Collapsed);
		UImage* Icon = AddImage(EquipWindow, nullptr, S.bRight ? 252.f : 4.f, Y - 12.f, 24.f, 24.f);
		Icon->SetVisibility(ESlateVisibility::Collapsed);
		EquipIcons.Add(Icon);
		// Nome em ate 2 linhas de 80: esquerda alinhada a esquerda, direita alinhada a direita
		UTextBlock* Name = AddText(EquipWindow, S.bRight ? 168.f : 32.f, Y - 12.f, 8, RoWinText);
		Name->SetAutoWrapText(true);
		Name->SetJustification(S.bRight ? ETextJustify::Right : ETextJustify::Left);
		Name->SetClipping(EWidgetClipping::ClipToBounds);
		if (UCanvasPanelSlot* NS = Cast<UCanvasPanelSlot>(Name->Slot)) { NS->SetAutoSize(false); NS->SetSize(FVector2D(80.f, 26.f) * UIScale); }
		EquipNames.Add(Name);
		EquipIcons.Add(Back); // pares (icone, fundo) no mesmo array: indice 2*i e 2*i+1
		EquipCellIndex.Add(-1);
	}
	// Coluna do meio: municao equipada em cima (so aparece com municao) e o botao redondo "off" do falcao/Peco embaixo
	EquipAmmoFrame = AddImage(EquipWindow, FMythRoUITextures::Get(RoWinArt(TEXT("itemwin_mid"))), 124.f, EqTop + 2.f, 32.f, 32.f);
	EquipAmmoIcon = AddImage(EquipWindow, nullptr, 128.f, EqTop + 6.f, 24.f, 24.f);
	EquipAmmoIcon->SetVisibility(ESlateVisibility::Collapsed);
	EquipAmmoName = AddText(EquipWindow, 102.f, EqTop + 34.f, 6, RoWinText);
	EquipAmmoName->SetJustification(ETextJustify::Center);
	if (UCanvasPanelSlot* NameSlot = Cast<UCanvasPanelSlot>(EquipAmmoName->Slot))
	{
		NameSlot->SetAutoSize(false);
		NameSlot->SetSize(FVector2D(76.f, 12.f) * UIScale);
	}
	EquipMountBtn = AddImage(EquipWindow, FMythRoUITextures::Get(RoWinArt(TEXT("btn_off"))), 122.f, EqTop + 90.f, 36.f, 36.f);
	// Itens do carrinho (btn_items, roBrowser EquipmentV4: no meio da coluna central), so com carrinho
	EquipCartBtn = AddButtonEx(EquipWindow, RoWinArt(TEXT("btn_items_off")), RoWinArt(TEXT("btn_items_on")), RoWinArt(TEXT("btn_items_on")), 125.f, EqTop + 66.f);
	EquipCartBtn->SetVisibility(ESlateVisibility::Collapsed);
	EquipTabNote = AddText(EquipWindow, 0.f, EqTop + 56.f, 8, RoWinFaint);
	EquipTabNote->SetJustification(ETextJustify::Center);
	if (UCanvasPanelSlot* NS = Cast<UCanvasPanelSlot>(EquipTabNote->Slot)) { NS->SetAutoSize(false); NS->SetSize(FVector2D(280.f, 16.f) * UIScale); }
	EquipTabNote->SetVisibility(ESlateVisibility::Collapsed);
	for (int32 i = 0; i < 8; ++i)
	{
		UImage* Hit = AddColor(EquipWindow, FLinearColor(0.f, 0.f, 0.f, 0.03f), 8.f, EqTop + 4.f + i * 16.f, 264.f, 15.f);
		Hit->SetVisibility(ESlateVisibility::Collapsed);
		EquipTitleHits.Add(Hit);
		UTextBlock* T = AddText(EquipWindow, 14.f, EqTop + 5.f + i * 16.f, 8, RoWinText);
		T->SetVisibility(ESlateVisibility::Collapsed);
		EquipTitleTexts.Add(T);
	}

	// Rodape: linha fina em cima, caixa "Mostrar equip." a esquerda; atributos (23 x 20), troca rapida e tirar tudo (40 x 20) a direita
	const float FootY = EqTop + 130.f;
	AddColor(EquipWindow, FLinearColor(0.78f, 0.8f, 0.86f, 1.f), 0.f, FootY, 280.f, 1.f);
	EquipShowCheck = AddImage(EquipWindow, FMythRoUITextures::Get(TEXT("checkbox_0.bmp")), 8.f, FootY + 6.f, 10.f, 10.f);
	EquipShowCheck->SetVisibility(ESlateVisibility::Visible);
	AddText(EquipWindow, 22.f, FootY + 4.f, 8, RoWinText)->SetText(FText::FromString(TEXT("Mostrar equip.")));
	EquipBtnStatus = AddButtonEx(EquipWindow, RoWinArt(TEXT("btn_e_ability_a")), RoWinArt(TEXT("btn_e_ability_b")), RoWinArt(TEXT("btn_e_ability_b")), 168.f, FootY + 1.f);
	EquipBtnSwitch = AddButtonEx(EquipWindow, RoWinArt(TEXT("btn_e_change_a")), RoWinArt(TEXT("btn_e_change_b")), RoWinArt(TEXT("btn_e_change_c")), 193.f, FootY + 1.f);
	EquipBtnOffAll = AddButtonEx(EquipWindow, RoWinArt(TEXT("btn_e_off_a")), RoWinArt(TEXT("btn_e_off_b")), RoWinArt(TEXT("btn_e_off_c")), 236.f, FootY + 1.f);
	UpdateEquipMount();
}

void UMythRoHUDWidget::UpdateEquipPreview()
{
	// Redesenha so quando o sprite do jogador recarregou (classe, cabelo, acessorios, arma, escudo ou capa mudaram)
	if (!EquipPreview || !EquipWindow || !EquipWindow->IsVisible())
	{
		return;
	}
	const APawn* Pawn = GetOwningPlayerPawn();
	const UMythRoSpriteComponent* Sprite = Pawn ? Pawn->FindComponentByClass<UMythRoSpriteComponent>() : nullptr;
	const bool bShow = Sprite && EquipTab <= 1;
	if (bShow && Sprite->GetLoadVersion() != EquipPreviewVersion)
	{
		EquipPreviewVersion = Sprite->GetLoadVersion();
		EquipPreviewTex = Sprite->RenderPortrait(static_cast<int32>(EqPreviewW), 130, 114);		EquipPreview->SetBrush(RoWinBrush(EquipPreviewTex, FVector2D(EqPreviewW, 130.f) * UIScale));
	}
	EquipPreview->SetVisibility(bShow && EquipPreviewTex ? ESlateVisibility::HitTestInvisible : ESlateVisibility::Collapsed);
}

void UMythRoHUDWidget::BuildSkillsWindow()
{
	// Habilidades (Alt+S) em arvore, como a janela do cliente moderno: abas por etapa da classe, grade de 7 colunas
	// na posicao do skilltreeview.lub, linhas ate os pre-requisitos e o nivel/"+" em cada skill
	const float W = RoTreeLeft + 24.f + FMythRoSkillTree::Columns * RoTreeCellW;
	const float H = RoTreeTop + RoTreeRows * RoTreeCellH + 40.f;
	SkillsWindow = MakeRoPanel(TEXT("Habilidades"), FVector2D(W, H), FVector2D(340.f, 110.f));
	for (int32 t = 0; t < 4; ++t)
	{
		// Aba vertical (print 13: "1a", "2a", "3a", "Etc" na lateral esquerda)
		UImage* Back = AddColor(SkillsWindow, FLinearColor::Transparent, 3.f, RoTreeTop + 4.f + t * 54.f, 22.f, 50.f);
		Back->SetVisibility(ESlateVisibility::Collapsed);
		SkillTabBacks.Add(Back);
		UTextBlock* TabText = AddText(SkillsWindow, 6.f, RoTreeTop + 4.f + t * 54.f + 17.f, 8, RoWinText);
		TabText->SetJustification(ETextJustify::Left);
		SkillTabTexts.Add(TabText);
	}
	// Linhas primeiro (ficam por baixo dos icones)
	for (int32 i = 0; i < 90; ++i)
	{
		UImage* Line = AddColor(SkillsWindow, FLinearColor(0.62f, 0.66f, 0.74f, 1.f), 0.f, 0.f, 1.f, 1.f);
		Line->SetVisibility(ESlateVisibility::Collapsed);
		SkillLinePool.Add(Line);
	}
	for (int32 i = 0; i < RoTreeRows * FMythRoSkillTree::Columns; ++i)
	{
		const float X = RoTreeLeft + (i % FMythRoSkillTree::Columns) * RoTreeCellW;
		const float Y = RoTreeTop + (i / FMythRoSkillTree::Columns) * RoTreeCellH;
		// Celula do print 13: nome em cima (cortado com reticencias), moldura 32 x 32 com o icone, nivel embaixo e o "+" ao lado
		UTextBlock* Title = AddText(SkillsWindow, X, Y, 7, RoWinLabel);
		Title->SetJustification(ETextJustify::Center);
		if (UCanvasPanelSlot* TS = Cast<UCanvasPanelSlot>(Title->Slot)) { TS->SetAutoSize(false); TS->SetSize(FVector2D(RoTreeCellW, 12.f) * UIScale); }
		SkillTitles.Add(Title);
		UImage* Frame = AddImage(SkillsWindow, FMythRoUITextures::Get(RoWinArt(TEXT("itemwin_mid"))), X + 18.f, Y + 13.f, 32.f, 32.f);
		Frame->SetVisibility(ESlateVisibility::Collapsed);
		SkillCellFrames.Add(Frame);
		UImage* Icon = AddImage(SkillsWindow, nullptr, X + 22.f, Y + 17.f, 24.f, 24.f);
		Icon->SetVisibility(ESlateVisibility::Collapsed);
		SkillIcons.Add(Icon);
		UTextBlock* LevelText = AddText(SkillsWindow, X, Y + 45.f, 7, RoWinText); // nivel "3/10"
		LevelText->SetJustification(ETextJustify::Center);
		if (UCanvasPanelSlot* LS = Cast<UCanvasPanelSlot>(LevelText->Slot)) { LS->SetAutoSize(false); LS->SetSize(FVector2D(RoTreeCellW, 12.f) * UIScale); }
		SkillNames.Add(LevelText);
		SkillLevels.Add(nullptr);
		UImage* Up = AddImage(SkillsWindow, FMythRoUITextures::Get(RoWinArt(TEXT("lv_up_on"))), X + 50.f, Y + 22.f, 14.f, 14.f);
		Up->SetVisibility(ESlateVisibility::Collapsed);
		SkillUpButtons.Add(Up);
	}
	const float BarY = H - 32.f;
	SkillBottomBar.Add(AddImage(SkillsWindow, FMythRoUITextures::Get(RoWinArt(TEXT("skill_botbar_left"))), 0.f, BarY, 12.f, 32.f));
	SkillBottomBar.Add(AddImage(SkillsWindow, FMythRoUITextures::Get(RoWinArt(TEXT("skill_botbar_mid"))), 12.f, BarY, W - 24.f, 32.f));
	SkillBottomBar.Add(AddImage(SkillsWindow, FMythRoUITextures::Get(RoWinArt(TEXT("skill_botbar_right"))), W - 12.f, BarY, 12.f, 32.f));
	SkillPointsText = AddText(SkillsWindow, 88.f, BarY + 9.f, 8, RoWinText); // depois do personagem, como no print 13
	SkillBottomBar.Add(SkillPointsText);
	SkillApplyImg = AddButton(SkillsWindow, TEXT("ok"), W - 96.f, BarY + 6.f);
	SkillResetImg = AddButton(SkillsWindow, TEXT("reset"), W - 52.f, BarY + 6.f);
	SkillBottomBar.Add(SkillApplyImg);
	SkillBottomBar.Add(SkillResetImg);
	// Personagem no canto de baixo, por cima da barra (print 13)
	SkillPortrait = AddImage(SkillsWindow, nullptr, 2.f, BarY - SkillPortraitLift, 80.f, 100.f);
	SkillPortrait->SetVisibility(ESlateVisibility::Collapsed);
	SkillBottomBar.Add(SkillPortrait);
}

void UMythRoHUDWidget::ResizeSkillTree(int32 Rows)
{
	// A janela cresce/encolhe com a arvore da aba (Super Aprendiz usa 9 linhas; a 1a classe, 2 ou 3)
	if (!SkillsWindow)
	{
		return;
	}
	const float W = RoTreeLeft + 24.f + FMythRoSkillTree::Columns * RoTreeCellW;
	const float H = RoTreeTop + Rows * RoTreeCellH + 40.f;
	const float BarY = H - 32.f;
	if (UCanvasPanelSlot* S = Cast<UCanvasPanelSlot>(SkillsWindow->Slot))
	{
		S->SetSize(FVector2D(W, H) * UIScale);
	}
	if (UCanvasPanelSlot* S = SkillsWindow->GetChildrenCount() > 0 ? Cast<UCanvasPanelSlot>(SkillsWindow->GetChildAt(0)->Slot) : nullptr)
	{
		S->SetSize(FVector2D(W, H - 17.f) * UIScale); // fundo branco do MakeRoPanel
	}
	for (UWidget* Part : SkillBottomBar)
	{
		if (UCanvasPanelSlot* S = Part ? Cast<UCanvasPanelSlot>(Part->Slot) : nullptr)
		{
			const FVector2D Pos = S->GetPosition();
			const float Y = Part == SkillPointsText ? BarY + 9.f : (Part == SkillApplyImg || Part == SkillResetImg ? BarY + 6.f : (Part == SkillPortrait ? BarY - SkillPortraitLift : BarY));
			S->SetPosition(FVector2D(Pos.X, Y * UIScale)); // Pos.X ja esta na escala da interface
		}
	}
}

// ------------------------------------------------------------------------------------------------ abrir / fechar

void UMythRoHUDWidget::ToggleWindow(EMythRoWindow Window)
{
	UCanvasPanel* Target = nullptr;
	switch (Window)
	{
	case EMythRoWindow::Status:  Target = StatusWindow; break;
	case EMythRoWindow::Items:   Target = ItemsWindow; break;
	case EMythRoWindow::Equip:   Target = EquipWindow; break;
	case EMythRoWindow::Skills:  Target = SkillsWindow; break;
	case EMythRoWindow::Basic:   Target = BasicWindow; break;
	case EMythRoWindow::Minimap: Target = MinimapWindow; break;
	case EMythRoWindow::Party:   Target = PartyWindow; break;
	case EMythRoWindow::Emotions: Target = EmotionWindow; break;
	case EMythRoWindow::Macros:  Target = MacroWindow; break;
	case EMythRoWindow::Friends: Target = FriendsWindow; break;
	case EMythRoWindow::ChatRoom: OpenChatRoomWindow(); return;
	case EMythRoWindow::Bank: OpenBankWindow(); return;
	case EMythRoWindow::Achievements: OpenAchievementWindow(); return;
	case EMythRoWindow::EquipSwitch: Target = EquipSwitchWindow; break;
	case EMythRoWindow::Mail: OpenMailWindow(); return;
	case EMythRoWindow::WorldMap: ToggleWorldMap(); return;
	case EMythRoWindow::Guild: ToggleGuildWindow(); return;
	case EMythRoWindow::Pet: TogglePetWindow(); return;
	case EMythRoWindow::Homun: ToggleHomunWindow(); return;
	case EMythRoWindow::Navi:
		if (NaviWindow && NaviWindow->IsVisible()) { NaviWindow->SetVisibility(ESlateVisibility::Collapsed); } else { OpenNaviWindow(); }
		return;
	case EMythRoWindow::Quests: Target = QuestWindow; break;
	case EMythRoWindow::Shortcuts: Target = ShortcutWindow; break;
	}
	if (!Target)
	{
		return;
	}
	const bool bOpen = Target->GetVisibility() == ESlateVisibility::Collapsed;
	Target->SetVisibility(bOpen ? ESlateVisibility::SelfHitTestInvisible : ESlateVisibility::Collapsed);
	OpenOrder.Remove(Target);
	if (bOpen)
	{
		OpenOrder.Add(Target);
		if (UCanvasPanelSlot* S = Cast<UCanvasPanelSlot>(Target->Slot))
		{
			S->SetZOrder(++TopZOrder); // a janela aberta vem para a frente
		}
		WindowRefreshTimer = 0.f;
		RefreshWindows();
	}
}

bool UMythRoHUDWidget::CloseTopWindow()
{
	while (OpenOrder.Num() > 0)
	{
		UCanvasPanel* Win = OpenOrder.Pop().Get();
		// Informacoes basicas e minimapa ficam abertos por padrao: o Esc fecha so as janelas de Alt+A/E/Q/S
		if (Win && Win != BasicWindow && Win != MinimapWindow && Win->GetVisibility() != ESlateVisibility::Collapsed)
		{
			if (Win == ShopWindow || Win == DealWindow)
			{
				CloseShop(true);
				return true;
			}
			if (Win == StorageWindow)
			{
				CloseStorage(true);
				return true;
			}
			if (Win == PartyInviteWindow)
			{
				AnswerPartyInvite(false);
				return true;
			}
			if (Win == GuildInviteWindow)
			{
				AnswerGuildInvite(false);
				return true;
			}
			if (Win == TradeAskWindow)
			{
				if (URagnarokNetworkSubsystem* TNet = GetNet()) { TNet->SendTradeReply(false); }
				Win->SetVisibility(ESlateVisibility::Collapsed);
				OpenOrder.Remove(Win);
				return true;
			}
			if (Win == TradeWindow)
			{
				if (URagnarokNetworkSubsystem* TNet = GetNet()) { TNet->SendTradeCancel(); }
				return true;
			}
			Win->SetVisibility(ESlateVisibility::Collapsed);
			return true;
		}
	}
	return false;
}

// ------------------------------------------------------------------------------------------------ dados do servidor

void UMythRoHUDWidget::RefreshWindows()
{
	RefreshHotbar();
	if (StatusWindow && StatusWindow->IsVisible()) { RefreshStatusWindow(); }
	if (ItemsWindow && ItemsWindow->IsVisible()) { RefreshItemsWindow(); }
	if (EquipWindow && EquipWindow->IsVisible()) { RefreshEquipWindow(); }
	if (SkillsWindow && SkillsWindow->IsVisible()) { RefreshSkillsWindow(); }
	if (StorageWindow && StorageWindow->IsVisible()) { RefreshStorage(); }
	RefreshCart(); // tambem mostra ou esconde o botao do carrinho no Equipamento
	RefreshVending();
	if (PartyWindow && PartyWindow->IsVisible()) { RefreshParty(); }
	if (GuildWindow && GuildWindow->IsVisible()) { RefreshGuild(); }
	if (PetWindow && PetWindow->IsVisible()) { RefreshPet(); }
	if (HomunWindow && HomunWindow->IsVisible()) { RefreshHomun(); }
	if (EmotionWindow && EmotionWindow->IsVisible()) { RefreshEmotionList(); }
	if (FriendsWindow && FriendsWindow->IsVisible()) { RefreshFriends(); }
	if (QuestWindow && QuestWindow->IsVisible()) { RefreshQuests(); }
	UpdateSettingsTabs();
	// Icone do correio com o aviso "novo" quando ha carta nao lida (arte bt_mail_new)
	if (const URagnarokNetworkSubsystem* BadgeNet = GetNet())
	{
		const bool bUnread = BadgeNet->HasUnreadMail();
		if (bUnread != bMailBadge)
		{
			bMailBadge = bUnread;
			for (int32 i = 0; i < MenuIcons.Num(); ++i)
			{
				if (MenuIconNames[i] == TEXT("mail"))
				{
					MenuIcons[i]->SetBrush(RoWinBrush(FMythRoUITextures::Get(bUnread ? TEXT("menu_icon/bt_mail_new.bmp") : TEXT("menu_icon/bt_mail.bmp")), FVector2D(32.f, 34.f) * UIScale));
				}
			}
		}
	}
	if (BankWindow && BankWindow->IsVisible()) { RefreshBank(); }
	if (AchievementWindow && AchievementWindow->IsVisible()) { RefreshAchievements(); }
	if (EquipSwitchWindow && EquipSwitchWindow->IsVisible()) { RefreshEquipSwitch(); }
}

void UMythRoHUDWidget::RefreshStatusWindow()
{
	const URagnarokNetworkSubsystem* Net = GetNet();
	const FRagnarokPlayerStats Ps = Net ? Net->GetPlayerStats() : FRagnarokPlayerStats();
	const int32 Base[6] = { Ps.Str, Ps.Agi, Ps.Vit, Ps.Int, Ps.Dex, Ps.Luk };
	const int32 Need[6] = { Ps.StrNeed, Ps.AgiNeed, Ps.VitNeed, Ps.IntNeed, Ps.DexNeed, Ps.LukNeed };
	for (int32 i = 0; i < 6 && i < StatTexts.Num(); ++i)
	{
		StatTexts[i]->SetText(Ps.StatBonus[i] != 0 ? FText::FromString(FString::Printf(TEXT("%d+%d"), Base[i], Ps.StatBonus[i])) : FText::AsNumber(Base[i]));
		StatCostTexts[i]->SetText(FText::AsNumber(Need[i])); // custo do proximo ponto como o servidor manda (Renovacao: sobe mais rapido acima de 100)
		// A seta so aparece com pontos livres (o custo real e validado pelo servidor)
		StatUpArrows[i]->SetVisibility(Ps.StatusPoints > 0 ? ESlateVisibility::Visible : ESlateVisibility::Collapsed);
	}
	auto Plus = [](int32 A, int32 B) { return FText::FromString(FString::Printf(TEXT("%d + %d"), A, B)); };
	if (DerivedTexts.Num() >= 10)
	{
		DerivedTexts[0]->SetText(Plus(Ps.Atk, Ps.Atk2));
		DerivedTexts[1]->SetText(Plus(Ps.Def, Ps.Def2));
		DerivedTexts[2]->SetText(Plus(Ps.MatkMin, Ps.MatkMax)); // Renovacao: MATK de atributo + MATK de equipamento (pc_leftside/rightside_matk)
		DerivedTexts[3]->SetText(Plus(Ps.Mdef, Ps.Mdef2));
		DerivedTexts[4]->SetText(FText::AsNumber(Ps.Hit));
		DerivedTexts[5]->SetText(Plus(Ps.Flee, Ps.Flee2));
		DerivedTexts[6]->SetText(FText::AsNumber(Ps.Critical));
		DerivedTexts[7]->SetText(FText::AsNumber(Ps.Aspd > 0 ? (2000 - Ps.Aspd) / 10 : 0)); // o servidor manda o amotion (ms x 10); o cliente mostra 200 - amotion / 10
		DerivedTexts[8]->SetText(FText::AsNumber(Ps.StatusPoints));
		DerivedTexts[9]->SetText(FText::FromString(TEXT("-")));
	}
}

void UMythRoHUDWidget::RefreshItemsWindow()
{
	const URagnarokNetworkSubsystem* Net = GetNet();
	if (ItemTabImage)
	{
		ItemTabImage->SetBrush(RoWinBrush(FMythRoUITextures::Get(RoWinArt(*FString::Printf(TEXT("tab_pri_%02d"), ItemTab + 1))), FVector2D(21.f, 134.f) * UIScale));
	}
	int32 Cell = 0;
	int32 Total = 0;
	if (Net)
	{
		for (const FRagnarokInventoryItem& Item : Net->GetInventoryItems())
		{
			if ((Item.bIsEquipped && Item.Type != 10) || Item.Amount <= 0)
			{
				continue; // equipados aparecem na janela de equipamento (a municao equipada continua em Diversos, como no cliente)
			}
			++Total;
			if (RoItemTabOf(Item) != ItemTab || Cell >= ItemCells.Num())
			{
				continue;
			}
			UTexture2D* Icon = FMythRoClientTables::ItemIcon(Item.ItemID);
			ItemIcons[Cell]->SetBrush(RoWinBrush(Icon, FVector2D(24.f, 24.f) * UIScale));
			ItemIcons[Cell]->SetVisibility(Icon ? ESlateVisibility::HitTestInvisible : ESlateVisibility::Collapsed);
			ItemAmounts[Cell]->SetText(Item.Amount > 1 ? FText::AsNumber(Item.Amount) : FText::GetEmpty());
			ItemCellIndex[Cell] = Item.Index;
			++Cell;
		}
	}
	for (; Cell < ItemCells.Num(); ++Cell)
	{
		ItemIcons[Cell]->SetVisibility(ESlateVisibility::Collapsed);
		ItemAmounts[Cell]->SetText(FText::GetEmpty());
		ItemCellIndex[Cell] = -1;
	}
	if (ItemCountText)
	{
		ItemCountText->SetText(FText::FromString(FString::Printf(TEXT("%d/100"), Total)));
	}
	// Linha de informacao: nome do item sob o cursor (como a dica do cliente original)
	FString Info;
	if (!Net || Net->GetNetworkState() != ERagnarokNetworkState::ConnectedMap)
	{
		Info = TEXT("Inventario vem do servidor (desconectado)");
	}
	for (int32 i = 0; i < ItemCells.Num() && Net; ++i)
	{
		if (ItemCellIndex[i] >= 0 && ItemCells[i]->IsHovered())
		{
			for (const FRagnarokInventoryItem& Item : Net->GetInventoryItems())
			{
				if (Item.Index == ItemCellIndex[i])
				{
					Info = FMythRoClientTables::ItemName(Item.ItemID);
					if (Item.Refine > 0)
					{
						Info = FString::Printf(TEXT("+%d %s"), Item.Refine, *Info);
					}
					if (Item.Amount > 1)
					{
						Info += FString::Printf(TEXT(": %d un."), Item.Amount);
					}
					break;
				}
			}
		}
	}
	if (ItemInfoText)
	{
		ItemInfoText->SetText(FText::FromString(Info));
	}
}

void UMythRoHUDWidget::RefreshEquipWindow()
{
	UpdateEquipMount();
	auto EquipMask = [this](int32 SlotNo) -> uint32
	{
		// Especial: esquerda costume topo/meio/baixo, capa de visual e municao; direita arma, armadura, escudo, botas e acessorio sombrios
		static const uint32 Special[10] = { 0x0400, 0x0800, 0x1000, 0x2000, 0x8000, 0x20000, 0x10000, 0x40000, 0x80000, 0x100000 };
		return EquipTab == 1 ? Special[SlotNo] : RoEquipSlots[SlotNo].Mask;
	};
	// Abas 0 Geral e 1 Especial mostram encaixes; 2 Titulo e 3 Fonte de dano so um aviso (o servidor nao manda esses dados ainda)
	const bool bSlots = EquipTab <= 1;
	if (EquipBack)
	{
		EquipBack->SetBrush(RoWinBrush(FMythRoUITextures::Get(RoWinArt(EquipTab == 1 ? TEXT("equipwin_special") : TEXT("equipwin_bg"))), FVector2D(280.f, 130.f) * UIScale));
		EquipBack->SetVisibility(bSlots ? ESlateVisibility::HitTestInvisible : ESlateVisibility::Collapsed);
	}
	for (int32 t = 0; t < EquipTabBoxes.Num(); ++t)
	{
		EquipTabBoxes[t]->SetColorAndOpacity(t == EquipTab ? FLinearColor::White : FLinearColor(0.84f, 0.86f, 0.91f, 1.f));
	}
	// Aba Titulo: "(sem titulo)" + os titulos das conquistas com o premio recebido; o atual fica marcado
	EquipTitleIds.Reset();
	if (EquipTab == 2)
	{
		EquipTitleIds.Add(0);
		if (const URagnarokNetworkSubsystem* TitleNet = GetNet())
		{
			for (const FRoAchievement& A : TitleNet->GetAchievements())
			{
				if (const int32* Title = RoTitles().Achievements.Find(A.Id); Title && A.bRewardTaken)
				{
					EquipTitleIds.AddUnique(*Title);
				}
			}
		}
	}
	const int32 CurrentTitle = GetNet() ? GetNet()->GetCurrentTitle() : 0;
	for (int32 i = 0; i < EquipTitleTexts.Num(); ++i)
	{
		const bool bRow = EquipTitleIds.IsValidIndex(i);
		EquipTitleHits[i]->SetVisibility(bRow ? ESlateVisibility::Visible : ESlateVisibility::Collapsed);
		EquipTitleTexts[i]->SetVisibility(bRow ? ESlateVisibility::HitTestInvisible : ESlateVisibility::Collapsed);
		if (bRow)
		{
			const int32 Id = EquipTitleIds[i];
			const FString* Name = RoTitles().Names.Find(Id);
			EquipTitleTexts[i]->SetText(FText::FromString(Id == 0 ? FString(TEXT("(sem título)")) : (Name ? *Name : FString::Printf(TEXT("Título %d"), Id))));
			EquipTitleHits[i]->SetColorAndOpacity(Id == CurrentTitle ? FLinearColor(0.78f, 0.86f, 0.98f, 1.f) : FLinearColor(0.f, 0.f, 0.f, 0.03f));
		}
	}
	if (EquipTabNote)
	{
		// Titulo: aviso so quando nao ha nenhum titulo ganho (fica abaixo da linha "sem titulo")
		const bool bNote = EquipTab == 3 || (EquipTab == 2 && EquipTitleIds.Num() <= 1);
		EquipTabNote->SetText(FText::FromString(EquipTab == 2 ? TEXT("Nenhum título conquistado ainda.") : TEXT("Fonte de dano: padrão.")));
		EquipTabNote->SetVisibility(bNote ? ESlateVisibility::HitTestInvisible : ESlateVisibility::Collapsed);
	}
	if (EquipShowCheck)
	{
		const URagnarokNetworkSubsystem* CheckNet = GetNet();
		EquipShowCheck->SetBrush(RoWinBrush(FMythRoUITextures::Get(CheckNet && CheckNet->IsShowEquip() ? TEXT("checkbox_1.bmp") : TEXT("checkbox_0.bmp")), FVector2D(10.f, 10.f) * UIScale));
	}
	UpdateEquipPreview();
	const URagnarokNetworkSubsystem* Net = GetNet();
	for (int32 i = 0; i < UE_ARRAY_COUNT(RoEquipSlots) && i < EquipNames.Num(); ++i)
	{
		const FRagnarokInventoryItem* Found = nullptr;
		if (Net && bSlots)
		{
			for (const FRagnarokInventoryItem& Item : Net->GetInventoryItems())
			{
				if (Item.bIsEquipped && (static_cast<uint32>(Item.EquipLocation) & EquipMask(i)) != 0)
				{
					Found = &Item;
					break;
				}
			}
		}
		UImage* Icon = EquipIcons[2 * i];
		UImage* Back = EquipIcons[2 * i + 1];
		if (Found)
		{
			UTexture2D* Tex = FMythRoClientTables::ItemIcon(Found->ItemID);
			Icon->SetBrush(RoWinBrush(Tex, FVector2D(24.f, 24.f) * UIScale));
			Icon->SetVisibility(Tex ? ESlateVisibility::Visible : ESlateVisibility::Collapsed);
			Back->SetVisibility(ESlateVisibility::Collapsed);
			FString Name = FMythRoClientTables::ItemName(Found->ItemID);
			if (Found->Refine > 0)
			{
				Name = FString::Printf(TEXT("+%d %s"), Found->Refine, *Name);
			}
			EquipNames[i]->SetText(FText::FromString(Name)); // quebra em 2 linhas de 80 (o resto e cortado), como no print
			EquipCellIndex[i] = Found->Index;
		}
		else
		{
			Icon->SetVisibility(ESlateVisibility::Collapsed);
			Back->SetVisibility(ESlateVisibility::Collapsed);
			EquipNames[i]->SetText(FText::GetEmpty());
			EquipCellIndex[i] = -1;
		}
	}
}

void UMythRoHUDWidget::RefreshSkillsWindow()
{
	const int32 Cells = SkillIcons.Num();
	SkillRowID.Init(0, Cells);
	SkillRowLevel.Init(0, Cells);
	const URagnarokNetworkSubsystem* Net = GetNet();
	const int32 FreePoints = Net ? Net->GetPlayerStats().SkillPoints : 0;
	const int32 Points = FMath::Max(0, FreePoints - PendingSkillTotal());
	const int32 Job = Net ? Net->GetPlayerStats().JobClass : 0;

	// Abas do LATAM: 1a (Aprendiz + 1a classe), 2a (2a classe + transclasse), 3a e Etc; sem escolha, abre na ultima com skills
	{
		const TArray<int32> Chain = FMythRoSkillTree::JobChain(Job);
		SkillTabJobs.Init(TArray<int32>(), 4);
		for (int32 c = 0; c < Chain.Num(); ++c)
		{
			SkillTabJobs[c < 2 ? 0 : 1].Add(Chain[c]); // Aprendiz e 1a classe na 1a aba; 2a classe e transclasse na 2a
		}
	}
	if (!SkillTabJobs.IsValidIndex(SkillTab))
	{
		SkillTab = SkillTabJobs[1].Num() > 0 ? 1 : 0;
	}
	for (int32 t = 0; t < SkillTabTexts.Num(); ++t)
	{
		static const TCHAR* TabLabels[4] = { TEXT("1a"), TEXT("2a"), TEXT("3a"), TEXT("Etc") };
		SkillTabTexts[t]->SetText(FText::FromString(TabLabels[FMath::Clamp(t, 0, 3)]));
		SkillTabTexts[t]->SetColorAndOpacity(FSlateColor(t == SkillTab ? RoWinText : RoWinLabel));
		SkillTabBacks[t]->SetVisibility(ESlateVisibility::Visible);
		SkillTabBacks[t]->SetColorAndOpacity(t == SkillTab ? FLinearColor(1.f, 1.f, 1.f, 0.85f) : FLinearColor(0.85f, 0.88f, 0.94f, 0.6f));
	}

	// Nivel de cada skill que o servidor mandou (ZC_SKILLINFO_LIST): ausente = ainda bloqueada
	TMap<int32, const FRagnarokSkillInfo*> Known;
	if (Net)
	{
		for (const FRagnarokSkillInfo& S : Net->GetSkillList())
		{
			Known.Add(S.SkillID, &S);
		}
	}
	// Grade da aba: as grades das classes da aba somadas (a classe mais avancada vence se uma casa repetir)
	TMap<int32, int32> TabGrid;
	if (SkillTabJobs.IsValidIndex(SkillTab))
	{
		for (const int32 TabJob : SkillTabJobs[SkillTab])
		{
			if (const TMap<int32, int32>* JobGrid = FMythRoSkillTree::Grid(TabJob))
			{
				TabGrid.Append(*JobGrid);
			}
		}
	}
	const TMap<int32, int32>* Grid = &TabGrid;
	TMap<int32, int32> CellOfSkill;
	int32 LastCell = 0;
	if (Grid)
	{
		for (const TPair<int32, int32>& Cell : *Grid)
		{
			LastCell = FMath::Max(LastCell, Cell.Key);
		}
	}
	ResizeSkillTree(FMath::Clamp(LastCell / FMythRoSkillTree::Columns + 1, 6, RoTreeRows));
	if (SkillPortrait)
	{
		// Mesmo desenho da previa do Equipamento, virado para a esquerda (direcao 1), redesenhado quando o visual muda
		const APawn* Pawn = GetOwningPlayerPawn();
		const UMythRoSpriteComponent* Sprite = Pawn ? Pawn->FindComponentByClass<UMythRoSpriteComponent>() : nullptr;
		if (Sprite && Sprite->GetLoadVersion() != SkillPortraitVersion)
		{
			SkillPortraitVersion = Sprite->GetLoadVersion();
			SkillPortraitTex = Sprite->RenderPortrait(80, 100, 94, 1);
			SkillPortrait->SetBrush(RoWinBrush(SkillPortraitTex, FVector2D(80.f, 100.f) * UIScale));
		}
		SkillPortrait->SetVisibility(SkillPortraitTex ? ESlateVisibility::HitTestInvisible : ESlateVisibility::Collapsed);
	}
	for (int32 i = 0; i < Cells; ++i)
	{
		const int32* SkillID = Grid ? Grid->Find(i) : nullptr;
		const bool bUsed = SkillID && *SkillID > 0;
		// Celula sem skill: so a moldura apagada (o print mostra as casas vazias pontilhadas)
		SkillCellFrames[i]->SetVisibility(ESlateVisibility::HitTestInvisible);
		SkillCellFrames[i]->SetColorAndOpacity(bUsed ? FLinearColor::White : FLinearColor(1.f, 1.f, 1.f, 0.35f));
		SkillTitles[i]->SetText(FText::GetEmpty());
		if (!bUsed)
		{
			SkillIcons[i]->SetVisibility(ESlateVisibility::Collapsed);
			SkillUpButtons[i]->SetVisibility(ESlateVisibility::Collapsed);
			SkillNames[i]->SetText(FText::GetEmpty());
			continue;
		}
		CellOfSkill.Add(*SkillID, i);
		{
			const FMythTreeSkill* TitleTree = FMythRoSkillTree::Skill(*SkillID);
			FString Title = TitleTree ? TitleTree->Name : FMythRoClientTables::SkillName(*SkillID);
			SkillTitles[i]->SetText(FText::FromString(Title.Len() > 12 ? Title.Left(11) + TEXT("…") : Title));
		}
		const FRagnarokSkillInfo* Info = Known.FindRef(*SkillID);
		const FMythTreeSkill* Tree = FMythRoSkillTree::Skill(*SkillID);
		const int32 Level = Info ? Info->Level : 0;
		SkillRowID[i] = *SkillID;
		const int32 Sel = Level > 0 ? FMath::Clamp(SkillSelLevel.Contains(*SkillID) ? SkillSelLevel[*SkillID] : Level, 1, Level) : 0;
		SkillRowLevel[i] = Sel;
		UTexture2D* Icon = FMythRoClientTables::SkillIcon(*SkillID);
		SkillIcons[i]->SetBrush(RoWinBrush(Icon, FVector2D(24.f, 24.f) * UIScale));
		SkillIcons[i]->SetVisibility(Icon ? ESlateVisibility::Visible : ESlateVisibility::Collapsed); // Visible: arrastavel
		// Aprendida: cor cheia; liberada (o servidor mandou com nivel 0): um pouco apagada; bloqueada: cinza
		SkillIcons[i]->SetColorAndOpacity(Level > 0 ? FLinearColor::White : (Info ? FLinearColor(1.f, 1.f, 1.f, 0.7f) : FLinearColor(0.35f, 0.35f, 0.38f, 0.55f)));
		const int32 Pending = PendingSkillUps.FindRef(*SkillID);
		SkillNames[i]->SetText(FText::FromString(Pending > 0 ? FString::Printf(TEXT("%d+%d"), Level, Pending) : (Level > 1 ? FString::Printf(TEXT("< %d/%d >"), Sel, Level) : FString::Printf(TEXT("%d/%d"), Level, Tree ? Tree->MaxLevel : Level))));
		SkillNames[i]->SetColorAndOpacity(FSlateColor(Level > 0 ? RoWinText : RoWinLabel));
		SkillUpButtons[i]->SetVisibility(Points > 0 && Info && Info->bUpgradable ? ESlateVisibility::Visible : ESlateVisibility::Collapsed);
	}

	// Linhas ate os pre-requisitos da mesma aba: desce do pre-requisito, anda na horizontal no vao acima da skill e
	// desce ate ela. Azul quando o pre-requisito ja esta no nivel pedido
	auto Place = [this](UImage* Img, float X, float Y, float W, float H, const FLinearColor& Color)
	{
		if (UCanvasPanelSlot* S = Cast<UCanvasPanelSlot>(Img->Slot))
		{
			S->SetPosition(FVector2D(X, Y) * UIScale);
			S->SetSize(FVector2D(FMath::Max(W, 2.f), FMath::Max(H, 2.f)) * UIScale);
		}
		Img->SetColorAndOpacity(Color);
		Img->SetVisibility(ESlateVisibility::HitTestInvisible);
	};
	int32 Used = 0;
	for (const TPair<int32, int32>& Pair : CellOfSkill)
	{
		const FMythTreeSkill* Tree = FMythRoSkillTree::Skill(Pair.Key);
		if (!Tree)
		{
			continue;
		}
		for (const FIntPoint& Need : Tree->Need)
		{
			const int32* From = CellOfSkill.Find(Need.X);
			if (!From || Used + 3 > SkillLinePool.Num())
			{
				continue;
			}
			const FRagnarokSkillInfo* Have = Known.FindRef(Need.X);
			const FLinearColor Color = Have && Have->Level >= Need.Y ? FLinearColor(0.35f, 0.55f, 0.9f, 1.f) : FLinearColor(0.68f, 0.71f, 0.78f, 1.f);
			const float FX = RoTreeLeft + (*From % FMythRoSkillTree::Columns) * RoTreeCellW + 34.f;
			const float FY = RoTreeTop + (*From / FMythRoSkillTree::Columns) * RoTreeCellH;
			const float TX = RoTreeLeft + (Pair.Value % FMythRoSkillTree::Columns) * RoTreeCellW + 34.f;
			const float TY = RoTreeTop + (Pair.Value / FMythRoSkillTree::Columns) * RoTreeCellH;
			if (FMath::IsNearlyEqual(FY, TY))
			{
				Place(SkillLinePool[Used++], FMath::Min(FX, TX), FY + 29.f, FMath::Abs(TX - FX), 2.f, Color); // mesma linha
				continue;
			}
			const float MidY = TY + 8.f; // vao entre o nivel da linha de cima e o nome da skill
			Place(SkillLinePool[Used++], FX - 1.f, FY + 45.f, 2.f, MidY - (FY + 45.f), Color);
			Place(SkillLinePool[Used++], FMath::Min(FX, TX) - 1.f, MidY, FMath::Abs(TX - FX) + 2.f, 2.f, Color);
			Place(SkillLinePool[Used++], TX - 1.f, MidY, 2.f, TY - MidY, Color);
		}
	}
	for (int32 i = Used; i < SkillLinePool.Num(); ++i)
	{
		SkillLinePool[i]->SetVisibility(ESlateVisibility::Collapsed);
	}

	if (SkillPointsText)
	{
		// Barra de baixo: pontos e a skill selecionada (clique) com nome, nivel e SP
		FString Line = FString::Printf(TEXT("Pontos: %d"), Points);
		if (SkillRowID.IsValidIndex(SkillScroll) && SkillRowID[SkillScroll] > 0)
		{
			const int32 Id = SkillRowID[SkillScroll];
			const FMythTreeSkill* Tree = FMythRoSkillTree::Skill(Id);
			const FRagnarokSkillInfo* Info = Known.FindRef(Id);
			FString Name = Tree ? Tree->Name : FMythRoClientTables::SkillName(Id);
			Line += FString::Printf(TEXT("   %s %d/%d"), *Name.Left(13), SkillRowLevel[SkillScroll], Tree ? Tree->MaxLevel : 0); // curto: os botoes OK e Cancelar ocupam o canto
			if (!Info && Tree && Tree->Need.Num() > 0)
			{
				Line += TEXT(" (req: ");
				for (int32 n = 0; n < Tree->Need.Num(); ++n)
				{
					const FMythTreeSkill* Req = FMythRoSkillTree::Skill(Tree->Need[n].X);
					Line += FString::Printf(TEXT("%s%s %d"), n ? TEXT(", ") : TEXT(""), Req ? *Req->Name : TEXT("?"), Tree->Need[n].Y);
				}
				Line += TEXT(")");
			}
		}
		if (PendingSkillTotal() > 0)
		{
			Line += FString::Printf(TEXT("  (pendentes: %d)"), PendingSkillTotal());
		}
		SkillPointsText->SetText(FText::FromString(Line));
	}
}

// ------------------------------------------------------------------------------------------------ cliques

bool UMythRoHUDWidget::HandleWindowClick(const FVector2D& ScreenPos)
{
	if (HandleFriendPartySwitch(ScreenPos))
	{
		return true;
	}
	if (EquipWindow && EquipWindow->IsVisible())
	{
		for (int32 t = 0; t < EquipTabBoxes.Num(); ++t)
		{
			if (IsUnder(EquipTabBoxes[t], ScreenPos))
			{
				EquipTab = t;
				RefreshEquipWindow();
				return true;
			}
		}
	}
	if (HandleMinimapClick(ScreenPos))
	{
		return true;
	}
	if (HandleChatClick(ScreenPos))
	{
		return true;
	}
	if (HandleHotbarClick(ScreenPos))
	{
		return true;
	}
	if (HandleShopClick(ScreenPos))
	{
		return true;
	}
	if (HandleWorldMapClick(ScreenPos))
	{
		return true;
	}
	if (HandleVendingClick(ScreenPos))
	{
		return true;
	}
	if (HandleSelectListClick(ScreenPos, false))
	{
		return true;
	}
	if (HandleCartClick(ScreenPos))
	{
		return true;
	}
	if (HandleStorageClick(ScreenPos))
	{
		return true;
	}
	if (HandlePartyClick(ScreenPos))
	{
		return true;
	}
	if (HandleGuildClick(ScreenPos))
	{
		return true;
	}
	if (HandlePetClick(ScreenPos))
	{
		return true;
	}
	if (HandleHomunClick(ScreenPos))
	{
		return true;
	}
	if (HandleOptionsClick(ScreenPos))
	{
		return true;
	}
	if (HandleTradeClick(ScreenPos))
	{
		return true;
	}
	if (HandleFriendsClick(ScreenPos))
	{
		return true;
	}
	if (HandleChatRoomClick(ScreenPos))
	{
		return true;
	}
	if (HandleQuestClick(ScreenPos))
	{
		return true;
	}
	if (HandleBankClick(ScreenPos))
	{
		return true;
	}
	if (HandleAchievementClick(ScreenPos))
	{
		return true;
	}
	if (HandleEquipSwitchClick(ScreenPos))
	{
		return true;
	}
	if (HandleNaviClick(ScreenPos))
	{
		return true;
	}
	if (HandleMailClick(ScreenPos))
	{
		return true;
	}
	if (HandleEmotionClick(ScreenPos))
	{
		return true;
	}
	if (HandleGameMenuClick(ScreenPos) || HandleBasicMenuClick(ScreenPos))
	{
		return true;
	}
	if (HandleMinimizeClick(ScreenPos))
	{
		return true;
	}
	if (EquipWindow && EquipWindow->IsVisible())
	{
		URagnarokNetworkSubsystem* EqNet = GetNet();
		if (IsUnder(EquipMountBtn, ScreenPos))
		{
			if (EqNet) { EqNet->SendRemoveOption(); } // o servidor tira o falcao ou o Peco e avisa pelo estado
			return true;
		}
		for (int32 i = 0; i < EquipTitleIds.Num() && i < EquipTitleHits.Num(); ++i)
		{
			if (IsUnder(EquipTitleHits[i], ScreenPos))
			{
				if (EqNet) { EqNet->SendChangeTitle(EquipTitleIds[i]); } // a marca muda quando o servidor confirma
				return true;
			}
		}
		if (IsUnder(EquipShowCheck, ScreenPos))
		{
			if (EqNet) { EqNet->SendShowEquipConfig(!EqNet->IsShowEquip()); } // a caixa muda quando o servidor confirma (ZC_CONFIG)
			return true;
		}
		if (IsUnder(EquipBtnStatus, ScreenPos))
		{
			ToggleWindow(EMythRoWindow::Status);
			return true;
		}
		if (IsUnder(EquipBtnSwitch, ScreenPos))
		{
			ToggleWindow(EMythRoWindow::EquipSwitch);
			return true;
		}
		if (IsUnder(EquipBtnOffAll, ScreenPos))
		{
			// Tirar tudo: so os encaixes da aba aberta (Geral = equipamento, Especial = visual e sombrios), como o cliente
			if (EqNet) { EqNet->SendTakeOffAll(EquipTab == 1 ? 0x1F3C00u : 0x0003FFu); }
			return true;
		}
	}
	for (const TPair<TWeakObjectPtr<UImage>, TWeakObjectPtr<UCanvasPanel>>& Pair : CloseButtons)
	{
		UCanvasPanel* Win = Pair.Value.Get();
		if (Win && Win->IsVisible() && IsUnder(Pair.Key.Get(), ScreenPos))
		{
			if (Win == ShopWindow || Win == DealWindow)
			{
				CloseShop(true); // fechar a loja libera o NPC no servidor
				return true;
			}
			if (Win == StorageWindow)
			{
				CloseStorage(true);
				return true;
			}
			if (Win == PartyInviteWindow)
			{
				AnswerPartyInvite(false);
				return true;
			}
			if (Win == TradeAskWindow)
			{
				if (URagnarokNetworkSubsystem* TNet = GetNet()) { TNet->SendTradeReply(false); }
				Win->SetVisibility(ESlateVisibility::Collapsed);
				OpenOrder.Remove(Win);
				return true;
			}
			if (Win == TradeWindow)
			{
				if (URagnarokNetworkSubsystem* TNet = GetNet()) { TNet->SendTradeCancel(); }
				return true;
			}
			Win->SetVisibility(ESlateVisibility::Collapsed);
			OpenOrder.Remove(Win);
			return true;
		}
	}
	URagnarokNetworkSubsystem* Net = GetNet();
	if (StatusWindow && StatusWindow->IsVisible())
	{
		for (int32 i = 0; i < StatUpArrows.Num(); ++i)
		{
			if (IsUnder(StatUpArrows[i], ScreenPos))
			{
				if (Net)
				{
					Net->SendStatusUp(RoSpStr + i, 1); // CZ_STATUS_CHANGE: o servidor valida e devolve o novo valor
				}
				return true;
			}
		}
	}
	if (SkillsWindow && SkillsWindow->IsVisible())
	{
		for (int32 t = 0; t < SkillTabBacks.Num(); ++t)
		{
			if (SkillTabJobs.IsValidIndex(t) && IsUnder(SkillTabBacks[t], ScreenPos))
			{
				SkillTab = t; // aba de outra etapa da classe
				SkillScroll = -1;
				RefreshSkillsWindow();
				return true;
			}
		}
		for (int32 r = 0; r < SkillNames.Num() && r < SkillRowID.Num(); ++r)
		{
			// Setas "< n/max >": esquerda diminui, direita aumenta o nivel usado (cliente LATAM, print da arvore de skills)
			const FRagnarokSkillInfo* Learned = nullptr;
			if (Net)
			{
				for (const FRagnarokSkillInfo& S : Net->GetSkillList())
				{
					if (S.SkillID == SkillRowID[r]) { Learned = &S; break; }
				}
			}
			if (SkillRowID[r] != 0 && Learned && Learned->Level > 1 && IsUnder(SkillNames[r], ScreenPos))
			{
				const FGeometry& NGeo = SkillNames[r]->GetCachedGeometry();
				const bool bLeft = NGeo.AbsoluteToLocal(ScreenPos).X < NGeo.GetLocalSize().X * 0.5f;
				SkillSelLevel.Add(SkillRowID[r], FMath::Clamp(SkillRowLevel[r] + (bLeft ? -1 : 1), 1, Learned->Level));
				RefreshSkillsWindow();
				return true;
			}
		}
		for (int32 r = 0; r < SkillIcons.Num() && r < SkillRowID.Num(); ++r)
		{
			if (SkillRowID[r] != 0 && IsUnder(SkillIcons[r], ScreenPos) && !IsUnder(SkillUpButtons[r], ScreenPos))
			{
				SkillScroll = r; // selecionada: nome, nivel e requisitos na barra de baixo (o arrasto continua valendo)
				RefreshSkillsWindow();
				break;
			}
		}
		if (IsUnder(SkillApplyImg, ScreenPos) && Net)
		{
			for (const TPair<int32, int32>& P : PendingSkillUps)
			{
				for (int32 n = 0; n < P.Value; ++n)
				{
					Net->SendSkillUp(P.Key); // CZ_UPGRADE_SKILLLEVEL: o servidor valida e devolve a skill atualizada
				}
			}
			PendingSkillUps.Reset();
			RefreshSkillsWindow();
			return true;
		}
		if (IsUnder(SkillResetImg, ScreenPos))
		{
			PendingSkillUps.Reset();
			RefreshSkillsWindow();
			return true;
		}
		for (int32 r = 0; r < SkillUpButtons.Num() && r < SkillRowID.Num(); ++r)
		{
			if (SkillRowID[r] != 0 && SkillUpButtons[r]->IsVisible() && IsUnder(SkillUpButtons[r], ScreenPos))
			{
				PendingSkillUps.FindOrAdd(SkillRowID[r])++; // so vira pacote no OK
				RefreshSkillsWindow();
				return true;
			}
		}
	}
	if (ItemsWindow && ItemsWindow->IsVisible() && IsUnder(ItemTabImage, ScreenPos))
	{
		const FGeometry& Geo = ItemTabImage->GetCachedGeometry();
		const float LocalY = Geo.AbsoluteToLocal(ScreenPos).Y / FMath::Max(Geo.GetLocalSize().Y, 1.f);
		ItemTab = FMath::Clamp(FMath::FloorToInt(LocalY * 4.f), 0, 3);
		RefreshItemsWindow();
		return true;
	}
	return false;
}

FReply UMythRoHUDWidget::NativeOnMouseButtonDoubleClick(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent)
{
	const FVector2D Screen = InMouseEvent.GetScreenSpacePosition();
	if (HandleNpcClick(Screen, true))
	{
		return FReply::Handled();
	}
	if (HandleSelectListClick(Screen, true))
	{
		return FReply::Handled();
	}
	URagnarokNetworkSubsystem* Net = GetNet();
	if (Net && ItemsWindow && ItemsWindow->IsVisible())
	{
		for (int32 i = 0; i < ItemCells.Num(); ++i)
		{
			if (ItemCellIndex[i] < 0 || !IsUnder(ItemCells[i], Screen))
			{
				continue;
			}
			if (Net->IsTradeOpen())
			{
				// Com a troca aberta, o duplo clique oferece o item (Shift: a pilha toda)
				const bool bAll = GetOwningPlayer() && (GetOwningPlayer()->IsInputKeyDown(EKeys::LeftShift) || GetOwningPlayer()->IsInputKeyDown(EKeys::RightShift));
				for (const FRagnarokInventoryItem& Offered : Net->GetInventoryItems())
				{
					if (Offered.Index == ItemCellIndex[i] && !Offered.bIsEquipped)
					{
						TryTradeAddFromInventory(Offered.Index, bAll ? Offered.Amount : 1);
					}
				}
				return FReply::Handled();
			}
			// Duplo clique: usa o consumivel ou veste o equipamento (como no cliente original)
			for (const FRagnarokInventoryItem& Item : Net->GetInventoryItems())
			{
				if (Item.Index == ItemCellIndex[i])
				{
					if (RoItemTabOf(Item) == 1)
					{
						if (InMouseEvent.IsControlDown())
						{
							Net->SendEquipSwitchAdd(Item.Index, Item.EquipLocation); // Ctrl + duplo clique: vai para a troca rapida
						}
						else
						{
							Net->EquipItem(Item.Index, Item.EquipLocation);
						}
					}
					else if (Item.Type == 10)
					{
						if (Item.bIsEquipped) { Net->UnequipItem(Item.Index); } else { Net->EquipItem(Item.Index, 0x8000); } // municao: duplo clique equipa ou desequipa
					}
					else if (Item.Type == 6)
					{
						Net->SendCardListRequest(Item.Index); // carta: o servidor manda a lista de equipamentos que a aceitam
					}
					else if (RoItemTabOf(Item) == 0)
					{
						Net->UseItem(Item.Index);
					}
					break;
				}
			}
			return FReply::Handled();
		}
	}
	if (SkillsWindow && SkillsWindow->IsVisible())
	{
		for (int32 r = 0; r < SkillIcons.Num() && r < SkillRowID.Num(); ++r)
		{
			if (SkillRowID[r] != 0 && (IsUnder(SkillIcons[r], Screen) || IsUnder(SkillNames[r], Screen)))
			{
				if (AMythPlayerController* PC = Cast<AMythPlayerController>(GetOwningPlayer()))
				{
					PC->BeginSkill(SkillRowID[r], SkillRowLevel[r]); // duplo clique na skill: usa
				}
				return FReply::Handled();
			}
		}
	}
	if (Net && EquipWindow && EquipWindow->IsVisible())
	{
		if (EquipAmmoIndex >= 0 && IsUnder(EquipAmmoIcon, Screen))
		{
			Net->UnequipItem(EquipAmmoIndex); // duplo clique na municao a desequipa
			return FReply::Handled();
		}
		for (int32 i = 0; i < EquipCellIndex.Num(); ++i)
		{
			if (EquipCellIndex[i] >= 0 && IsUnder(EquipIcons[2 * i], Screen))
			{
				Net->UnequipItem(EquipCellIndex[i]); // duplo clique no encaixe tira o equipamento
				return FReply::Handled();
			}
		}
	}
	return Super::NativeOnMouseButtonDoubleClick(InGeometry, InMouseEvent);
}

FReply UMythRoHUDWidget::NativeOnMouseWheel(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent)
{
	if (HandleAchievementWheel(InMouseEvent.GetScreenSpacePosition(), InMouseEvent.GetWheelDelta()))
	{
		return FReply::Handled();
	}
	if (HandleQuestWheel(InMouseEvent.GetScreenSpacePosition(), InMouseEvent.GetWheelDelta()))
	{
		return FReply::Handled();
	}
	if (HandleGuildWheel(InMouseEvent.GetScreenSpacePosition(), InMouseEvent.GetWheelDelta()))
	{
		return FReply::Handled();
	}
	if (HandleNaviWheel(InMouseEvent.GetScreenSpacePosition(), InMouseEvent.GetWheelDelta()))
	{
		return FReply::Handled();
	}
	if (HandleChatWheel(InMouseEvent.GetScreenSpacePosition(), InMouseEvent.GetWheelDelta()))
	{
		return FReply::Handled();
	}
	if (HandleShopWheel(InMouseEvent.GetScreenSpacePosition(), InMouseEvent.GetWheelDelta()))
	{
		return FReply::Handled();
	}
	if (HandleSelectListWheel(InMouseEvent.GetScreenSpacePosition(), InMouseEvent.GetWheelDelta()))
	{
		return FReply::Handled();
	}
	if (HandleCartWheel(InMouseEvent.GetScreenSpacePosition(), InMouseEvent.GetWheelDelta()))
	{
		return FReply::Handled();
	}
	if (HandleStorageWheel(InMouseEvent.GetScreenSpacePosition(), InMouseEvent.GetWheelDelta()))
	{
		return FReply::Handled();
	}
	if (HandleOptionsWheel(InMouseEvent.GetScreenSpacePosition(), InMouseEvent.GetWheelDelta()))
	{
		return FReply::Handled();
	}
	// Roda do mouse sobre a arvore de habilidades: troca de aba (a grade inteira cabe na janela)
	if (SkillsWindow && SkillsWindow->IsVisible() && IsUnder(SkillsWindow, InMouseEvent.GetScreenSpacePosition()))
	{
		if (SkillTabJobs.Num() > 0)
		{
			SkillTab = FMath::Clamp(SkillTab + (InMouseEvent.GetWheelDelta() > 0.f ? -1 : 1), 0, SkillTabJobs.Num() - 1);
			SkillScroll = -1;
			RefreshSkillsWindow();
		}
		return FReply::Handled();
	}
	return Super::NativeOnMouseWheel(InGeometry, InMouseEvent);
}

bool UMythRoHUDWidget::HandleMinimizeClick(const FVector2D& ScreenPos)
{
	for (FRoMinimize& M : Minimizables)
	{
		UCanvasPanel* Win = M.Win.Get();
		if (!Win || !Win->IsVisible() || !IsUnder(M.Button.Get(), ScreenPos))
		{
			continue;
		}
		M.bMin = !M.bMin;
		if (UCanvasPanelSlot* S = Cast<UCanvasPanelSlot>(Win->Slot))
		{
			S->SetSize(FVector2D(M.FullSize.X, M.bMin ? 17.f : M.FullSize.Y) * UIScale);
		}
		if (M.bMin) { MinimizedWindows().Add(Win); } else { MinimizedWindows().Remove(Win); }
		return true;
	}
	return false;
}

void UMythRoHUDWidget::UpdateEquipMount()
{
	// Aba Geral: municao equipada (encaixe em cima do personagem) e o botao de soltar falcao / descer do Peco Peco, so se houver
	const URagnarokNetworkSubsystem* Net = GetNet();
	const bool bGeneral = EquipTab == 0;
	const FRagnarokInventoryItem* Ammo = nullptr;
	if (Net)
	{
		for (const FRagnarokInventoryItem& Item : Net->GetInventoryItems())
		{
			if (Item.bIsEquipped && Item.Type == 10)
			{
				Ammo = &Item;
				break;
			}
		}
	}
	EquipAmmoIndex = Ammo ? Ammo->Index : -1;
	if (EquipAmmoFrame) { EquipAmmoFrame->SetVisibility(bGeneral && Ammo ? ESlateVisibility::HitTestInvisible : ESlateVisibility::Collapsed); }
	if (EquipAmmoIcon)
	{
		UTexture2D* Tex = Ammo ? FMythRoClientTables::ItemIcon(Ammo->ItemID) : nullptr;
		if (Tex) { EquipAmmoIcon->SetBrush(RoWinBrush(Tex, FVector2D(24.f, 24.f) * UIScale)); }
		EquipAmmoIcon->SetVisibility(bGeneral && Tex ? ESlateVisibility::Visible : ESlateVisibility::Collapsed);
	}
	if (EquipAmmoName)
	{
		EquipAmmoName->SetText(FText::FromString(bGeneral && Ammo ? FMythRoClientTables::ItemName(Ammo->ItemID).Left(14) : FString()));
	}
	const int32 Option = Net ? Net->GetSelfOption() : 0;
	const bool bFalcon = (Option & 0x10) != 0;
	const bool bRiding = (Option & 0x20) != 0;
	// Botao "off" (btn_off) so com falcao ou montaria, como no print 08
	if (EquipMountBtn)
	{
		EquipMountBtn->SetVisibility(bGeneral && (bFalcon || bRiding) ? ESlateVisibility::Visible : ESlateVisibility::Collapsed);
	}
}

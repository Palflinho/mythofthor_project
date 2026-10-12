// Navegacao (/navi, Alt+N), como a janela do LATAM (print image-1791661011151): linha de busca com voltar, filtro (Todos, Mapas,
// Monstros), texto e lupa; a esquerda "Resultado[n]"; a direita o nome e a imagem do mapa, a lista do mapa e dos monstros dele
// (ou a rota, depois de "Procurar") e os botoes Compartilhar, Procurar e Mover.
// Dados do cliente LATAM: mapas e ligacoes em Config/navi.json (tools/gen_navi.py); monstros por mapa em Config/navi_mobs.json
// (tools/gen_navi_mobs.py, nomes/nivel/tamanho do banco do rAthena). A rota e a de menos portais; "Mover" anda sozinho por ela.

#include "UI/MythRoHUDWidget.h"
#include "UI/MythRoUITextures.h"
#include "World/MythCellMoverComponent.h"
#include "GameFramework/Pawn.h"
#include "Player/MythPlayerController.h"

#include "Blueprint/WidgetTree.h"
#include "Components/CanvasPanel.h"
#include "Components/CanvasPanelSlot.h"
#include "Components/EditableTextBox.h"
#include "Components/Image.h"
#include "Components/TextBlock.h"
#include "Dom/JsonObject.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "Serialization/JsonReader.h"
#include "Serialization/JsonSerializer.h"

namespace
{
	const FLinearColor NvText(0.12f, 0.12f, 0.12f);
	const FLinearColor NvLink(0.12f, 0.3f, 0.65f);
	const FLinearColor NvGray(0.55f, 0.55f, 0.55f);
	const FLinearColor NvSystem(0.55f, 0.95f, 0.55f);
	const FLinearColor NvLine(0.78f, 0.8f, 0.86f, 1.f);
	constexpr float NvW = 286.f;
	constexpr float NvH = 300.f;
	constexpr float NvRowH = 16.f;
	constexpr float NvLeftX = 4.f;
	constexpr float NvLeftW = 134.f;
	constexpr float NvRightX = 144.f;
	constexpr float NvRightW = 138.f;
	constexpr float NvListY = 62.f;          // primeira linha da lista da esquerda
	constexpr int32 NvResults = 14;          // linhas visiveis da esquerda
	constexpr float NvMapListY = 164.f;      // primeira linha da lista da direita
	constexpr int32 NvMapRows = 5;           // linhas visiveis da direita
	FSlateBrush NvBrush(UTexture2D* Tex, const FVector2D& Size)
	{
		FSlateBrush B;
		B.SetResourceObject(Tex);
		B.ImageSize = Size;
		B.DrawAs = ESlateBrushDrawType::Image;
		return B;
	}
	FString NvArtS(const TCHAR* Name) { return FString::Printf(TEXT("navigation_interface3/%s.bmp"), Name); }
	const TCHAR* NvFilterNames[3] = { TEXT("Todos"), TEXT("Mapas"), TEXT("Monstros") };
	const TCHAR* NvSizeNames[3] = { TEXT("pequeno"), TEXT("médio"), TEXT("grande") };

	struct FNaviLink { FString To; int32 X = 0, Y = 0; };
	struct FNaviMob { FString Name; int32 Level = 1; int32 Size = 0; };
	struct FNaviData
	{
		TMap<FString, FString> Names;
		TMap<FString, TArray<FNaviLink>> Links;
		TMap<FString, TArray<FNaviMob>> Mobs;
		bool bLoaded = false;
	};

	FNaviData& Navi()
	{
		static FNaviData Data;
		if (!Data.bLoaded)
		{
			Data.bLoaded = true;
			FString Text;
			TSharedPtr<FJsonObject> Root;
			if (FFileHelper::LoadFileToString(Text, *FPaths::Combine(FPaths::ProjectConfigDir(), TEXT("navi.json")))
				&& FJsonSerializer::Deserialize(TJsonReaderFactory<>::Create(Text), Root) && Root.IsValid())
			{
				if (const TSharedPtr<FJsonObject>* Maps; Root->TryGetObjectField(TEXT("maps"), Maps))
				{
					for (const TPair<FString, TSharedPtr<FJsonValue>>& P : (*Maps)->Values)
					{
						Data.Names.Add(P.Key, P.Value->AsString());
					}
				}
				const TArray<TSharedPtr<FJsonValue>>* Links;
				if (Root->TryGetArrayField(TEXT("links"), Links))
				{
					for (const TSharedPtr<FJsonValue>& V : *Links)
					{
						const TArray<TSharedPtr<FJsonValue>>& L = V->AsArray();
						if (L.Num() >= 6)
						{
							FNaviLink Link;
							Link.To = L[3]->AsString();
							Link.X = static_cast<int32>(L[1]->AsNumber());
							Link.Y = static_cast<int32>(L[2]->AsNumber());
							Data.Links.FindOrAdd(L[0]->AsString()).Add(Link);
						}
					}
				}
			}
			TSharedPtr<FJsonObject> MobRoot;
			if (FFileHelper::LoadFileToString(Text, *FPaths::Combine(FPaths::ProjectConfigDir(), TEXT("navi_mobs.json")))
				&& FJsonSerializer::Deserialize(TJsonReaderFactory<>::Create(Text), MobRoot) && MobRoot.IsValid())
			{
				if (const TSharedPtr<FJsonObject>* Maps; MobRoot->TryGetObjectField(TEXT("mapas"), Maps))
				{
					for (const TPair<FString, TSharedPtr<FJsonValue>>& P : (*Maps)->Values)
					{
						TArray<FNaviMob>& List = Data.Mobs.FindOrAdd(P.Key);
						for (const TSharedPtr<FJsonValue>& E : P.Value->AsArray())
						{
							const TArray<TSharedPtr<FJsonValue>>& A = E->AsArray();
							if (A.Num() >= 3)
							{
								List.Add({ A[0]->AsString(), static_cast<int32>(A[1]->AsNumber()), FMath::Clamp(static_cast<int32>(A[2]->AsNumber()), 0, 2) });
							}
						}
					}
				}
			}
			else
			{
				UE_LOG(LogTemp, Warning, TEXT("MythNavi: Config/navi_mobs.json ausente (tools/gen_navi_mobs.py)"));
			}
		}
		return Data;
	}

	FString NaviLabelShort(const FString& Id)
	{
		const FString* Name = Navi().Names.Find(Id);
		const FString Label = Name && !Name->IsEmpty() ? *Name : Id;
		return Label.Len() > 24 ? Label.Left(23) + TEXT("…") : Label;
	}

	/** Corta o texto para caber numa linha da lista (fonte 7 da interface, ~5 px por letra). */
	FString NaviFit(const FString& S, float Width)
	{
		const int32 Max = FMath::Max(4, FMath::FloorToInt(Width / 5.2f));
		return S.Len() > Max ? S.Left(Max - 1) + TEXT("…") : S;
	}
}

void UMythRoHUDWidget::BuildNaviWindow()
{
	NaviWindow = MakeRoPanel(TEXT("Navegação"), FVector2D(NvW, NvH), FVector2D(-120.f, -40.f));
	// Linha de busca (y 21): voltar, filtro com a seta da lista, campo de texto e lupa
	NaviBackBtn = AddButtonEx(NaviWindow, NvArtS(TEXT("bt_back")), NvArtS(TEXT("bt_back_on")), NvArtS(TEXT("bt_back_on")), 4.f, 21.f);
	NaviFilterBtn = AddImage(NaviWindow, FMythRoUITextures::Get(NvArtS(TEXT("bg_combo"))), 28.f, 21.f, 48.f, 18.f);
	NaviFilterBtn->SetVisibility(ESlateVisibility::Visible);
	NaviFilterText = AddText(NaviWindow, 31.f, 23.f, 7, NvText);
	NaviFilterText->SetText(FText::FromString(NvFilterNames[0]));
	AddImage(NaviWindow, FMythRoUITextures::Get(NvArtS(TEXT("dropdown_off"))), 76.f, 21.f, 18.f, 18.f);
	AddImage(NaviWindow, FMythRoUITextures::Get(NvArtS(TEXT("bg_search"))), 98.f, 21.f, 132.f, 18.f);
	NaviBox = WidgetTree->ConstructWidget<UEditableTextBox>();
	NaviBox->OnTextCommitted.AddDynamic(this, &UMythRoHUDWidget::OnNaviCommitted);
	if (UCanvasPanelSlot* S = NaviWindow->AddChildToCanvas(NaviBox))
	{
		S->SetPosition(FVector2D(99.f, 21.f) * UIScale);
		S->SetSize(FVector2D(130.f, 18.f) * UIScale);
	}
	NaviSearchBtn = AddButtonEx(NaviWindow, NvArtS(TEXT("btn_searchbar_normal")), NvArtS(TEXT("btn_searchbar_over")), NvArtS(TEXT("btn_searchbar_press")), 232.f, 21.f);
	AddColor(NaviWindow, NvLine, 0.f, 42.f, NvW, 1.f);
	AddColor(NaviWindow, NvLine, 140.f, 42.f, 1.f, NvH - 42.f);

	// Esquerda: lupa + "Resultado[n]" e a lista (icone verde = mapa, vermelho = monstro)
	AddImage(NaviWindow, FMythRoUITextures::Get(NvArtS(TEXT("btn_searchbar_normal"))), NvLeftX, 44.f, 18.f, 13.f);
	NaviCount = AddText(NaviWindow, NvLeftX + 20.f, 45.f, 7, NvText);
	for (int32 i = 0; i < NvResults; ++i)
	{
		const float Y = NvListY + i * NvRowH;
		UImage* Icon = AddImage(NaviWindow, FMythRoUITextures::Get(NvArtS(TEXT("icon_list_map"))), NvLeftX + 2.f, Y + 2.f, 11.f, 11.f);
		Icon->SetVisibility(ESlateVisibility::Collapsed);
		NaviRowIcons.Add(Icon);
		NaviRows.Add(AddText(NaviWindow, NvLeftX + 16.f, Y, 7, NvLink));
	}

	// Direita: nome do mapa, imagem 100 x 100, lista com barra de rolagem e os botoes
	NaviTitle = AddText(NaviWindow, NvRightX, 45.f, 7, NvLink);
	NaviPreview = AddImage(NaviWindow, nullptr, NvRightX + (NvRightW - 100.f) * 0.5f, 59.f, 100.f, 100.f);
	NaviPreview->SetColorAndOpacity(FLinearColor::Transparent);
	NaviListSel = AddColor(NaviWindow, FLinearColor(0.78f, 0.86f, 0.98f, 1.f), NvRightX, NvMapListY, NvRightW - 10.f, NvRowH);
	NaviListSel->SetVisibility(ESlateVisibility::Collapsed);
	for (int32 i = 0; i < NvMapRows; ++i)
	{
		const float Y = NvMapListY + i * NvRowH;
		UImage* Icon = AddImage(NaviWindow, FMythRoUITextures::Get(NvArtS(TEXT("icon_list_mob"))), NvRightX + 1.f, Y + 2.f, 11.f, 11.f);
		Icon->SetVisibility(ESlateVisibility::Collapsed);
		NaviListIcons.Add(Icon);
		NaviListRows.Add(AddText(NaviWindow, NvRightX + 15.f, Y, 7, NvText));
	}
	NaviScrollUp = AddImage(NaviWindow, FMythRoUITextures::Get(NvArtS(TEXT("navi_scroll_up"))), NvRightX + NvRightW - 8.f, NvMapListY, 7.f, 7.f);
	NaviScrollUp->SetVisibility(ESlateVisibility::Visible);
	AddImage(NaviWindow, FMythRoUITextures::Get(NvArtS(TEXT("navi_bar_mid"))), NvRightX + NvRightW - 8.f, NvMapListY + 8.f, 7.f, NvMapRows * NvRowH - 16.f);
	NaviScrollDown = AddImage(NaviWindow, FMythRoUITextures::Get(NvArtS(TEXT("navi_scroll_down"))), NvRightX + NvRightW - 8.f, NvMapListY + NvMapRows * NvRowH - 7.f, 7.f, 7.f);
	NaviScrollDown->SetVisibility(ESlateVisibility::Visible);

	// Botoes do cliente (120 x 20, icone a esquerda): Compartilhar e Procurar lado a lado, Mover na largura toda
	const float BtnY = NvMapListY + NvMapRows * NvRowH + 6.f;
	auto NaviButton = [this](const TCHAR* Base, float X, float Y, float W, const TCHAR* Label, bool bKeepLabel) -> UImage*
	{
		UImage* Img = AddButtonEx(NaviWindow, NvArtS(*FString::Printf(TEXT("%s_normal"), Base)), NvArtS(*FString::Printf(TEXT("%s_over"), Base)), NvArtS(*FString::Printf(TEXT("%s_press"), Base)), X, Y);
		FSlateBrush Brush = Img->GetBrush();
		Brush.DrawAs = ESlateBrushDrawType::Box; // o icone (30 px da esquerda) nao achata quando o botao fica mais estreito que a arte
		Brush.Margin = FMargin(0.25f, 0.f, 0.1f, 0.f);
		Img->SetBrush(Brush);
		if (UCanvasPanelSlot* S = Cast<UCanvasPanelSlot>(Img->Slot)) { S->SetSize(FVector2D(W, 20.f) * UIScale); }
		UTextBlock* T = AddText(NaviWindow, X + 26.f, Y + 3.f, 7, NvText);
		T->SetText(FText::FromString(Label));
		if (bKeepLabel) { NaviAutoBtn = T; }
		return Img;
	};
	NaviShareBtn = NaviButton(TEXT("btn_share"), NvRightX, BtnY, 67.f, TEXT("Enviar"), false);
	NaviFindBtn = NaviButton(TEXT("btn_find"), NvRightX + 71.f, BtnY, 67.f, TEXT("Procurar"), false);
	NaviMoveBtn = NaviButton(TEXT("btn_getdirections"), NvRightX, BtnY + 24.f, NvRightW, TEXT("Mover"), true);

	// Lista suspensa do filtro (por cima de tudo, por isso criada por ultimo)
	NaviFilterMenuBack = AddRounded(NaviWindow, FLinearColor::White, FLinearColor(0.55f, 0.6f, 0.75f, 1.f), 28.f, 39.f, 66.f, 3 * 15.f + 4.f, 2.f, 2.f);
	NaviFilterMenuBack->SetVisibility(ESlateVisibility::Collapsed);
	for (int32 i = 0; i < 3; ++i)
	{
		UTextBlock* T = AddText(NaviWindow, 32.f, 41.f + i * 15.f, 7, NvText);
		T->SetText(FText::FromString(NvFilterNames[i]));
		if (UCanvasPanelSlot* S = Cast<UCanvasPanelSlot>(T->Slot)) { S->SetAutoSize(false); S->SetSize(FVector2D(60.f, 14.f) * UIScale); }
		T->SetVisibility(ESlateVisibility::Collapsed);
		NaviFilterItems.Add(T);
	}
	RefreshNaviLists();
}

void UMythRoHUDWidget::OpenNaviWindow(const FString& Query)
{
	if (!NaviWindow)
	{
		return;
	}
	NaviWindow->SetVisibility(ESlateVisibility::SelfHitTestInvisible);
	OpenOrder.Remove(NaviWindow);
	OpenOrder.Add(NaviWindow);
	if (!Query.IsEmpty())
	{
		NaviBox->SetText(FText::FromString(Query));
		SearchNavi(Query);
	}
	else
	{
		if (NaviSelected.IsEmpty() && !CurrentMapId.IsEmpty())
		{
			ShowNaviRoute(CurrentMapId.ToLower()); // abre mostrando o mapa onde voce esta, como o cliente
		}
		NaviBox->SetKeyboardFocus();
	}
}

void UMythRoHUDWidget::OnNaviCommitted(const FText& Text, ETextCommit::Type CommitMethod)
{
	if (CommitMethod == ETextCommit::OnEnter)
	{
		SearchNavi(Text.ToString());
	}
}

void UMythRoHUDWidget::SearchNavi(const FString& Query)
{
	NaviResults.Reset();
	NaviResultScroll = 0;
	const FString Needle = Query.TrimStartAndEnd().ToLower();
	const FNaviData& Data = Navi();
	if (!Needle.IsEmpty())
	{
		if (NaviFilter != 2)
		{
			TArray<FString> Maps;
			for (const TPair<FString, FString>& P : Data.Names)
			{
				if (!P.Value.IsEmpty() && (P.Value.ToLower().Contains(Needle) || P.Key.ToLower().Contains(Needle)))
				{
					Maps.Add(P.Key);
				}
			}
			// Nome mais curto primeiro (a cidade antes das masmorras e campos que a citam), depois pelo id
			Maps.Sort([&Data](const FString& A, const FString& B)
			{
				const FString& NA = Data.Names[A];
				const FString& NB = Data.Names[B];
				return NA.Len() != NB.Len() ? NA.Len() < NB.Len() : A < B;
			});
			for (const FString& Id : Maps)
			{
				NaviResults.Add({ 0, Id, FString::Printf(TEXT("%s (%s)"), *Data.Names[Id], *Id) });
			}
		}
		if (NaviFilter != 1)
		{
			// Monstro: um resultado por mapa onde ele nasce, "Nome (mapa)"
			for (const TPair<FString, TArray<FNaviMob>>& P : Data.Mobs)
			{
				for (const FNaviMob& M : P.Value)
				{
					if (M.Name.ToLower().Contains(Needle))
					{
						NaviResults.Add({ 1, P.Key, FString::Printf(TEXT("%s (%s)"), *M.Name, *P.Key) });
					}
				}
			}
		}
	}
	NaviCount->SetText(FText::FromString(Needle.IsEmpty() ? FString() : FString::Printf(TEXT("Resultado[%d]"), NaviResults.Num())));
	if (NaviResults.Num() == 1 || (NaviResults.Num() > 0 && NaviResults[0].Type == 0 && Data.Names[NaviResults[0].MapId].ToLower() == Needle))
	{
		ShowNaviRoute(NaviResults[0].MapId); // um resultado so, ou o nome exato do mapa: ja mostra
	}
	RefreshNaviLists();
}

void UMythRoHUDWidget::ShowNaviRoute(const FString& Destination)
{
	if (!NaviSelected.IsEmpty() && NaviSelected != Destination)
	{
		NaviHistory.Add(NaviSelected);
	}
	NaviSelected = Destination;
	NaviListScroll = 0;
	bNaviShowRoute = false;
	const FNaviData& Data = Navi();
	const FString* DestName = Data.Names.Find(Destination);
	NaviTitle->SetText(FText::FromString(FString::Printf(TEXT("[%s]"), *NaviFit(DestName && !DestName->IsEmpty() ? *DestName : Destination, NvRightW - 8.f))));
	if (UTexture2D* Tex = FMythRoUITextures::Get(FString::Printf(TEXT("map/%s.bmp"), *Destination)))
	{
		FSlateBrush B;
		B.SetResourceObject(Tex);
		B.ImageSize = FVector2D(100.f, 100.f) * UIScale;
		B.DrawAs = ESlateBrushDrawType::Image;
		NaviPreview->SetBrush(B);
		NaviPreview->SetColorAndOpacity(FLinearColor::White);
	}
	else
	{
		NaviPreview->SetColorAndOpacity(FLinearColor::Transparent);
	}

	// Rota pelos portais (busca em largura: o caminho com menos mapas no meio)
	NaviSteps.Reset();
	const FString From = CurrentMapId.ToLower();
	if (From != Destination)
	{
		struct FStep { FString Prev; int32 X = 0, Y = 0; };
		TMap<FString, FStep> Came;
		TArray<FString> Queue;
		Queue.Add(From);
		Came.Add(From, FStep());
		for (int32 Head = 0; Head < Queue.Num() && !Came.Contains(Destination); ++Head)
		{
			const TArray<FNaviLink>* Out = Data.Links.Find(Queue[Head]);
			if (!Out)
			{
				continue;
			}
			for (const FNaviLink& Link : *Out)
			{
				if (!Came.Contains(Link.To))
				{
					Came.Add(Link.To, FStep{ Queue[Head], Link.X, Link.Y });
					Queue.Add(Link.To);
				}
			}
		}
		if (Came.Contains(Destination))
		{
			for (FString Cur = Destination; Cur != From;)
			{
				const FStep& S = Came[Cur];
				NaviSteps.Insert(FNaviStep{ S.Prev, S.X, S.Y, Cur }, 0);
				Cur = S.Prev;
			}
		}
	}
	RefreshNaviLists();
}

void UMythRoHUDWidget::RefreshNaviLists()
{
	if (!NaviWindow)
	{
		return;
	}
	const FNaviData& Data = Navi();
	// Esquerda: resultados
	for (int32 i = 0; i < NvResults; ++i)
	{
		const int32 Index = NaviResultScroll + i;
		const bool bHas = NaviResults.IsValidIndex(Index);
		NaviRows[i]->SetText(FText::FromString(bHas ? NaviFit(NaviResults[Index].Label, NvLeftW - 18.f) : FString()));
		if (bHas)
		{
			NaviRowIcons[i]->SetBrush(NvBrush(FMythRoUITextures::Get(NvArtS(NaviResults[Index].Type == 1 ? TEXT("icon_list_mob") : TEXT("icon_list_map"))), FVector2D(11.f, 11.f) * UIScale));
		}
		NaviRowIcons[i]->SetVisibility(bHas ? ESlateVisibility::HitTestInvisible : ESlateVisibility::Collapsed);
	}

	// Direita: o mapa e os monstros dele ("Nome (tamanho)"), ou a rota (partida, cada portal e o destino)
	struct FLine { const TCHAR* Icon; FString Text; FLinearColor Color; };
	TArray<FLine> Lines;
	if (!NaviSelected.IsEmpty())
	{
		const FString* Name = Data.Names.Find(NaviSelected);
		const FString MapName = Name && !Name->IsEmpty() ? *Name : NaviSelected;
		if (bNaviShowRoute)
		{
			const FString From = CurrentMapId.ToLower();
			if (From == NaviSelected)
			{
				Lines.Add({ TEXT("icon_list_target"), TEXT("Você já está nesse mapa."), NvText });
			}
			else if (NaviSteps.IsEmpty())
			{
				Lines.Add({ TEXT("icon_list_target"), TEXT("Sem caminho conhecido daqui."), NvText });
			}
			else
			{
				Lines.Add({ TEXT("icon_list_start"), NaviLabelShort(From), NvText });
				for (const FNaviStep& S : NaviSteps)
				{
					Lines.Add({ TEXT("icon_list_riute"), FString::Printf(TEXT("(%d, %d) %s"), S.X, S.Y, *NaviLabelShort(S.To)), NvText });
				}
				Lines.Last().Icon = TEXT("icon_list_target");
			}
		}
		else
		{
			Lines.Add({ TEXT("icon_list_map"), MapName, NvText });
			if (const TArray<FNaviMob>* Mobs = Data.Mobs.Find(NaviSelected))
			{
				for (const FNaviMob& M : *Mobs)
				{
					Lines.Add({ TEXT("icon_list_mob"), FString::Printf(TEXT("%s (%s)"), *M.Name, NvSizeNames[M.Size]), NvText });
				}
			}
		}
	}
	NaviListScroll = FMath::Clamp(NaviListScroll, 0, FMath::Max(0, Lines.Num() - NvMapRows));
	for (int32 i = 0; i < NvMapRows; ++i)
	{
		const int32 Index = NaviListScroll + i;
		const bool bHas = Lines.IsValidIndex(Index);
		NaviListRows[i]->SetText(FText::FromString(bHas ? NaviFit(Lines[Index].Text, NvRightW - 26.f) : FString()));
		if (bHas)
		{
			NaviListRows[i]->SetColorAndOpacity(FSlateColor(Lines[Index].Color));
			NaviListIcons[i]->SetBrush(NvBrush(FMythRoUITextures::Get(NvArtS(Lines[Index].Icon)), FVector2D(11.f, 11.f) * UIScale));
		}
		NaviListIcons[i]->SetVisibility(bHas ? ESlateVisibility::HitTestInvisible : ESlateVisibility::Collapsed);
	}
	// A primeira linha (o proprio mapa) fica marcada, como no print
	NaviListSel->SetVisibility(!bNaviShowRoute && Lines.Num() > 0 && NaviListScroll == 0 ? ESlateVisibility::HitTestInvisible : ESlateVisibility::Collapsed);
	NaviFilterText->SetText(FText::FromString(NvFilterNames[NaviFilter]));
}

void UMythRoHUDWidget::StartNaviAuto()
{
	// Ir ate o mapa: o cliente anda ate o portal de cada etapa da rota (o servidor decide o caminho de celulas e o aviso de mapa)
	if (NaviSelected.IsEmpty())
	{
		return;
	}
	if (bNaviAuto)
	{
		bNaviAuto = false;
		AddChatLine(TEXT("Navegação automática cancelada."), NvSystem);
		return;
	}
	if (NaviSteps.IsEmpty())
	{
		ShowNaviRoute(NaviSelected);
	}
	if (NaviSteps.IsEmpty())
	{
		return;
	}
	bNaviAuto = true;
	NaviAutoDest = NaviSelected;
	NaviAutoTimer = 0.f;
	NaviProbe = 0;
	AddChatLine(FString::Printf(TEXT("Navegação automática: %s."), *NaviLabelShort(NaviAutoDest)), NvSystem);
}

void UMythRoHUDWidget::TickNaviAuto(float DeltaTime)
{
	if (NaviAutoBtn)
	{
		const FString Want = bNaviAuto ? TEXT("Parar") : TEXT("Mover");
		if (NaviAutoBtn->GetText().ToString() != Want)
		{
			NaviAutoBtn->SetText(FText::FromString(Want));
		}
	}
	if (!bNaviAuto)
	{
		return;
	}
	URagnarokNetworkSubsystem* Net = GetNet();
	if (!Net || Net->GetNetworkState() != ERagnarokNetworkState::ConnectedMap)
	{
		bNaviAuto = false;
		return;
	}
	if (CurrentMapId.ToLower() == NaviAutoDest)
	{
		bNaviAuto = false;
		AddChatLine(TEXT("Você chegou ao destino."), NvSystem);
		return;
	}
	NaviAutoTimer -= DeltaTime;
	if (NaviAutoTimer > 0.f)
	{
		return;
	}
	// A rota e refeita a partir do mapa atual, entao um portal usado (ou um desvio) nao quebra a navegacao
	if (NaviSteps.IsEmpty() || NaviSteps[0].From != CurrentMapId.ToLower())
	{
		NaviProbe = 0;
		ShowNaviRoute(NaviAutoDest);
		if (NaviSteps.IsEmpty())
		{
			bNaviAuto = false;
			AddChatLine(TEXT("Navegação automática: sem caminho a partir daqui."), NvSystem);
			return;
		}
	}
	// O servidor so aceita rotas curtas (MAX_WALKPATH): o cliente tira o caminho no .gat e manda um trecho de cada vez
	APawn* Pawn = GetOwningPlayerPawn();
	UMythCellMoverComponent* Mover = Pawn ? Pawn->FindComponentByClass<UMythCellMoverComponent>() : nullptr;
	if (!Mover)
	{
		return;
	}
	const FIntPoint Me = Mover->GetCurrentCell();
	const FIntPoint Goal(NaviSteps[0].X, NaviSteps[0].Y);
	const float Now = static_cast<float>(FPlatformTime::Seconds());
	const bool bNear = FMath::Max(FMath::Abs(Me.X - NaviLastTarget.X), FMath::Abs(Me.Y - NaviLastTarget.Y)) <= (bNaviBackoff ? 0 : 2);
	if (!bNear && Now - NaviLastSend < 3.f)
	{
		NaviAutoTimer = 0.3f;
		return; // ainda andando ate o trecho anterior
	}
	const FIntPoint Delta = Goal - Me;
	const int32 Dist = FMath::Max(FMath::Abs(Delta.X), FMath::Abs(Delta.Y));
	TArray<FIntPoint> Path;
	if (Dist <= 24)
	{
		Path = UMythCellMoverComponent::FindPath(GetWorld(), Me, Goal);
	}
	if (Path.Num() == 0)
	{
		// Longe demais para uma rota so (o servidor limita o trecho): escolhe um ponto intermediario na direcao do destino,
		// tentando distancias menores e desvios para os lados ate achar um caminho livre
		const FVector2D Dir = FVector2D(Delta.X, Delta.Y).GetSafeNormal();
		const FVector2D Side(-Dir.Y, Dir.X);
		for (int32 Reach : { 22, 16, 11, 7 })
		{
			for (int32 Off : { 0, 4, -4, 8, -8, 12, -12 })
			{
				const FVector2D P = FVector2D(Me.X, Me.Y) + Dir * FMath::Min(Reach, Dist) + Side * Off;
				const FIntPoint Cand(FMath::RoundToInt(P.X), FMath::RoundToInt(P.Y));
				if (Cand == Me || !UMythCellMoverComponent::IsWalkable(GetWorld(), Cand))
				{
					continue;
				}
				Path = UMythCellMoverComponent::FindPath(GetWorld(), Me, Cand);
				if (Path.Num() > 0)
				{
					break;
				}
			}
			if (Path.Num() > 0)
			{
				break;
			}
		}
	}
	FIntPoint Target = Goal;
	if (Path.Num() > 0)
	{
		Target = Path.Last();
	}
	if (Target == Goal && NaviLastTarget == Goal && Now - NaviLastSend >= 1.5f && FMath::Max(FMath::Abs(Me.X - Goal.X), FMath::Abs(Me.Y - Goal.Y)) <= 1)
	{
		// Chegou e o mapa nao mudou. O ponto da tabela do cliente fica na borda do portal do nosso servidor (a area dele pode
		// comecar 1 a 3 celulas adiante): tenta pisar em celulas ao redor ate o aviso de mapa; esgotadas, para (ex.: Kafra)
		static const FIntPoint Probes[] = { {0,-3}, {0,3}, {3,0}, {-3,0}, {0,-2}, {0,2}, {2,0}, {-2,0}, {3,-3}, {-3,-3}, {3,3}, {-3,3} };
		while (NaviProbe < UE_ARRAY_COUNT(Probes))
		{
			const FIntPoint Cell = Goal + Probes[NaviProbe++];
			if (UMythCellMoverComponent::IsWalkable(GetWorld(), Cell))
			{
				NaviLastSend = Now;
				NaviAutoTimer = 1.5f;
				if (AMythPlayerController* PC = Cast<AMythPlayerController>(GetOwningPlayer()))
				{
					PC->MythWalk(Cell.X, Cell.Y);
				}
				return;
			}
		}
		bNaviAuto = false;
		AddChatLine(FString::Printf(TEXT("Você chegou ao ponto da rota (%d, %d). Continue por aqui até %s."), Goal.X, Goal.Y, *NaviLabelShort(NaviSteps[0].To)), NvSystem);
		return;
	}
	if (Target == Goal && NaviLastTarget == Goal && Now - NaviLastSend < 4.f)
	{
		NaviAutoTimer = 0.5f;
		return; // ja mandou o passo final do portal: espera o aviso de mapa em vez de reiniciar a caminhada
	}
	if (Target == Goal) { bNaviBackoff = false; }
	NaviLastTarget = Target;
	NaviLastSend = Now;
	UE_LOG(LogTemp, Log, TEXT("MythNavi: de (%d,%d) para (%d,%d), trecho ate (%d,%d), caminho %d celulas"), Me.X, Me.Y, Goal.X, Goal.Y, Target.X, Target.Y, Path.Num());
	if (AMythPlayerController* PC = Cast<AMythPlayerController>(GetOwningPlayer()))
	{
		PC->MythWalk(Target.X, Target.Y); // anda como um clique no chao: o cliente move e manda o pedido ao servidor
	}
	NaviAutoTimer = 0.5f;
}

bool UMythRoHUDWidget::HandleNaviClick(const FVector2D& ScreenPos)
{
	if (!NaviWindow || !NaviWindow->IsVisible())
	{
		return false;
	}
	// Lista suspensa do filtro aberta: escolhe ou fecha
	if (NaviFilterMenuBack && NaviFilterMenuBack->IsVisible())
	{
		for (int32 i = 0; i < NaviFilterItems.Num(); ++i)
		{
			if (IsUnder(NaviFilterItems[i], ScreenPos))
			{
				NaviFilter = i;
				SearchNavi(NaviBox->GetText().ToString());
				break;
			}
		}
		NaviFilterMenuBack->SetVisibility(ESlateVisibility::Collapsed);
		for (UTextBlock* T : NaviFilterItems) { T->SetVisibility(ESlateVisibility::Collapsed); }
		RefreshNaviLists();
		return true;
	}
	if (IsUnder(NaviFilterBtn, ScreenPos))
	{
		NaviFilterMenuBack->SetVisibility(ESlateVisibility::Visible);
		for (UTextBlock* T : NaviFilterItems) { T->SetVisibility(ESlateVisibility::Visible); }
		return true;
	}
	if (IsUnder(NaviSearchBtn, ScreenPos)) { SearchNavi(NaviBox->GetText().ToString()); return true; }
	if (IsUnder(NaviBackBtn, ScreenPos))
	{
		// Volta ao mapa visto antes (sem empilhar de novo)
		if (NaviHistory.Num() > 0)
		{
			const FString Prev = NaviHistory.Pop();
			NaviSelected.Reset();
			ShowNaviRoute(Prev);
		}
		return true;
	}
	if (IsUnder(NaviScrollUp, ScreenPos)) { --NaviListScroll; RefreshNaviLists(); return true; }
	if (IsUnder(NaviScrollDown, ScreenPos)) { ++NaviListScroll; RefreshNaviLists(); return true; }
	if (IsUnder(NaviFindBtn, ScreenPos) && !NaviSelected.IsEmpty())
	{
		ShowNaviRoute(NaviSelected); // recalcula a partir de onde voce esta agora
		bNaviShowRoute = true;
		RefreshNaviLists();
		return true;
	}
	if (IsUnder(NaviMoveBtn, ScreenPos)) { StartNaviAuto(); return true; }
	if (IsUnder(NaviShareBtn, ScreenPos) && !NaviSelected.IsEmpty() && ChatInput)
	{
		// Enviar: deixa o comando no chat para voce mandar a quem quiser
		ChatInput->SetText(FText::FromString(FString::Printf(TEXT("/navi %s"), *NaviSelected)));
		FocusChat();
		return true;
	}
	for (int32 i = 0; i < NaviRows.Num(); ++i)
	{
		const int32 Index = NaviResultScroll + i;
		if (NaviResults.IsValidIndex(Index) && IsUnder(NaviRows[i], ScreenPos))
		{
			ShowNaviRoute(NaviResults[Index].MapId);
			return true;
		}
	}
	return false;
}

bool UMythRoHUDWidget::HandleNaviWheel(const FVector2D& ScreenPos, float Delta)
{
	if (!NaviWindow || !NaviWindow->IsVisible() || !IsUnder(NaviWindow, ScreenPos))
	{
		return false;
	}
	const int32 Step = Delta > 0.f ? -1 : 1;
	const FGeometry& Geo = NaviWindow->GetCachedGeometry();
	const float LocalX = Geo.AbsoluteToLocal(ScreenPos).X / FMath::Max(UIScale, 0.01f);
	if (LocalX < 140.f)
	{
		NaviResultScroll = FMath::Clamp(NaviResultScroll + Step * 3, 0, FMath::Max(0, NaviResults.Num() - NvResults));
	}
	else
	{
		NaviListScroll += Step;
	}
	RefreshNaviLists();
	return true;
}

FString UMythRoHUDWidget::NaviDisplayName(const FString& MapId)
{
	const FString* Name = Navi().Names.Find(MapId.ToLower());
	return Name && !Name->IsEmpty() ? *Name : MapId;
}

// Navegacao (/navi, Alt+N): procura um mapa pelo nome e mostra o caminho pelos portais a partir de onde voce esta.
// As ligacoes entre mapas e os nomes em portugues vem do cliente LATAM (Config/navi.json, tools/gen_navi.py); o caminho e
// o mais curto em numero de portais. Mapas que o servidor nao tem ainda aparecem na lista mas sem rota.

#include "UI/MythRoHUDWidget.h"
#include "UI/MythRoUITextures.h"

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
	constexpr float NvW = 292.f;
	constexpr float NvRowH = 17.f;
	constexpr int32 NvResults = 12;
	constexpr int32 NvRouteLines = 7;

	struct FNaviLink { FString To; int32 X = 0, Y = 0; };
	struct FNaviData
	{
		TMap<FString, FString> Names;
		TMap<FString, TArray<FNaviLink>> Links;
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
		}
		return Data;
	}

	FString MapLabel(const FString& Id)
	{
		const FString* Name = Navi().Names.Find(Id);
		return Name && !Name->IsEmpty() ? FString::Printf(TEXT("%s (%s)"), **Name, *Id) : Id;
	}

	FString NaviLabelShort(const FString& Id)
	{
		const FString* Name = Navi().Names.Find(Id);
		const FString Label = Name && !Name->IsEmpty() ? *Name : Id;
		return Label.Len() > 24 ? Label.Left(23) + TEXT("…") : Label;
	}
}

void UMythRoHUDWidget::BuildNaviWindow()
{
	// Navegacao (print do /navi): a esquerda a lista de resultados com a busca em cima; a direita o mapa escolhido, o nome e a rota
	// pelos portais; botoes Procurar e Rota do cliente (navigation_interface3)
	const float H = 17.f + 330.f;
	NaviWindow = MakeRoPanel(TEXT("Navegação"), FVector2D(NvW, H), FVector2D(-120.f, -40.f));
	AddImage(NaviWindow, FMythRoUITextures::Get(TEXT("navigation_interface3/bg_search.bmp")), 36.f, 22.f);
	NaviBox = WidgetTree->ConstructWidget<UEditableTextBox>();
	NaviBox->OnTextCommitted.AddDynamic(this, &UMythRoHUDWidget::OnNaviCommitted);
	if (UCanvasPanelSlot* S = NaviWindow->AddChildToCanvas(NaviBox))
	{
		S->SetPosition(FVector2D(38.f, 22.f) * UIScale);
		S->SetSize(FVector2D(110.f, 18.f) * UIScale);
	}
	NaviFind = AddButtonHit(NaviWindow, TEXT("find"), 150.f, 21.f);
	NaviCount = AddText(NaviWindow, 8.f, 46.f, 7, NvGray);
	for (int32 i = 0; i < NvResults; ++i)
	{
		const float Y = 62.f + i * NvRowH;
		AddImage(NaviWindow, FMythRoUITextures::Get(TEXT("navigation_interface3/icon_list_map.bmp")), 8.f, Y + 3.f, 11.f, 11.f);
		NaviRows.Add(AddText(NaviWindow, 22.f, Y, 7, NvLink));
	}
	NaviTitle = AddText(NaviWindow, 8.f, 62.f + NvResults * NvRowH + 6.f, 8, NvLink);
	NaviPreview = AddImage(NaviWindow, nullptr, 170.f, 56.f, 108.f, 108.f);
	NaviPreview->SetColorAndOpacity(FLinearColor::Transparent);
	for (int32 i = 0; i < NvRouteLines; ++i)
	{
		NaviRoute.Add(AddText(NaviWindow, 8.f, 62.f + NvResults * NvRowH + 28.f + i * NvRowH, 7, NvText));
	}
	NaviGo = AddButtonHit(NaviWindow, TEXT("view"), 170.f, 172.f);
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
	NaviMatches.Reset();
	const FString Needle = Query.TrimStartAndEnd().ToLower();
	if (!Needle.IsEmpty())
	{
		for (const TPair<FString, FString>& P : Navi().Names)
		{
			if (!P.Value.IsEmpty() && (P.Value.ToLower().Contains(Needle) || P.Key.ToLower() == Needle))
			{
				NaviMatches.Add(P.Key);
			}
		}
		// Nome mais curto primeiro (a cidade antes das masmorras e campos que a citam), depois pelo id
		NaviMatches.Sort([](const FString& A, const FString& B)
		{
			const FString& NA = Navi().Names[A];
			const FString& NB = Navi().Names[B];
			return NA.Len() != NB.Len() ? NA.Len() < NB.Len() : A < B;
		});
	}
	for (int32 i = 0; i < NvResults; ++i)
	{
		NaviRows[i]->SetText(FText::FromString(NaviMatches.IsValidIndex(i) ? NaviLabelShort(NaviMatches[i]) : FString()));
	}
	NaviCount->SetText(FText::FromString(Needle.IsEmpty() ? FString() : FString::Printf(TEXT("Resultados [%d]"), NaviMatches.Num())));
	for (UTextBlock* L : NaviRoute) { L->SetText(FText::GetEmpty()); }
	if (NaviMatches.Num() == 1 || (NaviMatches.Num() > 0 && Navi().Names[NaviMatches[0]].ToLower() == Needle))
	{
		ShowNaviRoute(NaviMatches[0]); // um resultado so, ou o nome exato: ja mostra o caminho
	}
}

void UMythRoHUDWidget::ShowNaviRoute(const FString& Destination)
{
	NaviSelected = Destination;
	if (const FString* DestName = Navi().Names.Find(Destination))
	{
		NaviTitle->SetText(FText::FromString(FString::Printf(TEXT("[%s]"), **DestName)));
	}
	if (UTexture2D* Tex = FMythRoUITextures::Get(FString::Printf(TEXT("map/%s.bmp"), *Destination)))
	{
		FSlateBrush B;
		B.SetResourceObject(Tex);
		B.ImageSize = FVector2D(108.f, 108.f) * UIScale;
		B.DrawAs = ESlateBrushDrawType::Image;
		NaviPreview->SetBrush(B);
		NaviPreview->SetColorAndOpacity(FLinearColor::White);
	}
	else
	{
		NaviPreview->SetColorAndOpacity(FLinearColor::Transparent);
	}
	for (UTextBlock* L : NaviRoute) { L->SetText(FText::GetEmpty()); }
	const FNaviData& Data = Navi();
	const FString From = CurrentMapId.ToLower();
	if (From == Destination)
	{
		NaviRoute[0]->SetText(FText::FromString(TEXT("Você já está nesse mapa.")));
		return;
	}
	// Busca em largura pelos portais: o caminho com menos mapas no meio
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
				FStep S;
				S.Prev = Queue[Head];
				S.X = Link.X;
				S.Y = Link.Y;
				Came.Add(Link.To, S);
				Queue.Add(Link.To);
			}
		}
	}
	if (!Came.Contains(Destination))
	{
		NaviRoute[0]->SetText(FText::FromString(TEXT("Sem caminho conhecido a partir daqui.")));
		return;
	}
	TArray<FString> Lines;
	for (FString Cur = Destination; Cur != From;)
	{
		const FStep& S = Came[Cur];
		Lines.Insert(FString::Printf(TEXT("%s: portal em (%d, %d)  ->  %s"), *MapLabel(S.Prev), S.X, S.Y, *MapLabel(Cur)), 0);
		Cur = S.Prev;
	}
	for (int32 i = 0; i < NvRouteLines && i < Lines.Num(); ++i)
	{
		NaviRoute[i]->SetText(FText::FromString(i == NvRouteLines - 1 && Lines.Num() > NvRouteLines ? FString(TEXT("...")) : Lines[i]));
	}
}

bool UMythRoHUDWidget::HandleNaviClick(const FVector2D& ScreenPos)
{
	if (!NaviWindow || !NaviWindow->IsVisible())
	{
		return false;
	}
	if (IsUnder(NaviFind, ScreenPos)) { SearchNavi(NaviBox->GetText().ToString()); return true; }
	if (IsUnder(NaviGo, ScreenPos) && !NaviSelected.IsEmpty()) { ShowNaviRoute(NaviSelected); return true; }
	for (int32 i = 0; i < NvResults && i < NaviMatches.Num(); ++i)
	{
		if (IsUnder(NaviRows[i], ScreenPos))
		{
			ShowNaviRoute(NaviMatches[i]);
			return true;
		}
	}
	return false;
}

// Navegacao (/navi, Alt+N): procura um mapa pelo nome e mostra o caminho pelos portais a partir de onde voce esta.
// As ligacoes entre mapas e os nomes em portugues vem do cliente LATAM (Config/navi.json, tools/gen_navi.py); o caminho e
// o mais curto em numero de portais. Mapas que o servidor nao tem ainda aparecem na lista mas sem rota.

#include "UI/MythRoHUDWidget.h"

#include "Blueprint/WidgetTree.h"
#include "Components/CanvasPanel.h"
#include "Components/CanvasPanelSlot.h"
#include "Components/EditableTextBox.h"
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
	constexpr float NvW = 360.f;
	constexpr float NvRowH = 17.f;
	constexpr int32 NvResults = 8;
	constexpr int32 NvRouteLines = 10;

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
}

void UMythRoHUDWidget::BuildNaviWindow()
{
	const float H = 22.f + 30.f + NvResults * NvRowH + 12.f + NvRouteLines * NvRowH + 12.f;
	NaviWindow = MakeRoPanel(TEXT("Navegação"), FVector2D(NvW, H), FVector2D(-120.f, -40.f));
	AddText(NaviWindow, 10.f, 24.f, 8, NvGray)->SetText(FText::FromString(TEXT("Digite o nome do mapa e aperte Enter:")));
	NaviBox = WidgetTree->ConstructWidget<UEditableTextBox>();
	NaviBox->OnTextCommitted.AddDynamic(this, &UMythRoHUDWidget::OnNaviCommitted);
	if (UCanvasPanelSlot* S = NaviWindow->AddChildToCanvas(NaviBox))
	{
		S->SetPosition(FVector2D(10.f, 40.f) * UIScale);
		S->SetSize(FVector2D(NvW - 20.f, 20.f) * UIScale);
	}
	for (int32 i = 0; i < NvResults; ++i)
	{
		NaviRows.Add(AddText(NaviWindow, 12.f, 68.f + i * NvRowH, 8, NvLink));
	}
	for (int32 i = 0; i < NvRouteLines; ++i)
	{
		NaviRoute.Add(AddText(NaviWindow, 12.f, 68.f + NvResults * NvRowH + 12.f + i * NvRowH, 8, NvText));
	}
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
		NaviRows[i]->SetText(FText::FromString(NaviMatches.IsValidIndex(i) ? MapLabel(NaviMatches[i]) : (i == 0 && !Needle.IsEmpty() ? FString(TEXT("Nenhum mapa encontrado.")) : FString())));
	}
	for (UTextBlock* L : NaviRoute) { L->SetText(FText::GetEmpty()); }
	if (NaviMatches.Num() == 1 || (NaviMatches.Num() > 0 && Navi().Names[NaviMatches[0]].ToLower() == Needle))
	{
		ShowNaviRoute(NaviMatches[0]); // um resultado so, ou o nome exato: ja mostra o caminho
	}
}

void UMythRoHUDWidget::ShowNaviRoute(const FString& Destination)
{
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

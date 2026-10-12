// Navegacao (/navi, Alt+N): procura um mapa pelo nome e mostra o caminho pelos portais a partir de onde voce esta.
// As ligacoes entre mapas e os nomes em portugues vem do cliente LATAM (Config/navi.json, tools/gen_navi.py); o caminho e
// o mais curto em numero de portais. Mapas que o servidor nao tem ainda aparecem na lista mas sem rota.

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
	const FLinearColor ColSystem(0.55f, 0.95f, 0.55f);
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
	NaviAutoBtn = AddTextButton(NaviWindow, TEXT("Ir até o mapa"), 170.f, 198.f, 108.f);
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
	NaviSteps.Reset();
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
	NaviSteps.Reset();
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
	for (FString Cur = Destination; Cur != From;)
	{
		const FStep& S = Came[Cur];
		FNaviStep Step;
		Step.From = S.Prev;
		Step.X = S.X;
		Step.Y = S.Y;
		Step.To = Cur;
		NaviSteps.Insert(Step, 0);
		Cur = S.Prev;
	}
	for (int32 i = 0; i < NvRouteLines && i < NaviSteps.Num(); ++i)
	{
		const FNaviStep& S = NaviSteps[i];
		NaviRoute[i]->SetText(FText::FromString(i == NvRouteLines - 1 && NaviSteps.Num() > NvRouteLines ? FString(TEXT("...")) :
			FString::Printf(TEXT("%s (%d,%d) > %s"), *NaviLabelShort(S.From).Left(14), S.X, S.Y, *NaviLabelShort(S.To).Left(14))));
	}
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
		AddChatLine(TEXT("Navegação automática cancelada."), ColSystem);
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
	AddChatLine(FString::Printf(TEXT("Navegação automática: %s."), *NaviLabelShort(NaviAutoDest)), ColSystem);
}

void UMythRoHUDWidget::TickNaviAuto(float DeltaTime)
{
	if (NaviAutoBtn)
	{
		const FString Want = bNaviAuto ? TEXT("Parar") : TEXT("Ir até o mapa");
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
		AddChatLine(TEXT("Você chegou ao destino."), ColSystem);
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
			AddChatLine(TEXT("Navegação automática: sem caminho a partir daqui."), ColSystem);
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
		AddChatLine(FString::Printf(TEXT("Você chegou ao ponto da rota (%d, %d). Continue por aqui até %s."), Goal.X, Goal.Y, *NaviLabelShort(NaviSteps[0].To)), ColSystem);
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
	if (IsUnder(NaviFind, ScreenPos)) { SearchNavi(NaviBox->GetText().ToString()); return true; }
	if (IsUnder(NaviGo, ScreenPos) && !NaviSelected.IsEmpty()) { ShowNaviRoute(NaviSelected); return true; }
	if (IsUnder(NaviAutoBtn, ScreenPos)) { StartNaviAuto(); return true; }
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

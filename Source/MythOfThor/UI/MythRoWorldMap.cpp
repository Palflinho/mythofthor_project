// Mapa-mundi (Ctrl+`), como o do cliente: a imagem worldmap.jpg (1280 x 1024) com os retangulos de cada mapa do
// data/mappostable.txt do cliente LATAM ("regiao#mapa.rsw#x1#y1#x2#y2#"). O mapa onde voce esta fica marcado em vermelho,
// o mapa sob o mouse fica destacado com o nome (tabela da Navegacao) e o clique abre a Navegacao com a rota ate ele.

#include "UI/MythRoHUDWidget.h"

#include "Sprites/MythRoSpriteComponent.h"
#include "UI/MythRoUITextures.h"

#include "Components/CanvasPanel.h"
#include "Components/CanvasPanelSlot.h"
#include "Components/Image.h"
#include "Components/TextBlock.h"
#include "Framework/Application/SlateApplication.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"

namespace
{
	constexpr float WmScale = 0.45f;              // 1280 x 1024 -> 576 x 461 (cabe em 720p com a escala 1,35 da interface)
	constexpr float WmW = 1280.f * WmScale;
	constexpr float WmH = 1024.f * WmScale;
	constexpr float WmTop = 17.f;

	struct FWorldMapRect { FString MapId; FBox2D Box; };

	const TArray<FWorldMapRect>& WorldMapRects()
	{
		static TArray<FWorldMapRect> Rects;
		static bool bLoaded = false;
		if (!bLoaded)
		{
			bLoaded = true;
			TArray<FString> Lines;
			const FString Path = FPaths::Combine(UMythRoSpriteComponent::GetAssetsRoot(), TEXT("data"), TEXT("mappostable.txt"));
			if (FFileHelper::LoadFileToStringArray(Lines, *Path))
			{
				for (const FString& Line : Lines)
				{
					TArray<FString> F;
					Line.ParseIntoArray(F, TEXT("#"), true);
					if (F.Num() < 6 || !F[2].IsNumeric())
					{
						continue; // comentario ou cabecalho
					}
					FString Map = F[1].ToLower();
					Map.RemoveFromEnd(TEXT(".rsw"));
					Rects.Add({ Map, FBox2D(FVector2D(FCString::Atof(*F[2]), FCString::Atof(*F[3])), FVector2D(FCString::Atof(*F[4]), FCString::Atof(*F[5]))) });
				}
			}
			UE_LOG(LogTemp, Log, TEXT("MythWorldMap: %d mapas no mappostable"), Rects.Num());
		}
		return Rects;
	}

	/** Retangulo do mapa sob o ponto da imagem (o menor, para o interior de cidade ganhar do campo em volta). */
	const FWorldMapRect* WorldMapAt(const FVector2D& ImagePos)
	{
		const FWorldMapRect* Best = nullptr;
		for (const FWorldMapRect& R : WorldMapRects())
		{
			if (R.Box.IsInside(ImagePos) && (!Best || R.Box.GetArea() < Best->Box.GetArea()))
			{
				Best = &R;
			}
		}
		return Best;
	}

	void PlaceBox(UImage* Img, const FBox2D& Box, float UIScale)
	{
		if (UCanvasPanelSlot* S = Img ? Cast<UCanvasPanelSlot>(Img->Slot) : nullptr)
		{
			S->SetPosition(FVector2D(Box.Min.X * WmScale, WmTop + Box.Min.Y * WmScale) * UIScale);
			S->SetSize(Box.GetSize() * WmScale * UIScale);
		}
	}
}

void UMythRoHUDWidget::BuildWorldMapWindow()
{
	WorldMapWindow = MakeRoPanel(TEXT("Mapa-múndi"), FVector2D(WmW, WmTop + WmH), FVector2D(0.f, 0.f));
	WorldMapImage = AddImage(WorldMapWindow, FMythRoUITextures::Get(TEXT("worldmap.jpg")), 0.f, WmTop, WmW, WmH);
	WorldMapImage->SetVisibility(ESlateVisibility::Visible); // recebe o clique
	WorldMapHere = AddColor(WorldMapWindow, FLinearColor(1.f, 0.15f, 0.1f, 0.45f), 0.f, 0.f, 1.f, 1.f);
	WorldMapHere->SetVisibility(ESlateVisibility::Collapsed);
	WorldMapHover = AddColor(WorldMapWindow, FLinearColor(1.f, 1.f, 1.f, 0.35f), 0.f, 0.f, 1.f, 1.f);
	WorldMapHover->SetVisibility(ESlateVisibility::Collapsed);
	WorldMapLabel = AddText(WorldMapWindow, 0.f, 0.f, 9, FLinearColor::White, true);
	WorldMapLabel->SetVisibility(ESlateVisibility::Collapsed);
	WorldMapTitle = AddText(WorldMapWindow, 8.f, WmTop + WmH - 22.f, 9, FLinearColor::White, true);
}

void UMythRoHUDWidget::ToggleWorldMap()
{
	if (!WorldMapWindow)
	{
		return;
	}
	if (WorldMapWindow->IsVisible())
	{
		WorldMapWindow->SetVisibility(ESlateVisibility::Collapsed);
		return;
	}
	WorldMapWindow->SetVisibility(ESlateVisibility::SelfHitTestInvisible);
	OpenOrder.Remove(WorldMapWindow);
	OpenOrder.Add(WorldMapWindow);
	if (UCanvasPanelSlot* S = Cast<UCanvasPanelSlot>(WorldMapWindow->Slot))
	{
		S->SetZOrder(++TopZOrder);
	}
	// Onde voce esta
	const FString Here = CurrentMapId.ToLower();
	const FWorldMapRect* Rect = WorldMapRects().FindByPredicate([&Here](const FWorldMapRect& R) { return R.MapId == Here; });
	if (Rect)
	{
		PlaceBox(WorldMapHere, Rect->Box, UIScale);
	}
	WorldMapHere->SetVisibility(Rect ? ESlateVisibility::HitTestInvisible : ESlateVisibility::Collapsed);
	WorldMapTitle->SetText(FText::FromString(FString::Printf(TEXT("Você está em: %s"), *NaviDisplayName(Here))));
	WorldMapHoverId.Reset();
	TickWorldMap();
}

void UMythRoHUDWidget::TickWorldMap()
{
	if (!WorldMapWindow || !WorldMapWindow->IsVisible() || !FSlateApplication::IsInitialized())
	{
		return;
	}
	const FGeometry& Geo = WorldMapImage->GetCachedGeometry();
	const FVector2D Local = Geo.AbsoluteToLocal(FSlateApplication::Get().GetCursorPos());
	const FVector2D Size = Geo.GetLocalSize();
	const FWorldMapRect* Rect = nullptr;
	if (Size.X > 0.f && Local.X >= 0.f && Local.Y >= 0.f && Local.X <= Size.X && Local.Y <= Size.Y)
	{
		Rect = WorldMapAt(FVector2D(Local.X / Size.X * 1280.f, Local.Y / Size.Y * 1024.f));
	}
	const FString Id = Rect ? Rect->MapId : FString();
	if (Id == WorldMapHoverId)
	{
		return;
	}
	WorldMapHoverId = Id;
	WorldMapHover->SetVisibility(Rect ? ESlateVisibility::HitTestInvisible : ESlateVisibility::Collapsed);
	WorldMapLabel->SetVisibility(Rect ? ESlateVisibility::HitTestInvisible : ESlateVisibility::Collapsed);
	if (Rect)
	{
		PlaceBox(WorldMapHover, Rect->Box, UIScale);
		WorldMapLabel->SetText(FText::FromString(FString::Printf(TEXT("%s (%s)"), *NaviDisplayName(Id), *Id)));
		if (UCanvasPanelSlot* S = Cast<UCanvasPanelSlot>(WorldMapLabel->Slot))
		{
			// Nome logo abaixo do retangulo (ou acima, perto da borda de baixo)
			const float Y = Rect->Box.Max.Y * WmScale + 2.f;
			S->SetPosition(FVector2D(Rect->Box.Min.X * WmScale, WmTop + (Y > WmH - 20.f ? Rect->Box.Min.Y * WmScale - 18.f : Y)) * UIScale);
		}
	}
}

bool UMythRoHUDWidget::HandleWorldMapClick(const FVector2D& ScreenPos)
{
	if (!WorldMapWindow || !WorldMapWindow->IsVisible() || !IsUnder(WorldMapImage, ScreenPos))
	{
		return false;
	}
	TickWorldMap();
	if (!WorldMapHoverId.IsEmpty())
	{
		// Abre a Navegacao ja com o mapa escolhido e a rota a partir de onde voce esta
		OpenNaviWindow();
		ShowNaviRoute(WorldMapHoverId);
		bNaviShowRoute = true;
		RefreshNaviLists();
	}
	return true;
}

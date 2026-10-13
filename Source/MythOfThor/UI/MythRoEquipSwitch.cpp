// Troca rapida de equipamento (Alt+W, docs/ref/hud_latam/INDICE.md; ZC 0x0a9b, 0x0a98, 0x0a9a, 0x0a9d): um segundo conjunto de
// equipamentos que o servidor guarda; "Trocar tudo" veste o conjunto de uma vez. Para colocar um item no conjunto: Ctrl +
// duplo clique nele na janela de itens. Quem veste, tira e valida e o servidor.

#include "UI/MythRoHUDWidget.h"
#include "Core/MythRoClientTables.h"

#include "Network/RagnarokNetworkSubsystem.h"

#include "Components/CanvasPanel.h"
#include "Components/TextBlock.h"

namespace
{
	const FLinearColor EsText(0.12f, 0.12f, 0.12f);
	const FLinearColor EsLink(0.12f, 0.3f, 0.65f);
	const FLinearColor EsGray(0.55f, 0.55f, 0.55f);
	constexpr float EsW = 280.f;
	constexpr float EsRowH = 18.f;
	constexpr float EsListY = 44.f;
	constexpr int32 EsRows = 10;
}

void UMythRoHUDWidget::BuildEquipSwitchWindow()
{
	const float H = EsListY + EsRows * EsRowH + 38.f;
	EquipSwitchWindow = MakeRoPanel(TEXT("Quick swap"), FVector2D(EsW, H), FVector2D(-60.f, 140.f));
	AddText(EquipSwitchWindow, 10.f, 22.f, 7, EsGray)->SetText(FText::FromString(TEXT("Ctrl + double-click an equipment item in your items to add it.")));
	for (int32 i = 0; i < EsRows; ++i)
	{
		const float Y = EsListY + i * EsRowH;
		EsNames.Add(AddText(EquipSwitchWindow, 12.f, Y, 8, EsText));
		EsRemove.Add(AddText(EquipSwitchWindow, EsW - 66.f, Y, 7, EsLink));
	}
	EsSwitchAll = AddTextButton(EquipSwitchWindow, TEXT("Swap all"), 90.f, H - 30.f, 100.f);
}

void UMythRoHUDWidget::RefreshEquipSwitch()
{
	const URagnarokNetworkSubsystem* Net = GetNet();
	if (!EquipSwitchWindow || !Net || !EquipSwitchWindow->IsVisible())
	{
		return;
	}
	const TArray<FIntPoint>& List = Net->GetEquipSwitch();
	for (int32 i = 0; i < EsRows; ++i)
	{
		const bool bHas = List.IsValidIndex(i);
		EsNames[i]->SetVisibility(bHas ? ESlateVisibility::Visible : ESlateVisibility::Collapsed);
		EsRemove[i]->SetVisibility(bHas ? ESlateVisibility::Visible : ESlateVisibility::Collapsed);
		if (!bHas)
		{
			continue;
		}
		FString Name = FString::Printf(TEXT("Item %d"), List[i].X);
		for (const FRagnarokInventoryItem& It : Net->GetInventoryItems())
		{
			if (It.Index == List[i].X)
			{
				Name = FMythRoClientTables::ItemName(It.ItemID);
				if (It.Refine > 0) { Name = FString::Printf(TEXT("+%d %s"), It.Refine, *Name); }
				break;
			}
		}
		EsNames[i]->SetText(FText::FromString(Name));
		EsRemove[i]->SetText(FText::FromString(TEXT("[Remove]")));
	}
}

bool UMythRoHUDWidget::HandleEquipSwitchClick(const FVector2D& ScreenPos)
{
	URagnarokNetworkSubsystem* Net = GetNet();
	if (!Net || !EquipSwitchWindow || !EquipSwitchWindow->IsVisible())
	{
		return false;
	}
	if (IsUnder(EsSwitchAll, ScreenPos))
	{
		Net->SendEquipSwitchAll();
		return true;
	}
	for (int32 i = 0; i < EsRows && i < Net->GetEquipSwitch().Num(); ++i)
	{
		if (IsUnder(EsRemove[i], ScreenPos))
		{
			Net->SendEquipSwitchRemove(Net->GetEquipSwitch()[i].X);
			return true;
		}
	}
	return false;
}

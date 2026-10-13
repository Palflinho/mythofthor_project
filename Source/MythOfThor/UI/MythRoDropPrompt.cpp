// Drop quantity prompt: dragging a stack out of the inventory onto the ground. Like the original client: a single item is
// dropped at once, a stack asks how many to drop (the box starts with the whole stack).

#include "UI/MythRoHUDWidget.h"

#include "Network/RagnarokNetworkSubsystem.h"

#include "Blueprint/WidgetTree.h"
#include "Components/CanvasPanel.h"
#include "Components/CanvasPanelSlot.h"
#include "Components/EditableTextBox.h"
#include "Components/TextBlock.h"

void UMythRoHUDWidget::BuildDropPrompt()
{
	DropPromptWindow = MakeRoPanel(TEXT("Drop item"), FVector2D(230.f, 17.f + 78.f), FVector2D(0.f, -40.f));
	DropPromptWindow->SetVisibility(ESlateVisibility::Collapsed);
	DropPromptLabel = AddText(DropPromptWindow, 14.f, 26.f, 8, FLinearColor(0.12f, 0.12f, 0.12f), true);
	DropAmountBox = WidgetTree->ConstructWidget<UEditableTextBox>();
	if (UCanvasPanelSlot* S = DropPromptWindow->AddChildToCanvas(DropAmountBox))
	{
		S->SetPosition(FVector2D(14.f, 17.f + 30.f) * UIScale);
		S->SetSize(FVector2D(100.f, 20.f) * UIScale);
	}
	DropOkLink = AddTextButton(DropPromptWindow, TEXT("OK"), 124.f, 17.f + 30.f, 46.f);
	DropCancelLink = AddTextButton(DropPromptWindow, TEXT("Cancel"), 174.f, 17.f + 30.f, 46.f);
}

void UMythRoHUDWidget::AskDropAmount(int32 Index, int32 Count)
{
	URagnarokNetworkSubsystem* Net = GetNet();
	if (!Net || Index == INDEX_NONE || Count <= 0)
	{
		return;
	}
	if (Count == 1 || !DropPromptWindow)
	{
		Net->SendDropItem(Index, Count);
		return;
	}
	DropPromptIndex = Index;
	DropPromptMax = Count;
	DropPromptLabel->SetText(FText::FromString(FString::Printf(TEXT("How many to drop? (1 - %d)"), Count)));
	DropAmountBox->SetText(FText::AsNumber(Count));
	DropPromptWindow->SetVisibility(ESlateVisibility::SelfHitTestInvisible);
	OpenOrder.Remove(DropPromptWindow);
	OpenOrder.Add(DropPromptWindow);
}

bool UMythRoHUDWidget::HandleDropPromptClick(const FVector2D& ScreenPos)
{
	if (!DropPromptWindow || !DropPromptWindow->IsVisible())
	{
		return false;
	}
	URagnarokNetworkSubsystem* Net = GetNet();
	if (IsUnder(DropOkLink, ScreenPos))
	{
		const int32 Amount = FMath::Clamp(FCString::Atoi(*DropAmountBox->GetText().ToString().Replace(TEXT("."), TEXT("")).Replace(TEXT(","), TEXT(""))), 1, DropPromptMax);
		if (Net && DropPromptIndex != INDEX_NONE)
		{
			Net->SendDropItem(DropPromptIndex, Amount);
		}
		DropPromptIndex = INDEX_NONE;
		DropPromptWindow->SetVisibility(ESlateVisibility::Collapsed);
		return true;
	}
	if (IsUnder(DropCancelLink, ScreenPos))
	{
		DropPromptIndex = INDEX_NONE;
		DropPromptWindow->SetVisibility(ESlateVisibility::Collapsed);
		return true;
	}
	return IsUnder(DropPromptWindow, ScreenPos);
}

// Mascote (pet) do UMythRoHUDWidget: "Janela de Mascote" (Alt+J, msg 584) como o PetInformations do roBrowserLegacy:
// retrato, nome (renomear uma vez), nivel, fome e lealdade em palavras (limites do cliente: fome 10/25/75/90 -> msgs 667..671,
// lealdade 100/250/600/900 -> 672, 673, 669, 674, 675), acessorio, autoalimentacao e os comandos (Alimentar, Brincar,
// Voltar para o ovo, Desequipar). Pacotes em Network/RagnarokNetworkPet.cpp.

#include "UI/MythRoHUDWidget.h"
#include "Core/MythRoClientTables.h"
#include "UI/MythRoUITextures.h"

#include "Network/RagnarokNetworkSubsystem.h"
#include "Sprites/MythMonster.h"
#include "Sprites/MythRoSpriteComponent.h"

#include "Blueprint/WidgetTree.h"
#include "Components/CanvasPanel.h"
#include "Components/CanvasPanelSlot.h"
#include "Components/EditableTextBox.h"
#include "Components/Image.h"
#include "Components/TextBlock.h"
#include "EngineUtils.h"

namespace
{
	const FLinearColor PetText(0.1f, 0.1f, 0.1f);
	const FLinearColor PetGray(0.45f, 0.45f, 0.45f);
	constexpr float PetW = 320.f;
	constexpr float PetH = 176.f;

	FString PetMsg(int32 Index) { return FMythRoClientTables::Message(Index).TrimStartAndEnd(); }

	FString PetHungerText(int32 Hunger)
	{
		return PetMsg(Hunger < 10 ? 667 : Hunger < 25 ? 668 : Hunger < 75 ? 669 : Hunger < 90 ? 670 : 671);
	}

	FString PetIntimacyText(int32 Intimacy)
	{
		return PetMsg(Intimacy < 100 ? 672 : Intimacy < 250 ? 673 : Intimacy < 600 ? 669 : Intimacy < 900 ? 674 : 675);
	}
}

void UMythRoHUDWidget::BuildPetWindow()
{
	PetWindow = MakeRoPanel(PetMsg(584), FVector2D(PetW, PetH), FVector2D(-260.f, 60.f));
	AddColor(PetWindow, FLinearColor(0.93f, 0.94f, 0.96f, 1.f), 8.f, 24.f, 96.f, 96.f);
	PetPortrait = AddImage(PetWindow, nullptr, 8.f, 24.f, 96.f, 96.f);
	AddText(PetWindow, 114.f, 27.f, 8, PetText)->SetText(FText::FromString(PetMsg(328))); // Nome
	PetNameBox = WidgetTree->ConstructWidget<UEditableTextBox>();
	if (UCanvasPanelSlot* S = PetWindow->AddChildToCanvas(PetNameBox))
	{
		S->SetPosition(FVector2D(156.f, 24.f) * UIScale);
		S->SetSize(FVector2D(100.f, 20.f) * UIScale);
	}
	PetRenameBtn = AddTextButton(PetWindow, TEXT("Renomear"), 260.f, 24.f, 54.f);
	for (int32 i = 0; i < 4; ++i)
	{
		PetLines.Add(AddText(PetWindow, 114.f, 50.f + i * 17.f, 8, PetText));
	}
	PetAutoFeed = AddText(PetWindow, 8.f, 126.f, 8, PetText);
	PetAutoFeed->SetVisibility(ESlateVisibility::Visible);
	static const int32 Msgs[4] = { 592, 593, 594, 595 };
	static const float Xs[4] = { 8.f, 76.f, 140.f, 244.f };
	static const float Ws[4] = { 64.f, 60.f, 100.f, 68.f };
	for (int32 i = 0; i < 4; ++i)
	{
		PetCmdBtns.Add(AddTextButton(PetWindow, PetMsg(Msgs[i]), Xs[i], PetH - 28.f, Ws[i]));
	}
}

void UMythRoHUDWidget::TogglePetWindow()
{
	URagnarokNetworkSubsystem* Net = GetNet();
	if (!PetWindow || !Net)
	{
		return;
	}
	if (PetWindow->IsVisible())
	{
		PetWindow->SetVisibility(ESlateVisibility::Collapsed);
		OpenOrder.Remove(PetWindow);
		return;
	}
	if (!Net->HasPet())
	{
		AddChatLine(TEXT("Você não tem um mascote chocado."), FLinearColor(1.f, 0.85f, 0.4f));
		return;
	}
	PetWindow->SetVisibility(ESlateVisibility::SelfHitTestInvisible);
	OpenOrder.Remove(PetWindow);
	OpenOrder.Add(PetWindow);
	if (UCanvasPanelSlot* S = Cast<UCanvasPanelSlot>(PetWindow->Slot))
	{
		S->SetZOrder(++TopZOrder);
	}
	PetShownName.Reset();
	Net->SendPetMenu(0); // como o cliente: pede os dados atuais (ZC_PROPERTY_PET)
	RefreshPet();
}

void UMythRoHUDWidget::OnPetEvent(uint8 Event)
{
	if (Event == 2 && PetWindow)
	{
		PetWindow->SetVisibility(ESlateVisibility::Collapsed);
		OpenOrder.Remove(PetWindow);
		PetPortraitKey.Reset();
		return;
	}
	RefreshPet();
}

void UMythRoHUDWidget::RefreshPet()
{
	const URagnarokNetworkSubsystem* Net = GetNet();
	if (!PetWindow || !Net || !PetWindow->IsVisible())
	{
		return;
	}
	const FRoPetInfo& P = Net->GetPetInfo();
	// Retrato: primeiro quadro do sprite do proprio mascote no mundo
	for (TActorIterator<AMythMonster> It(GetWorld()); It; ++It)
	{
		if (It->NetID == Net->GetPetId() && It->Sprite && !It->Sprite->MonsterSpriteName.IsEmpty())
		{
			const FString Key = It->Sprite->MonsterSpriteFolder / It->Sprite->MonsterSpriteName;
			if (Key != PetPortraitKey)
			{
				if (UTexture2D* Tex = FMythRoUITextures::GetSpriteFrame(Key, 0))
				{
					PetPortraitKey = Key;
					PetPortrait->SetBrushFromTexture(Tex, false);
					const float Scale = FMath::Min(1.f, 90.f / FMath::Max(Tex->GetSizeX(), Tex->GetSizeY()));
					const FVector2D Size(Tex->GetSizeX() * Scale, Tex->GetSizeY() * Scale);
					PetPortrait->SetDesiredSizeOverride(Size * UIScale);
					if (UCanvasPanelSlot* S = Cast<UCanvasPanelSlot>(PetPortrait->Slot))
					{
						S->SetPosition(FVector2D(8.f + (96.f - Size.X) * 0.5f, 24.f + (96.f - Size.Y) * 0.5f) * UIScale);
						S->SetSize(Size * UIScale);
					}
				}
			}
			break;
		}
	}
	if (!P.Name.IsEmpty() && P.Name != PetShownName)
	{
		if (!PetShownName.IsEmpty())
		{
			// msg 2931 "Nome alterado para ^0000ff%s^000000." (sem os codigos de cor)
			FString Msg = PetMsg(2931).Replace(TEXT("%s"), *P.Name).Replace(TEXT("^0000ff"), TEXT("")).Replace(TEXT("^000000"), TEXT(""));
			AddChatLine(Msg, FLinearColor(0.6f, 0.8f, 1.f));
		}
		PetShownName = P.Name;
		PetNameBox->SetText(FText::FromString(P.Name));
	}
	PetNameBox->SetIsReadOnly(P.bRenamed);
	SetTextButtonShown(PetRenameBtn, !P.bRenamed);
	const FString Lines[4] = {
		FString::Printf(TEXT("%s: %d"), *PetMsg(329), P.Level),
		FString::Printf(TEXT("%s: %s"), *PetMsg(585), *PetHungerText(P.Hunger)),
		FString::Printf(TEXT("%s: %s"), *PetMsg(586), *PetIntimacyText(P.Intimacy)),
		FString::Printf(TEXT("%s: %s"), *PetMsg(597), P.Accessory ? *PetMsg(598) : TEXT("-")),
	};
	for (int32 i = 0; i < 4; ++i)
	{
		PetLines[i]->SetText(FText::FromString(Lines[i]));
	}
	PetAutoFeed->SetText(FText::FromString(FString::Printf(TEXT("[%s] %s"), Net->IsPetAutoFeed() ? TEXT("x") : TEXT("  "), *PetMsg(2577))));
	SetTextButtonShown(PetCmdBtns[3], P.Accessory != 0);
}

bool UMythRoHUDWidget::HandlePetClick(const FVector2D& ScreenPos)
{
	URagnarokNetworkSubsystem* Net = GetNet();
	if (!Net || !PetWindow || !PetWindow->IsVisible())
	{
		return false;
	}
	for (int32 i = 0; i < PetCmdBtns.Num(); ++i)
	{
		if (IsUnder(PetCmdBtns[i], ScreenPos))
		{
			Net->SendPetMenu(i + 1); // 1 alimentar, 2 brincar, 3 voltar para o ovo, 4 desequipar
			return true;
		}
	}
	if (IsUnder(PetRenameBtn, ScreenPos))
	{
		const FString Name = PetNameBox->GetText().ToString().TrimStartAndEnd();
		if (!Name.IsEmpty() && Name != Net->GetPetInfo().Name)
		{
			Net->SendPetRename(Name);
		}
		return true;
	}
	if (IsUnder(PetAutoFeed, ScreenPos))
	{
		Net->SendConfig(2, !Net->IsPetAutoFeed());
		return true;
	}
	return false;
}

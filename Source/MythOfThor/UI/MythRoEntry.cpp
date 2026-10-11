// Entrada do jogo (docs/ref/22 3 e hud_latam/INDICE.md): login, escolha de personagem e criacao. As regras (conta, senha,
// quantos personagens, nomes validos) sao do servidor; aqui so se mostram as telas e se mandam os pacotes que o
// subsistema de rede ja sabe enviar (CA_LOGIN, CH_SELECT_CHAR, CH_MAKE_CHAR).

#include "UI/MythRoHUDWidget.h"

#include "Network/RagnarokNetworkSubsystem.h"
#include "UI/MythRoUITextures.h"

#include "Blueprint/WidgetTree.h"
#include "Components/CanvasPanel.h"
#include "Components/CanvasPanelSlot.h"
#include "Components/EditableTextBox.h"
#include "Components/Image.h"
#include "Components/TextBlock.h"
#include "Kismet/KismetSystemLibrary.h"
#include "Misc/ConfigCacheIni.h"

namespace
{
	const FLinearColor EnText(0.12f, 0.12f, 0.12f);
	const FLinearColor EnLink(0.12f, 0.3f, 0.65f);
	const FLinearColor EnGray(0.5f, 0.5f, 0.5f);
	const FLinearColor EnError(0.75f, 0.1f, 0.1f);
	const FLinearColor EnGold(0.96f, 0.82f, 0.45f);
	constexpr int32 EnCardCols = 3;
	constexpr float EnCardW = 214.f;
	constexpr float EnCardH = 74.f;
	const TCHAR* EntrySection = TEXT("Myth.Entry");

	FString EnJobName(int32 Job) { return URagnarokNetworkSubsystem::GetJobNameFromClass(Job); }
}

void UMythRoHUDWidget::BuildEntryScreens()
{
	// Fundo da tela inteira (cobre o mundo e o resto da interface); o mundo continua rodando por tras
	EntryBackdrop = WidgetTree->ConstructWidget<UCanvasPanel>();
	EntryBackdrop->SetVisibility(ESlateVisibility::Collapsed);
	if (UCanvasPanelSlot* S = Root->AddChildToCanvas(EntryBackdrop))
	{
		S->SetAnchors(FAnchors(0.f, 0.f, 1.f, 1.f));
		S->SetOffsets(FMargin(0.f));
		S->SetZOrder(9000);
	}
	UImage* Fill = WidgetTree->ConstructWidget<UImage>();
	Fill->SetColorAndOpacity(FLinearColor(0.03f, 0.05f, 0.11f, 1.f));
	Fill->SetVisibility(ESlateVisibility::Visible);
	if (UCanvasPanelSlot* S = EntryBackdrop->AddChildToCanvas(Fill))
	{
		S->SetAnchors(FAnchors(0.f, 0.f, 1.f, 1.f));
		S->SetOffsets(FMargin(0.f));
	}
	UTextBlock* Title = WidgetTree->ConstructWidget<UTextBlock>();
	Title->SetFont(FMythRoUITextures::RoFont(FMath::RoundToInt(34 * UIScale), true));
	Title->SetColorAndOpacity(FSlateColor(EnGold));
	Title->SetShadowOffset(FVector2D(2.f, 2.f));
	Title->SetShadowColorAndOpacity(FLinearColor(0.f, 0.f, 0.f, 0.8f));
	Title->SetText(FText::FromString(TEXT("MYTH OF THOR")));
	Title->SetJustification(ETextJustify::Center);
	if (UCanvasPanelSlot* S = EntryBackdrop->AddChildToCanvas(Title))
	{
		S->SetAnchors(FAnchors(0.5f, 0.12f));
		S->SetAlignment(FVector2D(0.5f, 0.5f));
		S->SetAutoSize(true);
	}
	UTextBlock* Foot = WidgetTree->ConstructWidget<UTextBlock>();
	Foot->SetFont(FMythRoUITextures::RoFont(FMath::RoundToInt(8 * UIScale), false));
	Foot->SetColorAndOpacity(FSlateColor(FLinearColor(0.55f, 0.6f, 0.75f, 1.f)));
	Foot->SetText(FText::FromString(TEXT("Projeto de fãs, sem fins lucrativos. Ragnarok Online © Gravity Co., Ltd. / Lee MyoungJin.")));
	if (UCanvasPanelSlot* S = EntryBackdrop->AddChildToCanvas(Foot))
	{
		S->SetAnchors(FAnchors(0.5f, 0.97f));
		S->SetAlignment(FVector2D(0.5f, 0.5f));
		S->SetAutoSize(true);
	}

	// ---- Login
	{
		const FVector2D Size(340.f, 17.f + 170.f);
		EntryLoginPanel = WidgetTree->ConstructWidget<UCanvasPanel>();
		EntryLoginPanel->SetVisibility(ESlateVisibility::SelfHitTestInvisible);
		if (UCanvasPanelSlot* S = EntryBackdrop->AddChildToCanvas(EntryLoginPanel))
		{
			S->SetAnchors(FAnchors(0.5f, 0.5f));
			S->SetAlignment(FVector2D(0.5f, 0.5f));
			S->SetSize(Size * UIScale);
		}
		AddRounded(EntryLoginPanel, FLinearColor(0.965f, 0.965f, 0.98f, 1.f), FLinearColor::Transparent, 0.f, 9.f, Size.X, Size.Y - 9.f, 0.f, 7.f)->SetVisibility(ESlateVisibility::Visible);
		AddTitleBar(EntryLoginPanel, Size.X, TEXT("Entrar"));
		AddText(EntryLoginPanel, 18.f, 30.f, 9, EnText)->SetText(FText::FromString(TEXT("Usuário")));
		AddText(EntryLoginPanel, 18.f, 66.f, 9, EnText)->SetText(FText::FromString(TEXT("Senha")));
		EntryUserBox = WidgetTree->ConstructWidget<UEditableTextBox>();
		EntryUserBox->OnTextCommitted.AddDynamic(this, &UMythRoHUDWidget::OnEntryCommitted);
		if (UCanvasPanelSlot* S = EntryLoginPanel->AddChildToCanvas(EntryUserBox))
		{
			S->SetPosition(FVector2D(18.f, 44.f) * UIScale);
			S->SetSize(FVector2D(Size.X - 36.f, 20.f) * UIScale);
		}
		EntryPassBox = WidgetTree->ConstructWidget<UEditableTextBox>();
		EntryPassBox->SetIsPassword(true);
		EntryPassBox->OnTextCommitted.AddDynamic(this, &UMythRoHUDWidget::OnEntryCommitted);
		if (UCanvasPanelSlot* S = EntryLoginPanel->AddChildToCanvas(EntryPassBox))
		{
			S->SetPosition(FVector2D(18.f, 80.f) * UIScale);
			S->SetSize(FVector2D(Size.X - 36.f, 20.f) * UIScale);
		}
		EntryStatus = AddText(EntryLoginPanel, 18.f, 108.f, 8, EnGray);
		AddText(EntryLoginPanel, 18.f, 122.f, 7, EnGray)->SetText(FText::FromString(TEXT("Conta nova: usuário terminando em _M ou _F (se o servidor permitir).")));
		EntryLoginLink = AddText(EntryLoginPanel, 60.f, 148.f, 11, EnLink);
		EntryLoginLink->SetText(FText::FromString(TEXT("Entrar")));
		EntryQuitLink = AddText(EntryLoginPanel, 210.f, 148.f, 11, EnLink);
		EntryQuitLink->SetText(FText::FromString(TEXT("Sair")));
	}

	// ---- Escolha de personagem
	{
		const float W = 10.f + EnCardCols * (EnCardW + 6.f) + 4.f;
		const float H = 17.f + 28.f + 3 * (EnCardH + 6.f) + 70.f;
		const FVector2D Size(W, H);
		EntryCharPanel = WidgetTree->ConstructWidget<UCanvasPanel>();
		EntryCharPanel->SetVisibility(ESlateVisibility::Collapsed);
		if (UCanvasPanelSlot* S = EntryBackdrop->AddChildToCanvas(EntryCharPanel))
		{
			S->SetAnchors(FAnchors(0.5f, 0.52f));
			S->SetAlignment(FVector2D(0.5f, 0.5f));
			S->SetSize(Size * UIScale);
		}
		AddRounded(EntryCharPanel, FLinearColor(0.965f, 0.965f, 0.98f, 1.f), FLinearColor::Transparent, 0.f, 9.f, Size.X, Size.Y - 9.f, 0.f, 7.f)->SetVisibility(ESlateVisibility::Visible);
		AddTitleBar(EntryCharPanel, Size.X, TEXT("Escolha seu personagem"));
		for (int32 i = 0; i < EntryCardCount; ++i)
		{
			const float X = 10.f + (i % EnCardCols) * (EnCardW + 6.f);
			const float Y = 24.f + (i / EnCardCols) * (EnCardH + 6.f);
			UImage* Back = AddRounded(EntryCharPanel, FLinearColor(0.82f, 0.85f, 0.93f, 1.f), FLinearColor::Transparent, X, Y, EnCardW, EnCardH, 6.f, 6.f);
			Back->SetVisibility(ESlateVisibility::Visible);
			EntryCardBack.Add(Back);
			EntryCardName.Add(AddText(EntryCharPanel, X + 10.f, Y + 8.f, 10, EnText));
			EntryCardInfo.Add(AddText(EntryCharPanel, X + 10.f, Y + 30.f, 8, EnText));
			EntryCardInfo2.Add(AddText(EntryCharPanel, X + 10.f, Y + 46.f, 7, EnGray));
		}
		EntryPlayLink = AddText(EntryCharPanel, 40.f, H - 38.f, 11, EnLink);
		EntryPlayLink->SetText(FText::FromString(TEXT("Jogar")));
		EntryCreateLink = AddText(EntryCharPanel, 160.f, H - 38.f, 11, EnLink);
		EntryCreateLink->SetText(FText::FromString(TEXT("Criar personagem")));
		EntryBackLink = AddText(EntryCharPanel, W - 120.f, H - 38.f, 11, EnLink);
		EntryBackLink->SetText(FText::FromString(TEXT("Voltar")));
		EntryCharStatus = AddText(EntryCharPanel, 12.f, H - 62.f, 8, EnGray);
	}

	// ---- Criacao de personagem
	{
		const FVector2D Size(340.f, 17.f + 200.f);
		EntryCreatePanel = WidgetTree->ConstructWidget<UCanvasPanel>();
		EntryCreatePanel->SetVisibility(ESlateVisibility::Collapsed);
		if (UCanvasPanelSlot* S = EntryBackdrop->AddChildToCanvas(EntryCreatePanel))
		{
			S->SetAnchors(FAnchors(0.5f, 0.5f));
			S->SetAlignment(FVector2D(0.5f, 0.5f));
			S->SetSize(Size * UIScale);
		}
		AddRounded(EntryCreatePanel, FLinearColor(0.965f, 0.965f, 0.98f, 1.f), FLinearColor::Transparent, 0.f, 9.f, Size.X, Size.Y - 9.f, 0.f, 7.f)->SetVisibility(ESlateVisibility::Visible);
		AddTitleBar(EntryCreatePanel, Size.X, TEXT("Criar personagem"));
		AddText(EntryCreatePanel, 18.f, 30.f, 9, EnText)->SetText(FText::FromString(TEXT("Nome")));
		EntryNameBox = WidgetTree->ConstructWidget<UEditableTextBox>();
		EntryNameBox->OnTextCommitted.AddDynamic(this, &UMythRoHUDWidget::OnEntryCommitted);
		if (UCanvasPanelSlot* S = EntryCreatePanel->AddChildToCanvas(EntryNameBox))
		{
			S->SetPosition(FVector2D(18.f, 44.f) * UIScale);
			S->SetSize(FVector2D(Size.X - 36.f, 20.f) * UIScale);
		}
		AddText(EntryCreatePanel, 18.f, 78.f, 9, EnText)->SetText(FText::FromString(TEXT("Cabelo")));
		EntryHairLeft = AddText(EntryCreatePanel, 120.f, 78.f, 10, EnLink);
		EntryHairLeft->SetText(FText::FromString(TEXT("◀")));
		EntryHairText = AddText(EntryCreatePanel, 148.f, 78.f, 9, EnText);
		EntryHairRight = AddText(EntryCreatePanel, 190.f, 78.f, 10, EnLink);
		EntryHairRight->SetText(FText::FromString(TEXT("▶")));
		AddText(EntryCreatePanel, 18.f, 104.f, 9, EnText)->SetText(FText::FromString(TEXT("Cor do cabelo")));
		EntryColorLeft = AddText(EntryCreatePanel, 120.f, 104.f, 10, EnLink);
		EntryColorLeft->SetText(FText::FromString(TEXT("◀")));
		EntryColorText = AddText(EntryCreatePanel, 148.f, 104.f, 9, EnText);
		EntryColorRight = AddText(EntryCreatePanel, 190.f, 104.f, 10, EnLink);
		EntryColorRight->SetText(FText::FromString(TEXT("▶")));
		AddText(EntryCreatePanel, 18.f, 130.f, 8, EnGray)->SetText(FText::FromString(TEXT("Todo personagem começa como Aprendiz.")));
		EntryCreateStatus = AddText(EntryCreatePanel, 18.f, 148.f, 8, EnError);
		EntryDoCreateLink = AddText(EntryCreatePanel, 60.f, 172.f, 11, EnLink);
		EntryDoCreateLink->SetText(FText::FromString(TEXT("Criar")));
		EntryCancelCreateLink = AddText(EntryCreatePanel, 210.f, 172.f, 11, EnLink);
		EntryCancelCreateLink->SetText(FText::FromString(TEXT("Cancelar")));
	}
}

void UMythRoHUDWidget::SetEntryPanel(int32 State)
{
	EntryState = State;
	if (!EntryBackdrop)
	{
		return;
	}
	EntryBackdrop->SetVisibility(State == 0 ? ESlateVisibility::Collapsed : ESlateVisibility::SelfHitTestInvisible);
	EntryLoginPanel->SetVisibility(State == 1 || State == 2 ? ESlateVisibility::SelfHitTestInvisible : ESlateVisibility::Collapsed);
	EntryCharPanel->SetVisibility(State == 3 ? ESlateVisibility::SelfHitTestInvisible : ESlateVisibility::Collapsed);
	EntryCreatePanel->SetVisibility(State == 4 ? ESlateVisibility::SelfHitTestInvisible : ESlateVisibility::Collapsed);
}

void UMythRoHUDWidget::ShowEntryLogin(const FString& Message)
{
	SetEntryPanel(1);
	FString User;
	if (EntryUserBox && EntryUserBox->GetText().IsEmpty() && GConfig && GConfig->GetString(EntrySection, TEXT("User"), User, GGameUserSettingsIni))
	{
		EntryUserBox->SetText(FText::FromString(User));
	}
	if (EntryPassBox)
	{
		EntryPassBox->SetText(FText::GetEmpty());
	}
	EntryStatus->SetText(FText::FromString(Message));
	EntryStatus->SetColorAndOpacity(FSlateColor(Message.IsEmpty() ? EnGray : EnError));
	if (EntryUserBox && EntryPassBox)
	{
		(EntryUserBox->GetText().IsEmpty() ? EntryUserBox : EntryPassBox)->SetKeyboardFocus();
	}
}

void UMythRoHUDWidget::ReturnToCharacterSelect()
{
	URagnarokNetworkSubsystem* Net = GetNet();
	if (!Net)
	{
		return;
	}
	Net->Disconnect();
	ShowEntryLogin();
	if (Net->GetSessionUser().IsEmpty())
	{
		return; // sem login guardado: digite de novo
	}
	FTimerHandle Handle;
	GetWorld()->GetTimerManager().SetTimer(Handle, FTimerDelegate::CreateWeakLambda(this, [this]() { if (URagnarokNetworkSubsystem* N = GetNet()) { EntryFillAndLogin(N->GetSessionUser(), N->GetSessionPass()); } }), 0.6f, false);
}

void UMythRoHUDWidget::HideEntry()
{
	SetEntryPanel(0);
}

void UMythRoHUDWidget::EntryLoginFailed(const FString& Message)
{
	if (EntryState != 0)
	{
		ShowEntryLogin(Message.IsEmpty() ? FString(TEXT("Usuário ou senha inválidos.")) : Message);
	}
}

void UMythRoHUDWidget::SubmitEntryLogin()
{
	URagnarokNetworkSubsystem* Net = GetNet();
	if (!Net || !EntryUserBox)
	{
		return;
	}
	const FString User = EntryUserBox->GetText().ToString().TrimStartAndEnd();
	const FString Pass = EntryPassBox->GetText().ToString();
	if (User.IsEmpty() || Pass.IsEmpty())
	{
		EntryStatus->SetText(FText::FromString(TEXT("Digite o usuário e a senha.")));
		EntryStatus->SetColorAndOpacity(FSlateColor(EnError));
		return;
	}
	if (GConfig)
	{
		GConfig->SetString(EntrySection, TEXT("User"), *User, GGameUserSettingsIni);
		GConfig->Flush(false, GGameUserSettingsIni);
	}
	FString Host = TEXT("127.0.0.1");
	GConfig->GetString(TEXT("Myth"), TEXT("NetHost"), Host, GGameIni);
	SetEntryPanel(2);
	EntryStatus->SetText(FText::FromString(TEXT("Conectando...")));
	EntryStatus->SetColorAndOpacity(FSlateColor(EnGray));
	if (!Net->ConnectAndLogin(Host, User, Pass))
	{
		ShowEntryLogin(TEXT("Não foi possível conectar ao servidor."));
	}
}

void UMythRoHUDWidget::ShowEntryCharacters(const TArray<FRagnarokCharacterSlotInfo>& Chars)
{
	EntryChars = Chars;
	SetEntryPanel(3);
	if (!EntryChars.IsValidIndex(EntrySelected) || EntrySelected < 0)
	{
		EntrySelected = -1;
		for (int32 i = 0; i < EntryCardCount; ++i)
		{
			if (EntrySlotChar(i))
			{
				EntrySelected = i;
				break;
			}
		}
	}
	RefreshEntryCharacters();
}

const FRagnarokCharacterSlotInfo* UMythRoHUDWidget::EntrySlotChar(int32 SlotIndex) const
{
	for (const FRagnarokCharacterSlotInfo& C : EntryChars)
	{
		if (C.Slot == SlotIndex)
		{
			return &C;
		}
	}
	return nullptr;
}

void UMythRoHUDWidget::RefreshEntryCharacters()
{
	for (int32 i = 0; i < EntryCardCount; ++i)
	{
		const FRagnarokCharacterSlotInfo* C = EntrySlotChar(i);
		EntryCardBack[i]->SetColorAndOpacity(i == EntrySelected ? FLinearColor(1.f, 0.95f, 0.7f, 1.f) : (C ? FLinearColor(0.82f, 0.88f, 1.f, 1.f) : FLinearColor(0.88f, 0.9f, 0.95f, 1.f)));
		EntryCardName[i]->SetText(FText::FromString(C ? C->Name : FString(TEXT("+  Vazio"))));
		EntryCardName[i]->SetColorAndOpacity(FSlateColor(C ? EnText : EnGray));
		EntryCardInfo[i]->SetText(C ? FText::FromString(FString::Printf(TEXT("%s  Nv %d / %d"), *EnJobName(C->JobClass), C->BaseLevel, C->JobLevel)) : FText::GetEmpty());
		EntryCardInfo2[i]->SetText(C ? FText::FromString(FString::Printf(TEXT("PV %lld   SP %lld"), C->MaxHP, C->MaxSP)) : FText::FromString(TEXT("Clique para criar")));
	}
	const FRagnarokCharacterSlotInfo* Sel = EntrySlotChar(EntrySelected);
	EntryPlayLink->SetColorAndOpacity(FSlateColor(Sel ? EnLink : EnGray));
	EntryCharStatus->SetText(FText::FromString(Sel ? FString::Printf(TEXT("Selecionado: %s"), *Sel->Name) : FString(TEXT("Escolha um personagem, ou um espaço vazio para criar."))));
}

void UMythRoHUDWidget::ShowEntryCreate(int32 SlotIndex)
{
	EntryCreateSlot = SlotIndex;
	EntryHair = 1;
	EntryColor = 0;
	if (EntryNameBox)
	{
		EntryNameBox->SetText(FText::GetEmpty());
		EntryNameBox->SetKeyboardFocus();
	}
	EntryCreateStatus->SetText(FText::GetEmpty());
	SetEntryPanel(4);
	EntryHairText->SetText(FText::FromString(FString::Printf(TEXT("%d"), EntryHair)));
	EntryColorText->SetText(FText::FromString(FString::Printf(TEXT("%d"), EntryColor)));
}

void UMythRoHUDWidget::EntryCharCreateResult(bool bOk, const FString& Message)
{
	if (EntryState == 0)
	{
		return;
	}
	if (bOk)
	{
		SetEntryPanel(3); // a lista vem em seguida pelo OnCharacterList
		return;
	}
	if (EntryState == 4)
	{
		EntryCreateStatus->SetText(FText::FromString(Message));
	}
}

void UMythRoHUDWidget::OnEntryCommitted(const FText& Text, ETextCommit::Type CommitMethod)
{
	if (CommitMethod != ETextCommit::OnEnter)
	{
		return;
	}
	if (EntryState == 1)
	{
		SubmitEntryLogin();
	}
	else if (EntryState == 4 && EntryNameBox)
	{
		if (URagnarokNetworkSubsystem* Net = GetNet())
		{
			const FString Name = EntryNameBox->GetText().ToString().TrimStartAndEnd();
			if (Name.Len() < 4)
			{
				EntryCreateStatus->SetText(FText::FromString(TEXT("O nome precisa ter pelo menos 4 letras.")));
			}
			else
			{
				Net->CreateCharacter(Name, static_cast<uint8>(EntryCreateSlot), EntryHair, EntryColor, 0);
			}
		}
	}
}

bool UMythRoHUDWidget::HandleEntryClick(const FVector2D& ScreenPos)
{
	if (EntryState == 0)
	{
		return false;
	}
	URagnarokNetworkSubsystem* Net = GetNet();
	if (EntryState == 1)
	{
		if (IsUnder(EntryLoginLink, ScreenPos)) { SubmitEntryLogin(); }
		else if (IsUnder(EntryQuitLink, ScreenPos)) { UKismetSystemLibrary::QuitGame(this, GetOwningPlayer(), EQuitPreference::Quit, false); }
	}
	else if (EntryState == 3)
	{
		for (int32 i = 0; i < EntryCardCount; ++i)
		{
			if (IsUnder(EntryCardBack[i], ScreenPos))
			{
				if (EntrySlotChar(i))
				{
					EntrySelected = i;
					RefreshEntryCharacters();
				}
				else
				{
					ShowEntryCreate(i);
				}
				return true;
			}
		}
		if (IsUnder(EntryPlayLink, ScreenPos) && Net && EntrySlotChar(EntrySelected))
		{
			SetEntryPanel(2);
			EntryStatus->SetText(FText::FromString(TEXT("Entrando no mundo...")));
			Net->SelectCharacter(EntrySelected);
		}
		else if (IsUnder(EntryCreateLink, ScreenPos))
		{
			for (int32 i = 0; i < EntryCardCount; ++i)
			{
				if (!EntrySlotChar(i)) { ShowEntryCreate(i); break; }
			}
		}
		else if (IsUnder(EntryBackLink, ScreenPos))
		{
			ShowEntryLogin();
		}
	}
	else if (EntryState == 4)
	{
		if (IsUnder(EntryHairLeft, ScreenPos)) { EntryHair = EntryHair <= 1 ? 27 : EntryHair - 1; }
		else if (IsUnder(EntryHairRight, ScreenPos)) { EntryHair = EntryHair >= 27 ? 1 : EntryHair + 1; }
		else if (IsUnder(EntryColorLeft, ScreenPos)) { EntryColor = EntryColor <= 0 ? 8 : EntryColor - 1; }
		else if (IsUnder(EntryColorRight, ScreenPos)) { EntryColor = EntryColor >= 8 ? 0 : EntryColor + 1; }
		else if (IsUnder(EntryDoCreateLink, ScreenPos)) { OnEntryCommitted(FText(), ETextCommit::OnEnter); }
		else if (IsUnder(EntryCancelCreateLink, ScreenPos)) { SetEntryPanel(3); RefreshEntryCharacters(); }
		EntryHairText->SetText(FText::FromString(FString::Printf(TEXT("%d"), EntryHair)));
		EntryColorText->SetText(FText::FromString(FString::Printf(TEXT("%d"), EntryColor)));
	}
	return true; // enquanto a entrada esta aberta, todo clique e dela
}

void UMythRoHUDWidget::EntryFillAndLogin(const FString& User, const FString& Pass)
{
	if (EntryUserBox && EntryPassBox)
	{
		EntryUserBox->SetText(FText::FromString(User));
		EntryPassBox->SetText(FText::FromString(Pass));
		SubmitEntryLogin();
	}
}

void UMythRoHUDWidget::EntryTestAction(int32 Action, const FString& Name)
{
	URagnarokNetworkSubsystem* Net = GetNet();
	if (!Net)
	{
		return;
	}
	if (Action == 0 && EntrySlotChar(EntrySelected))
	{
		Net->SelectCharacter(EntrySelected);
	}
	else if (Action == 1)
	{
		for (int32 i = 0; i < EntryCardCount; ++i)
		{
			if (!EntrySlotChar(i))
			{
				ShowEntryCreate(i);
				EntryNameBox->SetText(FText::FromString(Name));
				EntryHair = 5;
				EntryColor = 3;
				OnEntryCommitted(FText(), ETextCommit::OnEnter);
				break;
			}
		}
	}
}

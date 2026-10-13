// Menu do jogo (Esc sem janela aberta): continuar, configuracoes graficas, som e sair. As opcoes graficas sao as de
// jogos atuais (modo de tela, resolucao, qualidade, sombras, Lumen, anti-aliasing, escala, FPS, V-Sync) e ficam
// salvas pelo UGameUserSettings (GameUserSettings.ini) + [Myth.Graphics] para as que a Unreal nao guarda.

#include "UI/MythRoHUDWidget.h"

#include "UI/MythRoUITextures.h"
#include "Sprites/MythRoSound.h"
#include "Network/RagnarokNetworkSubsystem.h"

#include "AudioDevice.h"
#include "Components/CanvasPanel.h"
#include "Components/Image.h"
#include "Components/TextBlock.h"
#include "Engine/Engine.h"
#include "GameFramework/GameUserSettings.h"
#include "HAL/IConsoleManager.h"
#include "Kismet/KismetSystemLibrary.h"
#include "Misc/ConfigCacheIni.h"

namespace
{
	const FLinearColor MenuText(0.12f, 0.12f, 0.12f);
	const FLinearColor MenuLink(0.12f, 0.3f, 0.65f);
	const TCHAR* MythGfxSection = TEXT("Myth.Graphics");

	FString MenuBasic(const TCHAR* Name) { return FString::Printf(TEXT("basic_interface/%s.bmp"), Name); }

	void SetCVar(const TCHAR* Name, int32 Value)
	{
		if (IConsoleVariable* V = IConsoleManager::Get().FindConsoleVariable(Name))
		{
			V->Set(Value, ECVF_SetByGameSetting);
		}
	}

	const TArray<FString>& QualityNames()
	{
		static const TArray<FString> N = { TEXT("Baixa"), TEXT("Medium"), TEXT("Alta"), TEXT("Epic"), TEXT("Cinematic") };
		return N;
	}
}

// ------------------------------------------------------------------------------------------------ estado das opcoes

void UMythRoHUDWidget::LoadGraphicsOptions()
{
	UGameUserSettings* S = GEngine ? GEngine->GetGameUserSettings() : nullptr;
	if (!S)
	{
		return;
	}
	Gfx.Resolutions.Reset();
	TArray<FIntPoint> Supported;
	if (UKismetSystemLibrary::GetSupportedFullscreenResolutions(Supported))
	{
		for (const FIntPoint& R : Supported)
		{
			if (R.X >= 1024 && !Gfx.Resolutions.Contains(R)) { Gfx.Resolutions.Add(R); }
		}
	}
	if (Gfx.Resolutions.IsEmpty()) { Gfx.Resolutions = { {1280, 720}, {1600, 900}, {1920, 1080} }; }
	Gfx.ResolutionIndex = FMath::Max(0, Gfx.Resolutions.IndexOfByKey(S->GetScreenResolution()));
	if (!Gfx.Resolutions.IsValidIndex(Gfx.ResolutionIndex)) { Gfx.ResolutionIndex = Gfx.Resolutions.Num() - 1; }
	switch (S->GetFullscreenMode())
	{
	case EWindowMode::Fullscreen: Gfx.WindowMode = 2; break;
	case EWindowMode::WindowedFullscreen: Gfx.WindowMode = 1; break;
	default: Gfx.WindowMode = 0; break;
	}
	Gfx.Quality = FMath::Clamp(S->GetOverallScalabilityLevel(), 0, 4);
	if (Gfx.Quality < 0) { Gfx.Quality = 3; }
	Gfx.Shadows = FMath::Clamp(S->GetShadowQuality(), 0, 4);
	Gfx.ResolutionScale = FMath::Clamp(FMath::RoundToInt(S->GetResolutionScaleNormalized() * 10.f), 5, 10);
	const float Fps = S->GetFrameRateLimit();
	Gfx.FpsIndex = Fps <= 0.f ? 4 : Fps <= 30.f ? 0 : Fps <= 60.f ? 1 : Fps <= 120.f ? 2 : 3;
	Gfx.bVSync = S->IsVSyncEnabled();
	GConfig->GetInt(MythGfxSection, TEXT("Lumen"), Gfx.Lumen, GGameUserSettingsIni);
	GConfig->GetInt(MythGfxSection, TEXT("AntiAliasing"), Gfx.AntiAliasing, GGameUserSettingsIni);
	GConfig->GetInt(MythGfxSection, TEXT("Volume"), Gfx.Volume, GGameUserSettingsIni);
	ApplyMythGraphics(); // Lumen/AA/volume nao sao guardados pela Unreal: reaplica ao abrir o jogo
}

void UMythRoHUDWidget::ApplyMythGraphics()
{
	// Lumen: iluminacao global e reflexos (o toque HD-2D); desligado = mais FPS em PC fraco
	SetCVar(TEXT("r.DynamicGlobalIlluminationMethod"), Gfx.Lumen ? 1 : 0);
	SetCVar(TEXT("r.ReflectionMethod"), Gfx.Lumen ? 1 : 2);
	// 0 nenhum, 1 FXAA, 2 TAA, 4 TSR
	static const int32 AAValues[4] = { 0, 1, 2, 4 };
	SetCVar(TEXT("r.AntiAliasingMethod"), AAValues[FMath::Clamp(Gfx.AntiAliasing, 0, 3)]);
	if (GEngine)
	{
		if (FAudioDeviceHandle Audio = GEngine->GetMainAudioDevice())
		{
			Audio->SetTransientPrimaryVolume(Gfx.Volume / 10.f);
		}
	}
}

void UMythRoHUDWidget::ApplyGraphicsOptions()
{
	UGameUserSettings* S = GEngine ? GEngine->GetGameUserSettings() : nullptr;
	if (!S)
	{
		return;
	}
	static const EWindowMode::Type Modes[3] = { EWindowMode::Windowed, EWindowMode::WindowedFullscreen, EWindowMode::Fullscreen };
	S->SetFullscreenMode(Modes[FMath::Clamp(Gfx.WindowMode, 0, 2)]);
	if (Gfx.Resolutions.IsValidIndex(Gfx.ResolutionIndex))
	{
		S->SetScreenResolution(Gfx.Resolutions[Gfx.ResolutionIndex]);
	}
	S->SetOverallScalabilityLevel(Gfx.Quality);
	S->SetShadowQuality(Gfx.Shadows); // depois do geral: a sombra escolhida vale por cima da qualidade geral
	S->SetResolutionScaleNormalized(Gfx.ResolutionScale / 10.f);
	static const float FpsValues[5] = { 30.f, 60.f, 120.f, 144.f, 0.f };
	S->SetFrameRateLimit(FpsValues[FMath::Clamp(Gfx.FpsIndex, 0, 4)]);
	S->SetVSyncEnabled(Gfx.bVSync);
	S->ApplySettings(false);
	S->SaveSettings();
	GConfig->SetInt(MythGfxSection, TEXT("Lumen"), Gfx.Lumen, GGameUserSettingsIni);
	GConfig->SetInt(MythGfxSection, TEXT("AntiAliasing"), Gfx.AntiAliasing, GGameUserSettingsIni);
	GConfig->SetInt(MythGfxSection, TEXT("Volume"), Gfx.Volume, GGameUserSettingsIni);
	GConfig->Flush(false, GGameUserSettingsIni);
	ApplyMythGraphics();
	RefreshGraphicsWindow();
}

// ------------------------------------------------------------------------------------------------ janelas

void UMythRoHUDWidget::BuildGameMenu()
{
	// Menu principal ("Game setting window" do LATAM, print image-1791661013503): 300 de largura, 5 botoes largos de 262 x 20
	// com 6 px entre eles; ate aqui eram links de texto numa janela de 200
	const FVector2D MenuSize(300.f, 17.f + 12.f + 5 * 26.f + 4.f);
	GameMenuWindow = MakeRoPanel(TEXT("Menu"), MenuSize, FVector2D::ZeroVector);
	const TCHAR* Items[5] = { TEXT("Character select"), TEXT("Game settings"), TEXT("Shortcuts"), TEXT("Exit game"), TEXT("Back to game") };
	for (int32 i = 0; i < 5; ++i)
	{
		UTextBlock* T = AddTextButton(GameMenuWindow, Items[i], 19.f, 17.f + 12.f + i * 26.f, 262.f);
		GameMenuItems.Add(T);
	}
	GameMenuWindow->SetVisibility(ESlateVisibility::Collapsed);

	// Configuracoes graficas: "rotulo   <  valor  >" por linha
	const TCHAR* Labels[GfxRowCount] = { TEXT("Screen mode"), TEXT("Resolution"), TEXT("Qualidade geral"), TEXT("Sombras"),
		TEXT("Lighting (Lumen)"), TEXT("Anti-aliasing"), TEXT("Resolution scale"), TEXT("FPS limit"), TEXT("V-Sync") };
	const FVector2D Size(470.f, 17.f + 22.f + GfxRowCount * 24.f + 44.f);
	GraphicsWindow = MakeRoPanel(TEXT("Settings"), Size, FVector2D::ZeroVector);
	AddSettingsTabs(GraphicsWindow);
	UTexture2D* Left = FMythRoUITextures::Get(MenuBasic(TEXT("arw_left")));
	UTexture2D* Right = FMythRoUITextures::Get(MenuBasic(TEXT("arw_right")));
	for (int32 i = 0; i < GfxRowCount; ++i)
	{
		const float Y = 48.f + i * 24.f;
		AddText(GraphicsWindow, 14.f, Y, 8, MenuText)->SetText(FText::FromString(Labels[i]));
		UImage* L = AddImage(GraphicsWindow, Left, 186.f, Y + 2.f, 11.f, 11.f);
		UImage* R = AddImage(GraphicsWindow, Right, 352.f, Y + 2.f, 11.f, 11.f);
		L->SetVisibility(ESlateVisibility::Visible);
		R->SetVisibility(ESlateVisibility::Visible);
		GfxLeft.Add(L);
		GfxRight.Add(R);
		GfxValues.Add(AddText(GraphicsWindow, 206.f, Y, 8, MenuLink));
	}
	const float BY = Size.Y - 30.f;
	GfxApply = AddImage(GraphicsWindow, FMythRoUITextures::Get(TEXT("gamesettingsui/btn_optionapply_out.bmp")), Size.X - 150.f, BY);
	GfxApply->SetVisibility(ESlateVisibility::Visible);
	GfxClose = AddButtonHit(GraphicsWindow, TEXT("close"), Size.X - 56.f, BY);
	GfxHint = AddText(GraphicsWindow, 14.f, BY + 4.f, 7, FLinearColor(0.4f, 0.4f, 0.4f));
	GraphicsWindow->SetVisibility(ESlateVisibility::Collapsed);
	LoadGraphicsOptions();

	// Som e interface
	const TCHAR* SndLabels[SndRowCount] = { TEXT("Volume geral"), TEXT("Music"), TEXT("Efeitos"), TEXT("Interface scale") };
	const FVector2D SSize = Size; // mesmo tamanho: troca de aba sem a janela "pular"
	SoundWindow = MakeRoPanel(TEXT("Settings"), SSize, FVector2D::ZeroVector);
	AddSettingsTabs(SoundWindow);
	for (int32 i = 0; i < SndRowCount; ++i)
	{
		const float Y = 48.f + i * 24.f;
		AddText(SoundWindow, 14.f, Y, 8, MenuText)->SetText(FText::FromString(SndLabels[i]));
		UImage* L = AddImage(SoundWindow, Left, 186.f, Y + 2.f, 11.f, 11.f);
		UImage* R = AddImage(SoundWindow, Right, 352.f, Y + 2.f, 11.f, 11.f);
		L->SetVisibility(ESlateVisibility::Visible);
		R->SetVisibility(ESlateVisibility::Visible);
		SndLeft.Add(L);
		SndRight.Add(R);
		SndValues.Add(AddText(SoundWindow, 206.f, Y, 8, MenuLink));
	}
	SndClose = AddButtonHit(SoundWindow, TEXT("close"), SSize.X - 56.f, SSize.Y - 30.f);
	SndHint = AddText(SoundWindow, 14.f, SSize.Y - 26.f, 7, FLinearColor(0.4f, 0.4f, 0.4f));
	SoundWindow->SetVisibility(ESlateVisibility::Collapsed);

	// Janela de morte: sempre oferece voltar ao ponto de salvamento (CZ_RESTART 0); o fechar/Aguardar so a esconde
	const FVector2D DeathSize(260.f, 17.f + 74.f);
	DeathWindow = MakeRoPanel(TEXT("You died"), DeathSize, FVector2D(0.f, -120.f));
	AddText(DeathWindow, 14.f, 26.f, 8, MenuText)->SetText(FText::FromString(TEXT("You were defeated.")));
	DeathSaveLink = AddText(DeathWindow, 14.f, 44.f, 9, MenuLink);
	DeathSaveLink->SetText(FText::FromString(TEXT("Return to save point")));
	DeathWaitLink = AddText(DeathWindow, 14.f, 64.f, 9, MenuLink);
	DeathWaitLink->SetText(FText::FromString(TEXT("Aguardar socorro")));
	DeathWindow->SetVisibility(ESlateVisibility::Collapsed);
	SoundWindow->SetVisibility(ESlateVisibility::Collapsed);
	SndMusic = FMath::RoundToInt(UMythRoSound::GetMusicVolume() * 10.f);
	SndEffects = FMath::RoundToInt(UMythRoSound::GetEffectsVolume() * 10.f);
	SndUIScale = UIScale;
}

void UMythRoHUDWidget::RefreshSoundWindow()
{
	if (!SoundWindow || SndValues.Num() < SndRowCount)
	{
		return;
	}
	SndValues[0]->SetText(FText::FromString(FString::Printf(TEXT("%d%%"), Gfx.Volume * 10)));
	SndValues[1]->SetText(FText::FromString(FString::Printf(TEXT("%d%%"), SndMusic * 10)));
	SndValues[2]->SetText(FText::FromString(FString::Printf(TEXT("%d%%"), SndEffects * 10)));
	SndValues[3]->SetText(FText::FromString(FString::Printf(TEXT("%d%%"), FMath::RoundToInt(SndUIScale * 100.f))));
	SndHint->SetText(FText::FromString(FMath::IsNearlyEqual(SndUIScale, UIScale) ? TEXT("") : TEXT("The scale applies the next time the game starts.")));
}

void UMythRoHUDWidget::StepSoundOption(int32 Row, int32 Delta)
{
	// O som muda na hora e ja fica salvo (como nas opcoes de som do cliente original)
	switch (Row)
	{
	case 0:
		Gfx.Volume = FMath::Clamp(Gfx.Volume + Delta, 0, 10);
		GConfig->SetInt(MythGfxSection, TEXT("Volume"), Gfx.Volume, GGameUserSettingsIni);
		ApplyMythGraphics();
		break;
	case 1: SndMusic = FMath::Clamp(SndMusic + Delta, 0, 10); break;
	case 2: SndEffects = FMath::Clamp(SndEffects + Delta, 0, 10); break;
	case 3:
		SndUIScale = FMath::Clamp(SndUIScale + Delta * 0.1f, 0.8f, 2.5f);
		GConfig->SetFloat(TEXT("Myth.UI"), TEXT("Scale"), SndUIScale, GGameUserSettingsIni);
		break;
	default:
		return;
	}
	UMythRoSound::SetVolumes(SndMusic / 10.f, SndEffects / 10.f); // tambem grava o ini
	RefreshSoundWindow();
}

void UMythRoHUDWidget::RefreshGraphicsWindow()
{
	if (!GraphicsWindow || GfxValues.Num() < GfxRowCount)
	{
		return;
	}
	static const TCHAR* Modes[3] = { TEXT("Window"), TEXT("Fullscreen (borderless)"), TEXT("Tela cheia") };
	static const TCHAR* AA[4] = { TEXT("None"), TEXT("FXAA (recomendado)"), TEXT("TAA"), TEXT("TSR") };
	static const TCHAR* Fps[5] = { TEXT("30"), TEXT("60"), TEXT("120"), TEXT("144"), TEXT("Unlimited") };
	const FIntPoint Res = Gfx.Resolutions.IsValidIndex(Gfx.ResolutionIndex) ? Gfx.Resolutions[Gfx.ResolutionIndex] : FIntPoint(1600, 900);
	const FString Values[GfxRowCount] = {
		Modes[FMath::Clamp(Gfx.WindowMode, 0, 2)],
		FString::Printf(TEXT("%d x %d"), Res.X, Res.Y),
		QualityNames()[FMath::Clamp(Gfx.Quality, 0, 4)],
		QualityNames()[FMath::Clamp(Gfx.Shadows, 0, 4)],
		Gfx.Lumen ? TEXT("Ligada") : TEXT("Desligada"),
		AA[FMath::Clamp(Gfx.AntiAliasing, 0, 3)],
		FString::Printf(TEXT("%d%%"), Gfx.ResolutionScale * 10),
		Fps[FMath::Clamp(Gfx.FpsIndex, 0, 4)],
		Gfx.bVSync ? TEXT("Ligado") : TEXT("Desligado") };
	for (int32 i = 0; i < GfxRowCount; ++i)
	{
		GfxValues[i]->SetText(FText::FromString(Values[i]));
	}
	GfxHint->SetText(FText::FromString(bGfxDirty ? TEXT("Click Apply to save.") : TEXT("")));
}

void UMythRoHUDWidget::StepGraphicsOption(int32 Row, int32 Delta)
{
	auto Cycle = [Delta](int32& V, int32 Count) { V = (V + Delta + Count) % Count; };
	switch (Row)
	{
	case 0: Cycle(Gfx.WindowMode, 3); break;
	case 1: Cycle(Gfx.ResolutionIndex, FMath::Max(1, Gfx.Resolutions.Num())); break;
	case 2: Cycle(Gfx.Quality, 5); Gfx.Shadows = Gfx.Quality; break; // a qualidade geral leva as sombras junto
	case 3: Cycle(Gfx.Shadows, 5); break;
	case 4: Gfx.Lumen = Gfx.Lumen ? 0 : 1; break;
	case 5: Cycle(Gfx.AntiAliasing, 4); break;
	case 6: Gfx.ResolutionScale = FMath::Clamp(Gfx.ResolutionScale + Delta, 5, 10); break;
	case 7: Cycle(Gfx.FpsIndex, 5); break;
	case 8: Gfx.bVSync = !Gfx.bVSync; break;
	default: return;
	}
	bGfxDirty = true;
	RefreshGraphicsWindow();
}

bool UMythRoHUDWidget::ToggleGameMenu()
{
	if (!GameMenuWindow)
	{
		return false;
	}
	const bool bOpen = GameMenuWindow->GetVisibility() == ESlateVisibility::Collapsed;
	GameMenuWindow->SetVisibility(bOpen ? ESlateVisibility::SelfHitTestInvisible : ESlateVisibility::Collapsed);
	if (bOpen)
	{
		OpenOrder.Remove(GameMenuWindow);
		OpenOrder.Add(GameMenuWindow);
	}
	return bOpen;
}

void UMythRoHUDWidget::ShowDeathWindow()
{
	if (DeathWindow)
	{
		DeathWindow->SetVisibility(ESlateVisibility::SelfHitTestInvisible);
		OpenOrder.Remove(DeathWindow);
		OpenOrder.Add(DeathWindow);
	}
}

void UMythRoHUDWidget::HideDeathWindow()
{
	if (DeathWindow)
	{
		DeathWindow->SetVisibility(ESlateVisibility::Collapsed);
	}
}

bool UMythRoHUDWidget::HandleGameMenuClick(const FVector2D& ScreenPos)
{
	if (DeathWindow && DeathWindow->IsVisible())
	{
		if (IsUnder(DeathSaveLink, ScreenPos))
		{
			if (URagnarokNetworkSubsystem* Net = GetGameInstance() ? GetGameInstance()->GetSubsystem<URagnarokNetworkSubsystem>() : nullptr)
			{
				Net->SendRestart(0); // o rAthena leva ao ponto de salvamento e manda o novo mapa
			}
			DeathWindow->SetVisibility(ESlateVisibility::Collapsed);
			return true;
		}
		if (IsUnder(DeathWaitLink, ScreenPos))
		{
			DeathWindow->SetVisibility(ESlateVisibility::Collapsed);
			return true;
		}
	}
	if (GameMenuWindow && GameMenuWindow->IsVisible())
	{
		for (int32 i = 0; i < GameMenuItems.Num(); ++i)
		{
			if (!IsUnder(GameMenuItems[i], ScreenPos))
			{
				continue;
			}
			GameMenuWindow->SetVisibility(ESlateVisibility::Collapsed);
			switch (i)
			{
			case 0:
				ReturnToCharacterSelect();
				break;
			case 1: // Game settings: abas Efeito, Controle, Outros e o link para graficos e som
				ShowOptionsWindow(0);
				break;
			case 2: // Set hotkeys: a lista de atalhos (Alt+M)
				ToggleWindow(EMythRoWindow::Macros);
				break;
			case 3:
				UKismetSystemLibrary::QuitGame(this, GetOwningPlayer(), EQuitPreference::Quit, false);
				break;
			default:
				break; // voltar ao jogo
			}
			return true;
		}
	}
	if (GraphicsWindow && GraphicsWindow->IsVisible())
	{
		for (int32 i = 0; i < GfxRowCount && i < GfxLeft.Num(); ++i)
		{
			if (IsUnder(GfxLeft[i], ScreenPos)) { StepGraphicsOption(i, -1); return true; }
			if (IsUnder(GfxRight[i], ScreenPos) || IsUnder(GfxValues[i], ScreenPos)) { StepGraphicsOption(i, 1); return true; }
		}
		if (IsUnder(GfxApply, ScreenPos))
		{
			ApplyGraphicsOptions();
			bGfxDirty = false;
			RefreshGraphicsWindow();
			return true;
		}
		if (IsUnder(GfxClose, ScreenPos))
		{
			GraphicsWindow->SetVisibility(ESlateVisibility::Collapsed);
			return true;
		}
		if (HandleSettingsTab(ScreenPos))
		{
			return true;
		}
		if (GfxTabOptions && IsUnder(GfxTabOptions, ScreenPos))
		{
			ShowOptionsWindow(0);
			return true;
		}
		if (IsUnder(GfxTabOther, ScreenPos))
		{
			GraphicsWindow->SetVisibility(ESlateVisibility::Collapsed);
			RefreshSoundWindow();
			SoundWindow->SetVisibility(ESlateVisibility::SelfHitTestInvisible);
			OpenOrder.Remove(SoundWindow);
			OpenOrder.Add(SoundWindow);
			return true;
		}
	}
	if (SoundWindow && SoundWindow->IsVisible())
	{
		for (int32 i = 0; i < SndRowCount && i < SndLeft.Num(); ++i)
		{
			if (IsUnder(SndLeft[i], ScreenPos)) { StepSoundOption(i, -1); return true; }
			if (IsUnder(SndRight[i], ScreenPos) || IsUnder(SndValues[i], ScreenPos)) { StepSoundOption(i, 1); return true; }
		}
		if (IsUnder(SndClose, ScreenPos))
		{
			SoundWindow->SetVisibility(ESlateVisibility::Collapsed);
			return true;
		}
		if (HandleSettingsTab(ScreenPos))
		{
			return true;
		}
		if (SndTabOptions && IsUnder(SndTabOptions, ScreenPos))
		{
			ShowOptionsWindow(0);
			return true;
		}
		if (IsUnder(SndTabOther, ScreenPos))
		{
			SoundWindow->SetVisibility(ESlateVisibility::Collapsed);
			RefreshGraphicsWindow();
			GraphicsWindow->SetVisibility(ESlateVisibility::SelfHitTestInvisible);
			OpenOrder.Remove(GraphicsWindow);
			OpenOrder.Add(GraphicsWindow);
			return true;
		}
	}
	return false;
}

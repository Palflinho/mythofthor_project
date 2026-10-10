// Menu do jogo (Esc sem janela aberta): continuar, configuracoes graficas, som e sair. As opcoes graficas sao as de
// jogos atuais (modo de tela, resolucao, qualidade, sombras, Lumen, anti-aliasing, escala, FPS, V-Sync) e ficam
// salvas pelo UGameUserSettings (GameUserSettings.ini) + [Myth.Graphics] para as que a Unreal nao guarda.

#include "UI/MythRoHUDWidget.h"

#include "UI/MythRoUITextures.h"

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

	FString Basic(const TCHAR* Name) { return FString::Printf(TEXT("basic_interface/%s.bmp"), Name); }

	void SetCVar(const TCHAR* Name, int32 Value)
	{
		if (IConsoleVariable* V = IConsoleManager::Get().FindConsoleVariable(Name))
		{
			V->Set(Value, ECVF_SetByGameSetting);
		}
	}

	const TArray<FString>& QualityNames()
	{
		static const TArray<FString> N = { TEXT("Baixa"), TEXT("Média"), TEXT("Alta"), TEXT("Épica"), TEXT("Cinemática") };
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
	// Menu principal (como a janela de "Sair" do RO, com as opcoes de hoje)
	const FVector2D MenuSize(200.f, 17.f + 4 * 24.f + 12.f);
	GameMenuWindow = MakeRoPanel(TEXT("Menu"), MenuSize, FVector2D::ZeroVector);
	const TCHAR* Items[4] = { TEXT("Voltar ao jogo"), TEXT("Configurações gráficas"), TEXT("Som e interface"), TEXT("Sair do jogo") };
	for (int32 i = 0; i < 4; ++i)
	{
		UTextBlock* T = AddText(GameMenuWindow, 18.f, 26.f + i * 24.f, 9, MenuLink);
		T->SetText(FText::FromString(Items[i]));
		GameMenuItems.Add(T);
	}
	GameMenuWindow->SetVisibility(ESlateVisibility::Collapsed);

	// Configuracoes graficas: "rotulo   <  valor  >" por linha
	const TCHAR* Labels[GfxRowCount] = { TEXT("Modo de tela"), TEXT("Resolução"), TEXT("Qualidade geral"), TEXT("Sombras"),
		TEXT("Iluminação (Lumen)"), TEXT("Anti-aliasing"), TEXT("Escala de resolução"), TEXT("Limite de FPS"), TEXT("V-Sync"), TEXT("Volume") };
	const FVector2D Size(380.f, 17.f + GfxRowCount * 24.f + 44.f);
	GraphicsWindow = MakeRoPanel(TEXT("Configurações"), Size, FVector2D::ZeroVector);
	UTexture2D* Left = FMythRoUITextures::Get(Basic(TEXT("arw_left")));
	UTexture2D* Right = FMythRoUITextures::Get(Basic(TEXT("arw_right")));
	for (int32 i = 0; i < GfxRowCount; ++i)
	{
		const float Y = 26.f + i * 24.f;
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
	GfxClose = AddText(GraphicsWindow, Size.X - 60.f, BY + 4.f, 9, MenuLink);
	GfxClose->SetText(FText::FromString(TEXT("Fechar")));
	GfxHint = AddText(GraphicsWindow, 14.f, BY + 4.f, 7, FLinearColor(0.4f, 0.4f, 0.4f));
	GraphicsWindow->SetVisibility(ESlateVisibility::Collapsed);
	LoadGraphicsOptions();
}

void UMythRoHUDWidget::RefreshGraphicsWindow()
{
	if (!GraphicsWindow || GfxValues.Num() < GfxRowCount)
	{
		return;
	}
	static const TCHAR* Modes[3] = { TEXT("Janela"), TEXT("Tela cheia (sem borda)"), TEXT("Tela cheia") };
	static const TCHAR* AA[4] = { TEXT("Nenhum"), TEXT("FXAA"), TEXT("TAA"), TEXT("TSR (recomendado)") };
	static const TCHAR* Fps[5] = { TEXT("30"), TEXT("60"), TEXT("120"), TEXT("144"), TEXT("Sem limite") };
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
		Gfx.bVSync ? TEXT("Ligado") : TEXT("Desligado"),
		FString::Printf(TEXT("%d%%"), Gfx.Volume * 10) };
	for (int32 i = 0; i < GfxRowCount; ++i)
	{
		GfxValues[i]->SetText(FText::FromString(Values[i]));
	}
	GfxHint->SetText(FText::FromString(bGfxDirty ? TEXT("Clique em Aplicar para salvar.") : TEXT("")));
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
	case 9: Gfx.Volume = FMath::Clamp(Gfx.Volume + Delta, 0, 10); ApplyMythGraphics(); break; // volume muda na hora
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

bool UMythRoHUDWidget::HandleGameMenuClick(const FVector2D& ScreenPos)
{
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
			case 1:
			case 2:
				LoadGraphicsOptions();
				bGfxDirty = false;
				RefreshGraphicsWindow();
				GraphicsWindow->SetVisibility(ESlateVisibility::SelfHitTestInvisible);
				OpenOrder.Remove(GraphicsWindow);
				OpenOrder.Add(GraphicsWindow);
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
	}
	return false;
}

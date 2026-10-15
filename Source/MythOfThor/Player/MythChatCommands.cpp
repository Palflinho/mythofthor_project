// Comandos de barra do cliente que agem no mundo (docs/ref/18 3.1 e 3.2). Os que so mexem em texto ou na rede
// (/w, /r, %, emocoes) ficam no widget do chat.

#include "Player/MythPlayerController.h"

#include "Network/RagnarokNetworkSubsystem.h"
#include "Player/MythPlayerCharacter.h"
#include "Sprites/MythAura99.h"
#include "Sprites/MythRoSound.h"
#include "Sprites/MythRoSpriteComponent.h"
#include "UI/MythRoHUDWidget.h"
#include "World/MythMapInfo.h"
#include "World/MythWeatherActor.h"
#include "World/MythDayNightActor.h"

bool AMythPlayerController::RunSlashCommand(const FString& Command, const FString& Args)
{
	const FString Cmd = Command.ToLower();
	auto Say = [this](const FString& Text)
	{
		if (HudWidget)
		{
			HudWidget->AddChatLine(Text, FLinearColor(0.7f, 0.9f, 1.f));
		}
	};
	AMythPlayerCharacter* Char = GetMythCharacter();
	URagnarokNetworkSubsystem* Net = GetGameInstance() ? GetGameInstance()->GetSubsystem<URagnarokNetworkSubsystem>() : nullptr;

	if (Cmd == TEXT("sit") || Cmd == TEXT("stand"))
	{
		OnHotkeySit();
		return true;
	}
	if (Cmd == TEXT("resetchar") || Cmd == TEXT("resetpersonagem"))
	{
		// Ferramenta de GM para testar a progressao: volta a Aprendiz nivel 1/1, sem pontos nem skills (so quem e GM no servidor)
		if (Net)
		{
			for (const TCHAR* Gm : { TEXT("@job 0"), TEXT("@blvl -98"), TEXT("@jlevel -98"), TEXT("@reset"), TEXT("@heal") })
			{
				Net->SendGlobalMessage(Gm);
			}
			Say(TEXT("Character reset: Novice 1/1 (GM commands sent)."));
		}
		return true;
	}
	if (Cmd == TEXT("where"))
	{
		const AMythMapInfo* Info = AMythMapInfo::Find(GetWorld());
		if (Info && Char)
		{
			const FIntPoint Cell = Info->WorldToCell(Char->GetActorLocation());
			Say(FString::Printf(TEXT("%s : %d, %d"), *Info->MapName, Cell.X, Cell.Y));
		}
		return true;
	}
	if (Cmd == TEXT("who"))
	{
		if (Net) { Net->SendGlobalMessage(TEXT("@who")); }
		return true;
	}
	if (Cmd == TEXT("noctrl") || Cmd == TEXT("nc"))
	{
		bContinuousAttack = !bContinuousAttack;
		Say(bContinuousAttack ? TEXT("Continuous attack: on.") : TEXT("Continuous attack: off (one hit per click)."));
		return true;
	}
	if (Cmd == TEXT("noshift") || Cmd == TEXT("ns"))
	{
		Say(TEXT("/noshift: support spells can already be cast without holding Shift."));
		return true;
	}
	if (Cmd == TEXT("doridori"))
	{
		bAutoDoridori = !bAutoDoridori;
		DoridoriTimer = 0.f;
		if (!bAutoDoridori && Char && Char->Sprite)
		{
			SetHeadTurn(0);
		}
		return true;
	}
	if ((Cmd == TEXT("bangbang") || Cmd == TEXT("bingbing")) && Char && Char->Sprite && Net)
	{
		// 45 graus: bangbang horario, bingbing anti-horario (direcoes do rAthena crescem no sentido anti-horario)
		const int32 Cur = Char->Sprite->GetFacingRoDir();
		const int32 Next = (Cur + (Cmd == TEXT("bangbang") ? 7 : 1)) % 8;
		Char->Sprite->SetFacingRoDir(static_cast<uint8>(Next));
		Net->SendChangeDirection(Char->Sprite->GetHeadDir(), static_cast<uint8>(Next));
		return true;
	}
	if (Cmd == TEXT("aura"))
	{
		// /aura alterna entre simplificada (1) e normal (2)
		AMythAura99::SetMode(AMythAura99::GetMode() == 2 ? 1 : 2);
		Say(AMythAura99::GetMode() == 2 ? TEXT("The aura effect is normal.") : TEXT("The aura effect is reduced."));
		return true;
	}
	if (Cmd == TEXT("aura2"))
	{
		AMythAura99::SetMode(AMythAura99::GetMode() == 0 ? 2 : 0);
		Say(AMythAura99::GetMode() == 0 ? TEXT("The aura effect is off.") : TEXT("The aura effect is on."));
		return true;
	}
	if (Cmd == TEXT("sound") || Cmd == TEXT("bgm"))
	{
		static float SavedMusic = -1.f, SavedEffects = -1.f;
		const bool bSound = Cmd == TEXT("sound");
		float Music = UMythRoSound::GetMusicVolume();
		float Effects = UMythRoSound::GetEffectsVolume();
		float& Saved = bSound ? SavedEffects : SavedMusic;
		float& Current = bSound ? Effects : Music;
		if (Current > 0.f) { Saved = Current; Current = 0.f; } else { Current = Saved > 0.f ? Saved : 1.f; }
		UMythRoSound::SetVolumes(Music, Effects);
		Say(FString::Printf(TEXT("%s: %s."), bSound ? TEXT("Sound effects") : TEXT("Music"), Current > 0.f ? TEXT("on") : TEXT("off")));
		return true;
	}
	if (Cmd == TEXT("rain") || Cmd == TEXT("chuva"))
	{
		if (AMythWeatherActor* W = AMythWeatherActor::FindOrSpawn(GetWorld()))
		{
			W->SetWeather(EMythWeather::Rain);
			Say(TEXT("Clima: Chuva ativada (gotas, chão molhado e reflexos)."));
		}
		return true;
	}
	if (Cmd == TEXT("snow") || Cmd == TEXT("neve"))
	{
		if (AMythWeatherActor* W = AMythWeatherActor::FindOrSpawn(GetWorld()))
		{
			W->SetWeather(EMythWeather::Snow);
			Say(TEXT("Clima: Neve ativada."));
		}
		return true;
	}
	if (Cmd == TEXT("clear") || Cmd == TEXT("limpo") || Cmd == TEXT("sun") || Cmd == TEXT("sol"))
	{
		if (AMythWeatherActor* W = AMythWeatherActor::FindOrSpawn(GetWorld()))
		{
			W->SetWeather(EMythWeather::Clear);
			Say(TEXT("Clima: Tempo limpo e ensolarado ativado."));
		}
		return true;
	}
	if (Cmd == TEXT("day") || Cmd == TEXT("dia"))
	{
		if (AMythDayNightActor* DN = AMythDayNightActor::Find(GetWorld()))
		{
			DN->SetTimeOfDay(12.f);
			Say(TEXT("Horário: 12:00 (Pleno dia / sol a pino)."));
		}
		return true;
	}
	if (Cmd == TEXT("night") || Cmd == TEXT("noite"))
	{
		if (AMythDayNightActor* DN = AMythDayNightActor::Find(GetWorld()))
		{
			DN->SetTimeOfDay(0.f);
			Say(TEXT("Horário: 00:00 (Meia-noite / tochas, lamparinas e iluminação ativadas)."));
		}
		return true;
	}
	if (Cmd == TEXT("time") || Cmd == TEXT("hora"))
	{
		if (AMythDayNightActor* DN = AMythDayNightActor::Find(GetWorld()))
		{
			if (!Args.IsEmpty())
			{
				const float H = FCString::Atof(*Args);
				DN->SetTimeOfDay(FMath::Clamp(H, 0.f, 24.f));
				Say(FString::Printf(TEXT("Horário ajustado para: %.1fh."), H));
			}
			else
			{
				Say(FString::Printf(TEXT("Horário atual: %.1fh (Fator noite: %.2f). Use /hora <0-24>."), DN->GetTimeOfDay(), DN->GetNightFactor()));
			}
		}
		return true;
	}
	return false;
}

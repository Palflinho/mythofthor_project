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
		Say(bContinuousAttack ? TEXT("Ataque contínuo: ligado.") : TEXT("Ataque contínuo: desligado (um golpe por clique)."));
		return true;
	}
	if (Cmd == TEXT("noshift") || Cmd == TEXT("ns"))
	{
		Say(TEXT("/noshift: as magias de suporte já podem ser lançadas sem segurar Shift."));
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
		Say(AMythAura99::GetMode() == 2 ? TEXT("O efeito de aura está normal.") : TEXT("O efeito de aura está reduzido."));
		return true;
	}
	if (Cmd == TEXT("aura2"))
	{
		AMythAura99::SetMode(AMythAura99::GetMode() == 0 ? 2 : 0);
		Say(AMythAura99::GetMode() == 0 ? TEXT("O efeito de aura está desligado.") : TEXT("O efeito de aura está ligado."));
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
		Say(FString::Printf(TEXT("%s: %s."), bSound ? TEXT("Efeitos sonoros") : TEXT("Música"), Current > 0.f ? TEXT("ligados") : TEXT("desligados")));
		return true;
	}
	return false;
}

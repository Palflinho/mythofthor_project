// Janela "Game settings" do cliente LATAM (docs/ref/hud_latam/INDICE.md): abas Efeito, Controle e Outros com chaves ON/OFF
// e botoes "aplicar" para os comandos de barra. Gráficos e Som seguem nas janelas de Configuracoes (MythRoGameMenu.cpp);
// as abas das tres janelas se alternam.

#include "UI/MythRoHUDWidget.h"

#include "Sprites/MythAura99.h"
#include "UI/MythOptions.h"
#include "World/MythWater.h"

#include "Components/CanvasPanel.h"
#include "Components/TextBlock.h"
#include "GameFramework/PlayerController.h"

namespace
{
	const FLinearColor OptText(0.12f, 0.12f, 0.12f);
	const FLinearColor OptLink(0.12f, 0.3f, 0.65f);
	const FLinearColor OptOn(0.1f, 0.55f, 0.2f);
	const FLinearColor OptOff(0.55f, 0.55f, 0.55f);
	const FLinearColor OptGray(0.4f, 0.4f, 0.4f);
	constexpr float OptW = 470.f;
	constexpr float OptRowH = 22.f;
	constexpr float OptListY = 46.f;

	struct FOptionDef
	{
		const TCHAR* Key;     // chave em FMythOptions, ou o comando de barra quando bApply
		const TCHAR* Label;
		const TCHAR* Desc;
		bool bApply;
	};

	const TArray<FOptionDef>& OptionsOfTab(int32 Tab)
	{
		static const TArray<FOptionDef> Effect = {
			{ TEXT("ShowAura"), TEXT("Show aura"), TEXT("Shows the aura (level 99)."), false },
			{ TEXT("AuraDisplay"), TEXT("Simplified aura"), TEXT("Shows the aura in a simplified way."), false },
			{ TEXT("ShowEffects"), TEXT("Show effects"), TEXT("Shows skill and item effects."), false },
			{ TEXT("ModernWater"), TEXT("Modern water"), TEXT("Water with reflections, waves and depth (off: classic RO water)."), false },
			{ TEXT("ShowShading"), TEXT("Show shading"), TEXT("Shows shading."), false },
			{ TEXT("DamageDisplay"), TEXT("Extended damage"), TEXT("Shows damage in extended format."), false },
			{ TEXT("MapName"), TEXT("Map name"), TEXT("Shows the map name when entering an area."), false },
			{ TEXT("Fog"), TEXT("Fog effect"), TEXT("Shows fog effects."), false },
			{ TEXT("Footprint"), TEXT("Footprints"), TEXT("Shows footprints of other players."), false },
			{ TEXT("ScreenShake"), TEXT("Screen shake"), TEXT("The screen shakes with powerful skills."), false },
			{ TEXT("HideSummons"), TEXT("Hide summons"), TEXT("Summons become semi-transparent."), false },
			{ TEXT("HidePlayers"), TEXT("Hide players"), TEXT("Other players become semi-transparent."), false },
		};
		static const TArray<FOptionDef> Control = {
			{ TEXT("MonsterSnap"), TEXT("Monster target"), TEXT("Targets monsters when the cursor is close."), false },
			{ TEXT("SkillSnap"), TEXT("Skill target"), TEXT("The cursor snaps to a nearby target when using skills."), false },
			{ TEXT("ItemSnap"), TEXT("Item snap"), TEXT("The cursor snaps to nearby ground items."), false },
			{ TEXT("NoCtrl"), TEXT("No Ctrl"), TEXT("Attacks monsters continuously without holding Ctrl."), false },
			{ TEXT("NoShift"), TEXT("No Shift"), TEXT("No need to hold Shift to attack monsters."), false },
			{ TEXT("ZoomOut"), TEXT("Zoom out"), TEXT("Shows a wider view when zooming out."), false },
			{ TEXT("MouseLock"), TEXT("Lock mouse"), TEXT("Keeps the mouse from leaving the game window."), false },
			{ TEXT("QuickSkill1"), TEXT("Quick skill 1"), TEXT("Uses the F9 skill with the right button."), false },
			{ TEXT("QuickSkill2"), TEXT("Quick skill 2"), TEXT("Uses the F7 and F8 skills with the mouse wheel."), false },
			{ TEXT("MiniSlot"), TEXT("Mini bar"), TEXT("Shows a mini bar with 2 slots."), false },
			{ TEXT("EquipCompare"), TEXT("Compare equipment"), TEXT("Shows a comparison window with the equipped item."), false },
		};
		static const TArray<FOptionDef> Other = {
			// Server commands as toggles (docs/sistemas/05_hud_sem_comandos.md): the client sends ,  and  for you
			{ TEXT("AutoLoot"), TEXT("Auto-loot"), TEXT("Picks up drops automatically (server command @autoloot)."), false },
			{ TEXT("ShowExp"), TEXT("Show EXP gains"), TEXT("Shows the EXP you gain in the chat (@showexp)."), false },
			{ TEXT("NoAsk"), TEXT("No requests"), TEXT("Declines party, trade and guild requests automatically (@noask)."), false },
			{ TEXT("@rates"), TEXT("Server rates"), TEXT("Click: shows the EXP and drop rates in the chat."), true },
			{ TEXT("@uptime"), TEXT("Server uptime"), TEXT("Click: shows how long the server has been up."), true },
			{ TEXT("NoTrade"), TEXT("/notrade"), TEXT("Automatically declines trades and friend requests."), false },
			{ TEXT("NoTalkMsg"), TEXT("/notalkmsg"), TEXT("Does not show chat messages."), false },
			{ TEXT("ShowName"), TEXT("/showname"), TEXT("Uses a thinner font and shows guild and party."), false },
			{ TEXT("Window"), TEXT("/window"), TEXT("Windows snap together when moved."), false },
			{ TEXT("Miss"), TEXT("/miss"), TEXT("Shows 'miss' when an attack fails."), false },
			{ TEXT("Shopping"), TEXT("/shopping"), TEXT("Opens shops with one click."), false },
			{ TEXT("Stateinfo"), TEXT("/stateinfo"), TEXT("Shows the description of status icons."), false },
			{ TEXT("Camera"), TEXT("/camera"), TEXT("Locks the character at the center of the screen."), false },
			{ TEXT("MonsterHp"), TEXT("/monsterhp"), TEXT("Shows the HP of attacked monsters."), false },
			{ TEXT("Minimap"), TEXT("/minimap"), TEXT("Shows buttons below the minimap."), false },
			{ TEXT("MinEffect"), TEXT("/mineffect"), TEXT("Shows simplified skill effects."), false },
			{ TEXT("Emblem"), TEXT("/emblem"), TEXT("Shows emblem images."), false },
			{ TEXT("Font"), TEXT("/font"), TEXT("Moves the character name above the head."), false },
			{ TEXT("ShowShop"), TEXT("/showshop"), TEXT("Shows merchant shops."), false },
			{ TEXT("CardIllust"), TEXT("/cardillust"), TEXT("Shows the card illustration in the item details."), false },
			{ TEXT("/hi"), TEXT("/hi"), TEXT("Greets friends."), true },
			{ TEXT("/sit"), TEXT("/sit"), TEXT("Sit or stand."), true },
			{ TEXT("/leave"), TEXT("/leave"), TEXT("Leaves the party."), true },
			{ TEXT("/where"), TEXT("/where"), TEXT("Shows the current location with coordinates."), true },
			{ TEXT("/memo"), TEXT("/memo"), TEXT("Saves the current location as a teleport point."), true },
			{ TEXT("/doridori"), TEXT("/doridori"), TEXT("The character shakes their head."), true },
			{ TEXT("/bangbang"), TEXT("/bangbang"), TEXT("The character turns left."), true },
			{ TEXT("/bingbing"), TEXT("/bingbing"), TEXT("The character turns right."), true },
			{ TEXT("/noctrl"), TEXT("/noctrl"), TEXT("Turns continuous attack on and off."), true },
		};
		return Tab == 0 ? Effect : (Tab == 1 ? Control : Other);
	}

	const TCHAR* TabNames[3] = { TEXT("Effects"), TEXT("Controls"), TEXT("Other") };
}

void UMythRoHUDWidget::BuildOptionsWindow()
{
	const float H = OptListY + OptRowCount * OptRowH + 40.f;
	OptionsWindow = MakeRoPanel(TEXT("Game options"), FVector2D(OptW, H), FVector2D(0.f, 0.f));
	AddSettingsTabs(OptionsWindow);
	for (int32 i = 0; i < OptRowCount; ++i)
	{
		const float Y = OptListY + i * OptRowH;
		OptRowName.Add(AddText(OptionsWindow, 14.f, Y, 8, OptText));
		OptRowToggle.Add(AddText(OptionsWindow, 150.f, Y, 8, OptOff));
		OptRowDesc.Add(AddText(OptionsWindow, 205.f, Y, 7, OptGray));
	}
	OptClose = AddButtonHit(OptionsWindow, TEXT("close"), OptW - 56.f, H - 30.f);
	OptionsWindow->SetVisibility(ESlateVisibility::Collapsed);
	ApplyOptionEffects();
}

void UMythRoHUDWidget::ApplyOptionEffects()
{
	// Aura: Mostrar aura desligada = nada; ligada com "simplificada" = so os brilhos; ligada = normal
	AMythWater::ApplyStyleToAll(GetWorld());
	AMythAura99::SetMode(!FMythOptions::Get(TEXT("ShowAura")) ? 0 : (FMythOptions::Get(TEXT("AuraDisplay")) ? 1 : 2));
}

void UMythRoHUDWidget::SyncServerOptions(bool bForce)
{
	// AutoLoot, ShowExp and NoAsk live on the server and reset at every login: send the saved choice once per session and then on each click
	URagnarokNetworkSubsystem* Net = GetNet();
	if (!Net || Net->GetNetworkState() != ERagnarokNetworkState::ConnectedMap)
	{
		return;
	}
	if (bForce)
	{
		ServerOptState.Empty();
	}
	struct FSrv { const TCHAR* Key; const TCHAR* On; const TCHAR* Off; };
	static const FSrv Defs[] = { { TEXT("AutoLoot"), TEXT("@autoloot 100"), TEXT("@autoloot 0") }, { TEXT("ShowExp"), TEXT("@showexp"), TEXT("@showexp") }, { TEXT("NoAsk"), TEXT("@noask"), TEXT("@noask") } };
	for (const FSrv& D : Defs)
	{
		const bool bWant = FMythOptions::Get(D.Key);
		const bool bHave = ServerOptState.FindRef(FName(D.Key));
		if (bWant != bHave)
		{
			Net->SendGlobalMessage(bWant ? D.On : D.Off);
			ServerOptState.Add(FName(D.Key), bWant);
		}
	}
}

void UMythRoHUDWidget::ShowOptionsWindow(int32 Tab)
{
	if (!OptionsWindow)
	{
		return;
	}
	OptTab = FMath::Clamp(Tab, 0, 2);
	OptScroll = 0;
	if (GraphicsWindow) { GraphicsWindow->SetVisibility(ESlateVisibility::Collapsed); }
	if (SoundWindow) { SoundWindow->SetVisibility(ESlateVisibility::Collapsed); }
	OptionsWindow->SetVisibility(ESlateVisibility::SelfHitTestInvisible);
	OpenOrder.Remove(OptionsWindow);
	OpenOrder.Add(OptionsWindow);
	RefreshOptions();
}

void UMythRoHUDWidget::RefreshOptions()
{
	if (!OptionsWindow)
	{
		return;
	}
	const TArray<FOptionDef>& Defs = OptionsOfTab(OptTab);
	OptScroll = FMath::Clamp(OptScroll, 0, FMath::Max(0, Defs.Num() - OptRowCount));
	for (int32 t = 0; t < OptTabTexts.Num(); ++t)
	{
		OptTabTexts[t]->SetText(FText::FromString(t == OptTab ? FString::Printf(TEXT("[ %s ]"), TabNames[t]) : FString(TabNames[t])));
		OptTabTexts[t]->SetColorAndOpacity(FSlateColor(t == OptTab ? OptText : OptLink));
	}
	for (int32 i = 0; i < OptRowCount; ++i)
	{
		const FOptionDef* D = Defs.IsValidIndex(OptScroll + i) ? &Defs[OptScroll + i] : nullptr;
		const ESlateVisibility Vis = D ? ESlateVisibility::Visible : ESlateVisibility::Collapsed;
		OptRowName[i]->SetVisibility(Vis);
		OptRowToggle[i]->SetVisibility(Vis);
		OptRowDesc[i]->SetVisibility(Vis);
		if (!D)
		{
			continue;
		}
		OptRowName[i]->SetText(FText::FromString(D->Label));
		OptRowDesc[i]->SetText(FText::FromString(D->Desc));
		if (D->bApply)
		{
			OptRowToggle[i]->SetText(FText::FromString(TEXT("[apply]")));
			OptRowToggle[i]->SetColorAndOpacity(FSlateColor(OptLink));
		}
		else
		{
			const bool bOn = FMythOptions::Get(D->Key);
			OptRowToggle[i]->SetText(FText::FromString(bOn ? TEXT("[ ON ]") : TEXT("[ OFF ]")));
			OptRowToggle[i]->SetColorAndOpacity(FSlateColor(bOn ? OptOn : OptOff));
		}
	}
}

bool UMythRoHUDWidget::HandleOptionsClick(const FVector2D& ScreenPos)
{
	if (!OptionsWindow || !OptionsWindow->IsVisible())
	{
		return false;
	}
	if (IsUnder(OptClose, ScreenPos))
	{
		OptionsWindow->SetVisibility(ESlateVisibility::Collapsed);
		return true;
	}
	if (HandleSettingsTab(ScreenPos))
	{
		return true;
	}
	const TArray<FOptionDef>& Defs = OptionsOfTab(OptTab);
	for (int32 i = 0; i < OptRowCount; ++i)
	{
		if (!Defs.IsValidIndex(OptScroll + i) || !(IsUnder(OptRowToggle[i], ScreenPos) || IsUnder(OptRowName[i], ScreenPos)))
		{
			continue;
		}
		const FOptionDef& D = Defs[OptScroll + i];
		if (D.bApply)
		{
			SubmitChat(D.Key);
		}
		else
		{
			FMythOptions::Set(D.Key, !FMythOptions::Get(D.Key));
			ApplyOptionEffects();
			SyncServerOptions(false);
		}
		RefreshOptions();
		return true;
	}
	return false;
}

bool UMythRoHUDWidget::HandleOptionsWheel(const FVector2D& ScreenPos, float Delta)
{
	if (!OptionsWindow || !OptionsWindow->IsVisible() || !IsUnder(OptionsWindow, ScreenPos))
	{
		return false;
	}
	OptScroll += Delta > 0.f ? -1 : 1;
	RefreshOptions();
	return true;
}

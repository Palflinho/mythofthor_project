// Janela "Game settings" do cliente LATAM (docs/ref/hud_latam/INDICE.md): abas Efeito, Controle e Outros com chaves ON/OFF
// e botoes "aplicar" para os comandos de barra. Gráficos e Som seguem nas janelas de Configuracoes (MythRoGameMenu.cpp);
// as abas das tres janelas se alternam.

#include "UI/MythRoHUDWidget.h"

#include "Sprites/MythAura99.h"
#include "UI/MythOptions.h"

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
			{ TEXT("ShowAura"), TEXT("Mostrar aura"), TEXT("Mostra a aura (nível 99)."), false },
			{ TEXT("AuraDisplay"), TEXT("Aura simplificada"), TEXT("Mostra a aura de forma simplificada."), false },
			{ TEXT("ShowEffects"), TEXT("Mostrar efeitos"), TEXT("Mostra os efeitos de skills e itens."), false },
			{ TEXT("ShowShading"), TEXT("Mostrar sombreado"), TEXT("Mostra o sombreado."), false },
			{ TEXT("DamageDisplay"), TEXT("Dano estendido"), TEXT("Mostra o dano em formato estendido."), false },
			{ TEXT("MapName"), TEXT("Nome do mapa"), TEXT("Mostra o nome do mapa ao entrar em uma área."), false },
			{ TEXT("Fog"), TEXT("Efeito de névoa"), TEXT("Mostra os efeitos de névoa."), false },
			{ TEXT("Footprint"), TEXT("Pegadas"), TEXT("Mostra as pegadas de outros jogadores."), false },
			{ TEXT("ScreenShake"), TEXT("Tremor de tela"), TEXT("A tela treme com skills poderosas."), false },
			{ TEXT("HideSummons"), TEXT("Esconder invocações"), TEXT("As invocações ficam semitransparentes."), false },
			{ TEXT("HidePlayers"), TEXT("Esconder jogadores"), TEXT("Os outros jogadores ficam semitransparentes."), false },
		};
		static const TArray<FOptionDef> Control = {
			{ TEXT("MonsterSnap"), TEXT("Alvo em monstro"), TEXT("Mira os monstros quando o cursor está perto."), false },
			{ TEXT("SkillSnap"), TEXT("Alvo em skill"), TEXT("O cursor gruda no alvo próximo ao usar skills."), false },
			{ TEXT("ItemSnap"), TEXT("Alvo em item"), TEXT("O cursor gruda nos itens do chão próximos."), false },
			{ TEXT("NoCtrl"), TEXT("Sem Ctrl"), TEXT("Ataca monstros continuamente sem segurar Ctrl."), false },
			{ TEXT("NoShift"), TEXT("Sem Shift"), TEXT("Não precisa segurar Shift para atacar monstros."), false },
			{ TEXT("ZoomOut"), TEXT("Zoom afastado"), TEXT("Mostra uma visão mais ampla ao afastar."), false },
			{ TEXT("MouseLock"), TEXT("Travar mouse"), TEXT("Impede o mouse de sair da janela do jogo."), false },
			{ TEXT("QuickSkill1"), TEXT("Skill rápida 1"), TEXT("Usa a skill F9 com o botão direito."), false },
			{ TEXT("QuickSkill2"), TEXT("Skill rápida 2"), TEXT("Usa as skills F7 e F8 com a roda do mouse."), false },
			{ TEXT("MiniSlot"), TEXT("Mini barra"), TEXT("Mostra uma mini barra de 2 espaços."), false },
			{ TEXT("EquipCompare"), TEXT("Comparar equipamento"), TEXT("Mostra uma janela de comparação com o equipado."), false },
		};
		static const TArray<FOptionDef> Other = {
			{ TEXT("NoTrade"), TEXT("/notrade"), TEXT("Recusa automaticamente trocas e pedidos de amizade."), false },
			{ TEXT("NoTalkMsg"), TEXT("/notalkmsg"), TEXT("Não mostra mensagens de chat."), false },
			{ TEXT("ShowName"), TEXT("/showname"), TEXT("Usa fonte mais fina e mostra guilda e grupo."), false },
			{ TEXT("Window"), TEXT("/window"), TEXT("As janelas se encaixam ao serem movidas."), false },
			{ TEXT("Miss"), TEXT("/miss"), TEXT("Mostra 'miss' quando um ataque falha."), false },
			{ TEXT("Shopping"), TEXT("/shopping"), TEXT("Abre lojas com um clique."), false },
			{ TEXT("Stateinfo"), TEXT("/stateinfo"), TEXT("Mostra a descrição dos ícones de status."), false },
			{ TEXT("Camera"), TEXT("/camera"), TEXT("Trava o personagem no centro da tela."), false },
			{ TEXT("MonsterHp"), TEXT("/monsterhp"), TEXT("Mostra o PV dos monstros atacados."), false },
			{ TEXT("Minimap"), TEXT("/minimap"), TEXT("Mostra botões abaixo do minimapa."), false },
			{ TEXT("MinEffect"), TEXT("/mineffect"), TEXT("Mostra efeitos de skill simplificados."), false },
			{ TEXT("Emblem"), TEXT("/emblem"), TEXT("Mostra as imagens de emblema."), false },
			{ TEXT("Font"), TEXT("/font"), TEXT("Move o nome do personagem para cima da cabeça."), false },
			{ TEXT("ShowShop"), TEXT("/showshop"), TEXT("Mostra as lojas de mercadores."), false },
			{ TEXT("CardIllust"), TEXT("/cardillust"), TEXT("Mostra a ilustração da carta nos detalhes do item."), false },
			{ TEXT("/hi"), TEXT("/hi"), TEXT("Cumprimenta os amigos."), true },
			{ TEXT("/sit"), TEXT("/sit"), TEXT("Senta ou levanta."), true },
			{ TEXT("/leave"), TEXT("/leave"), TEXT("Sai do grupo."), true },
			{ TEXT("/where"), TEXT("/where"), TEXT("Mostra a localização atual com coordenadas."), true },
			{ TEXT("/memo"), TEXT("/memo"), TEXT("Guarda o local atual como ponto de teleporte."), true },
			{ TEXT("/doridori"), TEXT("/doridori"), TEXT("O personagem balança a cabeça."), true },
			{ TEXT("/bangbang"), TEXT("/bangbang"), TEXT("O personagem vira à esquerda."), true },
			{ TEXT("/bingbing"), TEXT("/bingbing"), TEXT("O personagem vira à direita."), true },
			{ TEXT("/noctrl"), TEXT("/noctrl"), TEXT("Liga e desliga o ataque contínuo."), true },
		};
		return Tab == 0 ? Effect : (Tab == 1 ? Control : Other);
	}

	const TCHAR* TabNames[3] = { TEXT("Efeito"), TEXT("Controle"), TEXT("Outros") };
}

void UMythRoHUDWidget::BuildOptionsWindow()
{
	const float H = OptListY + OptRowCount * OptRowH + 40.f;
	OptionsWindow = MakeRoPanel(TEXT("Opções do jogo"), FVector2D(OptW, H), FVector2D(0.f, 0.f));
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
	AMythAura99::SetMode(!FMythOptions::Get(TEXT("ShowAura")) ? 0 : (FMythOptions::Get(TEXT("AuraDisplay")) ? 1 : 2));
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
			OptRowToggle[i]->SetText(FText::FromString(TEXT("[aplicar]")));
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

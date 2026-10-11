// Janela de atalhos de teclado (F12, docs/ref/hud_latam/INDICE.md "Shortcut"): tabela funcao -> tecla com as teclas que o
// jogo realmente usa. So mostra; as teclas sao as ligadas em AMythPlayerController::SetupInputComponent.

#include "UI/MythRoHUDWidget.h"

#include "Components/CanvasPanel.h"
#include "Components/TextBlock.h"

namespace
{
	const FLinearColor ScText(0.12f, 0.12f, 0.12f);
	const FLinearColor ScKey(0.12f, 0.3f, 0.65f);
	const FLinearColor ScGray(0.55f, 0.55f, 0.55f);

	struct FShortcutRow { const TCHAR* Function; const TCHAR* Key; };
	const FShortcutRow ShortcutRows[] = {
		{ TEXT("Atributos"), TEXT("Alt+A") },
		{ TEXT("Itens"), TEXT("Alt+E") },
		{ TEXT("Equipamento"), TEXT("Alt+Q") },
		{ TEXT("Habilidades"), TEXT("Alt+S") },
		{ TEXT("Informação básica"), TEXT("Alt+V") },
		{ TEXT("Grupo"), TEXT("Alt+Z") },
		{ TEXT("Amigos"), TEXT("Alt+H") },
		{ TEXT("Sala de chat"), TEXT("Alt+C") },
		{ TEXT("Missões"), TEXT("Alt+U") },
		{ TEXT("Lista de emoções"), TEXT("Alt+L") },
		{ TEXT("Lista de atalhos"), TEXT("Alt+M") },
		{ TEXT("Conquistas"), TEXT("Alt+Y") },
		{ TEXT("Troca rápida de equipamento"), TEXT("Alt+W") },
		{ TEXT("Banco"), TEXT("Ctrl+B") },
		{ TEXT("Correio"), TEXT("Alt+R") },
		{ TEXT("Navegação (também /navi)"), TEXT("Alt+N") },
		{ TEXT("Minimapa"), TEXT("Ctrl+Tab") },
		{ TEXT("Sentar / levantar"), TEXT("Insert") },
		{ TEXT("Atalhos da barra"), TEXT("F1 a F9") },
		{ TEXT("Menu do jogo"), TEXT("Esc") },
		{ TEXT("Esta janela"), TEXT("F12") },
	};
}

void UMythRoHUDWidget::BuildShortcutWindow()
{
	constexpr float RowH = 17.f;
	const int32 Count = UE_ARRAY_COUNT(ShortcutRows);
	ShortcutWindow = MakeRoPanel(TEXT("Atalhos de teclado"), FVector2D(250.f, 17.f + 12.f + Count * RowH + 8.f), FVector2D(300.f, -150.f));
	for (int32 i = 0; i < Count; ++i)
	{
		const float Y = 24.f + i * RowH;
		AddText(ShortcutWindow, 14.f, Y, 8, ScText)->SetText(FText::FromString(ShortcutRows[i].Function));
		AddText(ShortcutWindow, 160.f, Y, 8, ScKey)->SetText(FText::FromString(ShortcutRows[i].Key));
	}
}

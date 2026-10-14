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
		{ TEXT("Attributes"), TEXT("Alt+A") },
		{ TEXT("Items"), TEXT("Alt+E") },
		{ TEXT("Equipment"), TEXT("Alt+Q") },
		{ TEXT("Skills"), TEXT("Alt+S") },
		{ TEXT("Basic information"), TEXT("Alt+V") },
		{ TEXT("Party"), TEXT("Alt+Z") },
		{ TEXT("Friends"), TEXT("Alt+H") },
		{ TEXT("Chat room"), TEXT("Alt+C") },
		{ TEXT("Missions"), TEXT("Alt+U") },
		{ TEXT("Emotion list"), TEXT("Alt+L") },
		{ TEXT("Shortcut list"), TEXT("Alt+M") },
		{ TEXT("Achievements"), TEXT("Alt+Y") },
		{ TEXT("Quick equipment swap"), TEXT("Alt+W") },
		{ TEXT("Bank"), TEXT("Ctrl+B") },
		{ TEXT("Mail"), TEXT("Alt+R") },
		{ TEXT("Navigation (also /navi)"), TEXT("Alt+N") },
		{ TEXT("Minimap"), TEXT("Ctrl+Tab") },
		{ TEXT("Sentar / levantar"), TEXT("Insert") },
		{ TEXT("Bar shortcuts"), TEXT("F1 a F9") },
		{ TEXT("Game menu"), TEXT("Esc") },
		{ TEXT("This window"), TEXT("F12") },
	};
}

void UMythRoHUDWidget::BuildShortcutWindow()
{
	constexpr float RowH = 17.f;
	const int32 Count = UE_ARRAY_COUNT(ShortcutRows);
	ShortcutWindow = MakeRoPanel(TEXT("Keyboard shortcuts"), FVector2D(250.f, 17.f + 12.f + Count * RowH + 8.f), FVector2D(300.f, -150.f));
	for (int32 i = 0; i < Count; ++i)
	{
		const float Y = 24.f + i * RowH;
		AddText(ShortcutWindow, 14.f, Y, 8, ScText)->SetText(FText::FromString(ShortcutRows[i].Function));
		AddText(ShortcutWindow, 160.f, Y, 8, ScKey)->SetText(FText::FromString(ShortcutRows[i].Key));
	}
}

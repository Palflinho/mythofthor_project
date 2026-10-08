# HUD Clássico (UMG em C++)

`UMythClassicHUDWidget` (`Source/MythOfThor/UI/`) monta o layout inteiro em C++, sem Widget Blueprint. O `AMythPlayerController` o cria no `BeginPlay` e liga ao Pawn (`BindToPawn`) a cada `SetPawn`.

| Área | Conteúdo |
|---|---|
| Topo-esquerda | Nome, `Lv.`, barra HP verde e SP azul com valores numéricos |
| Rodapé | Barra de EXP base em largura total, 10 marcadores de 10%, e o percentual |
| Baixo-direita | Botão AUTO-BATTLE ON/OFF, vigor `HH:MM:SS / 03:00:00` com barra, peso do inventário e mensagens de erro (5 s) |

Fontes de dados (no Pawn):
- `UMythStatsComponent` (novo): nome, nível, HP/SP, EXP. A rede deve chamar `SetIdentity`, `SetVitals` e `SetExp` ao receber os pacotes de status.
- `UAutoBattleComponent`: ganhou `GetInventoryWeightPercent()`, `IsOverweight()` e o delegate `OnInventoryWeightChanged`.

Regras do botão: sempre dá para desligar; para ligar precisa não estar caído, ter vigor e peso < 90%. Com sobrepeso o texto fica vermelho e o botão desabilita.

Teste no PIE (console): `MythTestHud` (preenche HP/SP/EXP e alterna sobrepeso 92%/40%).

# Controles Híbridos & Câmera (Enhanced Input, C++)

Código em `Source/MythOfThor/Player/`. Os mapeamentos do Enhanced Input são criados em C++ (sem `.uasset`), então não há nada para configurar no editor.

| Ação | Entrada |
|---|---|
| Andar até o ponto | Botão esquerdo (segurar continua seguindo o cursor) |
| Andar direto | `W` `A` `S` `D` (relativo ao yaw da câmera; cancela o clique) |
| Alternar câmera Fixa 45° / Livre 360° | `C` (ou `ToggleCameraMode()` num menu UMG) |
| Girar (modo livre) | Botão direito + mouse, ou `Q` / `E` |
| Zoom (modo livre) | Scroll |

- `AMythGameMode` define `AMythPlayerCharacter` + `AMythPlayerController`; é o `GlobalDefaultGameMode` em `Config/DefaultEngine.ini` (substitui `BP_TopDownGameMode`).
- Todo destino é convertido em célula (`WorldToCell`, `CellSizeUU`, padrão 50, ajustar à escala dos mapas) e enviado via `URagnarokNetworkSubsystem::SendMoveRequest` (`CZ_REQUEST_MOVE`). O WASD projeta `WasdLookaheadCells` células à frente, com limite de 5 pacotes/s.
- Sem NavMesh no mapa, o clique usa movimento em linha reta; com NavMesh usa `SimpleMoveToLocation`.
- O sprite 2D (PaperZD) deve ser anexado num Blueprint filho de `AMythPlayerCharacter`, usando `GetCameraYaw()` em `URagnarokDirectionHelper::CalculateSpriteDirection`.

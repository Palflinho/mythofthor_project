# Branch `feature/hd2d-environment-gdd`: comparação com a `main`

Branch criada em 2026-10-14 (4 commits, 97 arquivos; a maior parte é `Content/` do Poring de teste de 2026-10-08). Ela nasceu de um commit **muito antigo** da main
(`4fbdb73`, o protótipo offline com `MythSpriteAnimComponent` + Paper2D). Por isso **não dá para fazer merge direto**: quase todo `Source/` que ela toca
foi reescrito na main (o jogo hoje é online, lê os sprites do GRF em `MythRoSpriteComponent` e quem manda no jogo é o servidor). O que vale é aproveitar as **ideias e os dados**.

## O que a branch traz, e o que fazer com cada coisa

| Item da branch | Equivalente na main | Decisão |
|---|---|---|
| `Config/map_environment_table.json` (199 mapas: fuso `grid_x`, latitude `grid_y`, masmorra, noite eterna, bioma, climas permitidos) | Nada. A main só tem `IsIndoor()` por heurística em `MythMapInfo.cpp` | **Aproveitar já**: dados prontos e bons. Cobre 196 dos nossos 1063 mapas (cidades, masmorras e campos principais); os mapas novos (mjolnir, yuno_fild, new_1-x...) não estão e caem no padrão |
| `AMythMapInfo::GetLocalHour` + hora local por fuso, luz mais quente ao sul e mais fria ao norte, masmorra sem sol, noite polar (`MythDayNightActor`) | A main tem o ciclo dia/noite por relógio do cliente, sem fuso nem latitude | **Aproveitar já** (opção "Regional time"/"Hora regional", ligada) |
| `MythWeatherComponent` (Niagara por bioma + `Wetness`/piso molhado) | Clima por pacote do servidor (`MythWeatherActor`, chuva/neve/sakura/nuvens/folhas/fogos) | **Depois**: o clima de verdade vem do servidor (1:1). Dá para usar `allowed_weathers`/`default_weather` do JSON como o clima de cada mapa, e o piso molhado já está no backlog de clima. Precisa criar os sistemas Niagara |
| `TriggerHitstop` / `TriggerHitFlash` / `TriggerMultiLayerHitFlash` | Nada | **Aproveitar já** em `MythRoSpriteComponent` (microcongelamento 30 ms e clarão de 1 quadro no alvo) |
| Reflexo "fantasma" do sprite na água | A água moderna da main tem reflexo, o sprite não | **Depois** (precisa do plano d'água por mapa que a main já gera em `Config/water/*.json`) |
| `MythSkillVFXSubsystem` / `FSkillVisualData` (arquétipos, cores por elemento, pool Niagara, decalque) | `MythSkillManager` data-driven com os efeitos **originais** (STR/SPR/3D) do cliente RO | **Não usar**: você definiu que magias e skills ficam 1:1 com o cliente original. O pool de Niagara não é preciso, porque os efeitos originais já são leves |
| `MythGATGridManager` (grade GAT, linha de visão, Hermite) | `MythCellMoverComponent` anda pelas células do `.gat`; linha de visão e alcance são do servidor | **Não usar**: duplicaria o servidor (que decide) |
| Câmera `Pitch -35°`, `FOV 40°`, braço 400-1200 | Câmera já ajustada e validada contra o RO | **Não mexer** sem você pedir (mudaria o enquadramento clássico) |
| Camadas separadas de sprite (robe/cabelo/chapéus/arma) com ordem por direção | `MythRoSpriteComponent` já compõe as camadas do ator do RO (`.act` com âncoras e ordem corretas) | **Não usar**: a main é mais fiel |
| `DefaultEngine.ini`: AA=1, sem motion blur, `r.CustomDepth=3` | A main usa FXAA/TAA escolhido no menu | **Aproveitar**: sem motion blur (combina com sprite 2D) |
| Importador: nível d'água e DoF/motion blur no pós-processo | A main já extrai a água (`gen_water*.py`, `MythWater`) | **Já coberto** |

## O que foi feito agora na main (a partir da branch)
1. `Config/map_environment_table.json` copiado (sem alteração de dados).
2. Fuso, latitude e noite eterna no ciclo dia/noite (`MythMapEnvironment` + `MythDayNightActor`), com a opção "Regional time" (ligada por padrão). O sinalizador de masmorra da tabela ainda não é usado: falta conferir a luz das masmorras importadas antes de apagar o sol nelas.
3. Hitstop e hit-flash no alvo de cada golpe.
4. Motion blur: já estava desligado na main (`r.DefaultFeature.MotionBlur=False`).

## Próximos passos sugeridos
- Estender a tabela de ambiente para os mapas que ficaram de fora (por prefixo: `yuno_fild` -> Yuno, `mjolnir` -> Mjolnir...).
- Piso molhado ligado ao clima do servidor.
- Reflexo do sprite na água.

## Resultado dos testes (2026-10-15)
- **A branch isolada** (worktree em `E:\Dev\mythofthor_hd2d`, já removido): compila sem erros (90 s) e roda, mas não serve como jogo: o mapa padrão dela é o `Lvl_TopDown` do template da Unreal (outro GameMode), e em Prontera com o `MythGameMode` da branch a câmera (pitch -35°, braço 800) mostra só o chão de perto, sem personagem, porque o protótipo offline não tem os sprites do cliente online. O ciclo dia/noite dela funciona (testei 12h e 22h).
- **O que foi trazido para a main foi testado no jogo online:**
  - `xmas_fild01` (noite eterna): às 12h do relógio do mundo continua noite azul (OK).
  - `morocc` (latitude sul): luz dourada quente às 17h; Prontera às 17h mais fria e escura (OK).
  - Hit-flash/hitstop: compila e roda; o efeito dura 30-60 ms, precisa de olho humano com o mouse para avaliar.

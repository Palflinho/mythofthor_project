# Ponto de partida para a nova conversa (2026-10-10)

Leia isto primeiro. Resume o projeto sem precisar do histórico.

## O projeto

**Myth of Thor:** remaster do Ragnarok Online pre-renewal em Unreal 5.8 (C++).

- **Servidor:** rAthena, em WSL/Docker, PACKETVER 20250402. Ele decide todas as regras; o cliente só mostra.
- **O que é 1:1 com o RO:** a jogabilidade.
- **O que é moderno:** o visual (HD-2D: Lumen, sombras, clima, dia/noite) e o conforto (HUD do RO LATAM, menu de
  configurações).
- **Escopo:** classes até a transclasse.

## Onde está cada coisa

| O quê | Onde |
|---|---|
| Plano por fases e status | `docs/ROADMAP.md` |
| Comportamento do cliente original (movimento, câmera, animação, combate, efeitos, janelas, sons, render, efeitos procedurais) | `docs/ref/01..11` |
| HUD do RO LATAM (prints oficiais + descrição de cada janela) | `docs/ref/12_hud_latam_prints.md` e `docs/ref/hud_latam/` (24 imagens) |
| Sistemas próprios do Myth (Síntese, Grade, Monstro raro, Códice de Mímir, HUD sem comandos) — o servidor já está pronto | `docs/sistemas/` |
| Coordenação com o Antigravity (outro agente: servidor e pesquisa) | `coord/LEIA-ME.md`, `coord/para_antigravity/`, `coord/para_claude/` |
| Comandos de teste | `docs/COMANDOS_TESTE.md` |

## Como trabalhar (o que funcionou)

- **Build:** `powershell -File tools/build_watch.ps1`.
  - Leva cerca de 30 s; a pasta `Intermediate` está no NVMe, via junction.
  - Ele **fecha o jogo aberto**: avise o usuário antes de compilar.
  - Respeite o `coord/build.lock`.
- **Teste automático com prints:** `UnrealEditor.exe <uproject> /Game/Maps/Prontera/Lvl_Prontera -game -windowed
  -ResX=1600 -ResY=900 -log -ExecCmds="MythDelayed 3 MythLogin, MythShot 26 -1 nome, MythQuitAfter 30"`.
  - Os prints ficam em `Saved/Screenshots/WindowsEditor`.
  - Com `MythDelayed`, use `+` no lugar dos espaços.
- **Contas:** a `mythtest` é do usuário; o Antigravity testa só com a `mythbot`.
- **Commits:** locais, sem push. Respostas ao usuário em pt-BR.

## Estado agora

- **Fase 1, ciclo básico:** pronta.
  - conexão, movimento do RO, combate, coleta, câmera, cursores, sentar;
  - portais e interiores de Prontera.
- **Fase 2, cara de RO:** quase pronta.
  - Feitos:
    - animações;
    - números de dano;
    - sons;
    - level up;
    - efeitos procedurais das skills: desenhador `AMythProcEffect` + `Config/skill_effects_full.json`. O Impacto
      Explosivo e a Cura foram aprovados pelo usuário.
  - Faltam:
    - os efeitos da 2ª classe e da transclasse (Antigravity, coord 008);
    - o tremor de tela;
    - as flechas;
    - o "doridori";
    - a aura do nível 99.
- **Fase 3, jogável mínimo:**
  - lojas de NPC;
  - Kafra;
  - troca de classe;
  - morrer e reviver;
  - mapas para upar (Antigravity, coord 009);
  - grupo, cochicho e troca.
- **Fase 4, HUD LATAM:** começou, no commit `d0f1d3a`.
  - Feitos:
    - informações básicas com a grade de ícones e o modo recolhido;
    - barra de atalhos no topo;
    - minimapa sem moldura;
    - chat em português;
    - diálogo de NPC;
    - árvore de skills (estilo árvore de talentos, decisão do usuário).
  - Faltam, seguindo os prints:
    - inventário;
    - equipamento;
    - status;
    - a árvore com Apply/Reset e "◀ nível ▶";
    - menu Esc;
    - Configurações com as abas e chaves ON/OFF;
    - barras de HP/SP sob o personagem.
- **Correção recente:** os monstros ficavam flutuando parados. Foi corrigido no commit `f4ca760`, mas falta o teste
  do usuário.

## Sugestão para começar

Realinhar com o usuário a prioridade entre a **Fase 3 (jogável mínimo)** e a **Fase 4 (HUD LATAM)**. Depois, seguir
uma por vez, com teste do usuário a cada item.

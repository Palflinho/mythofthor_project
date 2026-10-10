# Ponto de partida para a nova conversa (atualizado em 2026-10-10)

Leia isto primeiro. Resume o projeto sem precisar do histórico.

## O projeto

**Myth of Thor:** remaster do Ragnarok Online pre-renewal em Unreal 5.8 (C++).

- **Servidor:** rAthena em WSL/Docker, PACKETVER 20250402. Ele decide todas as regras; o cliente só mostra.
- **Prioridade atual:** o jogo ficar **1:1 com o RO que já existe** (decisão do usuário, 2026-10-10). Só depois vêm login,
  teste fechado, sistemas próprios, HD-2D e conteúdo.
- **Moderno permitido:** visual HD-2D (Lumen, sombras, clima, dia/noite) e conforto (HUD do RO LATAM, menu de configurações).
- **Escopo:** classes até a transclasse.

## Onde está cada coisa

| O quê | Onde |
|---|---|
| Ordem do trabalho (F0 a F4 e o que vem depois) | `docs/ROADMAP.md` |
| **O que ainda diverge do RO original, regra por regra** | `docs/FIDELIDADE.md` |
| Specs do cliente original (movimento, câmera, animação, combate, efeitos, janelas, sons, render) | `docs/ref/01..11` |
| HUD do RO LATAM (prints e descrição) | `docs/ref/12_hud_latam_prints.md`, `docs/ref/hud_latam/` |
| Sistemas próprios do Myth (servidor pronto) | `docs/sistemas/` |
| Fronteira servidor × cliente | `docs/PLANO_CLIENTE_FIEL.md` seção 1 |
| Coordenação com o Antigravity | `coord/LEIA-ME.md`, `coord/para_antigravity/`, `coord/para_claude/` |
| Formato das pesquisas (para a rodada R2) | `docs/HANDOFF_ANTIGRAVITY_PESQUISA_REFERENCIAS.md` |
| Comandos de teste | `docs/COMANDOS_TESTE.md` |
| Docs antigos (não usar como estado atual) | `docs/arquivo/` |

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
- **Fidelidade:** antes de mexer num sistema, leia o spec em `docs/ref/` e a seção dele em `FIDELIDADE.md`; ao
  terminar, atualize a linha da matriz. Se o spec não cobre, peça pesquisa ao Antigravity em vez de deduzir.

## Estado agora

- **F0 (auditoria):** feita, `docs/FIDELIDADE.md`.
- **F1 (divergências já especificadas):** não começou. Primeiro item: doridori (1.1), depois camadas de capa/carrinho,
  flechas, barras de HP/SP sob o personagem, inventário/equipamento/status completos.
- **F2 (pesquisa R2 para lojas, Kafra, morte, grupo, chat, itens, atributos, cast e buffs, troca, login, montaria):**
  precisa ser pedida ao Antigravity (nova tarefa em `coord/para_antigravity/`, status `pendente`).
- **F4 (auditoria do servidor):** idem, pedir ao Antigravity.
- **Teste pendente do usuário:** porings no chão (commit `f4ca760`), HUD LATAM (commit `d0f1d3a`), efeitos de skill da 2ª
  classe e transclasse.

## Já pronto

Conexão estável, movimento e câmera do RO, combate e coleta, animações, números de dano, sons, level up, portais e
interiores de Prontera, diálogo de NPC, chat em português, árvore de skills, efeitos procedurais (desenhador
`AMythProcEffect` + `Config/skill_effects_full.json`), 40 mapas importados, mapeamento das quests de troca de classe.

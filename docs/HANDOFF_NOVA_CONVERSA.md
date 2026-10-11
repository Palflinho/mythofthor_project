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
- **Contas:** a `mythtest` é do usuário (nunca usar nos testes automáticos: eles mudam classe, HP e visual do personagem); o Antigravity usa a `mythbot`; o Claude usa a `mythclaude` (`MythDelayed 3 MythLogin+mythclaude+mythclaude123`, personagem ClaudeTester, criada por `server/init_claude_account.sql`).
- **Versão de teste do usuário:** `tools/make_test_build.ps1 -Nota "..."` copia o jogo compilado para `E:DevMythTeste` (com `Jogar_Myth_Teste.bat`, `VERSAO.txt` e `COMO_TESTAR.txt` vindo de `docs/TESTE_JOGADOR.md`). Rode depois de compilar uma leva de novidades e atualize `docs/TESTE_JOGADOR.md` com o que testar. Se o jogo de teste estiver aberto, o script recusa. O `build_watch.ps1` não fecha mais a janela de teste do usuário.
- **Commits:** locais, sem push. Respostas ao usuário em pt-BR.
- **Fidelidade:** antes de mexer num sistema, leia o spec em `docs/ref/` e a seção dele em `FIDELIDADE.md`; ao
  terminar, atualize a linha da matriz. Se o spec não cobre, peça pesquisa ao Antigravity em vez de deduzir.

## Estado agora

- **F0 (auditoria):** feita, `docs/FIDELIDADE.md`.
- **F1 (divergências já especificadas), feito em 2026-10-10 e *ainda não testado pelo usuário jogando*:**
  doridori e direção do corpo no mundo (a câmera gira o sprite parado), capa/asas, carrinho do Mercador, flecha do arco,
  tremor de tela, barras de HP/SP sob os pés, memória da câmera entre mapas, sensibilidade do mouse (o eixo do mouse
  vinha multiplicado por 0,07).
- **F1, falta:** aura do 99 (pesquisa coord 012), clamp da câmera no terreno, `FixPathTime`, mira de skill em área,
  inventário/equipamento/status completos, skills com Apply/Reset, menu Esc com abas, flechas de monstros, posturas
  pickup/readyfight/stun, conferir tempos da flecha no OM.
- **Pesquisas pendentes no Antigravity (coord, status `pendente`):** 010 (R2: lojas, Kafra, morte, grupo, chat, itens,
  atributos, cast e buffs, troca, login, montaria), 011 (auditoria do servidor), 012 (aura do 99 e FUNC vazios).
- **Teste pendente do usuário:** tudo acima, mais porings no chão (`f4ca760`), HUD LATAM (`d0f1d3a`), efeitos de skill.
- **Cuidados de build:** o projeto compila em unity build; constantes em namespace anônimo com nome comum (`HeadDir`,
  `EffectDir`) colidem entre arquivos. Prefira nomes específicos.
- **Teste automático:** `Start-Process` precisa das aspas no `-ExecCmds="..."` dentro da string de argumentos, senão os
  espaços somem. O editor leva alguns minutos para abrir depois de recompilar.

## Já pronto

Conexão estável, movimento e câmera do RO, combate e coleta, animações, números de dano, sons, level up, portais e
interiores de Prontera, diálogo de NPC, chat em português, árvore de skills, efeitos procedurais (desenhador
`AMythProcEffect` + `Config/skill_effects_full.json`), 40 mapas importados, mapeamento das quests de troca de classe.

## Atualização de 2026-10-11 (sessão longa, o usuário testa depois)

- **Feito e commitado nesta rodada:** grupo (Alt+Z, `/organize /invite /leave /expel`), troca entre jogadores (menu do botão direito, `/exchange`),
  HP do monstro (0x0977), clique em janela não anda mais, skills de área sem alvo (0x0117), unidades de skill no chão
  (`Config/skill_units.json`), mapa skill→efeito completado do roBrowser (`tools/gen_skill_map_from_rb.py`), tintas de estado
  e adereços (sono, atordoado, maldição), efeitos de sprite (`AMythSprEffect`), falcão, postura de combate (readyfight),
  pegar item, cursor do tile e anel da skill de área (`Config/skill_area.json`), nome do mapa ao entrar, janela de opções
  (Esc > Opções do jogo, `FMythOptions`), lista de emoções (Alt+L) e de atalhos (Alt+M), cantos arredondados nas janelas,
  `/resetchar` (GM), conta `mythclaude2` para testes com 2 jogadores (`server/init_claude_account2.sql`).
- **Servidor:** o WSL desliga quando fica sem sessão aberta e derruba o Docker. Mantenha um terminal Ubuntu aberto
  (ou rode `wsl -d Ubuntu -- sleep infinity` em segundo plano) e suba com
  `cd "/mnt/e/Documentos/Repositórios/mythofthor_project/server" && docker compose up -d`. Se o login recusar com a
  conexão encerrada, reinicie `mythofthor-login mythofthor-char mythofthor-map` depois que o `db` estiver pronto.
- **Próximos itens do ROADMAP (ordem sugerida):** efeitos de skill restantes (`docs/EFEITOS_PENDENTES.md`), acabamento da
  HUD (1.15, janela por janela contra `docs/ref/hud_latam/INDICE.md`), janelas que faltam (1.16: sala de chat, F12;
  1.17: quests, mapa-múndi, /navi), Game settings aba Gráficos/Som dentro da mesma janela (1.22), login e seleção de
  personagem, depois sistemas próprios e HD-2D. Cobertura das referências: `docs/COBERTURA_REFERENCIAS.md`.
- **Teste do usuário pendente:** tudo acima (ver `docs/TESTE_JOGADOR.md`, seções "NOVO").

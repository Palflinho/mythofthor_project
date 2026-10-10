# Tarefa R1 para o Antigravity: pesquisa nos clientes de referência (especificações para o Claude)

**Decisão do usuário (10/10/2026):** o Antigravity lê os clientes abertos de RO e escreve **especificações de
comportamento**; o Claude reescreve no cliente Unreal a partir delas. Assim o comportamento deixa de ser
"deduzido" e passa a ser fiel ao cliente original.

## Repositórios (só leitura; não compilar nem rodar)

| Ref | Repositório | Licença | Uso |
|---|---|---|---|
| **OM** | github.com/Sziadan/open-midgard | GPL-3.0 | Reconstrução do cliente **pre-renewal 2008** com as classes originais (CGameMode, CView, GameActor, PathFinder, UI*Wnd, RagEffect...). **Referência principal.** |
| **RB** | github.com/MrAntares/roBrowserLegacy | GPL-3.0 | O mais completo em janelas e sistemas (pacotes até ~2021-11). |
| **KO** | github.com/vE5li/korangar | MIT | Cliente moderno com iluminação; ver as decisões de render e de UI. |

## Regra de licença (importante)

OM e RB são **GPL**: **não copie código** para os docs nem para o projeto. Escreva o **comportamento** em português,
com os **números exatos** (tempos, ângulos, tamanhos, fórmulas, ordem das camadas, posições). Um trecho curto de
pseudocódigo próprio pode, quando ajudar. Sempre cite a origem (`OM src/world/GameActor.cpp ~linha 812`) para o
Claude conferir. Do KO (MIT) pode citar trechos, com o crédito.

## Formato de cada especificação

Um arquivo por sistema em `docs/ref/NN_nome.md`, com:

1. **Resumo:** o que o sistema faz no cliente original, em 3 a 5 linhas.
2. **Dados e constantes:** tabela com valores e unidades (ms, células, graus, pixels).
3. **Regras:** passo a passo do comportamento, incluindo os casos de borda.
4. **Entradas e saídas:** pacotes recebidos e enviados (só os nomes; os offsets continuam vindo do rAthena),
   teclas e cliques.
5. **Diferenças entre referências:** quando OM e RB divergirem, anotar as duas e qual parece fiel ao original.
6. **Como testar:** o que o jogador vê e o que confirma que está igual ao RO.

## Ordem (o ciclo básico primeiro: é o que travou no teste do usuário)

1. **01_movimento.md** (OM `pathfinder/PathFinder.cpp`, `world/GameActor.cpp`; RB `Renderer/Entity/EntityWalk.js`):
   - algoritmo de caminho do cliente (custo, diagonais, cortar quinas, limite de busca);
   - interpolação entre células (posição por tempo, velocidade, diagonal);
   - como o cliente trata ZC_NOTIFY_PLAYERMOVE / ZC_NOTIFY_MOVE / ZC_STOPMOVE, inclusive a correção de posição
     quando cliente e servidor divergem;
   - clique segurado no chão (frequência de pedidos) e o que acontece clicando durante o carregamento do mapa;
   - direção do sprite (8 direções pela rota e pela câmera).
2. **02_camera.md** (OM `gamemode/View.cpp`): ângulos iniciais, limites de zoom e de inclinação, rotação com o botão
   direito (sensibilidade, passo), Shift, reset da câmera, câmera fixa em mapas internos.
3. **03_atores_animacao.md** (OM `world/GameActor.cpp`, `3dActor.cpp`; RB `EntityRender.js`, `EntityAnimations.js`):
   - ação por estado e quadro por tempo (delay do .act, ASPD no ataque, velocidade na caminhada);
   - "doridori" da cabeça parado; ordem das camadas (corpo, cabeça, acessórios, arma, escudo) por direção;
   - quando a arma aparece; morte, sentar e ser atingido;
   - sombra redonda (tamanho e posição).
4. **04_clique_alvo_combate.md** (OM `gamemode/GameMode.cpp`): o que o clique pega (prioridade
   NPC > monstro > item > chão), alcance de ataque, andar até o alvo (ZC_ATTACK_FAILURE_FOR_DISTANCE), ataque
   contínuo, cursores (atacar, falar, pegar), pegar item (distância, animação).
5. **05_efeitos_do_executavel.md** (OM `world/RagEffect.cpp`, `MsgEffect.cpp`, `res/EzEffectRes.cpp`;
   RB `Renderer/Effects/*`): os efeitos **desenhados em código, sem .str**, com os parâmetros de cada um:
   - portal (anéis, altura, velocidade, textura);
   - golpes, números de dano (trajetória, tempo, escala), "Miss", críticos, cura;
   - level up, aura de nível 99, teleporte, morte, efeitos de skill sem .str (Golpe Fulminante, Bola de Fogo...).
6. **06_janelas_basicas.md** (OM `ui/UI*Wnd.cpp`; RB `UI/Components/*`): para cada janela (BasicInfo, Item, Equip,
   Status, SkillList, ShortCut, Minimap, Chat, SayDialog/NpcMenu, ItemInfo, Storage), registrar:
   - posição inicial, tamanho e se é arrastável ou minimizável;
   - interações (clique, duplo clique, botão direito, arrastar, roda);
   - atalhos (Alt+E etc.) e o comportamento do Esc;
   - textos e formatos (peso, zeny, exp em %).
7. **07_sons.md:** quando cada som toca (passos pelo .act, golpes por tipo de arma, skills, UI, BGM por mapa e
   troca de mapa).
8. **08_ui_moderna_rb.md** (só RB): sistemas que o cliente de 2008 não tem e o RB tem (inventário com abas
   novas, navegação, quests, achievements...). Lista do que existe e de como funciona.
9. **09_render_korangar.md** (KO): como o korangar faz iluminação, sombras de sprite, render de mapa e efeitos.
   Diga o que vale para a Unreal e o que não vale.

## Pronto quando

- Cada doc segue o formato acima, com números e citações da origem.
- `docs/ref/00_INDICE.md` lista os docs, com uma linha de resumo cada e o que **ainda não foi encontrado** nas referências.
- **Nenhum trecho de código GPL copiado.**
- Commit só dos docs (não mexer em `Source/`).

## Coordenação

Esta tarefa não compila nada e não mexe no código, então pode rodar **ao mesmo tempo** que o Claude implementa.
Entregue doc por doc (commit a cada um): o Claude começa a implementar o 01 enquanto você escreve o 02.

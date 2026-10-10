# Matriz de fidelidade ao RO original (F0)

Gerada em 2026-10-10 lendo o código de `Source/` contra os specs de `docs/ref/`. É a fonte do que falta para o jogo ficar
1:1 com o Ragnarok Online. O `ROADMAP.md` aponta para cá.

**Legenda do estado:** ✅ igual ao spec (conferido no código) · ⚠️ diverge ou é parcial · ❌ ausente · ❓ não verifiquei
(precisa de teste jogando ou de leitura mais fina) · 🖥️ do servidor (o rAthena decide; o cliente só mostra).

**Legenda "Teste":** 👤 já aprovado por você jogando · ⬜ ainda não testado por você.
Hoje só 👤 o que o roadmap marcava como feito e testado; o resto está ⬜.

**Prioridade:** P1 é o que se vê o tempo todo jogando; P2 é visível em situações específicas; P3 é detalhe.

---

## 01 Movimento (`docs/ref/01_movimento.md`)

| Regra do spec | Estado | Teste | Onde / observação |
|---|---|---|---|
| Custos 10 (reto) e 14 (diagonal) | ✅ | 👤 | `MythCellMoverComponent.cpp:12-13` |
| Sem corte de quina (diagonal exige as 2 células retas livres) | ✅ | 👤 | `FindPath` e `StraightPath` |
| Reta primeiro, A* depois | ✅ | 👤 | `StraightPath` |
| A* idêntico ao `PathFinder.cpp` (fila FIFO de desempate, penalidade +20, máscara `freeMask`) | ⚠️ P3 | ⬜ | Nosso A* usa heurística octile e busca linear na lista aberta. O resultado é um caminho de custo mínimo, mas em empates pode escolher outro lado que o original. Só importa para a rota prevista antes da confirmação do servidor. |
| Raio máximo de busca | ⚠️ P3 | ⬜ | Usamos 40; o servidor aceita 17 (`max_walk_path`). Cliques longos são cortados em 14 células (`MythPlayerController.cpp:626`). Sem divergência visível. |
| Primeiro segmento parte da posição fracionária real | ✅ | 👤 | `BeginSegment` |
| Troca de rota andando: manter a posição interpolada e recalcular daí | ⚠️ P2 | ⬜ | Nós terminamos o passo atual e seguimos dele (`MoveToCell`, imitando `change_walk_target` do rAthena). O spec do cliente recalcula do ponto fracionário. Confirmar jogando se aparece "quina" em curvas com o botão segurado. |
| Duração do passo = `custo × speed / 10` | ✅ | 👤 | `TickComponent` |
| Tempo do servidor (`FixPathTime`, clamp ±2500 ms, mínimo 10 ms) | ❌ P2 | ⬜ | Usamos só o `MsPerCell` que vem do pacote; nenhuma redistribuição pela diferença de duração. Com ping alto o personagem pode chegar antes ou depois do servidor. |
| Parada do servidor atrasada é ignorada (`fixIndex ≤ activeIndex+1`) | ✅ | 👤 | `ServerStop` (usa `Passed` e `Path`) |
| Desync > 16 células: estalo direto | ✅ | 👤 | `MaxDesyncCells = 16` |
| Clique segurado: 75 ms entre pedidos, espera ack até 1000 ms | ✅ | 👤 | `FlushPendingMove` |
| Retarget: destino a ≤ 2 células mantém o movimento | ✅ | 👤 | `SendCellToServer` |
| Extensão de 4 células na direção do cursor ao segurar | ❌ P3 | ⬜ | Não projetamos além do cursor. |
| Janela "perto do fim do passo" (80/120/150 ms) para mudar de direção | ❌ P3 | ⬜ | |
| Shift+clique e sentado: só vira, `CZ_CHANGE_DIRECTION` | ✅ | 👤 | `TurnToward`; ❓ sentado + clique |
| Bloquear cliques durante o carregamento do mapa | ❓ P2 | ⬜ | Não achei trava explícita. Testar clicar durante a troca de mapa. |
| Direção do sprite em 8 setores relativos à câmera | ✅ | 👤 | `RagnarokDirectionHelper` |
| Movimento de terceiros (cada passo chega como 0x09fd) | ✅ 🖥️ | 👤 | `MythNetWorld` |

## 02 Câmera (`docs/ref/02_camera.md`)

| Regra | Estado | Teste | Observação |
|---|---|---|---|
| Sensibilidade 0,4°/px (giro) e 0,3°/px (inclinação) | ✅ | 👤 | `MouseRotateSensitivity`, `OnPitchMouse` |
| Zoom 150–700 (indoor 150–420), passo 60, padrão 500 (indoor 350) | ✅ | 👤 | `MythPlayerCharacter.h`; indoor em `BeginPlay` |
| Inclinação padrão −45° (−65 a −25); indoor −40° (−55 a −35) | ✅ | 👤 | idem |
| Indoor: longitude travada em 45° | ✅ | 👤 | `bCameraLockedByMap` |
| Suavização 10% por quadro, menor arco | ✅ | 👤 | `Tick` |
| Duplo clique direito reseta ângulos e mantém o zoom | ✅ | 👤 | `UpdateCameraReset`. Janela de 0,3 s; o spec usa o tempo de duplo clique do sistema (~0,5 s) → ⚠️ P3 |
| Shift+direito só inclina | ✅ | 👤 | |
| Ctrl+direito zoom por arrasto (só no RB) | ❌ P3 | ⬜ | Não é do cliente 2008; ignorar. |
| Câmera nunca abaixo do terreno | ❌ P2 | ⬜ | `bDoCollisionTest = false`; sem o clamp do `BuildViewMatrix`. Inclinação −25° perto de morros pode entrar no chão. |
| `viewpointtable.txt` (câmera por mapa) | ⚠️ P2 | ⬜ | Só existe o caso indoor genérico; mapas com tabela própria não usam seus limites. |
| Guardar zoom e inclinação entre mapas (indoor e outdoor separados) | ❌ P2 | ⬜ | Cada mapa começa no padrão. |
| Giro por teclado (`KeyRotateSpeed`) | ⚠️ P3 | ⬜ | Extra nosso, não está no spec. Decidir se mantém. |

## 03 Atores e animação (`docs/ref/03_atores_animacao.md`)

| Regra | Estado | Teste | Observação |
|---|---|---|---|
| Passo pela distância (`dist × 1,48 / delay`) | ✅ | 👤 | `GetFrameIndex` |
| Ataque escala por `attackMT / 432` | ✅ | 👤 | `SetState` |
| Golpe levado: 200 ms | ✅ | 👤 | |
| Recuo elástico de 6 unidades no golpe levado | ❓ P3 | ⬜ | Não vi no sprite. |
| Morte: para no quadro 4 | ✅ | 👤 | |
| Sentar sem sombra; morto sem sombra | ❓ P3 | ⬜ | |
| **"Doridori"** (cabeça girando parado: corpo `quadro = headDir`, cabelo e acessórios em 3 blocos) | 🟡 P1 | ⬜ | Feito em 2026-10-10: setas ← e → parado ou sentado, `CZ_CHANGE_DIRECTION`, `ZC_CHANGE_DIRECTION` (0x009c) e `headDir` das unidades. Conferido nos `.act` reais: corpo, cabeça e acessórios têm 3 quadros parados. Falta o teste do usuário (e `/doridori` no chat, depende da R2.5). |
| Direção do corpo no mundo: girar a câmera parado mostra outro lado do personagem (`docs/ref/01` 3.7) | 🟡 P1 | ⬜ | Achada na auditoria: o sprite guardava a direção relativa à câmera e só mudava ao andar. Corrigido em 2026-10-10 (`FacingWorld`); falta testar. Direção de spawn vem do pacote; a do próprio jogador ao entrar no mapa ainda não. |
| Fase compartilhada entre camadas | ❓ P2 | ⬜ | Corpo manda o `FrameIndex`; cabeça usa `FrameIndex % N`. Conferir com chapéu animado. |
| Ordem das camadas de frente (escudo atrás? carrinho, capa, corpo, cabeça, acessórios, arma, escudo) | 🟡 P1 | ⬜ | Capa/asas e carrinho feitos em 2026-10-10 (`RobeSource`/`CartSource`; carrinho = `이팩트/손수레1..5` pelo `option` do rAthena, pacotes de unidade e `ZC_STATE_CHANGE`). Falta a lista `RobeTopLayer` (226 ids, uso ainda não entendido) e confirmar a posição do carrinho no cliente original (usamos só os deslocamentos do `.act`). |
| Ordem de costas (3–5): capa por cima do corpo | ✅ | ⬜ | Testado com as asas de anjo (`@changelook 7 1`): atrás de frente, por cima de costas. |
| `.imf` da classe (ordem por arquivo) | ❌ P2 | ⬜ | Ordem fixa em código. Regra: se o `.imf` não existir usa `{7,0,1,4,3,2,5,6}`. |
| Arma guardada em idle e sentado (só aparece em combate) | ✅ | ⬜ | Os `.act` de arma já trazem as camadas vazias fora do golpe (`AddBodyLayer`), então segue o spec sem regra extra. |
| Posturas `READYFIGHT`, `PICKUP`, `FREEZE/STUN`, `ATTACK2` (segundo golpe/crítico) | ❓ P2 | ⬜ | Enum mostra Idle/Walk/Sit/Hit/Dead/ataque; conferir pickup (ação 24), readyfight e stun. |
| Sombra redonda (`shadow.spr`) com escala por tamanho do monstro | ⚠️ P2 | ⬜ | Existe `BlobShadowScale`; ❓ escala por `xSize/ySize` e por MVP. |
| Sombra projetada do sol (HD-2D) | ✅ extra | 👤 | Camada moderna permitida |

## 04 Clique, alvo e combate (`docs/ref/04_clique_alvo_combate.md`)

| Regra | Estado | Teste | Observação |
|---|---|---|---|
| Prioridade NPC > monstro > item > portal > chão | ✅ | 👤 | `MoveToCursor` |
| UI bloqueia o clique no mundo | ✅ | 👤 | `IsCursorOverUi` |
| Cursores: seta, falar, atacar, item, portal, proibido, mira de skill, girar câmera | ✅ | 👤 | `cursors.json`, `UpdateRoCursor` |
| Margem magnética de 12 px no monstro | ❓ P3 | ⬜ | Clique usa o raio do `GetHitResultUnderCursor`. |
| Ataque contínuo `CZ_REQUEST_ACT` tipo 7 | ✅ 🖥️ | 👤 | |
| Andar até o alvo fora do alcance | ✅ 🖥️ | 👤 | O rAthena anda até o alvo; o cliente não precisa calcular a célula de aproximação. |
| Pegar item: ≤ 1 célula, repetir a cada 400 ms | ✅ | 👤 | `UpdatePickup` (3 tentativas) |
| Animação de agachar ao pegar | ❓ P2 | ⬜ | |
| Clicar no NPC: `CZ_CONTACTNPC` e travar ações durante o diálogo | ✅ | 👤 | `IsNpcDialogOpen` |
| Mira de skill (círculo no chão / vermelha no alvo) | ⚠️ P2 | ⬜ | Só `Crosshairs`; falta o círculo de área na célula e o "proibido" por tipo de alvo. |
| Esc cancela a mira | ✅ | ⬜ | |
| `ZC_ATTACK_RANGE` (alcance da arma) | ❓ P3 | ⬜ | Só importa se o cliente decidir algo pelo alcance; hoje quem decide é o servidor. |

## 05 e 11 Efeitos e números (`docs/ref/05`, `docs/ref/11`)

| Regra | Estado | Teste | Observação |
|---|---|---|---|
| Números de dano (spr `숫자`, arco, 70 ticks; Miss 80; cura 120) | ✅ | 👤 | `MythHUD.cpp` |
| Crítico (estrela atrás) | ✅ | 👤 | |
| Level up (anjo) | ✅ | 👤 | |
| Aura do nível 99 | ❌ P2 | ⬜ | Os specs só descrevem em palavras. Pesquisa pedida ao Antigravity (coord 012). |
| Portal (anéis, textura, velocidade) | ✅ | 👤 | `MythWarpPortal` |
| Efeitos de skill da 1ª classe: desenhador procedural | 🟡 | 👤 só Impacto Explosivo e Cura | `MythProcEffect`, `skill_effects_full.json` |
| Efeitos 2ª classe e transclasse | ⚠️ P1 | ⬜ | Tabela completa gerada (coord 010/011), nenhuma testada jogando. |
| Efeitos `.str` | ✅ | ❓ | `MythStrEffect` |
| Tremor de tela (`FUNC quake`, 29 efeitos) | ✅ P2 | ⬜ | Feito em 2026-10-10: sacode a câmera na duração do efeito (50 a 200 ms), só se o efeito é a menos de 30 células do jogador; amplitude 15 uu (~10 px, do spec). Visto no log com o Martelo de Hammerfall; o balanço em si só você vê jogando. |
| Flechas (sprite do projétil) | 🟡 P1 | ⬜ | Feito em 2026-10-10: efeito 2003 (`skel_archer_arrow.spr`, ponta apontando para o alvo) no ataque básico com arco (arma view 11) e nas skills que usam 2003/2004. O número de dano e a reação do alvo esperam a flecha chegar. **Tempo conferido só pelo roBrowser** (soma 192 ms ao attackMT do arco; o spec não diz quando a flecha sai): confirmar no OM. Monstros arqueiros (tabela `AttackEffect.PROJECTILE`) ainda não. |
| Morte, teleporte, ressurreição | ❓ P2 | ⬜ | |
| Ícones de status/buff com barra de duração | ✅ P1 | ⬜ | Existem (`DrawStatusIcons`, `status_icons.json`); falta conferir posição, ordem e tooltip contra a pesquisa R2.8. |
| Barra de cast sobre o personagem | ✅ P1 | ⬜ | Existe (`DrawCastBarsAndShouts`); conferir tamanho e posição contra a R2.8 e a interrupção. |

## 06 e 12 Janelas e HUD (`docs/ref/06`, `docs/ref/12`)

O HUD LATAM tem prioridade sobre o 06 (cliente 2008), por decisão sua. Listo as janelas pelo que existe no código.

| Janela | Estado | Teste | Observação |
|---|---|---|---|
| Informações básicas (grade de ícones, modo recolhido) | ✅ | ⬜ | commit `d0f1d3a` |
| Barra de atalhos no topo | ✅ | ⬜ | `MythRoHotbar.cpp` |
| Minimapa sem moldura | ✅ | ⬜ | |
| Chat em português | ✅ | 👤 | ❓ abas, cochicho, histórico de 5 com setas, tamanho de fonte |
| Diálogo de NPC (360×160, Espaço/1–9/setas) | ✅ | 👤 | |
| Árvore de skills (estilo talentos) | ✅ | 👤 | Falta Apply/Reset e "◀ nível ▶" (print LATAM) |
| Inventário: abas, usar com duplo clique, equipar | ⚠️ P1 | ⬜ | Existe; faltam descarte (arrastar para o chão com quantidade), arrastar para o atalho, peso e zeny formatados ❓ |
| Descrição do item (botão direito) | ✅ | ⬜ | `MythRoItemInfo.cpp`; ❓ cores `^RRGGBB`, slots de carta |
| Equipamento (slots, duplo clique desequipa) | ⚠️ P1 | ⬜ | Existe `Equip`; conferir 10 slots e o personagem no centro |
| Status (6 atributos com +, derivados) | ⚠️ P1 | ⬜ | Existe; conferir "Base + Bônus" e todos os derivados |
| Menu Esc e Configurações (abas, ON/OFF) | ⚠️ P2 | ⬜ | `MythRoGameMenu.cpp` existe; faltam as abas do print |
| Barras de HP/SP sob o personagem | ✅ P1 | ⬜ | Feito em 2026-10-10 (`DrawSelfBars`): 60×4 sob os pés, HP verde/amarela/vermelha, SP azul. A barra do monstro atingido também foi para baixo dos pés (verde sobre roxo escuro), sem a porcentagem. |
| Armazém Kafra | ❌ P1 | ⬜ | |
| Loja de NPC (comprar/vender) | ❌ P1 | ⬜ | |
| Janela de grupo, amigos, troca | ❌ P2 | ⬜ | |
| Janela de reviver | ❌ P1 | ⬜ | |
| Navegação `/navi`, quests (Alt+U), banco | ❌ P3 | ⬜ | Ver `docs/ref/08` |

## 07 Sons (`docs/ref/07_sons.md`)

| Regra | Estado | Teste | Observação |
|---|---|---|---|
| WAV pelos eventos do `.act`, passos | ✅ | 👤 | `PlayFrameEvent` |
| Golpe por tipo de arma, armadura | ✅ | 👤 | `WeaponSwingWav` |
| Atenuação espacial (25 células) | ✅ | 👤 | `MythRoSound.cpp` |
| BGM por mapa (`mp3nametable`) | ✅ | 👤 | `map_bgm_table.json` |
| Sons de interface (cliques, janelas) | ❌ P3 | ⬜ | O cliente local não tem esses arquivos. |
| Sons de skill | ❓ P2 | ⬜ | Vêm no campo `wav` da tabela de efeitos. |

## 09 Render (`docs/ref/09_render_korangar.md`)

Camada moderna, não precisa ser 1:1. Normais cilíndricas nos sprites: ❌ (fase HD-2D).

## Sistemas sem spec ainda (entram na rodada R2)

Nenhum destes tem pesquisa nos repositórios de referência. A rede do rAthena já entrega os pacotes de vários.

| Sistema | Estado no cliente | Prioridade |
|---|---|---|
| Loja de NPC (compra, venda, lista, desconto, peso) | ❌ | P1 |
| Kafra (salvar, armazém, teleporte, taxas) | ❌ | P1 |
| Morte e reviver (janela, ponto de save, penalidade) | ❌ | P1 |
| Grupo (criar, convidar, divisão de EXP, HP dos membros) | ❌ | P1 |
| Chat completo (abas, cochicho, grupo, guilda, comandos `/`) | ⚠️ | P1 |
| Troca entre jogadores | ❌ | P2 |
| Emoções (Alt+0–9, `/e`) | ❌ | P2 |
| Barra de cast e interrupção | ❓ | P1 |
| Ícones de buff e debuff | ❓ | P1 |
| Uso de itens: cooldown, peso, "item usado", mensagens do servidor | ❓ | P1 |
| Tabela de atributos exibida (fórmulas ATK/DEF/FLEE etc.) | ❓ | P1 |
| Login, seleção e criação de personagem | ❌ | P2 (antes do teste fechado) |
| Montaria, carrinho, falcão, Peco Peco | ❌ | P2 |
| Mapa-múndi / minimapa com marcadores | ⚠️ | P3 |
| Equipamento visual de capa e carrinho | ❌ | P1 (ver 03) |

## O servidor (rAthena) também precisa ser 1:1

Conferência a ser feita pelo Antigravity (F4). Itens:

- `PACKETVER` (hoje 20250402 em `Config/packet_lengths_*.json`; `coord/para_antigravity/001` cita 20211103; alinhar a documentação);
- rates, drops e spawns do `pre-re` contra o oficial; quais valores já foram mexidos e por quê;
- scripts de quest de troca de classe (coord 012);
- skills do pre-renewal: custo, cast, delay e dano contra a `skill_db`;
- o que é customização do Myth (Síntese, Grade, Monstro raro, Auto-Battle) deve ficar isolado e documentado em `docs/sistemas/`.

## Lista de trabalho resultante (ordem proposta)

**Divergências P1 de sistemas já especificados:**
1. Doridori (03)
2. Capa/asas e carrinho nas camadas, ordem de costas (03)
3. Flechas (05)
4. Barras de HP/SP sob o personagem (06)
5. Inventário, equipamento e status completos (06)
6. Teste jogando dos efeitos de 2ª e transclasse (05)

**Divergências P2:**
7. Câmera: clamp do terreno, `viewpointtable`, memória entre mapas
8. `FixPathTime` e correção de tempo
9. Bloqueio de clique durante o carregamento
10. `.imf`, arma guardada em idle, pickup/readyfight/stun
11. Tremor de tela, aura 99
12. Mira de skill em área

**Pesquisa R2 (Antigravity, em paralelo):** lojas, Kafra, morte, grupo, chat, troca, emoções, cast, buffs, itens, atributos, login, montaria.

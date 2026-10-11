# Roadmap do Myth of Thor

Replanejado em 2026-10-10. **Princípio:** primeiro o jogo fica 1:1 com o Ragnarok Online que já existe; só depois entram
login, teste fechado, sistemas próprios, visual HD-2D e conteúdo novo.

- Cada divergência do cliente com o RO está listada em [`FIDELIDADE.md`](FIDELIDADE.md). Aquele arquivo é a fonte do que
  falta; este aqui organiza a ordem.
- O comportamento esperado vem dos specs em `docs/ref/` (pesquisa do Antigravity nos clientes abertos OM, RB e KO).
- O servidor (rAthena) decide as regras; o cliente só mostra (`docs/PLANO_CLIENTE_FIEL.md`, seção 1).
- Uma etapa só fecha depois de **você testar jogando** e aprovar.

Legenda: ✅ feito e testado · 🟡 em parte · ⬜ falta

## F0. Auditoria de fidelidade ✅

Matriz de divergências em `docs/FIDELIDADE.md` (2026-10-10). Refazer a leitura de um sistema quando ele for mexido.

## F1. Fechar as divergências dos sistemas já especificados

Ordem do que se vê o tempo todo jogando. Quem: Claude, salvo indicação.

| # | Item (ref. na matriz) | Estado |
|---|---|---|
| 1.1 | Doridori da cabeça parada (03) e direção do corpo no mundo | 🟡 feito, falta teste |
| 1.2 | Camadas: capa/asas e carrinho feitos e vistos no jogo (03); falta teste do usuário | 🟡 |
| 1.3 | Posturas: pegar item feito (2026-10-11); faltam readyfight, stun e 2º golpe (03) | 🟡 |
| 1.4 | Flechas (05): arco do jogador feito; falta monstros arqueiros e conferir o tempo | 🟡 |
| 1.5 | Tremor de tela e aura do 99 feitos (05, 13); falta comparar a aura com o cliente real | 🟡 |
| 1.6 | Efeitos de 2ª classe e transclasse testados jogando (05, 11) | 🟡 receitas prontas, falta testar |
| 1.7 | Barra de cast, ícones de buff e debuff (05) | 🟡 existem; conferir com a R2.8 |
| 1.8 | Câmera: memória entre mapas feita; falta o clamp do terreno (precisa só da altura do GND, não dos prédios) e o `viewpointtable` (02) | 🟡 |
| 1.9 | Movimento: tempo do servidor (`FixPathTime`), bloqueio de clique ao carregar mapa (01) | ⬜ |
| 1.10 | Mira de skill em área (04) + cursor da imagem 24: anel branco no chão, ▼ e o nível da skill ao lado | 🟡 feito (anel, ▼ e nível); falta teste do usuário |
| 1.11 | Inventário, equipamento e status completos (06, 12): descarte, arrastar ao atalho, "Base + Bônus", slots | 🟡 |
| 1.12 | Barras de HP/SP sob o personagem (12) | 🟡 feito, falta teste |
| 1.13 | Skills com Apply/Reset e "◀ nível ▶", menu Esc com abas (12) | ⬜ |
| 1.14 | Porings no chão (corrigido no `f4ca760`, falta testar) | 🟡 |
| 1.15 | **Acabamento visual da HUD** contra os prints do LATAM (`docs/ref/hud_latam`, 10 e 12): texturas reais com transparência em vez de retângulos chapados, cantos arredondados, fundo translúcido nas janelas, hotbar, chat e minimapa; opção de opacidade | Claude | ⬜ pedido do usuário em 2026-10-11 |
| 1.16 | Janelas do cliente clássico que faltam (doc 12): sala de chat (Alt+C), lista de emoções (Alt+L), lista de atalhos (Alt+M / F12), info básica recolhida, nome do mapa ao entrar, **quadrado do tile sob o mouse** (cantos verdes, imagem 23), pegadas de outros jogadores (chave da aba Effect) | Claude | 🟡 sala de chat, F12 e lista de emoções feitas; faltam pegadas |
| 1.17 | Quests (Alt+U) com rastreador na tela, mapa-múndi (Ctrl+`) e `/navi` (docs 08, 12) | Claude | 🟨 quests + rastreador feitos; faltam mapa-múndi e /navi |
| 1.18 | Sobrepeso (50% trava regeneração, 90% trava combate), cores `^RRGGBB` nas descrições (doc 19) | Claude | 🟡 cores nas descrições existem; peso vermelho a 50% feito; efeitos são do servidor |
| 1.19 | Sistemas modernos do LATAM, **todos entram, adaptados depois ao pre-renovação** (decisão do usuário 2026-10-11): RODEX, Conquistas, Reputação, Agência de Aventureiros, Banco, `/navi`, troca rápida de equipamento. **JCoins fica de fora** (não haverá RO Premium/ROPs) | Claude | 🟡 Banco (Ctrl+B) feito; faltam RODEX, Conquistas, Reputação, Agência e troca rápida de equipamento |
| 1.20 | Sons de interface e o campo `wav` dos efeitos de skill (doc 07) | Claude | 🟡 sons de pegar item e equipar; botões e janelas sem arquivo no cliente local |
| 1.21 | Os 33 efeitos FUNC vazios e o terremoto (ID 666) (doc 13) | Claude | ⬜ |
| 1.22 | Menu Esc com os 5 botões (Seleção de personagem, Game settings, Set hotkeys, Sair, Voltar) e a janela **Game settings** com 5 abas (Default, Effect, Control, Graphics, Other com ON/OFF e "apply"), como em `hud_latam/INDICE.md` | Claude | 🟡 menu Esc com 5 itens (Seleção de personagem, Configurações, Atalhos, Sair, Voltar); falta unir Gráficos/Som/Efeito em abas únicas |

Conferência de cobertura das referências: `docs/COBERTURA_REFERENCIAS.md`.

Já conforme o spec e testado: movimento básico, combate e coleta, câmera, cursores, sons, números de dano, diálogo de NPC,
portais e interiores de Prontera, sentar com Insert, chat em português, árvore de skills.

## F2. Nova rodada de pesquisa R2 (Antigravity, em paralelo com F1)

Sem spec hoje. Mesmo formato do `docs/HANDOFF_ANTIGRAVITY_PESQUISA_REFERENCIAS.md` (comportamento e números, sem copiar
código GPL, citando a origem). Um doc por tema em `docs/ref/`.

| # | Tema | Prioridade |
|---|---|---|
| R2.1 | Lojas de NPC (compra, venda, lista, desconto, peso, mensagens) | P1 |
| R2.2 | Kafra (salvar, armazém, teleporte, taxas) | P1 |
| R2.3 | Morte e reviver (janela, ponto de save, penalidade de EXP) | P1 |
| R2.4 | Grupo (criar, convidar, divisão de EXP, HP dos membros) | P1 |
| R2.5 | Chat completo (abas, cochicho, grupo, guilda, comandos `/`, emoções) | P1 |
| R2.6 | Itens: usar, equipar, peso, tooltip, cartas, descarte | P1 |
| R2.7 | Atributos: fórmulas exibidas, pontos, bônus | P1 |
| R2.8 | Barra de cast, buffs e debuffs, ícones de status | P1 |
| R2.9 | Troca entre jogadores | P2 |
| R2.10 | Login, seleção e criação de personagem | P2 |
| R2.11 | Montaria, carrinho, falcão | P2 |

## F3. Implementar o que veio do R2 e fechar o ciclo Aprendiz → 2ª classe

| # | Item | Quem | Estado |
|---|---|---|---|
| 3.1 | Lojas (compra feita, falta testar venda), Kafra, morte e reviver (feito) | Claude | 🟡 |
| 3.2 | Troca de classe pelas quests originais (NPCs mapeados em coord 012) | os dois | 🟡 |
| 3.3 | Grupo (Alt+Z, /organize, /invite, /leave), cochicho, chat, emoções | Claude | 🟡 feito; faltam abas Grupo/Guilda e testar com 2 jogadores reais |
| 3.4 | Mapas de campos e cidades das classes | Antigravity → Claude | ✅ 40 mapas importados (coord 013) |
| 3.5 | Troca entre jogadores (feita e testada com 2 clientes em 2026-10-11: pedido, aceitar, oferta, travar, confirmar; menu do botão direito sobre jogador; `/exchange`); montaria, falcão e carrinho (visual) | Claude | 🟡 |
| 3.6 | Efeitos de skill: mapa completado do roBrowser (575 skills a mais), unidades no chão, skills de área sem alvo e tintas de estado (veneno, gelo, Berserk, Aceleração...) feitos em 2026-10-11. Faltam só os tipos SPR/QuadHorn/TRAIL (8 skills), os visuais anexos (sono, atordoar, maldição, silêncio) e armadilhas/canções (arquivos ausentes no GRF). Lista: `docs/EFEITOS_PENDENTES.md` | Claude | 🟡 |

## F4. Auditoria do servidor (Antigravity)

O rAthena também precisa ser 1:1 com o RO oficial:

- rates, drops, spawns e `mob_db` do pre-renewal contra o oficial; listar o que foi alterado e por quê;
- skills pre-renewal (custo, cast, delay, dano) contra a `skill_db`;
- scripts de troca de classe e de quests de habilidade;
- alinhar a documentação de `PACKETVER` (20250402 hoje);
- separar e documentar o que é customização do Myth (`docs/sistemas/`).

Saída: `docs/FIDELIDADE_SERVIDOR.md`.

## Depois da fidelidade

| Fase | Conteúdo | Quem |
|---|---|---|
| **Entrada do jogo** | 🟡 Feita em 2026-10-11 (`MythRoEntry.cpp`): login, lista de 9 personagens e criação, com fundo liso. Falta: arte, prévia do sprite, excluir personagem (o rAthena usa exclusão com prazo), PIN, criar conta | Claude |
| **Teste fechado** (~20 amigos, no seu PC) | Pacote do cliente sem editor; VPN (CGNAT da Vero), contas, backup do banco; checklist de abertura | os dois |
| **Sistemas próprios do Myth** | **Silhueta de unidades atrás de parede/árvore/prédio** (opção na aba Effect do Game settings; contorno por tipo: jogador, aliado, inimigo; pass extra com profundidade invertida ou Custom Depth; ideia do usuário 2026-10-11, **fica por último**);  Síntese, Grade, Monstro raro ★, Códice de Mímir, HUD sem comandos (o servidor já está pronto, `docs/sistemas/`); **opção a decidir:** bloqueio/aparar para escudo e duas mãos, papel de tank (`docs/sistemas/07_bloqueio_aparar.md`) | Claude |
| **Visual HD-2D** | Normais cilíndricas nos sprites (`docs/ref/09`), clima e dia/noite por mapa, poças, chuva nas unidades, **a aura do 99 (e os efeitos brilhantes em geral) precisam emitir luz no Lumen** (ponto de luz azul seguindo o personagem, como o `AMythLightFlash` dos efeitos de skill) | Claude |
| **Conteúdo** | Sprites, skills e efeitos de todas as classes até a transclasse; quests de habilidade; skills de 3ª classe adaptadas | os dois |

Sugestão, a decidir quando F3 fechar: uma sessão curta de teste com você e 2 ou 3 amigos antes do teste dos 20, para
validar a fidelidade com outras pessoas.

## Backlog (não bloqueia nada)

Movimento por WASD (código pronto, desligado), navegação `/navi`, quests (Alt+U) e banco (`docs/ref/08`), sons de
interface (o cliente local não tem os arquivos), extras de câmera (giro por teclado).

## Como retomar

`docs/HANDOFF_NOVA_CONVERSA.md`. Arquivos antigos: `docs/arquivo/`.

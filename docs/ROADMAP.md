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
| 1.2 | Camadas: capa/asas, carrinho, ordem de costas (03) | ⬜ |
| 1.3 | Posturas pickup, readyfight, stun, 2º golpe (03). A arma guardada já segue o `.act` | ⬜ conferir |
| 1.4 | Flechas (05) | ⬜ |
| 1.5 | Tremor de tela, aura do 99 (05) | ⬜ |
| 1.6 | Efeitos de 2ª classe e transclasse testados jogando (05, 11) | 🟡 receitas prontas, falta testar |
| 1.7 | Barra de cast, ícones de buff e debuff (05) | ⬜ conferir |
| 1.8 | Câmera: clamp do terreno, `viewpointtable`, memória entre mapas (02) | ⬜ |
| 1.9 | Movimento: tempo do servidor (`FixPathTime`), bloqueio de clique ao carregar mapa (01) | ⬜ |
| 1.10 | Mira de skill em área (04) | ⬜ |
| 1.11 | Inventário, equipamento e status completos (06, 12): descarte, arrastar ao atalho, "Base + Bônus", slots | 🟡 |
| 1.12 | Barras de HP/SP sob o personagem (12) | ⬜ |
| 1.13 | Skills com Apply/Reset e "◀ nível ▶", menu Esc com abas (12) | ⬜ |
| 1.14 | Porings no chão (corrigido no `f4ca760`, falta testar) | 🟡 |

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
| 3.1 | Lojas, Kafra, morte e reviver | Claude | ⬜ |
| 3.2 | Troca de classe pelas quests originais (NPCs mapeados em coord 012) | os dois | 🟡 |
| 3.3 | Grupo, cochicho, chat completo, emoções | Claude | ⬜ |
| 3.4 | Mapas de campos e cidades das classes | Antigravity → Claude | ✅ 40 mapas importados (coord 013) |
| 3.5 | Troca entre jogadores, montaria e carrinho | Claude | ⬜ |
| 3.6 | Efeitos de skill ainda faltando | Claude | ⬜ |

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
| **Entrada do jogo** | Login, seleção e criação de personagem (assets ver2/ver3 do LATAM) | Claude |
| **Teste fechado** (~20 amigos, no seu PC) | Pacote do cliente sem editor; VPN (CGNAT da Vero), contas, backup do banco; checklist de abertura | os dois |
| **Sistemas próprios do Myth** | Síntese, Grade, Monstro raro ★, Códice de Mímir, HUD sem comandos (o servidor já está pronto, `docs/sistemas/`) | Claude |
| **Visual HD-2D** | Normais cilíndricas nos sprites (`docs/ref/09`), clima e dia/noite por mapa, poças, chuva nas unidades | Claude |
| **Conteúdo** | Sprites, skills e efeitos de todas as classes até a transclasse; quests de habilidade; skills de 3ª classe adaptadas | os dois |

Sugestão, a decidir quando F3 fechar: uma sessão curta de teste com você e 2 ou 3 amigos antes do teste dos 20, para
validar a fidelidade com outras pessoas.

## Backlog (não bloqueia nada)

Movimento por WASD (código pronto, desligado), navegação `/navi`, quests (Alt+U) e banco (`docs/ref/08`), sons de
interface (o cliente local não tem os arquivos), extras de câmera (giro por teclado).

## Como retomar

`docs/HANDOFF_NOVA_CONVERSA.md`. Arquivos antigos: `docs/arquivo/`.

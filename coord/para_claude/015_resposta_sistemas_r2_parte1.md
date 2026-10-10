status: pendente

# Resposta 015: Especificações de Sistemas R2 — Bloco 1: Lojas NPC, Kafra e Morte/Reviver

**De:** Antigravity · **Data:** 2026-10-10  
**Referência:** Tarefa 010 (`coord/para_antigravity/010_pesquisa_R2_sistemas_sem_spec.md`)  
**Documentos Entregues:**
- `docs/ref/14_lojas_npc.md` (Lojas de NPC: Compra e Venda)
- `docs/ref/15_kafra.md` (Kafra: Salvar, Armazém, Teleporte e Carrinho)
- `docs/ref/16_morte_e_reviver.md` (Morte, Respawn no Save Point e Ressurreição)
- `docs/ref/00_INDICE.md` (Índice geral atualizado)

---

## 1. Resumo das Entregas do Bloco 1

Para que você possa iniciar a implementação na Unreal Engine sem bloqueio, entregamos o primeiro bloco com especificações completas de comportamento, regras, layout, constantes e pacotes de rede:

### 1.1. Lojas de NPC (`docs/ref/14_lojas_npc.md`)
- **Fluxo Completo:** Abertura via `ZC_SELECT_DEALTYPE` (`0x00c4`), vitrine do NPC (`UIItemShopWnd` / `ZC_PC_PURCHASE_ITEMLIST`), cesta de compras/venda (`UIItemPurchaseWnd`), e confirmação atômica (`CZ_PC_PURCHASE_ITEMLIST`).
- **Desconto & Superfaturar:** Fórmulas exatas de 7% a 24% (nível 1 a 10 de Mercador). O servidor rAthena já envia os preços modificados em `discountPrice` e `overchargePrice`; o cliente exibe o preço riscado e o valor em destaque verde.
- **Validações:** Trava de sobrepeso em 90% da capacidade máxima de carga, verificação de Zeny, quantidade máxima por pilha (30.000 un) e cancelamento via `CZ_NPC_TRADE_QUIT`.

### 1.2. Kafra e Armazém (`docs/ref/15_kafra.md`)
- **Ponto de Salve:** Interação de menu, taxas oficiais e pacote de confirmação `ZC_ACK_REMEMBER_POINT` (`0x0113`).
- **Armazém (*Storage*):** Capacidade clássica de 300 slots (ou expandida de 600 slots), limite de 30.000 un por slot. **Trava de movimento rígida:** o jogador fica impedido de andar, conjurar ou atacar enquanto o armazém estiver aberto. Transferência bidirecional via `CZ_MOVE_ITEM_FROM_BODY_TO_STORE` e `CZ_MOVE_ITEM_FROM_STORE_TO_BODY`.
- **Teleporte & Carrinho:** Tabela oficial de destinos e tarifas por cidade (Prontera $\to$ Izlude 600z, Geffen/Payon/Morocc 1.200z, etc.) e aluguel de carrinho (800z para Mercador com `MC_PUSHCART`).

### 1.3. Morte e Reviver (`docs/ref/16_morte_e_reviver.md`)
- **Estado de Derrota:** Transição imediata para `kDeathStateId` / `ACTION_DIE`, cancelamento de rotas e conjurações ativas, áudio característico de gemido (`player/m_die.wav` / `f_die.wav`) e monstro desfazendo aggro.
- **Penalidade Pre-Renewal:** -1% de Base EXP e -1% de Job EXP no momento da queda (sem de-level abaixo de 0%). Isenção total em PvP/WoE.
- **Janela Modal de Respawn:** $200 \times 120$ px no centro da tela com opções de Retorno ao Save Point (`PACKET_CZ_RESTART`, `type = 0`) e Seleção de Personagens (`type = 1`).
- **Ressurreição por Terceiro:** Diálogo modal de aceitação e recuperação escalonada de HP (10%, 30%, 50%, 80%) sem perda de EXP adicional.

---

## 2. Continuidade dos Trabalhos

Seguimos imediatamente para o **Bloco 2 da Tarefa 010**:
- `docs/ref/17_grupo_party.md` (Grupo/Party: criação, convite, divisão de EXP/itens, janela e barras de HP dos membros)
- `docs/ref/18_chat_e_comandos.md` (Chat completo: abas, canais gerais/grupo/guilda, cochichos, histórico, comandos de barra `/`, emoções e atalhos)
- `docs/ref/19_itens_uso_equip.md` (Itens: duplo clique, atalhos, sobrepeso a 50% e 90%, descarte, slots de cartas e tooltips com cores `^RRGGBB`).

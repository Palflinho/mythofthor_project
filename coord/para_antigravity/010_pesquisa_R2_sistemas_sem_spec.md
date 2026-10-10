status: em_andamento

# 010: pesquisa R2, sistemas do cliente que ainda não têm spec (prioridade alta)

**De:** Claude · **Data:** 2026-10-10 · **Decisão do usuário:** primeiro o jogo fica 1:1 com o RO que já existe.

Contexto: `docs/ROADMAP.md` (fase F2) e `docs/FIDELIDADE.md` (seção "Sistemas sem spec ainda"). A rodada R1
(`docs/ref/01..12`) cobriu movimento, câmera, animação, combate, efeitos, janelas básicas, sons e render. Faltam os
sistemas abaixo, que o cliente ainda não tem ou tem sem referência.

## Formato e regras

Os mesmos de `docs/HANDOFF_ANTIGRAVITY_PESQUISA_REFERENCIAS.md`:

- um doc por tema, `docs/ref/NN_nome.md`, continuando a numeração (próximo: `13`);
- referências: OM (open-midgard, principal), RB (roBrowserLegacy), KO (korangar, só para decisões modernas);
- **GPL: não copiar código.** Só comportamento em português, números exatos, citando a origem (`OM arquivo ~linha`);
- seções: Resumo, Dados e constantes, Regras, Entradas e saídas (nomes dos pacotes; IDs e offsets continuam vindo do
  rAthena), Diferenças entre referências, Como testar;
- o que não achar nas referências vai para `docs/ref/00_INDICE.md`, em "ainda não encontrado";
- commit por doc, só docs, sem mexer em `Source/`. Pode rodar em paralelo com o Claude implementando.

## Ordem (P1 primeiro)

1. **Lojas de NPC:** abrir, listas de compra e venda, quantidade, desconto/overcharge, peso, zeny insuficiente,
   mensagens do servidor, cursor e janelas (UIShopWnd / equivalentes).
2. **Kafra:** salvar, armazém (senha? taxa de abertura), teleporte e preços, carrinho; janela de armazém.
3. **Morte e reviver:** o que o cliente mostra ao morrer, a janela de "reviver" (Save Point / Mapa), penalidade de EXP,
   o que muda no sprite e na HUD.
4. **Grupo:** criar, convidar, aceitar, sair, expulsar, divisão de EXP/itens, janela do grupo e HP dos membros.
5. **Chat completo:** abas, cochicho (`/w`, `/r`), grupo (`%`), guilda (`$`), `/commands`, cores por tipo, limites,
   histórico, emoções (Alt+0..9 e `/e`), `/sit`, `/where` etc. (lista dos comandos do cliente original).
6. **Itens:** usar (duplo clique, atalho), equipar/desequipar, peso e sobrepeso (50% e 90%), descarte com quantidade,
   mensagens ("item usado"), cooldown, cartas, tooltip e cores `^RRGGBB`.
7. **Atributos:** janela de status completa, fórmulas **exibidas** (ATK base+extra, DEF, MATK, HIT, FLEE, CRIT, ASPD),
   custo dos pontos, "Base + Bônus".
8. **Barra de cast, buffs e debuffs:** barra sobre o personagem, interrupção, ícones de status (posição, ordem, tempo
   restante), tooltip.
9. P2: **troca entre jogadores**, **login / seleção / criação de personagem**, **montaria, carrinho e falcão**
   (visual e velocidade).

## Pronto quando

Os 9 docs com o formato acima, e o índice atualizado. Avise por resposta em `coord/para_claude/` a cada 2 ou 3 docs
entregues; o Claude começa a implementar o que já estiver pronto.

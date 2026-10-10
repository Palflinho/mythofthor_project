# Referência 14: Lojas de NPC (Compra e Venda)

> **Fonte:** Open-Midgard (`src/ui/UIChooseSellBuyWnd.cpp`, `UIItemShopWnd.cpp`, `UIItemPurchaseWnd.cpp`, `UIItemSellWnd.cpp`), roBrowserLegacy (`src/UI/Components/NpcStore/NpcStore.js`, `src/Engine/MapEngine/Store.js`), e rAthena (`src/map/clif.cpp`, `src/map/npc.cpp`).  
> **Escopo:** Fase 2 (F2) — Interface e protocolo de negociação comercial com NPCs mercadores.  
> **Prioridade:** P1 (Ciclo econômico básico).

---

## 1. Resumo

O sistema de Lojas de NPC permite ao jogador adquirir consumíveis, armas, munições e armaduras, além de vender itens do seu inventário para obter Zeny. A interação inicia-se ao clicar em um NPC comerciante, abrindo um diálogo de escolha ("Comprar", "Vender", "Cancelar") ou diretamente a vitrine de itens. Durante a negociação, o cliente exibe a lista do vendedor e uma cesta/carrinho de compras onde o jogador ajusta quantidades antes de confirmar a transação atômica com o servidor. O sistema aplica dinamicamente reduções e acréscimos nos preços caso o jogador possua as habilidades passivas *Desconto* ou *Superfaturar*.

---

## 2. Dados e Constantes

### 2.1. Dimensões e Posições das Janelas (Cliente Clássico / Open-Midgard)

| Janela | Largura ($W$) | Altura ($H$) | Posição Inicial $(X, Y)$ | Descrição |
|---|:---:|:---:|:---:|---|
| **`UIChooseSellBuyWnd`** | 180 px | 110 px | $(228, 146)$ | Diálogo inicial com botões "Comprar", "Vender" e "Cancelar". |
| **`UIItemShopWnd`** | 280 px | 274 px | $(128, 98)$ | Vitrine do NPC (lista de itens disponíveis para compra ou inventário para venda). |
| **`UIItemPurchaseWnd`** | 240 px | 274 px | $(412, 98)$ | Cesta de compras/venda (itens selecionados, quantidade e total em Zeny). |
| **`NpcStore` (roBrowser)** | 420 px | 360 px | Centralizado $(X, Y)$ | Interface unificada moderna com vitrine à esquerda e carrinho à direita. |

- Altura da linha de item (`kRowHeight`): **18 px**.
- Margem superior da lista (`kListTop`): **22 px**.
- Margem inferior para totalizador e botões (`kListBottomMargin`): **58 px**.
- Botões de Ação ("OK", "Cancelar"): $68 \times 20$ px ou $76 \times 22$ px.

### 2.2. Limites Operacionais e Regras de Negócio
- Quantidade máxima por transação em uma pilha: **30.000 unidades**.
- Preço máximo de um item ou transação: **2.147.483.647 Zeny** (limite de inteiro de 32 bits com sinal).
- Limite de Sobrepeso para Compra: **Não é permitido comprar se o peso resultante ultrapassar 90%** da capacidade máxima de carga do jogador (`weight > max_weight * 0.90`).
- Preço base de revenda de itens para NPCs: **50% do valor de compra padrão** (`price / 2`).

### 2.3. Fórmulas de Habilidades de Mercador

#### Desconto (*Discount* — Habilidade de Mercador `MC_DISCOUNT`):
Aplica redução percentual sobre o preço de compra de itens do NPC:
$$\text{PreçoFinal} = \left\lfloor \text{PreçoBase} \times (1.0 - \text{TaxaDesconto}) \right\rfloor$$

| Nível da Habilidade | 1 | 2 | 3 | 4 | 5 | 6 | 7 | 8 | 9 | 10 |
|---|:---:|:---:|:---:|:---:|:---:|:---:|:---:|:---:|:---:|:---:|
| **Desconto (%)** | 7% | 9% | 11% | 13% | 15% | 17% | 19% | 21% | 23% | **24%** |

#### Superfaturar (*Overcharge* — Habilidade de Mercador `MC_OVERCHARGE`):
Aplica bônus percentual sobre o valor recebido ao vender itens para o NPC:
$$\text{PreçoVendaFinal} = \left\lfloor (\text{PreçoBase} / 2) \times (1.0 + \text{TaxaSuperfaturar}) \right\rfloor$$

| Nível da Habilidade | 1 | 2 | 3 | 4 | 5 | 6 | 7 | 8 | 9 | 10 |
|---|:---:|:---:|:---:|:---:|:---:|:---:|:---:|:---:|:---:|:---:|
| **Bônus (%)** | 7% | 9% | 11% | 13% | 15% | 17% | 19% | 21% | 23% | **24%** |

> **Nota Técnica:** O cálculo oficial é realizado pelo servidor no rAthena (`pc_modifybuyvalue` e `pc_modifysellvalue`). O servidor envia na lista de compra o preço normal e o preço com desconto (`discountPrice`). O cliente exibe o preço riscado/cinza e o preço final em destaque verde.

---

## 3. Regras de Comportamento

### 3.1. Fluxo de Compra
1. O jogador clica no NPC comerciante (o cursor muda para balão de diálogo ou moeda).
2. O servidor envia `ZC_SELECT_DEALTYPE` (`0x00c4`). O cliente abre a janela `UIChooseSellBuyWnd`.
3. O jogador clica em "Comprar" (`ButtonBuy`):
   - O cliente envia a requisição de compra ao servidor (`CZ_REQ_PURCHASE_ITEMLIST`).
   - O servidor responde com `ZC_PC_PURCHASE_ITEMLIST` (`0x00c6`) contendo o vetor de itens com `{ itemId, itemType, price, discountPrice }`.
4. O cliente fecha `UIChooseSellBuyWnd` e abre `UIItemShopWnd` (esquerda) e `UIItemPurchaseWnd` (direita):
   - O mouse entra em modo de seleção comercial.
   - O jogador clica duas vezes ou arrasta um item da vitrine para a cesta.
   - Abre-se o diálogo modal de quantidade (`InputBox`): valor padrão = 1; botão "Max" calcula a quantidade máxima permitida considerando o Zeny atual e a folga de peso até 90%.
   - Ao confirmar a quantidade, o item é adicionado à lista da cesta.
   - O totalizador inferior atualiza em tempo real:
     $$\text{TotalZeny} = \sum (\text{item.discountPrice} \times \text{item.count})$$
     $$\text{PesoAdicional} = \sum (\text{item.weight} \times \text{item.count})$$
5. O jogador clica no botão "Comprar" (`OK`):
   - O cliente envia `CZ_PC_PURCHASE_ITEMLIST` (`0x00c9`) contendo o array de pares `{ ITID, count }`.
   - O servidor processa a transação e responde com `ZC_PC_PURCHASE_RESULT` (`0x00ca`).
   - Se `result == 0` (Sucesso):
     - Toca o som de moedas (`effect/coin.wav`).
     - Exibe mensagem azul no chat: `"Item(s) comprados com sucesso."` (msg 54).
     - As janelas de loja fecham.
   - Se houver falha:
     - `result == 1`: Zeny insuficiente (mensagem de erro vermelha: `"Você não possui Zeny suficiente."`, msg 55).
     - `result == 2`: Sobrepeso (mensagem de erro: `"Você está carregando muito peso para comprar."`, msg 56).
     - `result == 4`: Item fora de estoque (msg 230).

### 3.2. Fluxo de Venda
1. No diálogo inicial `UIChooseSellBuyWnd`, o jogador clica em "Vender" (`ButtonSell`).
2. O servidor responde com `ZC_PC_SELL_ITEMLIST` (`0x00c7`) contendo os itens do inventário que podem ser vendidos (`{ index, price, overchargePrice }`):
   - Itens equipados, alugados ou vinculados à conta não aparecem na lista.
3. O cliente exibe o inventário do jogador e a cesta de venda.
4. O jogador transfere os itens a serem vendidos para a cesta e define as quantidades.
5. Ao clicar em "Vender":
   - O cliente envia `CZ_PC_SELL_ITEMLIST` (`0x00cb`) com `{ index, count }`.
   - O servidor responde com `ZC_PC_SELL_RESULT` (`0x00cc`).
   - Em caso de sucesso, o Zeny é creditado no inventário do jogador e o peso reduzido imediatamente.

### 3.3. Cancelamento e Fechamento
- Pressionar `Esc` ou clicar em "Cancelar" em qualquer momento fecha as janelas e envia `CZ_NPC_TRADE_QUIT` (`0x00c8`), liberando o NPC para outros jogadores.

---

## 4. Entradas e Saídas (Protocolo de Pacotes)

### Pacotes Recebidos pelo Cliente (Servidor $\to$ Cliente):
- **`ZC_SELECT_DEALTYPE` (`0x00c4`):** Solicita ao cliente a escolha entre Compra ou Venda.
- **`ZC_PC_PURCHASE_ITEMLIST` (`0x00c6`):** Lista de itens à venda pelo NPC.
- **`ZC_PC_SELL_ITEMLIST` (`0x00c7`):** Lista de itens vendáveis do inventário do jogador com valores base e overcharge.
- **`ZC_PC_PURCHASE_RESULT` (`0x00ca`):** Resultado da compra (`0 = Sucesso`, `1 = Falta Zeny`, `2 = Sobrepeso`, `4 = Sem estoque`).
- **`ZC_PC_SELL_RESULT` (`0x00cc`):** Resultado da venda (`0 = Sucesso`, `1 = Falha`).

### Pacotes Enviados pelo Cliente (Cliente $\to$ Servidor):
- **`CZ_PC_PURCHASE_ITEMLIST` (`0x00c9`):** Confirmação de compra com lista de `{ ITID, count }`.
- **`CZ_PC_SELL_ITEMLIST` (`0x00cb`):** Confirmação de venda com lista de `{ index, count }`.
- **`CZ_NPC_TRADE_QUIT` (`0x00c8`):** Cancelamento ou fechamento da sessão de compra/venda.

---

## 5. Diferenças Entre Referências

1. **Janelas Separadas vs Janela Unificada:**
   - No **Open-Midgard** e no cliente clássico de 2008, o sistema utiliza duas janelas independentes flutuantes (`UIItemShopWnd` e `UIItemPurchaseWnd`).
   - No **roBrowserLegacy**, ambas foram unificadas no componente `NpcStore`, com layout de duas colunas mais responsivo. Para a Unreal Engine no Myth of Thor, o modelo clássico de duas colunas com posicionamento ajustável é o recomendado.
2. **Exibição do Desconto:**
   - No **OM**, itens com desconto mostram o preço base riscado em cinza escuro e o novo preço em azul/verde. No rAthena, o cálculo é 100% idêntico.

---

## 6. Como Testar

1. **Abertura do Menu:** Clicar em um NPC comerciante (ex: Vendedor de Ferramentas de Prontera `prt_in 126, 76`). Verificar se o menu "Comprar / Vender / Cancelar" abre centralizado.
2. **Carrinho de Compras:** Selecionar Poção Vermelha (50z), ajustar para 10 unidades. O totalizador deve marcar 500z.
3. **Validação de Saldo:** Tentar comprar com Zeny inferior ao total. Deve surgir a mensagem de erro no chat e a compra não deve ser consumada.
4. **Validação de Sobrepeso:** Tentar comprar uma quantidade de itens pesados que levaria o jogador a mais de 90% da capacidade de peso. A compra deve ser rejeitada com código 2.
5. **Mercador com Desconto:** Testar a mesma compra com um personagem Mercador possuindo Desconto nível 10. O preço da Poção Vermelha deve cair de 50z para 38z (24% de desconto).

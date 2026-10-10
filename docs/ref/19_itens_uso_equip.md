# Referência 19: Mecânica de Itens, Uso, Equipamentos, Peso e Tooltips

> **Fonte:** roBrowserLegacy (`src/UI/Components/Inventory/Inventory.js`, `src/UI/Components/Equipment/Equipment.js`, `src/UI/Components/ItemInfo/ItemInfo.js`, `src/DB/DBManager.js`), Open-Midgard (`src/ui/UIItemWnd.h`, `src/ui/UIEquipWnd.h`), rAthena (`src/map/itemdb.cpp`, `src/map/pc.cpp`, `src/map/clif.cpp`).  
> **Escopo:** Fase 2 (F2) — Ciclo de inventário, slots de equipamento, limites de carga e renderização de tooltips coloridas.  
> **Prioridade:** P1 (Gestão de itens e progressão de combate).

---

## 1. Resumo

O sistema de inventário gerencia a posse, transporte e ativação de itens do jogador. O inventário clássico suporta até **100 tipos de itens**, categorizados em consumíveis de cura, itens de utilidade, armas, armaduras, munições, cartas e itens diversos (*loot*). A ativação ocorre por duplo clique na mochila ou através da barra de atalhos rápidos (`F1..F9` ou `1..9`). O sistema aplica limites rígidos de sobrepeso com penalidades que afetam diretamente a regeneração e as capacidades de combate do personagem.

---

## 2. Dados e Constantes

### 2.1. Limites de Capacidade e Peso

| Parâmetro | Valor Oficial | Descrição |
|---|:---:|---|
| **Capacidade Máxima de Slots** | **100 itens distintos** | Tamanho máximo da mochila do personagem (`MAX_INVENTORY`). |
| **Limite por Pilha de Item** | **30.000 unidades** | Máximo acumulável em um único slot para itens consumíveis e de loot. |
| **Fórmula de Peso Máximo ($W_{\max}$)** | $2.000 + (\text{STR} \times 30) + \text{BônusClasse}$ | Cada 1 ponto de Força (STR) base concede **+30 unidades de capacidade de carga**. |
| **Bônus de Capacidade por Habilidade** | $+1.000 \dots +2.000$ de peso | Habilidade passiva *Aumentar Capacidade de Carga* (`MC_INCCARRY`) de Mercador concede até +2.000 de peso. |

### 2.2. Faixas de Carga e Penalidades de Sobrepeso

| Faixa de Peso | Status Visual na HUD | Efeito Mecânico no Personagem |
|---|:---:|---|
| **$0\% \dots 49.9\%$ (Normal)** | Sem ícone de alerta | **Regeneração natural ativa:** Recupera HP e SP a cada 6 segundos (em pé) ou a cada 3 segundos (sentado). Velocidade de movimento e ataque plenas. |
| **$50.0\% \dots 89.9\%$ (Sobrepeso Moderado)** | Ícone de balança **Amarela** | **Regeneração natural cessada:** O personagem **NÃO recupera HP nem SP espontaneamente** pelo tempo. Poções e magias de cura continuam funcionando. |
| **$\ge 90.0\%$ (Sobrepeso Crítico)** | Ícone de balança **Vermelha** | **Trava total de combate:** Regeneração cessada; **proibido atacar, proibido conjurar qualquer habilidade e proibido comprar itens em lojas**. |

### 2.3. Os 10 Slots Oficiais de Equipamento (`UIEquipWnd`)

| ID do Slot | Nome do Slot | Localização no Corpo | Tipos de Itens Aceitos |
|:---:|---|---|---|
| **1** | `HEAD_TOP` | Cabeça (Topo) | Elmos, chapéus, coroas, laços de cabelo. |
| **2** | `HEAD_MID` | Cabeça (Meio) | Máscaras, óculos, tapa-olhos, asas faciais. |
| **3** | `HEAD_LOW` | Cabeça (Baixo) | Tubos de fumaça, folhas na boca, flores, máscaras de tecido. |
| **4** | `WEAPON` | Mão Direita | Espadas, adagas, cajados, machados, arcos (2 mãos ocupam 4 e 5), maças, lanças. |
| **5** | `SHIELD` | Mão Esquerda | Escudos, adagas secundárias (Assassino), ou flecha/aljava. |
| **6** | `ARMOR` | Peitoral / Corpo | Armaduras, mantos mágicos, roupas de seda, trajes de batalha. |
| **7** | `GARMENT` | Costas / Ombros | Capas, capuzes, sobretudos, peles de monstro. |
| **8** | `SHOES` | Pés / Pernas | Sapatos, botas, coturnos, sandálias. |
| **9** | `ACCESSORY1` | Acessório Direito | Anéis, presilhas, brincos, amuletos, luvas. |
| **10** | `ACCESSORY2` | Acessório Esquerdo | Segundo acessório independente. |

---

## 3. Regras de Comportamento

### 3.1. Uso de Consumíveis
1. O jogador clica duas vezes sobre o ícone do item no inventário (`UIItemWnd`) ou pressiona o atalho correspondente na barra de atalhos.
2. O cliente envia `CZ_USE_ITEM` (`0x00a7`) contendo o índice do item na mochila.
3. O servidor processa o consumo:
   - Se for poção de cura: restaura instantaneamente a quantidade de HP/SP e reproduz o som `data/wav/effect/potion.wav`.
   - Se for alimento: reproduz o som `data/wav/effect/eat.wav`.
   - Se for Pergaminho de Habilidade ou Asa de Borboleta: aciona a conjuração ou teleporte correspondente.
4. O cliente decrementa em 1 a quantidade da pilha no inventário e atualiza o peso imediatamente.

### 3.2. Equipar e Desequipar
- **Equipar:** O jogador clica duas vezes no item no inventário. O cliente envia `CZ_REQ_WEAR_EQUIP` (`0x00a9`):
  - O servidor valida os requisitos do item (nível de base necessário, classe permitida e gênero).
  - Se aprovado, o item é transferido para o slot correspondente da janela de equipamentos (`UIEquipWnd`), aplicando imediatamente os bônus de ATK, DEF, atributos e cartas aos atributos do jogador.
  - Toca o som metálico de vestimenta (`data/wav/effect/equip_armor.wav` ou `equip_weapon.wav`).
- **Desequipar:** O jogador clica duas vezes sobre o slot ocupado na janela de equipamento. O cliente envia `CZ_REQ_TAKEOFF_EQUIP` (`0x00ab`). O item retorna para a mochila e seus bônus são removidos.

### 3.3. Descarte de Itens no Solo (*Drop*)
1. O jogador clica e arrasta um item para fora dos limites da janela do inventário, soltando-o sobre o chão do mapa.
2. Se o item for acumulável e houver mais de 1 unidade, abre-se a janela modal `InputBox` para selecionar a quantidade a ser jogada.
3. Ao confirmar, o cliente envia `CZ_ITEM_DROP` (`0x00a2`) com `{ index, count }`.
4. O servidor remove o item do inventário do jogador e o instancia como uma entidade tridimensional caída no piso (`CItemActor` / `DropItem`), acessível para coleta por outros jogadores após o tempo de reserva.

---

## 4. Janela de Detalhes do Item (`ItemInfo`) e Sintaxe de Cores

Ao clicar com o **botão direito do mouse** sobre qualquer item no inventário, equipamento, loja ou armazém, abre-se a janela flutuante `ItemInfo`:
- **Dimensões:** $280 \times 320$ px.
- **Ilustração Central:** Exibe a arte grande de coleção do item extraída de `data/texture/user_interface/collection/<Nome>.bmp`.
- **Cabeçalho:** Exibe o nome identificado com refinamento e slots (ex: `+8 Rondel [1]`). Itens quebrados exibem tarja vermelha e texto `[Danificado]`.
- **Slots de Cartas:** Exibe pequenos ícones de cartas incrustadas nos slots correspondentes. Clicar no botão de lupa abre a ilustração de carta em tela inteira (`CardIllustration`).

### 4.1. Sintaxe de Cores Hexadecimal Oficial (`^RRGGBB`)
A descrição dos itens no arquivo `iteminfo.lub` utiliza a sintaxe de escape `^RRGGBB` para colorir segmentos de texto sem tags HTML:
- Cada sequência `^` seguida por 6 dígitos hexadecimais altera a cor do texto subsequente até o próximo delimitador `^RRGGBB` ou até o final do parágrafo.
- **Padrão de Cores Oficiais:**
  - `^000088` ou `^0000FF`: Propriedades mágicas e bônus de atributos (ex: `^0000FFFOR + 2, INT + 1^000000`).
  - `^777777`: Requisitos de classe, nível mínimo, peso e tipo de arma (texto descritivo cinza).
  - `^FF0000`: Advertências de penalidade, itens consumíveis irreversíveis ou restrições de uso.
  - `^008800`: Efeitos passivos de conjuntos de equipamentos (*combo sets*).
  - `^000000`: Retorno à cor de texto padrão (preto).

---

## 5. Entradas e Saídas (Protocolo de Pacotes)

### Pacotes Recebidos pelo Cliente (Servidor $\to$ Cliente):
- **`ZC_ITEM_ENTRY` (`0x009d`):** Notifica item caído no chão no campo de visão.
- **`ZC_ITEM_DISAPPEAR` (`0x00a1`):** Item no chão foi recolhido ou expirou.
- **`ZC_ITEM_PICKUP_ACK` (`0x00af`):** Item recolhido do chão foi inserido no inventário.
- **`ZC_ACK_WEAR_EQUIP` (`0x00aa`):** Confirmação de equipamento inserido no slot.
- **`ZC_ACK_TAKEOFF_EQUIP` (`0x00ac`):** Confirmação de equipamento removido do slot.
- **`ZC_DELETE_ITEM_FROM_BODY` (`0x00af`):** Decremento de quantidade ou remoção de item da mochila.

### Pacotes Enviados pelo Cliente (Cliente $\to$ Servidor):
- **`CZ_USE_ITEM` (`0x00a7`):** Solicitação de consumo de item.
- **`CZ_REQ_WEAR_EQUIP` (`0x00a9`):** Solicitação para equipar item no corpo.
- **`CZ_REQ_TAKEOFF_EQUIP` (`0x00ab`):** Solicitação para retirar equipamento do corpo.
- **`CZ_ITEM_DROP` (`0x00a2`):** Solicitação para descartar item no solo.

---

## 6. Como Testar

1. **Consumo de Poções:** Colocar Poção Vermelha na barra de atalhos e pressionar `F1`. Conferir cura instantânea de HP, som de poção e decremento da quantidade na mochila.
2. **Equipamento:** Dar duplo clique em uma espada ou armadura. O item deve sumir da mochila e surgir na janela de equipamentos (`Alt+Q`), com aumento correspondente do ATK ou DEF na janela de atributos.
3. **Teste de Carga a 50%:** Carregar pedras ou poções até atingir 50% da capacidade de peso. Verificar se a balança amarela surge na HUD e se a regeneração natural de HP para de ocorrer.
4. **Teste de Carga a 90%:** Carregar peso até $\ge 90\%$. Verificar se a balança vermelha surge e se comandos de ataque e habilidades são rejeitados com a mensagem correspondente.
5. **Tooltips com Cor:** Clicar com o botão direito em um item com descrição formatada. Conferir se os bônus surgem em azul e os requisitos em cinza, sem que caracteres `^RRGGBB` vazem no texto puro.

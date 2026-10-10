# Especificação 08: Sistemas de Interface Moderna (roBrowserLegacy)

> **Origem das referências:**
> - RB: `src/UI/Components/Inventory/` (`InventoryV1`, `InventoryV2`, `InventoryV3`, `InventoryCommon.js`)
> - RB: `src/UI/Components/Navigation/` (`Navigation.js`, `Navigation.html`, `MapPathFinder.js`)
> - RB: `src/UI/Components/Quest/` (`Quest.js`, `QuestV1.html`, `QuestCommon.js`, `QuestHelperCommon.js`)
> - RB: `src/UI/Components/Achievement/` (`Achievement.js`, `Achievement.html`, `Achievement.css`)
> - RB: `src/UI/Components/Bank/` (`Bank.js`, `Bank.html`)
> - RB: `src/UI/Components/Rodex/` (`Rodex.js`, `Rodex.html`, `WriteRodex.js`, `ReadRodex.js`, `RodexIcon.js`)
> - RB: `src/UI/Components/SwitchEquip/` (`SwitchEquip.js`, `SwitchEquip.html`)
> - RB: `src/UI/Components/StatusIcons/` (`StatusIcons.js`, `StatusIcons.html`)
> - RB: `src/UI/Components/ItemCompare/` e `ItemPreview/`
> - RB: `src/UI/Components/ShortCut/` e `ShortCutOption/`

---

## 1. Resumo

O cliente `roBrowserLegacy` (RB) suporta a evolução da interface oficial do Ragnarok Online desde o modelo clássico de 2008 até revisões modernas (~2011–2021). Esses sistemas introduzem melhorias ergonômicas substanciais que não existiam no executável original de 2008: inventário particionado em abas funcionais (consumíveis, equipamentos, outros, favoritos) com travas de segurança; navegador geográfico com traçado de rotas e busca de NPCs/monstros; rastreador de missões integrado ao minimapa; conquistas com títulos e recompensas resgatáveis; correio assíncrono (RODEX); banco de Zeny com atalhos numéricos; troca rápida de equipamentos (Switch Equip) e ícones de status com relógio de recarga decrescente.

---

## 2. Dados e Constantes dos Sistemas Modernos

| Sistema / Componente | Dimensões Padrão (px) | Atalho Oficial | Recursos Principais de Interface | Origem |
|---|---|---|---|---|
| **Inventory V3** | 280 × 194 (expansível) | `Alt + E` | 4 Abas (`item`, `equip`, `etc`, `fav`), trava de drop, comparação de itens, ordenação automática | RB `InventoryV3.js:20`, `InventoryV3.html` |
| **Navigation** | 320 × 420 | `/navi <mapa> <x>/<y>` | Busca `ALL / NPC / MOB`, grafo de portais entre mapas, setas no chão e minimapa | RB `Navigation.html`, `Navigation.js` |
| **Quest & Helper** | 380 × 300 (Quest) / 220 × 90 (Helper) | `Alt + U` | Abas Ativas, Inativas, Todas; HUD flutuante de objetivos; marcadores no minimapa | RB `QuestV1.html`, `QuestHelperV1.html` |
| **Achievement** | 560 × 440 | Botão no Menu / Atalho | Rank Badge, barra de XP de conquista, categorias, títulos, botão de resgate | RB `Achievement.html`, `Achievement.js` |
| **Bank** | 280 × 240 | Kafra / Ícone no HUD | Exibição "In Bank" vs "On Hand", atalhos `+100k`, `+1M`, `+10M`, `Max` | RB `Bank.html`, `Bank.js` |
| **RODEX** | 350 × 380 | Ícone de envelope no HUD | Abas (Geral, Sistema), anexo de até 5 itens e Zeny, selo postal, `Delete All`, `Retrieve All` | RB `Rodex.html`, `WriteRodex.html` |
| **Switch Equip** | 240 × 310 | Botão na janela Alt+Q | 10 slots normais + 10 slots costume/shadow para troca em 1 clique | RB `SwitchEquip.html` |
| **StatusIcons** | Ícones de 24 × 24 | HUD fixo (topo direito) | Máscara de recarga decrescente, tick a cada 500 ms, duração infinita (`life = 9999`) | RB `StatusIcons.js:46-50` |
| **Item Compare** | 240 × 320 | Hover com Ctrl/Opção | Janela lado a lado comparando atributos com o item equipado | RB `ItemCompare.html` |
| **ShortCuts** | 4 linhas × 9 slots (36 total) | `F1–F9` / `1–9` / Teclas | Até 4 barras configuráveis via janela de opções de atalhos | RB `ShortCut.html`, `ShortCutOption.html` |

---

## 3. Regras de Comportamento e Funcionamento

### 3.1 Inventário Moderno (V1 a V3)

1. **Abas Funcionais:**
   - **Consumíveis (`item` - msg 1465):** Poções, pergaminhos, munições e comidas.
   - **Equipamentos (`equip` - msg 2050):** Armas, armaduras, escudos, capas, calçados e acessórios.
   - **Outros / Diversos (`etc` - msg 1471):** Drops de monstros, minérios e itens de fabricação.
   - **Favoritos (`fav` - msg 2051):** Itens marcados pelo jogador para impedir venda ou descarte acidental.
2. **Travas e Botões de Controle no Rodapé:**
   - **Trava de Descarte (`item_drop_lock` - msg 2843):** Quando ativada, arrastar qualquer item para fora da janela do inventário não dropa o item no chão (evita cliques errados em combate).
   - **Trava de Troca/Venda (`item_deal_lock` - msg 2952):** Bloqueia inclusão do item em janelas de negociação (`Trade`) ou lojas de jogadores (`Vending`).
   - **Comparador Rápido (`item_compare` - msg 2842):** Ativa a abertura automática da janela de comparação lado a lado ao passar o mouse sobre itens de equipamento.
   - **Ordenação Automática (`item_sort` - msg 3007):** Reorganiza os itens dentro da aba ativa por ID ou tipo alfabético.
   - **Contador e Expansão (`item_expansion` - msg 3560):** Exibe a contagem de itens ocupados sobre o total de slots (padrão 100 slots, expansível até 175 via itens consumíveis de expansão).

---

### 3.2 Sistema de Navegação (Navigation & WorldMap)

1. **Estrutura de Busca:**
   - Caixa de texto com filtro seletor:
     - `ALL`: Procura em todos os registros do cliente.
     - `NPC`: Procura na tabela de NPCs cadastrados no cliente.
     - `MOB`: Procura monstros e seus mapas de spawn nativos.
2. **Serviços de Cidade (Filtro por Checkbox):**
   - Exibe no mapa os ícones de utilidade: Distribuidor de Armas (`_weaponDealer`), Armaduras (`_armorDealer`), Utilidades (`_toolDealer`), Ferreiro (`_blacksmith`), Guia (`_guide`), Estalagem (`_inn`) e Kafra (`_kafra`).
3. **Cálculo de Rota Automática (`MapPathFinder.js`):**
   - Constrói um grafo topológico unindo mapas através de portais e rotas de teleporte de Kafra.
   - Quando o destino é selecionado (ou via comando `/navi <mapa> <x>/<y>`):
     - Calcula o menor caminho entre o mapa/célula atual e o destino.
     - Projeta **setas direcionais no chão** apontando para o próximo portal intermediário.
     - Desenha a rota pontilhada no minimapa.
     - Atualiza as coordenadas em tempo real na barra inferior: `Mouse: (X, Y)` e `Target: (X, Y)`.

---

### 3.3 Janela de Missões e Rastreador HUD (Quest & QuestHelper - Alt+U)

1. **Divisão de Painéis:**
   - Painel esquerdo: Filtros de estado:
     - `Active`: Missões em andamento recebidas do servidor (`ZC_ALL_QUEST_LIST` / `ZC_ADD_QUEST`).
     - `Inactive / Complete`: Missões finalizadas pelo personagem.
     - `All`: Histórico completo.
   - Painel direito: Lista de missões com título, ícone de estado e resumo expansível.
2. **Rastreador HUD Flutuante (`QuestHelper`):**
   - O jogador pode clicar no botão de "olho/view" de uma missão para fixá-la na tela principal como um widget semitransparente.
   - Exibe:
     - Nome da missão.
     - Alvo de monstros e contagem de abates em tempo real: `killed / limited` (ex: `Esporos: 12 / 20`).
     - Cronômetro de tempo limite ou tempo de espera para repetição (ex: `Restante: 01:23:45`).
3. **Marcadores no Minimapa:**
   - O cliente plota círculos coloridos ou ícones de pergaminho nas coordenadas `(x, y)` enviadas pelo pacote da missão para indicar onde o NPC ou objetivo se encontra.

---

### 3.4 Sistema de Conquistas e Títulos (Achievement)

1. **Visão Geral e Classificação (Rank):**
   - Exibe o emblema de Rank do personagem (`rank-badge`), pontos acumulados de conquista (`overall-points`) e barra de progresso para o próximo nível de conquista.
2. **Abas de Categoria:**
   - `General` (Geral): Exploração, interações sociais, chat, login.
   - `Adventure` (Aventura): Descoberta de cidades, mapas secretos, masmorras.
   - `Battle` (Batalha): Monstros derrotados (porings, chefes MVP, mortes em PvP).
   - `Quest` (Missões): Enredos concluídos, missões de mudança de classe.
   - `Event` (Eventos): Conquistas sazonais.
3. **Detalhes da Conquista e Recompensas:**
   - Selecionar uma conquista exibe:
     - Título e carimbo de aprovação (`detail_stamp.bmp`) se concluída.
     - Metas com barra de progresso numérica (ex: derrotar 500 monstros do tipo Amorfo).
     - Slots de recompensa:
       - **Título (`reward_title`):** Concede um título de exibição (ex: "O Caçador de Porings") que pode ser equipado acima do nome do jogador.
       - **Buff (`reward_buff`):** Efeito benéfico temporário.
       - **Item (`reward_item`):** Recompensa tangível transferida para o inventário ao clicar no botão "Receber" (`d-claim-btn`).

---

### 3.5 Banco Pessoal (Bank)

1. **Objetivo:**
   - Armazenar grandes fortunas de Zeny de forma segura e compartilhada entre os personagens da mesma conta, contornando o limite tradicional de 2.147.483.647 Zeny no inventário do personagem.
2. **Interface e Operação:**
   - Mostra simultaneamente `In Bank` (saldo guardado) e `On Hand` (saldo atual no inventário).
   - Caixa de entrada numérica para digitação manual do valor.
   - Botões de incremento rápido com soma acumulativa:
     - `+ 100,000` (100k)
     - `+ 1,000,000` (1M)
     - `+ 10,000,000` (10M)
     - `Max`: Preenche todo o Zeny em mãos (para depósito) ou o máximo disponível no banco/espaço restante (para saque).
   - Botão `Deposit`: Envia pacote de depósito ao servidor.
   - Botão `Withdraw`: Envia pacote de retirada ao servidor.

---

### 3.6 Correio Assíncrono (RODEX)

1. **Notificação no HUD (`RodexIcon`):**
   - Ícone de carta no canto superior do HUD pisca/acende ao receber o pacote avisando nova mensagem na caixa de entrada.
2. **Abas de Caixa Postal:**
   - `Normal` (Mensagens entre jogadores).
   - `System` (Mensagens automáticas de leilão, recompensas de manutenção, anúncios do servidor).
   - `Returns` (Correspondências devolvidas por tempo expirado ou rejeição).
3. **Envio de Correspondência (`WriteRodex`):**
   - Campos: Destinatário (nome do personagem), Título (assunto), Corpo da mensagem (texto livre).
   - Compartimento de Anexos: até **5 slots de itens** do inventário.
   - Campo de Zeny anexado.
   - **Taxa de Carimbo Postal:** Custo fixo (ex: 2.500 Zeny) + taxa proporcional de 2% sobre qualquer montante de Zeny enviado em anexo.
4. **Leitura e Coleta (`ReadRodex`):**
   - Prazo de expiração: 15 dias para cartas de jogadores; 30 dias para mensagens de sistema.
   - Botão `Retrieve All`: Coleta com um único clique todos os itens e Zeny contidos na carta para o inventário.
   - Botão `Delete All`: Remove em lote todas as cartas abertas que não contenham anexos pendentes.

---

### 3.7 Troca Rápida de Conjuntos de Equipamento (SwitchEquip)

1. **Estrutura de Conjuntos:**
   - Suporta alternar instantaneamente entre dois conjuntos completos de itens pré-configurados:
     - Conjunto 1 (Equipamento em uso) $\leftrightarrow$ Conjunto 2 (Reserva na mochila).
2. **Slots Suportados no Conjunto Alternativo:**
   - Topo, Meio, Baixo (Cabeça)
   - Armadura
   - Arma e Escudo
   - Capa e Calçados
   - Acessório 1 e Acessório 2
   - Slots de Equipamento Visual / Shadow (Costume Top/Mid/Bottom, Shadow Armor, Weapon, Shield, Shoes, Accessories).
3. **Disparo em Lote:**
   - Ao pressionar o botão `Swap` (`btn_change2_normal.bmp`), o cliente dispara a sequência ordenada de pacotes de desequipar e equipar para os slots configurados, sem exigir arrastar peça por peça manualmente.

---

### 3.8 Ícones de Status e Efeitos no HUD (StatusIcons)

1. **Posicionamento e Camada:**
   - Fixo no canto superior direito da tela, ordenado da direita para a esquerda em grade de ícones de 24 × 24 px.
   - Modo de clique passante (`mouseMode = CROSS`): cliques de movimentação ou ataque no chão passam através dos ícones sem travar o jogador.
2. **Temporizador Decrescente:**
   - Atualização visual a cada 500 ms (`_render_time`).
   - Máscara sombreada radial (relógio) que diminui no sentido horário conforme a proporção:
     $$\text{Progresso} = \frac{\text{TickAtual} - \text{TickInicio}}{\text{DuraçãoTotal}}$$
   - Caso especial: se a duração for informada como `9999` (ou $\infty$), o ícone permanece estático sem relógio de recarga (ex: buffs de montaria, itens fixos).
3. **Interação com Mouse:**
   - Ao passar o cursor (Hover): exibe tooltip com nome oficial do status, descrição breve e tempo restante formatado em minutos/segundos (`mm:ss`).
   - Botão direito (em buffs canceláveis pelo jogador, como buffs de comida ou montaria): envia pedido para cancelar o efeito.

---

## 4. Entradas e Saídas (Pacotes de Rede)

| Sistema | Pacote de Envio (Cliente $\rightarrow$ Servidor) | Pacote de Recebimento (Servidor $\rightarrow$ Cliente) |
|---|---|---|
| **Inventory V3** | `CZ_USE_ITEM`, `CZ_EQUIP_ITEM`, `CZ_ITEM_PICKUP` | `ZC_NORMAL_ITEMLIST3 / 4`, `ZC_EQUIPMENT_ITEMLIST3 / 4` |
| **Quest** | `CZ_QUEST_SEARCH` | `ZC_ALL_QUEST_LIST`, `ZC_ADD_QUEST`, `ZC_UPDATE_MISSION_HUNT` |
| **Achievement** | `CZ_REQ_ACHIEVEMENT_LIST`, `CZ_REQ_ACHIEVEMENT_REWARD` | `ZC_ACHIEVEMENT_LIST`, `ZC_ACHIEVEMENT_UPDATE` |
| **Bank** | `CZ_BANK_DEPOSIT`, `CZ_BANK_WITHDRAW`, `CZ_BANK_CHECK` | `ZC_BANK_DEPOSIT_ACK`, `ZC_BANK_WITHDRAW_ACK`, `ZC_BANK_STATUS` |
| **RODEX** | `CZ_REQ_RODEX_LIST`, `CZ_REQ_RODEX_WRITE`, `CZ_REQ_RODEX_RECEIVE` | `ZC_RODEX_LIST`, `ZC_RODEX_WRITE_RESULT`, `ZC_RODEX_NOTIFICATION` |
| **StatusIcons** | `CZ_DISMISS_STATUS` (se cancelável) | `ZC_STATUS_CHANGE`, `ZC_STATUS_CHANGE2`, `ZC_MSG_STATE_CHANGE` |

---

## 5. Diferenças entre Referências

1. **Disponibilidade por Época:**
   - **OM:** Não implementa esses sistemas, pois seu alvo é o executável *pre-renewal* de 2008 (onde o inventário era uma lista única de 100 itens sem abas, não existia Rodex, banco nem navegação com setas).
   - **RB:** Possui arquitetura modular para todos esses sistemas modernos (`src/UI/Components/*`), adaptando-se dinamicamente conforme a versão de pacote configurada no client (`UIVersionManager`).
   - **KO:** Focado na renderização gráfica e física da cena; suas janelas de UI são simplificadas em Rust/Egui, sem a suíte completa de Rodex/Conquistas.
2. **Recomendação de Adoção para o Myth of Thor (Unreal Engine):**
   - **Prioridade 1 (QoL Essencial):** Abas de inventário (`Inventory V3`), Ícones de Buff com temporizador (`StatusIcons`) e Barra de atalhos expandida (`ShortCuts`).
   - **Prioridade 2 (Exploração):** Rastreador de missões no HUD (`QuestHelper`) e Navegação com setas no chão (`Navigation`).
   - **Prioridade 3 (Sistemas Avançados):** Banco (`Bank`), Correio (`Rodex`) e Conquistas (`Achievement`).

---

## 6. Como Testar

1. **Abas de Inventário:**
   - Abrir o inventário (`Alt+E`): conferir se poções caem na aba Consumível, armas na aba Equipamento, e se arrastar um item com o cadeado ativado impede jogá-lo no chão.
2. **Ícones de Status:**
   - Usar uma habilidade de buff (ex: Aumentar Agilidade ou Bênção): conferir se o ícone correspondente surge no canto superior direito e se a máscara de recarga esvazia progressivamente até o buff expirar.
3. **Navegador `/navi`:**
   - Digitar `/navi prt_fild08 150/150`: conferir se a janela abre com a rota calculada e se as setas indicativas de direção surgem no chão do mundo apontando para o portal de saída de Prontera.

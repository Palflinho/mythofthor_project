# Referência 15: Serviços da Kafra e Armazém (Storage)

> **Fonte:** Open-Midgard (`src/ui/UIStorageWnd.h`, `src/gamemode/GameMode.cpp`), roBrowserLegacy (`src/UI/Components/Storage/StorageCommon.js`, `src/UI/Components/NpcMenu/NpcMenu.js`, `src/Engine/MapEngine/Storage.js`), rAthena (`npc/kafras/kafras.txt`, `src/map/storage.cpp`, `src/map/clif.cpp`).  
> **Escopo:** Fase 2 (F2) — Interface e mecânicas dos serviços da Corporação Kafra.  
> **Prioridade:** P1 (Gestão de inventário e locomoção global).

---

## 1. Resumo

A Corporação Kafra é a entidade de suporte mais vital do mundo de Ragnarok Online. Presente em todas as capitais, vilas e entradas de calabouços, suas funcionárias oferecem quatro serviços fundamentais ao jogador:
1. **Ponto de Retorno (Salvar):** Define o local de renascimento do jogador em caso de morte ou uso de Asas de Borboleta.
2. **Armazém da Conta (*Storage*):** Cofre compartilhado entre todos os personagens da mesma conta naquele servidor.
3. **Serviço de Teleporte:** Transporte instantâneo para cidades vizinhas ou pontos estratégicos mediante tarifa em Zeny.
4. **Aluguel de Carrinho:** Concede um carrinho de carga com capacidade adicional a personagens da classe Mercador e evoluções.

---

## 2. Dados e Constantes

### 2.1. Dimensões da Interface do Armazém (`UIStorageWnd`)

| Elemento | Dimensão | Posição Padrão | Descrição |
|---|:---:|:---:|---|
| **Janela de Armazém Clássica** | $280 \times 295$ px | $(200, 150)$ | Grade de armazenamento ao lado do inventário da mochila. |
| **Janela Expandida (roBrowser)** | $320 \times 360$ px | $(200, 180)$ | Versão com abas filtradas (Itens, Equipamentos, Etc, Cartas). |
| **Capacidade Clássica (Pre-Re)** | **300 slots** | — | Máximo de tipos de itens distintos armazenáveis por conta. |
| **Capacidade Expandida (Re)** | **600 slots** | — | Expansão de abas da Kafra para versões modernas. |
| **Limite por Pilha de Item** | **30.000 unidades** | — | Quantidade máxima acumulável em um único slot. |
| **Altura da Linha de Item** | 32 px (com ícone $24 \times 24$) | — | Ícone do item, nome, refinamento e quantidade à direita. |

### 2.2. Tabela de Tarifas de Serviços da Kafra (Pre-Renewal Oficial)

| Serviço | Preço em Zeny | Restrições / Condições |
|---|:---:|---|
| **Salvar Ponto de Retorno** | **Grátis** (0z) ou 20z/50z | Disponível para todos os personagens. Cidades especiais (como Alberta) cobram taxa simbólica de 20z. |
| **Abrir Armazém** | **20z a 40z** | Cidades básicas (Prontera, Geffen, Payon, Morocc): **20z**. Cidades avançadas (Al De Baran, Yuno, Comodo): **40z**. **Grátis para Aprendizes**. |
| **Aluguel de Carrinho** | **800z** | Exige classe Mercador, Ferreiro ou Mestre-Ferreiro e habilidade passiva *Usar Carrinho* (`MC_PUSHCART` Lv 1). |

### 2.3. Tabela de Rotas Oficiais de Teleporte e Tarifas

| Ponto de Origem | Destino | Tarifa em Zeny |
|---|---|:---:|
| **Prontera** | Izlude | 600z |
| **Prontera** | Geffen | 1.200z |
| **Prontera** | Payon | 1.200z |
| **Prontera** | Morocc | 1.200z |
| **Izlude** | Prontera | 600z |
| **Izlude** | Geffen | 1.200z |
| **Izlude** | Payon | 1.200z |
| **Izlude** | Morocc | 1.200z |
| **Geffen** | Prontera | 1.200z |
| **Geffen** | Al De Baran | 1.200z |
| **Payon** | Prontera | 1.200z |
| **Payon** | Alberta | 1.200z |
| **Morocc** | Prontera | 1.200z |
| **Morocc** | Alberta | 1.800z |
| **Morocc** | Comodo | 1.800z |
| **Alberta** | Prontera | 1.200z |
| **Alberta** | Payon | 1.200z |
| **Al De Baran** | Geffen | 1.200z |
| **Al De Baran** | Yuno | 1.800z |

---

## 3. Regras de Comportamento

### 3.1. Diálogo e Menu da Kafra
1. O jogador aproxima-se e clica na funcionária Kafra (alcance de interação $\le 3$ células).
2. O servidor envia o pacote de diálogo `ZC_SAY_DIALOG` e em seguida o menu de opções `ZC_SELECT_MENU` (`0x00b7`).
3. O cliente exibe o menu contextual estilizado (`NpcMenu`):
   - Opção 1: *Salvar Ponto de Retorno*
   - Opção 2: *Usar o Armazém*
   - Opção 3: *Serviço de Teleporte*
   - Opção 4: *Alugar um Carrinho* (oculto ou cinza se o personagem não for Mercador)
   - Opção 5: *Outros Serviços / Cancelar*

### 3.2. Ponto de Salve (*Save Point*)
- Ao selecionar a opção, o script do servidor debita a taxa (se houver) e executa o comando `savepoint map,x,y`.
- O servidor confirma a operação via pacote `ZC_ACK_REMEMBER_POINT` (`0x0113`).
- A Kafra agradece com o texto: `"Seu Ponto de Retorno foi salvo aqui com sucesso!"`.
- Toca o efeito sonoro de campainha de serviço (`effect/ting.wav`).

### 3.3. Uso do Armazém (*Storage*)
1. Ao escolher o armazém, o servidor verifica se o jogador possui a taxa de 20z/40z.
2. Se o saldo for suficiente, o servidor deduz o Zeny e envia a lista de itens armazenados através de `ZC_NOTIFY_STOREITEMLIST` (`0x00a5` ou pacote expandido).
3. **Trava de Movimento:** Enquanto o armazém estiver aberto, o jogador entra em estado de negociação/congelamento:
   - Não pode se mover com cliques no chão ou teclado WASD.
   - Não pode conjurar habilidades ou atacar.
   - Se o jogador sofrer dano, for empurrado (*Knockback*) ou o servidor ordenar teleporte, o armazém fecha automaticamente.
4. **Transferência de Itens (Inventário $\leftrightarrow$ Armazém):**
   - **Mochila $\to$ Armazém:** O jogador clica duas vezes no item da mochila ou o arrasta para a janela do armazém.
     - Se o item for acumulável (poções, munições, itens de refino), abre a janela modal de quantidade (`InputBox`): padrão = quantidade total na mochila.
     - O cliente envia `CZ_MOVE_ITEM_FROM_BODY_TO_STORE` (`0x00a7`) com `{ index, count }`.
     - O servidor atualiza o peso do jogador e adiciona o item ao armazém.
     - Itens não transferíveis (intransferíveis por script, alugados ou vinculados à alma) exibem erro no chat e não se movem.
   - **Armazém $\to$ Mochila:** O jogador clica duas vezes no item do armazém ou o arrasta para a mochila.
     - O cliente envia `CZ_MOVE_ITEM_FROM_STORE_TO_BODY` (`0x00a8`) com `{ index, count }`.
     - Se a transferência for causar sobrepeso superior a 90% da capacidade de carga do jogador, o servidor rejeita o comando.
5. **Fechamento do Armazém:**
   - Clicar no botão "Fechar" da janela ou pressionar a tecla `Esc` envia `CZ_CLOSE_STORE` (`0x00a9`).
   - O servidor responde com `ZC_CLOSE_STORE` (`0x00aa`), destravando o movimento do personagem.

### 3.4. Serviço de Teleporte
1. Ao selecionar a opção, abre-se um segundo menu com as cidades disponíveis a partir daquela Kafra e suas respectivas taxas.
2. Se o jogador tiver Zeny suficiente, o servidor debita o valor, cancela qualquer movimento pendente e dispara a rotina de teleporte (`warp map,x,y`).
3. O cliente executa a animação de teleporte e som de portal (`effect/teleport.wav`), entrando na tela de carregamento do mapa de destino.

### 3.5. Aluguel de Carrinho (*Pushcart*)
1. Apenas Mercadores, Ferreiros e Alquimistas (e suas transclasses) com a habilidade `MC_PUSHCART` aprendida (mínimo Nível 1) podem alugar.
2. Custo: 800 Zeny.
3. Ao confirmar, o servidor ativa a flag de montaria de carrinho no ator. O sprite do jogador passa a desenhar o carrinho engatado nas costas em todas as 8 direções.
4. O atalho `Alt+W` passa a abrir o inventário do carrinho (capacidade máxima de 8.000 unidades de peso e 100 slots).

---

## 4. Entradas e Saídas (Pacotes de Rede)

### Pacotes Recebidos pelo Cliente (Servidor $\to$ Cliente):
- **`ZC_NOTIFY_STOREITEMLIST` (`0x00a5` / `0x01e8`):** Lista inicial completa de todos os itens contidos no armazém.
- **`ZC_ADD_ITEM_TO_STORE` (`0x00a6`):** Notifica que um novo item foi inserido no armazém com sucesso.
- **`ZC_DELETE_ITEM_FROM_STORE` (`0x00a8`):** Notifica a remoção ou redução de quantidade de um item do armazém.
- **`ZC_CLOSE_STORE` (`0x00aa`):** Confirmação de que o armazém foi encerrado pelo servidor.
- **`ZC_ACK_REMEMBER_POINT` (`0x0113`):** Confirmação de registro do novo Ponto de Salve.

### Pacotes Enviados pelo Cliente (Cliente $\to$ Servidor):
- **`CZ_MOVE_ITEM_FROM_BODY_TO_STORE` (`0x00a7`):** Transfere item do inventário do corpo para o armazém `{ index, count }`.
- **`CZ_MOVE_ITEM_FROM_STORE_TO_BODY` (`0x00a8`):** Retira item do armazém para o corpo `{ index, count }`.
- **`CZ_CLOSE_STORE` (`0x00a9`):** Notifica o servidor que o jogador fechou a janela do armazém.

---

## 5. Diferenças Entre Referências

1. **Abas Filtradas:**
   - No cliente clássico de 2008 e no **Open-Midgard**, o armazém é uma lista contínua única de 300 slots sem paginação nem abas.
   - No **roBrowserLegacy**, há suporte a abas de separação rápida (Itens, Armaduras, Armas, Munições, Cartas, Etc), o que melhora consideravelmente a usabilidade em resoluções modernas sem violar a fidelidade das taxas e pacotes.

---

## 6. Como Testar

1. **Interação com a Kafra de Prontera (`prontera 152, 187`):**
   - Clicar na NPC e conferir abertura limpa do menu com 5 opções.
2. **Teste de Ponto de Salve:**
   - Selecionar "Salvar". Verificar se a mensagem de confirmação aparece no chat. Em seguida, usar comando `/die` ou ser derrotado: o personagem deve renascer em frente à Kafra de Prontera.
3. **Teste de Armazém:**
   - Abrir o armazém com saldo inferior a 20z: o servidor deve negar a abertura.
   - Abrir com saldo suficiente: a taxa deve ser debitada, a janela do armazém deve surgir ao lado do inventário e o personagem deve ficar impedido de andar.
   - Transferir 50 Poções Vermelhas: verificar atualização imediata de quantidade em ambas as janelas.
   - Pressionar `Esc`: o armazém deve fechar e o movimento com WASD/clique deve ser restaurado.
4. **Teste de Teleporte para Izlude:**
   - Escolher Teleporte $\to$ Izlude (600z): conferir débito do saldo e carregamento do mapa `izlude.gat`.

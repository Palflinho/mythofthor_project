# Especificação 04: Clique, Alvos e Combate — Prioridade, Alcance e Interações

> **Origem das referências:**
> - **OM (Open-Midgard):** `src/gamemode/GameMode.cpp` (linhas 3120-3225, 4185-4200, 4800-4950, 5165-5265, 5290-5320, 6430-6550, 6710-6865, 9810-9920), `src/world/World.cpp` (linhas 6230-6350), `src/world/GameActor.cpp` (linhas 4485-4515, 4805-4815).
> - **RB (roBrowserLegacy):** `src/Controls/MapControl.js` (linhas 100-160), `src/Engine/MapEngine/Main.js` (linhas 80-110, 1015-1025), `src/Renderer/Entity/EntityAction.js`.

---

## 1. Resumo

A interação do cursor do mouse com o mundo de Ragnarok Online obedece a uma hierarquia estrita de detecção e prioridade de clique: Interface $\rightarrow$ Habilidade Engatilhada $\rightarrow$ NPC $\rightarrow$ Monstro $\rightarrow$ Item no Chão $\rightarrow$ Portal $\rightarrow$ Terreno/Célula. O cursor altera dinamicamente sua animação conforme a entidade sob o ponteiro (Seta, Balão de Fala, Espada de Ataque, Mão de Pegar Item, Redemoinho de Portal ou Ícone de Bloqueio). O clique em monstros inicia combate contínuo automático (`0x07`); se o alvo estiver além do alcance da arma, o cliente move o personagem até a célula mais próxima de ataque antes de desferir os golpes, reagindo ao pacote de alcance insuficiente (`ZC_ATTACK_FAILURE_FOR_DISTANCE`). O recolhimento de itens exige distância máxima de 1 célula e aciona a animação de agachar (`PICKUP`).

---

## 2. Dados e Constantes

| Parâmetro | Valor | Unidade | Descrição / Referência |
|---|---|---|---|
| Prioridade Máxima de Clique | `UI > Skill > NPC > Monstro > Item > Portal > Chão` | ordem | Hierarquia de interceptação do raio do mouse (`GameMode.cpp:9810-9885`). |
| Margem Magnética de Monstro (`kEnemyHoverStickyPadding`) | `12.0` | pixels | Área de tolerância ao redor do sprite do monstro para facilitar o clique (`World.cpp:6310`). |
| Alcance Base de Ataque Corpo a Corpo | `1` | célula | Distância máxima Chebyshev ($\max(\|dx\|, \|dy\|) \le 1$) para armas de curta distância. |
| Alcance Padrão de Coleta de Itens | `1` | célula | Distância máxima para conseguir pegar um item do chão (`GameMode.cpp:5259`). |
| Intervalo de Reenvio de Coleta (`kPickupRetryIntervalMs`) | `400` | ms | Período mínimo entre tentativas consecutivas de pegar o mesmo item (`GameMode.cpp:4190`). |
| Intervalo de Reenvio de Ataque (`kAttackRetryIntervalMs`) | `1200` | ms | Período de segurança para reemitir pedido de ataque ao servidor (`GameMode.cpp:4191`). |
| Intervalo de Caminhada de Perseguição (`kAttackChaseRequestIntervalMs`) | `1200` | ms | Intervalo entre pacotes de movimento ao perseguir um monstro em fuga (`GameMode.cpp:4190`). |
| Código de Ação de Ataque Contínuo | `0x07` | byte | Ação enviada no pacote de ataque quando em modo contínuo (padrão `/noctrl`) (`GameMode.cpp:5168`). |
| Código de Ação de Ataque Único | `0x00` | byte | Ação enviada no pacote para desferir um único golpe (`GameMode.cpp:5165`). |
| Ação de Animação ao Pegar Item | `24` | baseAction | Família de ações de agachar (`kGameActorPickupStateId`) (`GameActor.cpp:4802`). |

---

## 3. Prioridade de Detecção e Formas do Cursor

### 3.1. Hierarquia de Detecção Sob o Cursor
A cada frame, `UpdateGameplayCursor` (`GameMode.cpp:3123-3225`) avalia o que está sob o mouse na seguinte ordem:
1. **Interface do Usuário (HUD/Janelas):**
   Se o cursor estiver sobre qualquer janela aberta, barra de atalhos, minimapa ou caixa de chat (`IsWorldHoverBlockedByUi`), a interação com o mundo é suspensa e o cursor permanece `Arrow`.
2. **Modo de Rotação de Câmera:**
   Se o botão direito estiver pressionado girando a visão (`m_canRotateView`), o cursor exibe o ícone de rotação (`CameraPan`).
3. **Modo de Uso de Habilidade (Skill Engatilhada):**
   - Habilidade de alvo em área (`GroundTarget`): exibe mira circular (`SkillTarget`) sobre células caminháveis ou `Forbidden` sobre áreas bloqueadas.
   - Habilidade de alvo em entidade (`ActorTarget`): exibe mira vermelha (`SkillTargetAlt`) sobre monstros ou jogadores hostis, e `Forbidden` sobre NPCs ou células vazias.
4. **NPC (Entidade de Diálogo):**
   Se houver um NPC sob o ponteiro (`IsNpcLikeHoverActor`), o cursor muda para **Balão de Diálogo** (`Talk`).
5. **Monstro (Alvo de Ataque):**
   Se houver um monstro sob o ponteiro (`IsMonsterLikeHoverActor`), o cursor muda para **Espada de Ataque** (`Attack`). O sistema aplica a margem magnética de 12 pixels (`kEnemyHoverStickyPadding`) para que o cursor não oscile com monstros velozes.
6. **Item no Chão:**
   Se não houver ator vivo clicável e houver um item descartado (`FindHoveredGroundItemScreen`), o cursor muda para **Mãozinha de Coleta** (`Item`).
7. **Portal / Warp:**
   Se houver um ponto de teleporte na célula apontada (`HasWarpAtAttrCell`), o cursor muda para **Redemoinho de Portal** (`Portal`).
8. **Chão do Mapa:**
   - Célula navegável (`flag == 0`): cursor normal (`Arrow`).
   - Célula intransponível (`flag != 0`): cursor de proibido (`Forbidden`).

---

## 4. Comportamento e Regras de Combate

### 4.1. Ataque Contínuo vs Ataque Único
- Por padrão no RO clássico (equivalente ao comando `/noctrl` ou com a tecla Ctrl pressionada), clicar em um monstro com o botão esquerdo ativa o **Ataque Contínuo**:
  - Envia o pacote `CZ_REQUEST_ACT` com o código `action = 0x07` (`kActionRequestContinuousAttack`).
  - O cliente trava o ID do monstro em `m_lastLockOnMonGid`.
  - O jogador ataca repetidamente no ritmo da sua ASPD sem necessidade de clicar de novo.
- Se o jogador clicar no chão, em outro monstro ou se o monstro morrer, o travamento de combate é cancelado (`ClearAttackChaseHint`).

### 4.2. Alcance e Aproximação ao Alvo
O alcance de ataque depende da arma equipada (1 célula para desarmado/adagas/espadas; 2 para lanças; 3 a 5 para lanças longas; 4 a 9 para arcos e armas de fogo).

1. **Monstro Dentro do Alcance:**
   - O personagem interrompe qualquer movimento e inicia o ciclo de ataque imediatamente.
2. **Monstro Fora do Alcance:**
   - O cliente envia `SendAttackRequestPacket(targetGid, 0x07)`.
   - O servidor rAthena responde informando que o alvo está longe (`ZC_ATTACK_FAILURE_FOR_DISTANCE` / `0x0139`).
   - O cliente calcula a célula de perseguição ideal (`ResolveAttackChaseCell` em `GameMode.cpp:4850-4920`):
     - Para combate corpo a corpo (alcance = 1): calcula o degrau de aproximação `stepX = Sign(targetX - sourceX)` e busca a célula livre adjacente mais favorável `(targetX - stepX, targetY - stepY)`.
     - Para combate à distância (alcance > 1): projeta um anel concêntrico de raio igual ao alcance do arco ao redor do alvo e seleciona a célula livre mais próxima do jogador.
   - O cliente envia `CZ_REQUEST_MOVE` para essa célula de aproximação.
   - Enquanto o jogador caminha, `PumpAttackChaseRequest` verifica a cada tick se o personagem atingiu a distância de disparo. Assim que entrar no raio, o movimento é concluído e o ataque contínuo começa.

---

## 5. Coleta de Itens do Chão

### 5.1. Regras de Distância e Animação
1. **Clique no Item:**
   - O jogador clica sobre um item no chão (cursor de mãozinha).
   - O cliente memoriza o identificador do item (`objectAid`) em `m_pickupReqItemNaidList`.
2. **Dentro do Alcance ($\le 1$ Célula):**
   - Se a distância Chebyshev entre o jogador e o item for menor ou igual a 1 célula:
     - Envia imediatamente o pacote `CZ_ITEM_PICKUP` (`SendTakeItemRequestPacket`).
     - O personagem toca a animação de agachar (`SetState(kGameActorPickupStateId)` $\rightarrow$ ação `24`), congelando brevemente o movimento até recolher o item.
3. **Fora do Alcance ($> 1$ Célula):**
   - O cliente calcula a célula caminhável livre mais próxima do item (`ResolveGroundItemPickupCell`).
   - Envia `CZ_REQUEST_MOVE` para caminhar até o item.
   - A cada frame, `PumpPendingPickupRequest` monitora a posição. Quando a distância chegar a 1 célula, despacha o pacote de coleta e reproduz a animação de pegar.

---

## 6. Interação com NPCs

1. **Clique no NPC:**
   - Quando o cursor exibe o balão `Talk` e o jogador clica com o botão esquerdo:
   - O cliente envia imediatamente o pacote `CZ_CONTACTNPC` (`0x0090`) com o `targetGid` do NPC (`GameMode.cpp:6445`).
2. **Travamento de Ações:**
   - A abertura de diálogo com NPC (`HasActiveNpcDialog`) **bloqueia imediatamente** novas requisições de movimento, ataques ou coleta pelo mouse (`GameMode.cpp:9813-9824`). Todos os cliques subsequentes na tela passam a ser processados pela janela de diálogo até que ela seja encerrada com `Close`.

---

## 7. Entradas e Saídas

### Entradas
- **Clique Esquerdo:** Posição do cursor em coordenadas de tela `(screenX, screenY)`.
- **Pacotes Recebidos:**
  - `ZC_ATTACK_RANGE` (`0x013a`): Atualiza o alcance de ataque da arma atual do jogador.
  - `ZC_ATTACK_FAILURE_FOR_DISTANCE` (`0x0139`): Avisa que o alvo está fora de alcance.
  - `ZC_ITEM_PICKUP_ACK`: Confirma a coleta do item e remove-o do chão.
  - `ZC_ITEM_FALL_ENTRY` / `ZC_ITEM_DISAPPEAR`: Entrada e saída de itens na área visível.

### Saídas
- **Pacote `CZ_REQUEST_ACT` / `CZ_REQUEST_ACTION`:**
  - `targetGid`: identificador do monstro ou jogador alvo.
  - `action`: `0x07` para ataque contínuo, `0x00` para ataque simples.
- **Pacote `CZ_ITEM_PICKUP` (`0x009f` / `0x0362`):**
  - `objectAid`: identificador do item no chão a ser recolhido.
- **Pacote `CZ_CONTACTNPC` (`0x0090`):**
  - `targetGid`: identificador do NPC para iniciar diálogo.

---

## 8. Diferenças entre as Referências

1. **Magnetismo de Seleção de Monstro:**
   - O **OM** implementa uma caixa estendida com margem de 12 pixels (`kEnemyHoverStickyPadding`) e cálculo de proximidade (`ComputePointToRectDistanceSq`) para evitar que monstros pequenos e rápidos saiam da mira durante cliques rápidos.
   - O **RB** utiliza o teste estrito da caixa delimitadora do sprite (`EntityRender.js:boundingRect`), com largura mínima de 60 pixels para monstros e 30 pixels para itens.
2. **Fila de Aproximação de Alvo:**
   - No **OM**, se o jogador clicar em um monstro distante, o cliente já inicia a rota de aproximação local paralelamente ao envio do pacote de ataque, acelerando o tempo de resposta do personagem.

---

## 9. Como Testar

1. **Teste de Prioridade (Item sob Monstro):**
   - Mate um monstro para dropar itens no chão. Deixe outro monstro andar exatamente por cima do item caído.
   - Passe o mouse sobre o monstro: o cursor deve ser de Espada (`Attack`).
   - Passe o mouse na borda do item fora do monstro: o cursor muda para Mãozinha (`Item`).
2. **Teste de Alcance e Caminhada Automática de Ataque:**
   - Fique a 10 células de distância de um monstro.
   - Dê um único clique sobre o monstro.
   - **Resultado esperado:** O personagem começa a caminhar em direção ao monstro, para exatamente a 1 célula de distância (ou no alcance do arco) e desfere ataques automáticos contínuos sem novos cliques.
3. **Teste de Coleta à Distância:**
   - Fique a 5 células de um item no chão e clique nele.
   - **Resultado esperado:** O personagem caminha até o item, para na célula adjacente, executa a animação de agachar (`PICKUP`) e o item entra no inventário.
4. **Teste de Diálogo com NPC:**
   - Clique em um NPC a distância. O pacote de diálogo é acionado e a janela de diálogo abre; enquanto a janela estiver aberta, cliques no chão não movem o personagem.

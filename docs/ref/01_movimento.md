# Especificação 01: Movimento e Pathfinding

> **Origem das referências:**
> - **OM (Open-Midgard):** `src/pathfinder/PathFinder.h`, `src/pathfinder/PathFinder.cpp` (linhas 1-320), `src/world/GameActor.h`, `src/world/GameActor.cpp` (linhas 165-200, 1090-1170, 1630-1660, 4390-4450), `src/gamemode/GameMode.cpp` (linhas 4185-4255, 4520-4570, 4750-4850, 6550-6650, 9650-9700), `src/gamemode/GameModePacket.cpp` (linhas 5530-5545, 5635-5700, 6740-6895, 6904-6945, 7930-8055).
> - **RB (roBrowserLegacy):** `src/Renderer/Entity/EntityWalk.js` (linhas 1-820), `src/Controls/MapControl.js` (linhas 60-165), `src/Engine/MapEngine.js` (linhas 250-290, 1030-1090).

---

## 1. Resumo

O sistema de movimento no cliente do Ragnarok Online é puramente preditivo com autoridade total do servidor (*client-side prediction* sincronizada por carimbo de data/hora do servidor). O mapa do cliente é baseado em uma grade 2D de células navegáveis (GAT, 0 = livre). Ao clicar ou arrastar o cursor no chão, o cliente tenta primeiro uma reta direta (Bresenham) e, se houver obstáculos, executa busca A* em 8 direções estritas com proibição de corte de quinas. Cada passo ortogonal tem custo 10 e o diagonal 14. O cliente interpola linearmente a posição fracionária baseando-se no tempo acumulado de cada nó, partindo sempre da posição real flutuante do ator no momento do clique (o que elimina estalos de posição ao mudar de rota). Pacotes de correção (`ZC_STOPMOVE`) que chegam atrasados em relação ao segmento já percorrido são descartados para evitar o efeito "passo para trás".

---

## 2. Dados e Constantes

| Parâmetro | Valor | Unidade | Significado / Contexto |
|---|---|---|---|
| Custo de Passo Ortogonal (`StepCost`) | `10` | pontos | Distância relativa para passos Norte, Sul, Leste, Oeste (`PathFinder.cpp ~linha 58`). |
| Custo de Passo Diagonal (`StepCost`) | `14` | pontos | Aproximação inteira clássica de $10 \times \sqrt{2} \approx 14.14$ (`PathFinder.cpp ~linha 58`). |
| Fator Diagonal Float (`DIAGONAL_FACTOR`) | `1.414` ou $\sqrt{2}$ | multiplicador | Multiplicador de tempo para segmentos diagonais no RB (`EntityWalk.js ~linha 26`). |
| Velocidade de Passo Padrão (`speedFactor` / `walk.speed`) | `150` | ms / célula | Tempo base para andar 1 célula ortogonal. Menor = mais rápido (ex: 100 com Montaria / AGI Up). |
| Duração de Passo Ortogonal | `speed` | ms | $(10 \times \text{speed}) / 10 = \text{speed}$ ms. |
| Duração de Passo Diagonal | `1.4 \times \text{speed}` | ms | $(14 \times \text{speed}) / 10 = 1.4 \times \text{speed}$ ms (ex: 150 ms base $\rightarrow$ 210 ms diagonal). |
| Intervalo de Clique Segurado (`kHeldMoveRequestIntervalMs`) | `75` | ms | Intervalo mínimo entre envios do pacote de movimento com botão segurado no OM (`GameMode.cpp ~linha 4185`). |
| Trava de Repetição no RB | `200` | ms | Delay mínimo de segurança antes de permitir novo pedido no RB (`MapEngine.js ~linha 1067`). |
| Timeout de Repetição do Timer de Andar | `500` | ms | Intervalo do timer `setTimeout(walkIntervalProcess, 500)` no RB (`MapEngine.js ~linha 1097`). |
| Limiar de Retarget de Clique Segurado | `2` | células | Variação máxima de destino antes de forçar novo pacote (`GameMode.cpp ~linha 4186`). |
| Extensão Direcional de Clique Segurado | `4` | células | Margem de projeção ao segurar o mouse (6 se speed $\le$ 60, 5 se speed $\le$ 90) (`GameMode.cpp ~linha 4806`). |
| Limiar Próximo ao Fim de Passo | `80` | ms | Janela antes do fim do caminho onde mudança de direção é permitida (150 ms se speed $\le$ 60, 120 ms se speed $\le$ 90). |
| Timeout de Confirmação de Movimento (`kMoveAckTimeoutMs`) | `1000` | ms | Tempo limite de espera pelo pacote do servidor antes de liberar novo envio (`GameMode.cpp ~linha 4189`). |
| Tolerância Máxima de Desync antes de Teleporte | `16` | células | Distância a partir da qual o cliente não interpola e estala direto para a posição do servidor (`EntityWalk.js ~linha 370`). |
| Limite de Ajuste Temporal (`FixPathTime`) | `[-2500, +2500]` | ms | Clamp da diferença total de tempo entre cliente e servidor redistribuída entre segmentos (`EntityWalk.js ~linha 460`). |
| Duração Mínima de Segmento Ajustado | `10` | ms | Nenhum passo pode durar menos de 10 ms mesmo com aceleração por desync (`EntityWalk.js ~linha 464`). |
| Divisões Angulares de Sprite (8 direções) | `8` | direções | $360^\circ / 8 = 45^\circ$ por setor. Setores centralizados com folga de $\pm 22.5^\circ$. |

---

## 3. Regras e Algoritmos

### 3.1. Navegabilidade e Corte de Quinas (*Corner Cutting*)
- **Verificação de Célula:** Uma célula `(x, y)` só é caminhável se estiver dentro dos limites da grade do mapa e `cell.flag == 0` (`PathFinder.cpp:78-85`).
- **Proibição Estrita de Corte de Quinas:** 
  Para dar um passo diagonal de `(sx, sy)` para `(sx + dx, sy + dy)` (onde $|dx| = 1$ e $|dy| = 1$):
  1. A célula destino `(sx + dx, sy + dy)` deve ser livre (`flag == 0`).
  2. **Ambas** as células ortogonais adjacentes `(sx + dx, sy)` e `(sx, sy + dy)` DEVEM ser livres (`flag == 0`).
  3. Se qualquer uma das duas células ortogonais for bloqueada, o movimento diagonal é **estritamente proibido** (`PathFinder.cpp:87-101`).

### 3.2. Algoritmo de Caminho: Linha Reta e A*
1. **Otimização de Linha Reta Direta:**
   - Antes de iniciar a busca A*, o cliente traça uma linha reta tipo Bresenham partindo de `(sx, sy)` até `(targetX, targetY)` incrementando `stepX = Sign(targetX - sx)` e `stepY = Sign(targetY - sy)`.
   - Se todas as células no trajeto forem caminháveis e respeitarem a regra de quina, o trajeto direto é utilizado imediatamente sem alocar nós A* (`PathFinder.cpp:138-175`).
2. **Busca A\* (Quando a reta estiver obstruída):**
   - Usa fila de prioridade ordenada por `cost` (e `serial` como desempate FIFO).
   - Heurística inicial: $10 \times (|targetX - sx| + |targetY - sy|)$ (Manhattan ponderada por 10).
   - Expansão dos 4 vizinhos ortogonais (Y+1, X-1, Y-1, X+1):
     - Distância acumulada soma `+10`.
     - Custo heurístico recebe penalidade de `+20` se afastar da coordenada de destino.
     - Monta uma máscara de bits (`freeMask`: bit 0 para Y+1, bit 1 para X-1, bit 2 para Y-1, bit 3 para X+1).
   - Expansão dos 4 vizinhos diagonais:
     - Só expande se ambos os bits ortogonais adjacentes estiverem setados em `freeMask`.
     - Distância acumulada soma `+14` (implementado como `dist + 4` em relação ao passo base `+10`).
     - Custo heurístico recebe `cost + dc_orto1 + dc_orto2 - 6`.

### 3.3. Interpolação de Posição sem Estalo (*No Snapping*)
- Cada nó na rota armazena sua coordenada em células `(x, y)` e o carimbo de chegada `arrivalTime` em ms:
  $$\text{arrivalTime}_{k} = \text{arrivalTime}_{k-1} + \frac{\text{StepCost} \times \text{speed}}{10}$$
- A cada quadro de renderização, o cliente busca o segmento ativo onde:
  $$\text{arrivalTime}_i \le \text{tempoAtual} < \text{arrivalTime}_{i+1}$$
- Fração de avanço no segmento:
  $$t = \text{clamp}\left(\frac{\text{tempoAtual} - \text{startTime}}{\text{endTime} - \text{startTime}}, 0.0, 1.0\right)$$
- **A Chave Contra o "Anda e Volta":**
  - No **primeiro segmento** ($i = 0$): o ponto de início não é o centro da célula inteira! É a posição contínua em float onde o ator estava no exato instante da requisição (`m_moveStartPos`), calculando o primeiro trecho pela distância euclidiana real até a próxima célula (`GameActor.cpp:1146-1147`, `EntityWalk.js:410-415`).
  - Nos segmentos seguintes ($i \ge 1$): o ponto de início passa a ser o centro exato da célula `TileToWorldCoord(startCell)`.
  - A coordenada vertical $Y$ (altura) é obtida da tabela de alturas na posição fracionária resultante $(X, Z)$.

### 3.4. Tratamento dos Pacotes do Servidor
1. **Confirmação de Movimento do Próprio Jogador (`ZC_NOTIFY_PLAYERMOVE` / `0x0087`):**
   - O pacote traz o tick de início do servidor e as coordenadas compactadas (origem e destino).
   - O cliente sincroniza o tempo local do servidor (`g_session.SetServerTime`).
   - Se o jogador já estiver se movendo, a posição flutuante atual é preservada como início da nova rota (`keepInterpolatedMoveStart = true`), e o A* é rodado do ponto atual até o novo destino (`GameModePacket.cpp:6765-6780`).
2. **Movimento de Terceiros (`ZC_NOTIFY_MOVE` / `0x0086`):**
   - O ator remoto calcula o caminho local e inicia o passo a partir da origem do pacote ou da sua posição atual se estiver dentro de uma distância tolerável.
3. **Parada ou Correção de Posição (`ZC_STOPMOVE` / `0x0088`):**
   - Quando chega uma coordenada de parada forçada do servidor:
   - O cliente checa se a célula informada é redundante via `ShouldIgnoreRedundantLocalFixPosition` (`GameModePacket.cpp:6919-6943`):
     - Localiza em qual índice da rota do cliente a célula `(x, y)` do pacote se encontra (`fixIndex`).
     - Localiza em qual segmento o cliente está no momento presente (`activeIndex`).
     - **Regra de ouro:** Se `fixIndex <= activeIndex + 1`, o pacote **é ignorado**, pois a posição do servidor está igual ou ligeiramente atrasada em relação à interpolação visual do cliente. Se não for ignorado, o personagem "daria um passo em falso para trás" a cada parada.
     - Se o pacote estiver além do caminho ou o personagem já terminou a rota, interrompe a caminhada, cancela `m_path`, reseta `m_isMoving = 0` e fixa o ator nas coordenadas exatas do servidor.

### 3.5. Clique Contínuo (Mouse Hold) e WASD
- Enquanto o botão esquerdo é mantido pressionado:
  - Uma nova requisição não é disparada a cada quadro; obedece ao intervalo de `kHeldMoveRequestIntervalMs` (75 ms no OM, 200 ms no RB).
  - Se houver confirmação pendente de movimento anterior (`m_isWaitingMoveAck == 1`), o cliente não envia outro pacote até o ack chegar ou expirar (1000 ms).
  - Se o novo destino sob o cursor mantiver o mesmo alinhamento direcional e a distância em células for menor ou igual a `kHeldMoveRetargetThresholdCells` (2 células), o cliente preserva o movimento em andamento e adia o novo envio para não sobrecarregar a fila de rede.
  - Ao usar WASD: o controlador do jogador gera pontos de destino virtuais projetados a partir da direção das teclas mantendo as mesmas regras de frequência de envio do clique contínuo.

### 3.6. Carregamento de Mapa (*Map Loading*)
- Durante a troca de mapa (tela de loading com barra de progresso, `m_mapLoadingStage != MapLoading_None` no OM, `!Mouse.intersect` no RB):
  - **Todos os cliques no chão são bloqueados e descartados.**
  - Nenhum pacote `CZ_REQUEST_MOVE` pode ser emitido.
  - O cursor do mouse fica forçado no formato comum (`Arrow`).
  - Pacotes de entidades recebidos durante o carregamento atualizam as listas em memória, mas a interação só é liberada após o término completo da carga e envio do `CZ_NOTIFY_ACTORINIT` / `SendLoadEndAckPacket`.

### 3.7. Direção do Sprite (Mapeamento em 8 Direções)
1. **Cálculo do Ângulo no Mundo:**
   $$\text{dx} = \text{pos.X} - \text{prevPos.X},\quad \text{dz} = \text{pos.Z} - \text{prevPos.Z}$$
   $$\text{rotacaoMundo} = \text{atan2}(\text{dx}, -\text{dz}) \times \frac{180}{\pi}$$
2. **Ângulo Relativo à Câmera:**
   $$\text{anguloRelativo} = \text{NormalizeAngle360}(-\text{cameraLongitude} - \text{rotacaoMundo})$$
3. **Quantização para 8 Direções:**
   - Para estados de Parado (`IDLE`), Andando (`WALK`) e Sentado (`SIT`), utiliza-se a direção arredondada com zonas de $45^\circ$ centralizadas:
     - Se $\text{anguloRelativo} < 22.5^\circ$ ou $\text{anguloRelativo} > 337.5^\circ$: Direção = `0` (Sul / frente para a câmera).
     - Caso contrário: $\text{Direção} = \left\lfloor\frac{\text{anguloRelativo} + 22.5^\circ}{45^\circ}\right\rfloor \ \& \ 7$.
   - **Tabela de Direções:**
     - `0`: Sul (olhando de frente para a câmera)
     - `1`: Sudoeste
     - `2`: Oeste
     - `3`: Noroeste
     - `4`: Norte (olhando de costas para a câmera)
     - `5`: Nordeste
     - `6`: Leste
     - `7`: Sudeste
4. **Jogador Sentado ou com Tecla SHIFT:**
   - Se o jogador estiver no estado `SIT` ou se a tecla `SHIFT` estiver pressionada no momento do clique no mapa:
     - O personagem **NÃO se move**.
     - Apenas atualiza a direção do olhar para a célula clicada (`lookTo(x, y)`).
     - Envia ao servidor o pacote `CZ_CHANGE_DIRECTION` contendo `headDir` e `dir` (`MapEngine.js:1040-1053`).

---

## 4. Entradas e Saídas

### Entradas (Jogador e Servidor)
- **Clique / Hold do Botão Esquerdo:** Célula do chão onde o raio do mouse intercepta o plano do mapa.
- **Tecla SHIFT:** Modificador para girar o corpo sem andar.
- **Teclas WASD:** Geração de vetores direcionais para cálculo de células contínuas de destino.
- **Pacotes Recebidos:**
  - `ZC_NOTIFY_PLAYERMOVE` (`0x0087`): Confirmação do movimento do próprio jogador com carimbo de tempo e origem/destino empacotados.
  - `ZC_NOTIFY_MOVE` (`0x0086`): Movimento de outros personagens, monstros ou NPCs visíveis.
  - `ZC_STOPMOVE` (`0x0088`): Parada forçada ou correção de posição da unidade.

### Saídas (Cliente para Servidor)
- **Pacote `CZ_REQUEST_MOVE` (Legado 5 bytes: opcode + 3 bytes Dest):**
  - Opcode correspondente à versão do executável (ex: `0x035f`, `0x0437`, `0x0934`).
  - Codificação dos 3 bytes de destino a partir de `(dstX, dstY)`:
    ```c
    dest[0] = (dstX >> 2) & 0xFF;
    dest[1] = ((dstX & 0x03) << 6) | ((dstY >> 4) & 0x3F);
    dest[2] = (dstY & 0x0F) << 4;
    ```
- **Pacote `CZ_REQUEST_MOVE2` (Moderno 8 bytes: opcode + 3 bytes padding + 3 bytes Dest):**
  - Utilizado em clientes mais recentes para compatibilidade com pacotes com padding.
- **Pacote `CZ_CHANGE_DIRECTION` (`0x009b`):**
  - Enviado ao clicar com SHIFT ou enquanto sentado para mudar a rotação corporal e da cabeça.

---

## 5. Diferenças entre as Referências

1. **Custos Numéricos do Pathfinding:**
   - **OM:** Utiliza estritamente inteiros na busca A*: custo 10 para ortogonal e 14 para diagonal (`PathFinder.cpp:58`). Cálculo de tempo: `(StepCost * speedFactor) / 10`.
   - **RB:** Utiliza ponto flutuante `DIAGONAL_FACTOR = 1.414` e calcula o primeiro segmento com a hipotenusa euclidiana float (`Math.hypot(dx, dy) * speed`) (`EntityWalk.js:414`).
   - *Fidelidade:* O OM reflete com precisão exata o código original C++ do cliente Ragnarok (`PathFinder.cpp`), enquanto o RB traduziu a lógica para floats em JavaScript mantendo o mesmo resultado prático.
2. **Ajuste de Latência e Sincronização:**
   - **OM:** Usa o carimbo de data/hora do servidor (`g_session.GetServerTime()`) recebido no cabeçalho de cada pacote para avançar o relógio de animação do mundo (`GameActor.cpp:4399`).
   - **RB:** Implementa explicitamente o algoritmo de compensação `FixPathTime` (`EntityWalk.js:456-465`), dividindo a diferença entre a duração estimada no cliente e a duração prescrita pelo servidor proporcionalmente entre os segmentos restantes:
     $$\text{subDiv} = \frac{\text{clamp}(\text{serverDuration} - \text{clientDuration}, -2500, +2500)}{\text{numSegmentos}}$$
     Somando `subDiv` a cada segmento restante (mínimo 10 ms).
   - *Fidelidade:* A fórmula do RB baseia-se diretamente no `CPathFinder::FixPathTime` do cliente oficial vazado de 2008.

---

## 6. Como Testar

1. **Teste do Primeiro Passo (Eliminação do "Anda e Volta"):**
   - Pare o personagem entre duas células (ex: após andar meio passo e parar).
   - Dê um novo clique no chão.
   - **Resultado esperado:** O personagem deve iniciar a nova caminhada a partir da sua posição fracionária atual, sem "teleportar" de volta para o centro da célula anterior.
2. **Teste de Quina em L (Proibição de Corte):**
   - Posicione o personagem ao lado de uma quina de parede fechada.
   - Clique na célula diagonal do outro lado da quina.
   - **Resultado esperado:** O personagem não deve "atravessar a ponta da parede". O A* deve gerar uma rota de 2 passos ortogonais contornando o obstáculo.
3. **Teste de Clique Segurado em Círculo:**
   - Segure o botão esquerdo do mouse e mova o cursor em círculos ao redor do personagem.
   - **Resultado esperado:** O personagem segue o cursor com curvas suaves; o cliente não deve engasgar a rede com 60 pacotes por segundo (deve respeitar os ~75-200 ms entre pedidos) e a animação de pernas não deve reiniciar a cada frame.
4. **Teste Sentado:**
   - Sente o personagem (`/sit` ou Insert) e clique em várias direções ao redor.
   - **Resultado esperado:** O personagem não levanta e não anda; apenas vira o corpo e a cabeça para a direção clicada, gerando o pacote `CZ_CHANGE_DIRECTION`.
5. **Teste Durante Loading de Mapa:**
   - Troque de mapa através de um portal.
   - Durante a exibição da tela de carregamento, clique repetidamente na tela.
   - **Resultado esperado:** Nenhum pacote de movimento é transmitido ao servidor e, assim que o mapa abrir, o personagem continua parado no ponto de spawn sem movimentos residuais.

# Especificação 02: Câmera — Controles, Limites e Mapas Internos

> **Origem das referências:**
> - **OM (Open-Midgard):** `src/gamemode/View.h`, `src/gamemode/View.cpp` (linhas 15-28, 140-160, 470-550, 725-830, 850-890), `src/gamemode/GameMode.cpp` (linhas 125-145, 2500-2675, 9900-10010, 10185-10210).
> - **RB (roBrowserLegacy):** `src/Renderer/Camera.js` (linhas 15-230, 270-420), `src/Preferences/Camera.js`.
> - **Tabelas do Cliente:** `data/viewpointtable.txt`, `data/indoorrswtable.txt`.

---

## 1. Resumo

A câmera do Ragnarok Online é uma câmera orbital em terceira pessoa (*chase camera*) centralizada no personagem do jogador, utilizando coordenadas esféricas: distância (*distance* / zoom), rotação horizontal (*longitude* / yaw) e inclinação vertical (*latitude* / pitch). O controle é realizado primordialmente pelo botão direito do mouse (arrastar para girar, scroll para zoom, duplo-clique para resetar a orientação). Mapas internos (*indoor*) restringem ou travam completamente a rotação horizontal, aproximam o zoom máximo e limitam a inclinação vertical para evitar que a câmera atravesse tetos e paredes. Toda movimentação e rotação possui amortecimento exponencial clássico com fator `0.1` por ciclo, garantindo suavidade natural sem paradas bruscas.

---

## 2. Dados e Constantes

### 2.1. Constantes de Controle e Sensibilidade

| Parâmetro | Valor | Unidade | Descrição / Referência |
|---|---|---|---|
| Sensibilidade Horizontal (`kDragLongitudePerPixel`) | `0.4` | graus / px | Rotação horizontal por pixel arrastado com botão direito (`View.cpp ~linha 17`). |
| Sensibilidade Vertical (`kDragLatitudePerPixel`) | `-0.3` | graus / px | Rotação vertical por pixel arrastado com botão direito (`View.cpp ~linha 18`). |
| Passo do Zoom da Roda (`kWheelDistancePerNotch`) | `-60.0` | unidades / notch | Variação de distância por entalhe padrão da roda do mouse (`WHEEL_DELTA = 120`) (`View.cpp ~linha 19`). |
| Fator de Interpolação / Suavização (`kRefInterpolateViewFactor`) | `0.1` | fração por frame | Amortecimento linear da interpolação: `cur += (dest - cur) * 0.1` (`View.cpp ~linha 25`). |
| Distância do Plano Próximo (`kNearPlane`) | `10.0` | unidades | Near clipping plane da projeção da câmera (`View.cpp ~linha 26`). |
| Limiar de Posição de Duplo Clique Direito | `GetSystemMetrics` (mín 4) | pixels | Caixa delimitadora em X e Y para detectar duplo clique do botão direito (`GameMode.cpp ~linha 9946`). |
| Janela de Tempo de Duplo Clique | `GetDoubleClickTime()` (~500) | ms | Janela máxima entre dois cliques com botão direito para resetar a câmera. |

### 2.2. Limites em Mapas Externos (*Outdoor Maps*)

| Parâmetro | Valor | Unidade | Descrição / Referência |
|---|---|---|---|
| Distância Mínima (`kRefCloseCameraDistance`) | `150.0` | unidades | Zoom máximo (mais próximo do personagem) (`GameMode.cpp:127`). |
| Distância Máxima (`kRefFarCameraDistance`) | `700.0` | unidades | Zoom mínimo padrão (mais afastado) (`GameMode.cpp:130`). |
| Distância Média Salva (`kRefAverageCameraDistance`) | `500.0` | unidades | Distância padrão inicial externa (`GameMode.cpp:129`). |
| Inclinação Mínima (`kRefOutdoorLatitudeMin`) | `-65.0` | graus | Ângulo mais íngreme / apontando quase para baixo (`GameMode.cpp:133`). |
| Inclinação Máxima (`kRefOutdoorLatitudeMax`) | `-25.0` | graus | Ângulo mais plano / olhando mais na horizontal (`GameMode.cpp:134`). |
| Inclinação Salva Padrão (`g_savedOutdoorCameraLatitude`) | `-45.0` | graus | Latitude inicial ao entrar em mapas externos (`GameMode.cpp:142`). |
| Longitude Padrão | `0.0` | graus | Olhando de frente para o Norte (`GameMode.cpp:2637`). |
| Rotação Horizontal Travada? (`lockLongitude`) | `false` | booleano | Livre rotação contínua $360^\circ$. |

### 2.3. Limites em Mapas Internos (*Indoor Maps*)

| Parâmetro | Valor | Unidade | Descrição / Referência |
|---|---|---|---|
| Distância Mínima | `150.0` | unidades | Idêntica à externa (`kRefCloseCameraDistance`). |
| Distância Máxima (`kRefIndoorCameraDistance`) | `420.0` | unidades | Limite de afastamento reduzido para não cortar paredes (`GameMode.cpp:128`). |
| Distância Salva Padrão (`g_savedIndoorCameraDistance`) | `350.0` | unidades | Distância inicial ao carregar mapa interno. |
| Inclinação Mínima (`kRefIndoorLatitudeMin`) | `-55.0` | graus | Inclinação mais íngreme permitida indoor (`GameMode.cpp:131`). |
| Inclinação Máxima (`kRefIndoorLatitudeMax`) | `-35.0` | graus | Inclinação mais rasa permitida indoor (`GameMode.cpp:132`). |
| Inclinação Salva Padrão (`g_savedIndoorCameraLatitude`) | `-40.0` | graus | Latitude inicial ao entrar em mapas internos. |
| Longitude Padrão Inicial | `45.0` | graus | Posição angular padrão isométrica clássica indoor (`GameMode.cpp:2636`). |
| Rotação Horizontal Travada? (`lockLongitude`) | `true` | booleano | Câmera fixa sem permitir rotação horizontal (`GameMode.cpp:2637`). |

---

## 3. Regras de Comportamento

### 3.1. Interação do Mouse e Entradas
1. **Arrastar com Botão Direito (Girar Câmera):**
   - No clique inicial do botão direito (`GameMsg_RButtonDown`):
     - Se houver uso pendente de habilidade ou perseguição de skill, o clique direito cancela a ação pendente e não inicia rotação (`GameMode.cpp:9938-9942`).
     - Caso contrário, ativa `m_canRotateView = 1`, captura o mouse (`SetCapture`) e armazena a posição inicial.
   - Durante o movimento do mouse com botão direito segurado (`GameMsg_MouseMove`):
     - Se `mouseX != m_oldMouseX` ou `mouseY != m_oldMouseY`, ativa o flag `m_rButtonDragged = 1`.
     - Executa:
       $$\Delta \text{Longitude} = (\text{mouseX} - \text{oldMouseX}) \times 0.4^\circ$$
       $$\Delta \text{Latitude} = (\text{mouseY} - \oldMouseY) \times (-0.3^\circ)$$
     - Limpa o hover da célula do chão (`m_view->ClearHoverCell()`).
   - Ao soltar o botão direito (`GameMsg_RButtonUp`):
     - Desativa `m_canRotateView = 0` e libera a captura do mouse (`ReleaseCapture`).
2. **Duplo Clique com Botão Direito (Reset da Orientação):**
   - Se o jogador clicar duas vezes com o botão direito dentro de `GetDoubleClickTime()` (~500 ms) e em uma área menor que o retângulo de duplo clique do sistema ($\le 4$ pixels):
     - Cancela a rotação em andamento.
     - Chama `m_view->ResetToDefaultOrientation()`.
     - Define `dest.longitude = defaultLongitude` (0° outdoor, 45° indoor) e `dest.latitude = defaultLatitude` (-45° outdoor, -40° indoor).
     - **Importante:** O reset não altera a distância atual de zoom, apenas os ângulos.
3. **Scroll do Mouse (Zoom In / Zoom Out):**
   - Ao rolar a roda do mouse (`GameMsg_MouseWheel`):
     $$\Delta \text{Notches} = \frac{\text{wheelDelta}}{120}$$
     $$\text{dest.distance} = \text{clamp}(\text{dest.distance} + \Delta \text{Notches} \times (-60.0), \text{minDistance}, \text{maxDistance})$$
4. **Modificadores com Teclas (Shift e Ctrl no RB):**
   - **Shift + Botão Direito (Apenas Inclinação Vertical):**
     - O movimento horizontal do mouse é ignorado; apenas o delta vertical altera a inclinação da câmera (`angleFinal[0]`), permitindo ajustar o ângulo sem girar o mapa (`RB Camera.js ~linha 277`).
     - Shift + Duplo Clique Direito: reseta apenas a inclinação vertical para a padrão do mapa (`RB Camera.js:243`).
   - **Ctrl + Botão Direito (Zoom por Arrastamento):**
     - Mover o mouse verticalmente com Ctrl pressionado altera suavemente a distância da câmera, funcionando como alternativa contínua ao scroll do mouse (`RB Camera.js ~linha 290`).
     - Ctrl + Duplo Clique Direito: reseta a distância para o padrão (`RB Camera.js:250`).

### 3.2. Suavização e Amortecimento (*Easing*)
A cada sub-passo do ciclo de atualização (`CView::InterpolateViewInfo` em `View.cpp:760-805`):
- O alvo de perseguição no mundo (`m_dest.at`) segue a posição real interpolada do ator do jogador (`ResolveTargetPosition()`).
- O ponto de foco suave (`m_cur.at`) se aproxima do alvo:
  $$\text{cur.at} = \text{cur.at} + (\text{dest.at} - \text{cur.at}) \times 0.1$$
- A rotação horizontal interpola pelo **menor arco** (diferença normalizada entre $[-180^\circ, +180^\circ]$):
  $$\Delta \text{Lon} = \text{NormalizeLongitudeDelta}(\text{dest.longitude} - \text{cur.longitude})$$
  $$\text{cur.longitude} = \text{cur.longitude} + \Delta \text{Lon} \times 0.1$$
  Mantendo o valor resultante no intervalo $[0^\circ, 360^\circ)$.
- A inclinação e a distância interpolam pelo mesmo fator:
  $$\text{cur.latitude} = \text{cur.latitude} + (\text{dest.latitude} - \text{cur.latitude}) \times 0.1$$
  $$\text{cur.distance} = \text{cur.distance} + (\text{dest.distance} - \text{cur.distance}) \times 0.1$$

### 3.3. Colisão com o Terreno (Prevenção de Câmera Subterrânea)
- Durante a construção da matriz de visão (`CView::BuildViewMatrix` em `View.cpp:808-830`):
  - A posição dos olhos da câmera (`m_from`) é calculada a partir de `m_cur.at`, `latitude`, `longitude` e `distance`.
  - O cliente consulta a altura do terreno na coordenada X/Z da câmera:
    $$\text{alturaTerreno} = \text{GetHeight}(m\_from.X, m\_from.Z)$$
  - Se a câmera estiver abaixo do terreno ou dentro de uma depressão:
    $$\text{se } \text{alturaTerreno} < m\_from.Y \implies m\_from.Y = \text{alturaTerreno}$$
    (No sistema original de coordenadas onde o eixo Y negativo aponta para cima no espaço de projeção do cliente).

### 3.4. Câmera em Mapas Internos e `viewpointtable.txt`
1. **Detecção de Mapas Internos:**
   - O cliente carrega `data/indoorrswtable.txt`. Qualquer mapa cujo nome de arquivo `.rsw` conste na lista é marcado como `isIndoor = true`.
2. **Parâmetros Personalizados por Mapa (`data/viewpointtable.txt`):**
   - Formato da linha:
     `rswName#minDistance#distanceScope#initialDistance#minLongitude#maxLongitude#initialLongitude#maxLatitude#minLatitude#initialLatitude#`
   - Onde:
     - `minDistance`: distância mínima permitida.
     - `maxDistance = minDistance + distanceScope`.
     - `defaultDistance = initialDistance`.
     - Se `minLongitude == maxLongitude`: o cliente define `lockLongitude = true` (rotação horizontal bloqueada).
     - Se `minLongitude > -360` ou `maxLongitude < 360`: ativa `constrainLongitude = true` (limita o giro ao arco especificado).
3. **Persistência entre Mapas:**
   - Ao sair de um mapa (`SaveCameraStateForMap` em `GameMode.cpp:2610-2622`):
     - Se o mapa atual for indoor, salva a última distância e latitude em `g_savedIndoorCamera*`.
     - Se for outdoor, salva em `g_savedOutdoorCamera*`.
   - Ao entrar em um novo mapa do mesmo tipo, as preferências de inclinação e zoom do jogador são preservadas, restaurando a experiência sem forçar o zoom padrão a cada portal.

---

## 4. Entradas e Saídas

### Entradas
- **Arrasto do Botão Direito:** Deltas `(deltaX, deltaY)` da posição do cursor.
- **Roda do Mouse:** `WHEEL_DELTA` positivo (afasta) ou negativo (aproxima).
- **Duplo Clique do Botão Direito:** Reset para os ângulos padrão.
- **Teclas Modificadoras:** `SHIFT` (trava horizontal / rotação vertical pura), `CTRL` (zoom contínuo por arrasto).

### Saídas
- **Matriz de Visualização (`m_viewMatrix` / `ViewMatrix`):**
  Matriz $4 \times 4$ atualizada a cada frame e consumida por:
  - Renderizador do chão e terreno 3D (`m_ground`).
  - Billboards de sprites de personagens, monstros e itens (`RenderBillboards`).
  - Projeção de raios para teste de colisão do cursor (*ScreenToAttrCell*).
  - Cálculo de ângulo relativo do sprite em relação à câmera (`ResolveRuntimeActorLongitudeForDirection`).

---

## 5. Diferenças entre as Referências

1. **Estrutura de Coordenadas e Convenções:**
   - **OM:** Utiliza convenção esférica onde `longitude` é rotação horizontal em torno do eixo Y e `latitude` é a inclinação vertical em relação ao plano do chão (valores negativos como $-45^\circ$, com $-85^\circ$ olhando para o chão e $-10^\circ$ olhando o horizonte).
   - **RB:** Utiliza vetor de ângulo `[altitude, rotation]`, onde altitude varia de $190^\circ$ a $270^\circ$ (equivalente a $10^\circ$ a $90^\circ$ de elevação).
2. **Modos de Câmera Adicionais:**
   - O **OM** implementa estritamente o modo isométrico padrão do cliente oficial de 2008.
   - O **RB** acrescentou modos opcionais de primeira pessoa (`enable1STPerson`) e terceira pessoa livre (`enable3RDPerson`) com variação dinâmica do FOV (15° no isométrico, 30° em 3ª pessoa, 50° em 1ª pessoa). Para o Myth of Thor, apenas o modo clássico isométrico é necessário para fidelidade 1:1.

---

## 6. Como Testar

1. **Teste de Rotação Suave:**
   - Segure o botão direito e mova o mouse para os lados e para cima/baixo em Prontera.
   - **Resultado esperado:** A câmera gira com resposta suave e para gradualmente devido ao amortecimento `0.1`, sem estalos instantâneos.
2. **Teste de Duplo Clique Direito:**
   - Gire a câmera para um ângulo incomum e dê um duplo clique rápido com o botão direito.
   - **Resultado esperado:** A câmera gira suavemente de volta para a orientação frontal padrão (Norte / 0°), mantendo a distância de zoom atual intacta.
3. **Teste de Limite de Zoom e Inclinação:**
   - Role a roda do mouse até o limite máximo e mínimo.
   - **Resultado esperado:** O zoom para suavemente em 150 unidades (máximo aproximação) e 700 unidades (máximo afastamento), sem saltos ou travamentos.
4. **Teste em Mapa Interno (Ex: Interior do Castelo de Prontera):**
   - Atravesse um portal para o interior de um prédio ou castelo.
   - Tente girar a câmera horizontalmente com o botão direito.
   - **Resultado esperado:** A rotação horizontal deve estar travada em 45°; o botão direito apenas permite ajustar ligeiramente a inclinação vertical dentro do intervalo permitido ($-35^\circ$ a $-55^\circ$), e o zoom máximo de afastamento é reduzido para 420 unidades.
5. **Teste de Shift + Botão Direito (RB):**
   - Segure Shift e arraste o botão direito na diagonal.
   - **Resultado esperado:** O mapa não gira lateralmente; apenas a inclinação vertical varia.

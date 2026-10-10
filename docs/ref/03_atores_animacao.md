# Especificação 03: Atores e Animação — Ações, Camadas, ASPD e Sombra

> **Origem das referências:**
> - **OM (Open-Midgard):** `src/world/GameActor.h`, `src/world/GameActor.cpp` (linhas 100-145, 250-285, 1630-1700, 2765-2850, 3160-3255, 4015-4105, 4480-4590, 4800-4890), `src/world/3dActor.cpp`, `src/world/World.cpp` (linhas 1015-1110).
> - **RB (roBrowserLegacy):** `src/Renderer/Entity/EntityAction.js` (linhas 20-135), `src/Renderer/Entity/EntityRender.js` (linhas 40-60, 330-650, 870-1010), `src/Renderer/Entity/EntityWalk.js`.
> - **Tabelas e Arquivos:** arquivos `.act` e `.spr` de personagens e monstros, `.imf` de prioridade de camadas.

---

## 1. Resumo

A renderização e animação de atores no Ragnarok Online combina sprites 2D compostos em camadas (*billboards*) orientados a tempo e distância. Cada estado lógico (Parado, Andando, Atacando, Levando Dano, Sentado, Morto) corresponde a uma família de 8 ações no arquivo `.act` (uma para cada direção cardinal/diagonal). O ciclo de passos ao caminhar é governado estritamente pela distância percorrida no mundo 3D para evitar deslizamento de pés. O ataque é ajustado pela velocidade de ataque (ASPD/attackMT). A cabeça do personagem suporta 3 posições ("doridori") quando parado, e a montagem das camadas (corpo, cabeça, chapéus, armas e escudo) segue prioridades definidas pelo arquivo `.imf` com inversão de profundidade quando o personagem está de costas para a câmera.

---

## 2. Dados e Constantes

| Parâmetro | Valor | Unidade | Descrição / Referência |
|---|---|---|---|
| Frequência Base de Animação do Ator | `24.0` | FPS | Taxa interna de amostragem: fator `0.041666668f` ($1/24$) (`GameActor.cpp:4073`). |
| Fator de Escala de Ataque (`kAttackMotionFactor`) | `0.0023148148` | $1/\text{ms}$ | Fator inverso exato de $1 / 432$ ms (`GameActor.cpp:105`). |
| Tempo de Ataque Padrão Fallback (`attackMT`) | `1440` | ms | Duração usada quando o servidor não fornece `attackMT` válido ($\le 0$) (`GameActor.cpp:4954`). |
| Delay Padrão de Movimento do .act (`kDefaultMotionDelay`) | `4.0` | ticks de 25ms | Delay clássico de animação (100 ms) se o `.act` não especificar (`GameActor.cpp:103`). |
| Fator de Passo de Caminhada Clássico | `1.48` | multiplicador | $0.37 \times 4 = 1.48$. Converte distância percorrida em índice de quadro (`GameActor.cpp:1660`). |
| Passo de Caminhada no RB (`WALK_DIST_TO_MOTION`) | `170.2` | constante | Escala de células do mapa para milissegundos do `.act` ($4.6 \times 0.37 \times 4 \times 25$) (`EntityRender.js:52`). |
| Duração da Reação a Dano (`kHitReactionDurationMs`) | `200` | ms | Tempo em que o ator permanece na animação de ser atingido (`GameActor.cpp:115`). |
| Deslocamento Visual de Recuo (`kHitReactionKnockbackDist`) | `6.0` | unidades / px | Recuo elástico da animação de golpe levado (`GameActor.cpp:116`). |
| Quadro Final de Morte (`kDeathMotion`) | `4` | índice de quadro | Quadro em que o personagem repousa no chão ao morrer (`GameActor.cpp:112`). |
| Posições da Cabeça / "Doridori" | `3` | posições | `0` = frente, `1` = direita, `2` = esquerda (`EntityRender.js:950-965`). |
| Offset Vertical do Sprite (`SPRITE_LIFT`) | `0.2` | unidades | Elevação da base do sprite em relação ao plano do chão (`EntityRender.js:192, 345`). |
| Elevação da Sombra no Chão | `0.05` | unidades | Altura acima da malha de colisão para evitar z-fighting (`EntityRender.js:338`). |
| Escala Padrão da Sombra (`m_shadowZoom`) | `1.0` | multiplicador | Escala de tamanho da elipse da sombra (`World.cpp:1023`). |

---

## 3. Mapeamento de Estados e Ações (.act)

### 3.1. Tabela de Estados e BaseAction
O arquivo `.act` contém grupos de 8 ações consecutivas correspondentes às 8 direções:
$$\text{AçãoReal} = \text{baseAction} + \text{Get8Dir}(\text{ânguloRelativo})$$

| Estado Lógico | ID do Estado | baseAction (PC) | baseAction (Monstro) | Comportamento de Reprodução |
|---|---|---|---|---|
| **IDLE** (Parado) | `0` | `0` | `0` | Loop contínuo ou estático com doridori (`0..7`). |
| **WALK** (Andando) | `1` | `8` | `8` | Controlado por distância no mundo (`8..15`). |
| **SIT** (Sentado) | `6` | `16` | N/A | Estático no quadro sentado (`16..23`). Sem sombra. |
| **PICKUP** (Pegar Item) | `5` | `24` | N/A | Reprodução única até o fim (`24..31`), volta para IDLE. |
| **READYFIGHT** (Postura de Combate) | `4` (pre) | `32` ou `40` | `0` | Aguardando ação com arma empunhada. |
| **HURT / HIT** (Levando Dano) | `4` | `48` | `48` | Dura 200 ms (`48..55`), interpola recuo e retorna a IDLE. |
| **FREEZE / STUN** | `7` | `56` | `56` | Congelado no quadro atual, sem animação. |
| **DIE / DEAD** (Morte) | `8` | `64` | `64` | Toca até o quadro 4 e congela deitado (`64..71`). Sem sombra. |
| **ATTACK1** (Ataque Normal) | `2` | `80` (ou arma) | `16` | Dura exatamente o `attackMT` do servidor. |
| **ATTACK2** (Segundo Ataque / Crítico) | `9` | `88` | `16` | Disparado quando `IsSecondAttack` for verdadeiro. |
| **SKILL / CAST** (Conjuração / Habilidade) | `3` / `7` | `80` / `96` | `16` | Congela no quadro de conjuração até o fim do cast. |

---

## 4. Regras e Fórmulas de Animação

### 4.1. Passo ao Caminhar Sincronizado por Distância (Sem "Moonwalk")
Ao caminhar, o avanço de quadros no sprite **não depende de um timer fixo**, mas sim do deslocamento euclidiano acumulado do ator no chão (`m_dist`):
1. **Fórmula Oficial C++ (`GameActor.cpp:1660`):**
   $$\text{motionDelay} = \max(0.0001, \text{bodyActRes.GetDelay}(\text{action}))$$
   $$\text{quadro} = \left\lfloor\frac{\text{distânciaAcumulada} \times 1.48}{\text{motionDelay}}\right\rfloor \pmod{\text{totalQuadros}}$$
2. **Fase Compartilhada entre Camadas (`EntityRender.js:980-1005`):**
   Para que a cabeça, o chapéu, a roupa e a arma não fiquem fora de fase entre si durante a caminhada, a fase da animação é calculada uma única vez pelo corpo (`body`) no início do frame e reutilizada por todas as outras camadas anexadas naquele tick.

### 4.2. Velocidade de Ataque (ASPD e attackMT)
1. **Duração do Ataque (`attackMT`):**
   O servidor envia o tempo total do ciclo de ataque em milissegundos (`attackMT` / `amotion`), derivado de:
   $$\text{amotion} = (200 - \text{ASPD}) \times 10\text{ ms}$$
2. **Ajuste de Velocidade do Quadro (`GameActor.cpp:4957`, `EntityRender.js:895`):**
   $$\text{fatorVelocidade} = \text{attackMT} \times 0.0023148148 = \frac{\text{attackMT}}{432}$$
   Os quadros de ataque da ação são escalados para que o impacto visual (frame de golpe) ocorra no momento exato prescrito pelo pacote de dano.

### 4.3. O "Doridori" da Cabeça Parada
Quando o personagem está no estado `IDLE` ou `SIT`, a cabeça pode girar para os lados:
- A variável `headDir` possui 3 estados:
  - `0`: Olhando para frente (neutro)
  - `1`: Olhando para a direita
  - `2`: Olhando para a esquerda
- **No Corpo (`body` / `robe`):**
  A ação IDLE do corpo possui quadros dedicados para a postura do pescoço; o quadro exibido é simplesmente:
  $$\text{quadroCorpo} = \text{headDir}$$
- **Na Cabeça e Chapéus (`head` / `accessory`):**
  Os arquivos `.act` de cabelo e acessórios contêm 3 blocos de animações idênticos em tamanho:
  $$\text{tamanhoBloco} = \left\lfloor\frac{\text{totalQuadros}}{3}\right\rfloor$$
  $$\text{quadroFinal} = (\text{quadroBase} \pmod{\text{tamanhoBloco}}) + \text{tamanhoBloco} \times \text{headDir}$$

### 4.4. Ordem das Camadas de Renderização (Layering)
No cliente do Ragnarok, o corpo e seus anexos são desenhados de trás para frente. A ordem depende do arquivo de informação de modelo `.imf` e do ângulo de visão em relação à câmera:

#### Ordem Padrão (Olhando de Frente: Direções 0, 1, 2, 6, 7):
1. **Escudo Traseiro** (se a orientação colocar o escudo atrás do corpo)
2. **Sombra do Carrinho** e **Carrinho** (se possuir carrinho de Mercador)
3. **Capa / Asas** (`robe`) de fundo
4. **Corpo** (`body` - sprite principal da classe)
5. **Cabeça / Cabelo** (`head`)
6. **Acessório Meio** (`accessory3` - óculos, máscaras)
7. **Acessório Topo** (`accessory2` - chapéus, laços)
8. **Acessório Baixo** (`accessory` - barba, flor na boca)
9. **Arma** (`weapon` + rastro de arma)
10. **Escudo Frontal** (`shield`)

#### Ordem com Personagem de Costas (Direções 3, 4, 5):
1. **Arma / Escudo** (atrás do tronco)
2. **Corpo** (`body`)
3. **Cabeça / Cabelo** (`head`)
4. **Acessórios de Cabeça**
5. **Capa / Asas** (`robe` - desenhada sobre as costas do personagem!)
6. **Carrinho** (fica em primeiro plano à frente das costas)

*Nota do `.imf` (`GameActor.cpp:2772`):* Se o arquivo `.imf` da classe não existir no GRF, a ordem padrão do vetor é `{ 7, 0, 1, 4, 3, 2, 5, 6 }` (Shield, Body, Head, HatTop, HatMid, HatBottom, Weapon, WeaponTrail).

### 4.5. Quando a Arma Aparece
- Em `IDLE` e `SIT`: o personagem **guarda a arma**. Ela não é desenhada nas mãos no cliente clássico pré-renovação, a menos que o personagem esteja no estado de combate (`READYFIGHT`) ou atacando (`ATTACK`).
- Apenas arcos e algumas armas específicas possuem postura armada em `READYFIGHT`.

### 4.6. Morte, Sentar e Levada de Golpe
- **Sentar (`SIT`):**
  - Trava no quadro correspondente de `baseAction = 16`.
  - A sombra redonda no chão é **desligada**.
- **Morte (`DIE`):**
  - Toca os quadros da ação `64` até o índice 4 (`kDeathMotion`).
  - Permanece imóvel deitado no chão (`m_isLieOnGround = 1`).
  - Sombra desligada.
- **Levando Dano (`HIT` / `HURT`):**
  - Toca ação `48` com duração total de 200 ms.
  - Interpola a posição visual com um pequeno recuo elástico de 6 unidades na direção oposta ao agressor, retornando suavemente à célula original.

### 4.7. Sombra Redonda no Chão
1. **Geometria:**
   - A sombra não é um billboard vertical; ela é desenhada como um quad horizontal plano colado no chão na altura do terreno:
     $$Y = \text{GetHeight}(X, Z) + 0.05$$
2. **Escala:**
   - Personagens humanos comuns utilizam escala `1.0`.
   - Monstros usam a escala baseada no seu tamanho (`xSize`, `ySize`):
     - Pequeno / Médio: `1.0`
     - Grande: `1.5` a `2.0`
     - Chefes MVP gigantes: até `3.0`
3. **Desativação:**
   - A sombra é automaticamente omitida quando:
     - Personagem está morto (`state == DIE`).
     - Personagem está sentado (`state == SIT`).
     - Habilidades de esconderijo ativas (`Hiding`, `Cloaking`, `Burrow`).
     - Portais ou NPCs invisíveis (`job == 45` / `kJobHiddenWarpNpc`).

---

## 5. Diferenças entre as Referências

1. **Cálculo da Animação de Caminhada:**
   - **OM:** Utiliza a fórmula C++ oficial baseada na distância real percorrida multiplicada por 1.48 e dividida pelo delay do `.act` (`GameActor.cpp:1660`).
   - **RB:** Converte as células de grade do mapa para a escala interna do cliente oficial via `WALK_DIST_TO_MOTION = 170.2` e divide por `act.delay` em milissegundos (`EntityRender.js:52`). Ambas produzem exatamente a mesma cadência de passos.
2. **Controle de Camadas:**
   - **OM:** Lê e aplica os arquivos `.imf` para resolver os pontos de ancoragem precisos e as permutações de camadas por frame (`BuildPlayerRenderLayerOrder`).
   - **RB:** Usa constantes de `zIndex` separadas para cada elemento e regras condicionais (`behind`, `direction > 2 && direction < 6`) para inverter a capa e o escudo.

---

## 6. Como Testar

1. **Teste de Cadência de Passo:**
   - Ande em velocidade normal (150 ms) e depois sob efeito de Aumentar Agilidade ou montaria (100 ms).
   - **Resultado esperado:** A animação de pernas deve acelerar proporcionalmente à velocidade de deslocamento no mundo; os pés nunca devem patinar ou deslizar no chão.
2. **Teste de Camadas (Frente vs Costas):**
   - Equipe um escudo, uma arma e uma capa.
   - Gire a câmera para ver o personagem de frente: o corpo e a cabeça devem ficar visíveis e a capa atrás.
   - Gire a câmera para ver o personagem de costas: a capa deve ser desenhada **por cima** do corpo e das costas.
3. **Teste do Doridori:**
   - Fique parado e use o comando `/doridori` ou aperte as setas esquerda e direita repetidamente.
   - **Resultado esperado:** A cabeça e os chapéus viram para os lados entre os 3 quadros (frente, direita, esquerda) enquanto o corpo permanece na mesma direção.
4. **Teste de Reação a Golpe (Flinch):**
   - Deixe um monstro atacar seu personagem.
   - **Resultado esperado:** Ao sofrer o dano, o personagem exibe a animação de recuo rápido (ação 48) com duração de ~200 ms sem cancelar o próximo comando de movimento.
5. **Teste de Sombra ao Sentar:**
   - Sente o personagem com a tecla Insert ou `/sit`.
   - **Resultado esperado:** O sprite senta e a sombra circular no chão desaparece. Ao levantar, a sombra reaparece imediatamente.

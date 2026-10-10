# Referência 13: Efeitos em Código ("type": "FUNC") e Aura de Nível 99

> **Fonte:** roBrowserLegacy (`src/Renderer/Effects/SwirlingAura.js`, `GroundAura.js`, `Level99Bubble.js`, `EntityAura.js`, `src/DB/Effects/EffectTable.js`, `src/Controls/ProcessCommand.js`) e engenharia reversa do cliente oficial Ragnarok Online.  
> **Escopo:** Fase 2.1 / Fase 7.1 — Efeitos renderizados diretamente por código/shaders da GPU (sem arquivo de animação `.str`) e gatilhos lógicos do cliente.  
> **Prioridade:** Alta (Aura 99 dos personagens e catálogo dos efeitos `FUNC` vazios da tabela de habilidades).

---

## 1. Visão Geral

No cliente do Ragnarok Online, certos efeitos gráficos e gatilhos de sistema não utilizam os arquivos tradicionais de animação por sprites `.str`. Em vez disso, o executável invoca funções de renderização procedural ou rotinas de controle de estado do ator, registradas na tabela de efeitos com `"type": "FUNC"`.

Dentre esses efeitos, o mais complexo e visualmente marcante é a **Aura do Nível 99**, atribuída a personagens que alcançam o nível de base máximo clássico (ou 175/185 no Renewal). Ela é composta por três sistemas simultâneos e independentes de geometria procedural:
1. **`EF_LEVEL99` (ID 200):** Três faixas em leque de cone espiral ascendente (`SwirlingAura`).
2. **`EF_LEVEL99_2` (ID 201):** Dois anéis em plano horizontal no chão com pulsação e respiração periódica (`GroundAura`).
3. **`EF_LEVEL99_3` (ID 202):** Dezesseis partículas de brilho em forma de diamante/bolhas flutuando em colunas verticais com oscilação harmônica (`Level99Bubble`).

Além da aura, a base de dados `Config/skill_effects_full.json` possui 33 entradas com `"type": "FUNC"` vazias (onde não há arquivo de animação nem função registrada no JSON simplificado) e 29 disparos de tremor de tela (`FUNC quake`). Este documento especifica com precisão matemática cada um desses sistemas.

---

## 2. Aura do Nível 99: Regras de Ativação e Modos

### 2.1. Condição de Disparo e Entidades Afetadas
- **Origem:** `roBrowserLegacy src/Renderer/Entity/EntityAura.js` (~linhas 30-105).
- **Alvo:** **Todas as entidades do tipo jogador em cena**, e não apenas o jogador local. Todo jogador recebido via pacote de visualização de unidade (`ZC_NOTIFY_STANDENTRY`, `ZC_NOTIFY_MOVEENTRY`, etc.) cujo nível de base satisfaça a condição ativa o efeito de aura.
- **Condição Numérica:** `entity.clevel >= settings.defaultLv` (valor padrão = **99**).
- **Visibilidade:** O efeito é exibido apenas se `entity.isVisible()` for verdadeiro. Se a entidade entrar em Furtividade (*Cloaking*), Esconderijo (*Hiding*), for ocultada pelo campo de visão ou estiver fora da câmera, a aura é imediatamente descarregada (`this.remove(effectManager)`).

### 2.2. Comandos de Controle `/aura` e `/aura2`
- **Origem:** `roBrowserLegacy src/Controls/ProcessCommand.js` (~linhas 120-155) e `src/Preferences/Map.js` (~linhas 60-70).
- Os comandos de chat permitem aos jogadores alternar o nível de detalhe visual da aura para otimizar desempenho ou reduzir poluição visual:

| Comando | Ação | Mensagem Oficial no Chat (`msgstringtable`) |
|---|---|---|
| `/aura` | Alterna entre **Modo 1** (Simplificado) e **Modo 2** (Normal) | Msg 711 (`"Aura effect is normal"`) / Msg 712 (`"Aura effect is minimized"`) |
| `/aura2` | Liga ou desliga totalmente (Modo 0 vs Modo 1/2) | Msg 2994 (`"Aura effect is OFF"`) / Msg 2995 (`"Aura effect is ON"`) |

### 2.3. Tabela de Modos de Renderização

| Modo | Valor de Preferência | Efeitos Renderizados | Descrição Visual |
|:---:|:---:|:---|---|
| **0** | `aura = 0` | *Nenhum* | Totalmente desativada. Nenhuma geometria ou partícula é instanciada. |
| **1** | `aura = 1` | `[EF_LEVEL99_3]` | **Aura Simplificada:** Apenas as partículas de brilho e diamantes flutuantes azuis sobem ao redor do personagem. O cilindro espiral e os anéis do chão não são desenhados. |
| **2** | `aura = 2` | `[EF_LEVEL99, EF_LEVEL99_2, EF_LEVEL99_3]` | **Aura Completa:** Todas as 3 camadas ativas simultaneamente (faixas espirais em cone + anéis pulsantes no piso + diamantes brilhantes). |

---

## 3. Especificação Numérica e Geométrica das Camadas da Aura

### 3.1. Camada 1: `EF_LEVEL99` (ID 200) — Faixas Espirais Cônicas (`SwirlingAura`)
- **Origem:** `roBrowserLegacy src/Renderer/Effects/SwirlingAura.js` (~linhas 1-220).
- **Textura:** `data/texture/effect/ring_blue.tga` (para a variante de nível 99 regular).
- **Modo de Mistura (Blend):** Aditivo (`SRC_ALPHA, ONE`). Não grava em Z-buffer de profundidade (`DepthWrite = false`).
- **Cor e Opacidade Base:**
  - Cor RGB: `(100/255, 100/255, 255/255)` = `(0.392, 0.392, 1.000)` (azul elétrico).
  - Alfa Base: `120 / 255` = **0.4706** (~47% de opacidade).
- **Topologia da Malha:**
  - 3 faixas independentes e sobrepostas (`ec = 0, 1, 2`), dispostas em ângulos defasados ao redor do ator.
  - Cada faixa é uma fita curva tridimensional segmentada em **21 subdivisões** (20 células quadrangulares = 40 triângulos e 42 vértices por faixa).
  - O arco total de varredura de cada faixa é de **315°** (`FULL_DISPLAY_ANGLE`).
  - Passo angular entre subdivisões: $\Delta \theta = 315^\circ / 20 = 15.75^\circ$.

#### Parâmetros Específicos por Faixa:

| Parâmetro | Faixa 0 (`ec = 0`) | Faixa 1 (`ec = 1`) | Faixa 2 (`ec = 2`) | Fórmula Geral |
|---|:---:|:---:|:---:|---|
| **Ângulo Inicial (`rotStart`)** | $0^\circ$ | $90^\circ$ | $180^\circ$ | $ec \times 90^\circ$ |
| **Altura Máxima (`maxHeight`)** | 15.0 un | 13.0 un | 11.0 un | $(15 - 2 \cdot ec) \cdot \text{scale}$ |
| **Raio da Base (`distance`)** | 2.34 un | 2.46 un | 2.58 un | $(3.9 + 0.2 \cdot ec) \cdot 0.6 \cdot \text{scale}$ |
| **Ângulo de Abertura do Cone (`riseAngle`)** | $55^\circ$ | $50^\circ$ | $45^\circ$ | $(55 - 5 \cdot ec)^\circ$ |
| **Velocidade de Rotação (`spinSpeed`)** | $3^\circ$ / tick | $4^\circ$ / tick | $5^\circ$ / tick | $(ec + 3)^\circ$ a cada 25 ms (120°/s a 200°/s) |

#### Fórmula do Perfil de Altura ($h[k]$):
Para a $k$-ésima subdivisão ($k \in [0, 20]$), o ponto central da faixa é $k = 10$:
$$\text{SinLimit}_k = 90^\circ + (k - 10) \times 9^\circ$$
- Em $k=10$, $\text{SinLimit} = 90^\circ \implies \sin(90^\circ) = 1.0$ (ponto mais alto no centro da fita).
- Em $k=0$ e $k=20$, $\text{SinLimit} = 0^\circ$ e $180^\circ \implies \sin = 0.0$ (a fita afunila até tocar a base nas extremidades).

Na fase de subida inicial (primeiros 90 ticks = 2,25 segundos após o spawn):
$$h_k(t) = \max\left(0, \; \text{maxHeight} \times \sin(\text{SinLimit}_k) \times \sin(process \cdot 1^\circ)\right)$$
Após $process \ge 90$ (regime permanente estacionário):
$$h_k = \text{maxHeight} \times \sin(\text{SinLimit}_k)$$

#### Geração de Vértices:
- **Vértice Inferior (Interno):**
  $$V_{\text{base}} = \left( \text{distance} \cdot \cos\theta, \; 0, \; \text{distance} \cdot \sin\theta \right), \quad \text{UV} = \left(\frac{k}{20}, \; 1.0\right)$$
- **Vértice Superior (Externo - projetado em cone aberto para fora):**
  $$R_x = \cos(\text{riseAngle}) \cdot h_k, \quad R_z = \sin(\text{riseAngle}) \cdot h_k$$
  $$V_{\text{top}} = \left( (\text{distance} + R_x) \cdot \cos\theta, \; R_z, \; (\text{distance} + R_x) \cdot \sin\theta \right), \quad \text{UV} = \left(\frac{k}{20}, \; 0.0\right)$$

---

### 3.2. Camada 2: `EF_LEVEL99_2` (ID 201) — Anel Pulsante de Chão (`GroundAura`)
- **Origem:** `roBrowserLegacy src/Renderer/Effects/GroundAura.js` (~linhas 1-180).
- **Textura:** `data/texture/effect/pikapika2.bmp`.
- **Modo de Mistura:** Aditivo (`SRC_ALPHA, ONE`).
- **Posicionamento:** Plano horizontal $XZ$ colado ao solo, com elevação de segurança $+0.05$ células (+5 cm em UE) acima da altura da célula do `.gat` para evitar Z-fighting com o mapa.
- **Cor e Opacidade:** Cor branca pura `(1.0, 1.0, 1.0)`, Alfa **0.80** (80%).
- **Geometria:** Dois quads horizontais (`aura[0]` e `aura[1]`), compostos por 2 triângulos cada (6 vértices).

#### Mecânica de Respiração e Pulsação:
- Parâmetros base: `size = 100`, `distance = 15.0`.
- Limite Mínimo de Tamanho: $\text{minSize} = \text{size} + \text{distance} = 115.0$.
- Limite Máximo de Tamanho: $\text{maxSize} = \text{size} + 2 \cdot \text{distance} = 130.0$.
- **Anel 0 (`aura[0]`):**
  - Tamanho inicial: $[115, 115]$.
  - Ângulo de rotação fixo: $0^\circ$.
  - Direção inicial: $+1$ (expandindo).
- **Anel 1 (`aura[1]`):**
  - Tamanho inicial: $[130, 130]$.
  - Ângulo de rotação fixo: $23^\circ$ de defasagem.
  - Direção inicial: $-1$ (contraindo).
- **Ciclo Temporal:**
  - A cada tick de 25 ms, o ângulo de fase avança $+3^\circ$ (velocidade de pulsação = $120^\circ/\text{s}$, ciclo completo de 3 segundos).
  - A cada $180^\circ$ percorrido, a direção inverte ($\times -1$), criando o efeito contínuo de expansão e contração alternada entre os dois anéis.

---

### 3.3. Camada 3: `EF_LEVEL99_3` (ID 202) — Partículas de Brilho / Diamantes (`Level99Bubble`)
- **Origem:** `roBrowserLegacy src/Renderer/Effects/Level99Bubble.js` (~linhas 1-280).
- **Textura:** `data/texture/effect/whitelight.tga`.
- **Modo de Mistura:** Aditivo (`SRC_ALPHA, ONE`), renderizado em **2 passes** (`passCount = 2`) para atingir a luminosidade brilhante e saturada do cliente oficial.
- **Cor:** Azul brilhante `(80/255, 80/255, 255/255)` = `(0.314, 0.314, 1.000)`.
- **Dimensão da Partícula:** Quads em billboard voltados diretamente para a câmera, com raio base $R = 2.4 \times 0.6 \times \text{scale} \approx 0.316$ unidades.
- **Número de Emissores e Partículas:**
  - **4 colunas verticais** distribuídas em leque simétrico em torno do personagem.
  - Cada coluna contém **4 âncoras** (`B_pre`, `B_now`, `T_pre`, `T_now`), totalizando **16 partículas ativas simultâneas**.

#### Padrões de Sinal e Jitter Horizontal:
Cada âncora de uma coluna oscila nos eixos $X$ e $Z$ de acordo com uma tabela de inversão de fase:
- Âncora 0 (`B_pre`): $k_x = +1, \; k_z = +1$ (fases 0 e 2).
- Âncora 1 (`B_now`): $k_x = -1, \; k_z = -1$ (fases 4 e 6).
- Âncora 2 (`T_pre`): $k_x = +1, \; k_z = -1$ (fases 8 e 10).
- Âncora 3 (`T_now`): $k_x = -1, \; k_z = +1$ (fases 12 e 14).

A perturbação harmônica contínua é dada por:
$$x(t) = k_x \cdot K_{\text{drift}} \cdot \sin(\theta_A), \quad z(t) = k_z \cdot K_{\text{drift}} \cdot \sin(\theta_B)$$
onde $K_{\text{drift}} = 0.15$ unidades e os ângulos de fase avançam entre $2^\circ$ e $3^\circ$ por quadro em direção a alvos randômicos de $0^\circ$ a $360^\circ$.

#### Cinemática Vertical e Respawn:
- As partículas nascem com altura inicial $y_{\text{spawn}} \in [0, 99]$ unidades e sobem em direção ao topo (velocidade constante $v = 0.15$ unidades por tick).
- **Limite de Vida:** Quando ultrapassam o teto $y \le -30$, a partícula é reiniciada na base do personagem com novo valor randômico $y \in [0, 99]$ e novas fases angulares re-sorteadas.
- **Curva de Alfa por Altura:**
  $$\alpha(y) = \text{clamp}\left(\frac{250 + 25 \cdot (y + 20)}{255}, \; 0.0, \; \frac{250}{255}\right)$$
  Essa curva garante fade-in suave na base do personagem, opacidade plena durante a subida pelo tronco, e desvanecimento suave ao atingir a cabeça.

---

### 3.4. Variações para Transclasses e Nível Estendido

| ID | Efeito | Classe Correspondente | Diferença Visual em Relação ao Nível 99 Regular |
|:---:|---|---|---|
| **362** | `EF_LEVEL99_4` | Transclasse 99 (Lord Knight, High Priest, etc.) | Partículas de brilho com tonalidade dourada/âmbar (`whitelight.tga` com cor amarela/laranja quente). |
| **397** | `EF_LEVEL99_5` | Transclasse 99 (Banda Central) | Faixa espiral ascendente com cor e textura dourada brilhante (efeito de labaredas flamejantes). |
| **398** | `EF_LEVEL99_6` | Transclasse 99 (Piso) | Anel horizontal do chão com tonalidade amarelo-ouro pulsante. |
| **881** | `EF_LEVEL99_150` | 3ª Classe Nível 150/175 | Aura azul etérea concentrada e densa. |
| **976** | `EF_LEVEL99_ORB1` | 3ª Classe Máxima | Esfera de luz orbital interna girando ao redor da cintura. |
| **977** | `EF_LEVEL99_ORB2` | 3ª Classe Máxima | Segunda esfera orbital em órbita elíptica elipsoidal oposta. |

---

## 4. Auditoria dos Efeitos com `"type": "FUNC"` Vazios em `skill_effects_full.json`

Na base de dados `Config/skill_effects_full.json`, existem **33 entradas** marcadas como `"type": "FUNC"` onde o campo `func` e `file` estão vazios. A pesquisa reversa em `roBrowserLegacy/src/DB/Effects/EffectTable.js` identificou a exata função de cada uma:

| ID no JSON | Nome Oficial RO | Classe/Sistema do Cliente | Comportamento Exato no Executável |
|:---:|---|---|---|
| **45** | `EF_FIREFLY` | Efeito Ambiental | Partículas fracas e lentas de vaga-lumes flutuando ao redor da unidade (`Faint Little Balls of Light`). |
| **60** | `EF_LOCKON` | Combate / Alvo | Retículo circular de mira travado no alvo de conjuração (`LockOnTarget.js`). |
| **161** | `EF_RAIN` | Sistema de Clima | Efeito de clima de Chuva no mapa (`RainWeatherEffect.startOrRestart()`). Partículas caindo em diagonal. |
| **162** | `EF_SNOW` | Sistema de Clima | Efeito de clima de Neve no mapa (`SnowWeatherEffect.startOrRestart()`). Flocos caindo e oscilando. |
| **163** | `EF_SAKURA` | Sistema de Clima | Efeito de Pétalas de Cerejeira caindo (`SakuraWeatherEffect.startOrRestart()`). |
| **200** | `EF_LEVEL99` | Aura Nível 99 | Camada 1: 3 faixas cônicas espirais ascendentes (`SwirlingAura.js`). |
| **201** | `EF_LEVEL99_2` | Aura Nível 99 | Camada 2: 2 anéis pulsantes no piso com respiração periódica (`GroundAura.js`). |
| **228** | `EF_CHOOKGI` | Esferas Espirituais | **Gatilho Lógico:** Atualiza e posiciona as esferas espirituais orbitais do ator (`SpiritSphere.update(entity)`). Não desenha malha direta; instancia o controlador de esferas. |
| **229** | `EF_CLOUD` | Sistema de Clima | Efeito de Neblina / Nuvens volumétricas baixas (`CloudWeatherEffect`). |
| **230** | `EF_CLOUD2` | Sistema de Clima | Nuvens baixas que flutuam abaixo do nível transitável (`GAT`) do mapa. |
| **233** | `EF_CLOUD3` | Sistema de Clima | Nuvens de poluição e vapor industrial de Einbroch. |
| **239** | `EF_BOTTOM_VO` | Habilidade de Solo | Terreno de Vulcão (*Volcano*) do Sábio: Malha procedural circular no chão com `ring_red.tga` (`PropertyGround.js`). |
| **240** | `EF_BOTTOM_DE` | Habilidade de Solo | Terreno de Dilúvio (*Deluge*) do Sábio: Malha procedural circular com `ring_blue.tga` (`PropertyGround.js`). |
| **241** | `EF_BOTTOM_VI` | Habilidade de Solo | Terreno de Furacão (*Violent Gale*) do Sábio: Malha circular com `ring_yellow.tga` (`PropertyGround.js`). |
| **242** | `EF_BOTTOM_LA` | Habilidade de Solo | Protetor de Terreno (*Magnetic Earth / Land Protector*) do Sábio: Grade de ladrilhos mágicos no solo (`LPEffect.js`). |
| **288_ground** | `EF_BOTTOM_POEMBRAGI` | Habilidade de Bardo | Área do Poema de Bragi no solo: Pauta musical e notas dançantes procedurais no piso (`Songs.js`) + áudio `effect/poem_bragi.wav`. |
| **297** | `EF_POKJUK` | Evento / Festivo | Efeito de Fogos de Artifício festivos explodindo no céu com partículas coloridas (`PokJukWeatherEffect.js`). |
| **333** | `EF_MAPLE` | Sistema de Clima | Efeito de Folhas de Outono/Bordo caindo (`SakuraWeatherEffect` configurado com textura de folhas avermelhadas). |
| **334** | `EF_BLIND` | Status Negativo | Pós-processamento de Cegueira: escurece a tela do jogador reduzindo o raio de visão a 1 círculo iluminado (`Blind.setActive(true)`) + som `_blind.wav`. |
| **335** | `EF_POISON` | Status Negativo | Pós-processamento de Envenenamento: vinheta esverdeada/púrpura pulsante nas bordas da tela (`Poison.setActive(true)`). |
| **404** | `EF_BOTTOM_SPIDER` | Habilidade de Solo | Teia de Aranha (*Fiber Lock*) do Professor: malha 3D de teia no chão grudada nas coordenadas do piso (`SpiderWeb.js`). |
| **421** | `EF_BABYBODY2` | Habilidade Eswoo | Redução de Escala: Encolhe a escala do ator temporariamente para modelo de bebê/pequeno (`EffectSmallBody`). |
| **423** | `EF_GIANTBODY2` | Habilidade Eswoo | Restauração de Escala: Restaura a escala normal ou amplia o ator (`EffectBigBody`). |
| **496_beforecast**| `EF_GUARD2` | Preservar (Stalker) | **Gatilho de Áudio/Pré-Cast:** Toca o efeito sonoro de concentração `black_maximize_power_sword_bic.wav`. |
| **504** | `EF_CHOOKGI2` | Esferas de Monge | **Gatilho Lógico:** Atualiza a quantidade e órbita das esferas espirituais de Monge/Mestre (`SpiritSphere.update`). |
| **513** | `EF_GROUNDSAMPLE`| Mira de Solo | Círculo de área de seleção mágica sob o cursor ao mirar habilidades de área de solo (`MagicTarget.js`). |
| **515** | `EF_CLOUD4` | Sistema de Clima | Nuvens em alta velocidade da rota de voo dos dirigíveis/aeroplanos. |
| **516** | `EF_CLOUD5` | Sistema de Clima | Camada de nuvens tempestuosas místicas da Torre de Thanatos. |
| **592** | `EF_CLOUD6` | Sistema de Clima | Nuvens de ambientação genérica para mapas de montanha e topo de penhasco. |
| **629** | `EF_CHOOKGI3` | Moedas de Justiceiro| **Gatilho Lógico:** Atualiza o contador de moedas orbitais de Justiceiro (`SpiritSphere.update`). |
| **697** | `EF_CLOUD7` | Sistema de Clima | Nuvens noturnas azul-escuras. |
| **698** | `EF_CLOUD8` | Sistema de Clima | Nuvens alaranjadas de entardecer/pôr-do-sol. |
| **833** | `EF_CHOOKGI_N` | Esferas 3ª Classe | **Gatilho Lógico:** Atualiza esferas de Guarda Real e classes Renewal. |

---

## 5. Auditoria dos Efeitos de Tremor de Tela (`FUNC quake`)

Na tabela `Config/skill_effects_full.json`, existem exatamente **29 registros** contendo `"func": "quake"`:

### 5.1. Duração e Amplitude Padrão
- Todas as 29 entradas individuais declaram a duração nominal idêntica de **`duration: 50` ms**.
- **Comportamento da Câmera:** No cliente original (`Camera.setQuake`), um tremor de 50 ms corresponde a um abalo curto e seco (impacto sísmico):
  - Deslocamento Vertical: oscilação estocástica entre $-8$ px e $+8$ px no plano de visualização.
  - Frequência de Atualização: recalculado a cada quadro com atenuação linear exponencial imediata ao longo dos 50 ms.

### 5.2. O Caso Especial do Terremoto (`ID 666` — *Earthquake*)
Diferente de todos os outros tremores de golpe único (como *Magnum Break* ou *Golpe Fulminante*), o efeito **666 (*Earthquake*)** é composto por uma rajada encadeada de tremores:
- **`duplicate`: 3** (três tremores em série).
- **`timeBetweenDupli`: 35 ms** (intervalo de 35 ms entre os pulsos).
- **`duration`: 50 ms** por pulso.
- **Resultado Prático:** Uma vibração contínua e estrondosa que dura aproximadamente **$35 \times 2 + 50 = 120$ a $150$ ms**, sincronizada com o arquivo sonoro `effect/earth_quake.wav` e os quads de rachaduras no solo (`어스퀘이크`).

---

## 6. Orientações para Implementação na Unreal Engine 5

1. **Aura 99 no `UEMyPlayerActor` e `UEMobActor`:**
   - Criar o componente `UAura99Component` anexado à raiz de qualquer ator derivado de unidade móvel.
   - Quando o nível de base do pacote de entrada for $\ge 99$, spawnar o componente.
   - Camada 1 (`SwirlingAura`): Gerar via Procedural Mesh Component com os 21 vértices e UVs exatos descritos na seção 3.1, ou via Material World Position Offset com seno harmônico sobre um cilindro base.
   - Camada 2 (`GroundAura`): Decalque no chão ou quad plano com escala dinâmica senoidal modulada no tick.
   - Camada 3 (`Level99Bubble`): Sistema Niagara GPU com 4 emissores de partículas e curvas de jitter senoidal combinadas.
2. **Gatilhos de `FUNC`:**
   - Para os IDs de clima (161, 162, 163, 229, etc.), conectar ao gerenciador de atmosfera do mapa (`AWeatherManager`), ligando/desligando emissores globais de chuva, neve ou nuvens.
   - Para Cegueira (334) e Envenenamento (335), ligar os pós-processos da câmera correspondentes (`PostProcessComponent`).
   - Para os IDs lógicos de esferas (228, 504, 629), chamar a rotina de posicionamento de atores orbitais do personagem.

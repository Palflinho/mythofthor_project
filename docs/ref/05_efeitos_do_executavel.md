# Especificação 05: Efeitos do Executável — Código Procedural (Sem .str)

> **Origem das referências:**
> - **OM (Open-Midgard):** `src/world/RagEffect.h`, `src/world/RagEffect.cpp` (linhas 10-48, 2900-3050, 3450-3500, 4520-4645, 4815-4875, 5000-5095), `src/world/MsgEffect.h`, `src/world/MsgEffect.cpp` (linhas 25-50, 350-480, 520-620, 1880-2050), `src/world/RagEffectCatalog.generated.inc`.
> - **RB (roBrowserLegacy):** `src/Renderer/Effects/GroundEffect.js`, `src/Renderer/Effects/TwoDEffect.js`, `src/Renderer/Entity/EntityAura.js`.
> - **Arquivos de Recursos:** `data/sprite/이펙트/숫자.spr` (números), `msg.spr` (miss/lucky), texturas em `data/texture/effect/`.

---

## 1. Resumo

Embora a maioria das habilidades avançadas de Ragnarok Online utilize animações pré-gravadas no formato `.str` (EzStr), os efeitos mais frequentes e vitais do jogo são inteiramente desenhados via código procedural dentro do executável. Isso inclui o portal de teleporte (composto de anéis concêntricos ascendentes, rotação cilíndrica e coluna de vento), a física e trajetória de números de dano (arco parabólico lateral e fade-out), as mensagens visuais ("Miss", "Lucky"), a cura (elevação com expansão de escala), a aura de nível 99 e a aparição do Super Anjo de Level Up.

---

## 2. Dados e Constantes

### 2.1. Números de Dano e Mensagens (`MsgEffect.cpp`)

| Parâmetro | Valor | Unidade | Descrição / Referência |
|---|---|---|---|
| Frequência de Atualização do Dano | `24.0` | ticks/s | Fator $1/24 \approx 0.0416667$ por milissegundo decorrido (`MsgEffect.cpp:1890`). |
| Duração Total do Número de Dano | `70` | ticks (~2916 ms) | Tempo até sumir da tela (`MsgEffect.cpp:2006`). |
| Gravidade / Arco Parabólico do Dano | $8.0 - (2.0 - t \times 0.0333) \times t$ | unidades | Trajetória vertical parabólica que sobe e cai (`MsgEffect.cpp:1976`). |
| Projeção Lateral do Dano | $(t \times 0.3333 + 3.0) \times \text{destPos}$ | unidades | Deslocamento horizontal em arco dependente da rotação do alvo (`MsgEffect.cpp:1974`). |
| Escala do Dano Normal | $\max(1.2, \text{orgZoom} - t \times 0.24)$ | escala | Começa grande e estabiliza em 1.2x (`MsgEffect.cpp:1977`). |
| Escala do Dano Crítico | $\max(1.0, \text{orgZoom} - t \times 0.144)$ | escala | Decaimento mais suave mantendo tamanho destacado (`MsgEffect.cpp:1931`). |
| Cor do Dano Normal (Causado) | `0xFFFFFFFF` | ARGB | Branco puro. |
| Cor do Dano Crítico | `0xFFFFFF00` / `0xFFFFD700` | ARGB | Amarelo dourado. |
| Cor do Dano Recebido pelo Jogador | `0xFFFF2020` | ARGB | Vermelho brilhante. |
| Cor de Cura (`Heal`) | `0xFF40FF40` | ARGB | Verde luminoso. |
| Duração da Animação de Cura | `120` | ticks (5000 ms) | Deslocamento vertical suave subindo (`MsgEffect.cpp:2024`). |
| Escala Máxima da Cura | `3.0` | escala | Cresce progressivamente de 1.0 até 3.0 (`MsgEffect.cpp:2018`). |
| Início do Fade-out da Cura | `90` | ticks (~3750 ms) | Fade linear acelerado a partir do tick 90 (`MsgEffect.cpp:2020`). |

### 2.2. Portal de Teleporte (`PP_PORTAL` e `PP_PORTALSTACK`)

| Parâmetro | Valor | Unidade | Descrição / Referência |
|---|---|---|---|
| Quantidade de Anéis da Pilha (`PP_PORTALSTACK`) | `3` | anéis | Anéis concêntricos ascendentes (`RagEffect.cpp:3475`). |
| Velocidade de Subida dos Anéis (`riseSpeed`) | `0.5` | unidades/tick | Taxa de elevação vertical de cada anel (`RagEffect.cpp:3477`). |
| Intervalo entre Anéis (`delayStep`) | `10` | ticks | Defasagem temporal de emissão de cada anel (`RagEffect.cpp:3478`). |
| Ciclo Completo da Pilha (`cycleTicks`) | `28` | ticks | Período de repetição do anel (`RagEffect.cpp:3479`). |
| Altura Máxima do Anel (`grow`) | `7.0` | unidades | Altura limite de dissipação do anel ascendente (`RagEffect.cpp:3488`). |
| Altura da Faixa Cilíndrica (`maxHeight`) | `6.0` | unidades | Altura do cilindro translúcido da base (`RagEffect.cpp:4550`). |
| Velocidades de Rotação das Bandas | `2.0`, `3.0`, `4.0` | graus/tick | Três camadas cilíndricas rotacionando em velocidades diferentes (`RagEffect.cpp:4550-4552`). |
| Textura Principal do Portal | `effect\ring_blue.tga` | arquivo | Textura do anel azul clássico (`RagEffect.cpp:4546`). |
| Som do Portal em Loop (`ef_readyportal.wav`) | A cada `14` | ticks | Intervalo de repetição do áudio ambiente (`RagEffect.cpp:4614`). |

---

## 3. Comportamento e Fórmulas dos Efeitos Procedurais

### 3.1. Números de Dano (Física do Salto)
A cada quadro, para cada dígito ou número de dano ativo:
$$t = (\text{tempoAtual} - \text{stateStartTick}) \times \frac{1}{24}$$
1. **Deslocamento Lateral:**
   O vetor de espalhamento lateral $\text{destPos}$ é perpendicular à rotação do alvo ($\text{roty} \pm 90^\circ$):
   $$\text{pos.X} = \text{orgPos.X} + (t \times 0.3333 + 3.0) \times \text{destPos.X}$$
   $$\text{pos.Z} = \text{orgPos.Z} + (t \times 0.3333 + 3.0) \times \text{destPos.Z}$$
2. **Arco Parabólico Vertical:**
   $$\text{pos.Y} = \text{orgPos.Y} + 8.0 - (2.0 - t \times 0.0333) \times t$$
   O número "salta" para cima nos primeiros ticks e depois decai suavemente pela gravidade.
3. **Fade-out:**
   $$\alpha = \max(0, 250 - t \times 3.4)$$
   Aos 70 ticks ($t \ge 70$), a opacidade atinge 0 e o efeito é destruído.

### 3.2. Efeito de Cura (`Heal`)
1. **Trajetória:** O valor de cura não salta para os lados; ele sobe verticalmente em linha reta:
   $$\text{pos.Y} = \text{orgPos.Y} - t \times 0.1$$
2. **Expansão de Escala:**
   $$\text{zoom} = \min(3.0, \text{zoom} + t \times 0.18)$$
   O número começa pequeno e expande até 3x o tamanho original.
3. **Desaparecimento:**
   Mantém opacidade total (250) até $t = 90$, quando desvanece rapidamente:
   $$\alpha = \max(0, 250 - (t - 90) \times 8.0)$$

### 3.3. Mensagens "Miss" e "Lucky Dodge"
- Utilizam o arquivo de sprite `data/sprite/이펙트/msg.spr` e `msg.act`:
  - Ação do Miss: exibe a palavra "Miss" em letras clássicas azuis com contorno branco, subindo verticalmente $0.54$ unidades por tick e durando 80 ticks.
  - Ação do Lucky: exibe o trevo e o texto "Lucky!", acompanhado pelo som de esquiva perfeita.

### 3.4. Portal de Teleporte Clássico
O portal do Ragnarok é composto por 3 elementos procedurais sincronizados:
1. **Anéis Ascendentes (`PP_PORTALSTACK`):**
   - 3 anéis desenhados com textura `ring_blue.tga` e blend aditivo (`D3DBLEND_ONE`).
   - Cada anel sobe a $0.5$ unidades/tick até a altura $7.0$, desvanecendo nos últimos instantes.
2. **Cilindro Base Rotativo (`PP_PORTAL`):**
   - 3 bandas cilíndricas concêntricas de altura 6.0 unidades com rotações defasadas ($0^\circ, 25^\circ, 50^\circ$) girando a 2, 3 e 4 graus por tick.
3. **Nuvem Central de Vento (`PP_WIND`):**
   - Textura `cloud11.tga` pulsando verticalmente no centro do portal.

### 3.5. Level Up e Super Anjo
1. **Level Up de Base (Efeito 371):**
   - Executa `angel.str` e toca `effect\levelup.wav` no frame 0.
   - Gera um feixe de luz vertical com partículas douradas subindo em espiral.
2. **Level Up de Job 50 (Efeito 337 / `SpawnJobLevelUp50`):**
   - Executa `joblvup.str` e dispara no frame 5 uma explosão radial verde/azul (`magic_green.tga` e `magic_blue.tga`) com partículas concêntricas.
3. **Super Anjo (Efeito 338 e 582 / `PP_SUPERANGEL`):**
   - Utilizado para classes especiais (Super Aprendiz, Taekwon 99, etc.).
   - Dispara asas procedurais translúcidas (`priest_angel_wing.bmp`) que batem em escala $1.15$x com tint dourado/esmeralda e som de `levelup.wav`.

### 3.6. Aura de Nível 99 (`EF_LEVEL99`, `EF_LEVEL99_2`, `EF_LEVEL99_3`)
- Ativada automaticamente quando o personagem alcança o nível máximo (`clevel >= 99`).
- Composta por:
  - Anéis azuis brilhantes girando no chão ao redor dos pés do personagem.
  - Partículas de energia e fagulhas de luz subindo continuamente até a altura da cabeça.
  - O comando `/aura` alterna entre modo completo (anéis + partículas) e simplificado (apenas anel no chão).

---

## 4. Entradas e Saídas

### Entradas
- **Pacote de Dano (`ZC_NOTIFY_ACTORINIT` / `ZC_ATTACK_RANGE` / `ZC_USE_SKILL`):**
  Informa o valor numérico do dano, flag de crítico (`damageKind`), esquiva (`miss`) ou valor de cura.
- **Pacote de Efeito Especial (`ZC_USE_SKILL` / `ZC_NOTIFY_EFFECT` / `ZC_EFFECT`):**
  Informa o ID do efeito (ex: 50001 para portal, 371 para level up, 200 para aura 99).

### Saídas
- **Renderizador de Partículas e Quads no Mundo 3D:**
  Emissão de quads com blend aditivo (`D3DBLEND_ONE`) ou alpha blend (`D3DBLEND_INVSRCALPHA`) projetados na matriz de visão.
- **Sons Especiais:**
  Disparo de `effect\levelup.wav`, `effect\ef_readyportal.wav`, `effect\ef_heal.wav`.

---

## 5. Diferenças entre as Referências

1. **Implementação de Shaders vs Funções Fixas:**
   - O **OM** utiliza o pipeline clássico de Direct3D 9 com quads billboard (`SubmitWorldQuad`) e cálculo trigonométrico de bandas cilíndricas (`RenderBandLowPolygonTeiRect`).
   - O **RB** implementa os efeitos de chão como shaders WebGL dedicados (`GroundEffect.vs` e `GroundEffect.fs`), gerando malhas dinâmicas no terreno (`Altitude.generatePlane`).
   - Para o Myth of Thor na Unreal Engine, materiais translúcidos com blend aditivo e Niagaras leves reproduzem com fidelidade idêntica.

---

## 6. Como Testar

1. **Teste de Dano Normal e Físico de Salto:**
   - Ataque um Poring.
   - **Resultado esperado:** O número de dano branco salta para cima e para o lado do monstro em um arco parabólico suave, desaparecendo após ~2,9 segundos.
2. **Teste de Golpe Crítico:**
   - Cause um dano crítico com adaga ou arma de crítico.
   - **Resultado esperado:** O número surge em amarelo com escala destacada e trajetória mais ampla.
3. **Teste de Miss:**
   - Tente acertar um monstro de esquiva alta.
   - **Resultado esperado:** O sprite `Miss` sobe verticalmente em linha reta sobre a cabeça do monstro.
4. **Teste de Portal:**
   - Aproxime-se de um portal de Prontera.
   - **Resultado esperado:** Três anéis azuis concêntricos sobem ciclicamente do chão enquanto as faixas cilíndricas giram na base e o som do portal toca a cada 14 ticks.
5. **Teste de Level Up:**
   - Suba um nível de personagem.
   - **Resultado esperado:** O som clássico `levelup.wav` toca imediatamente e o feixe do anjo sobe em torno do personagem.

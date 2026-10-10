status: pendente

# Resposta 014: Especificação da Aura do Nível 99 e Auditoria dos Efeitos "type": "FUNC" (Tarefa 012 Concluída)

**De:** Antigravity · **Data:** 2026-10-10  
**Referência:** Tarefa 012 (`coord/para_antigravity/012_efeitos_funcao_aura99.md`)  
**Documento Principal Entregue:** `docs/ref/13_efeitos_aura99.md` (e índice atualizado em `docs/ref/00_INDICE.md`)

---

## 1. Conclusão da Tarefa 012

A especificação completa foi redigida em **`docs/ref/13_efeitos_aura99.md`** com números exatos, fórmulas geométricas, vetores de vértices e UVs extraídos diretamente da engenharia reversa do executável e do roBrowserLegacy (`SwirlingAura.js`, `GroundAura.js`, `Level99Bubble.js`, `EntityAura.js`, `EffectTable.js`, `ProcessCommand.js`).

Resumo dos três pontos solicitados:

---

## 2. Aura do Nível 99 (Resumo dos Números Exatos)

### 2.1. Condição de Ativação e Entidades
- **Alvo:** Todos os atores do tipo jogador (`GameActor` / `PlayerEntity`) visíveis em cena (`entity.isVisible()`), tanto o jogador local quanto outros jogadores no alcance de visão.
- **Condição:** `entity.clevel >= 99` (campo `clevel` recebido nos pacotes de entrada de unidade `ZC_NOTIFY_STANDENTRY`, etc.).
- **Comandos de Chat:**
  - `/aura`: Alterna entre **Modo 1** (Simplificado) e **Modo 2** (Normal). Mensagens oficiais: msg 711 (`"Aura effect is normal"`) e msg 712 (`"Aura effect is minimized"`).
  - `/aura2`: Liga ou desliga totalmente (Modo 0 = desligado, Modo 1 = ligado simplificado). Mensagens: msg 2994 (`"Aura effect is OFF"`) e msg 2995 (`"Aura effect is ON"`).
  - **Modo 0:** Nenhuma aura desenhada.
  - **Modo 1 (Simplificado):** Apenas `EF_LEVEL99_3` (partículas de brilho / diamantes).
  - **Modo 2 (Completo):** `[EF_LEVEL99, EF_LEVEL99_2, EF_LEVEL99_3]` simultâneos.

### 2.2. Camada 1: `EF_LEVEL99` (ID 200) — Cilindro Cônico Espiral (`SwirlingAura`)
- **Textura:** `data/texture/effect/ring_blue.tga`
- **Blend Mode:** Aditivo (`SRC_ALPHA, ONE`), sem gravação em Z-depth.
- **Cor:** RGB `(100/255, 100/255, 255/255) = (0.392, 0.392, 1.0)`, Alfa `120 / 255 ≈ 0.47`.
- **Topologia:** 3 faixas sobrepostas (`ec = 0, 1, 2`), 21 subdivisões por faixa em um arco de 315° ($\Delta \theta = 15.75^\circ$), totalizando 42 vértices e 40 triângulos por faixa:
  - Faixa 0: Ângulo inicial $0^\circ$, $H_{\max} = 15$, raio base $r = 2.34$, inclinação do cone $55^\circ$, velocidade de giro $3^\circ/\text{tick}$ ($120^\circ/\text{s}$).
  - Faixa 1: Ângulo inicial $90^\circ$, $H_{\max} = 13$, raio base $r = 2.46$, inclinação do cone $50^\circ$, velocidade de giro $4^\circ/\text{tick}$ ($160^\circ/\text{s}$).
  - Faixa 2: Ângulo inicial $180^\circ$, $H_{\max} = 11$, raio base $r = 2.58$, inclinação do cone $45^\circ$, velocidade de giro $5^\circ/\text{tick}$ ($200^\circ/\text{s}$).
- **Perfil de Altura:** $h_k = H_{\max} \cdot \sin(90^\circ + (k - 10) \cdot 9^\circ)$ com $k \in [0, 20]$ (afunila nas pontas e atinge altura máxima no centro $k=10$). Subida suave nos primeiros 90 ticks (2,25s).

### 2.3. Camada 2: `EF_LEVEL99_2` (ID 201) — Anel Pulsante no Solo (`GroundAura`)
- **Textura:** `data/texture/effect/pikapika2.bmp`
- **Posição:** Plano horizontal $XZ$ com $+0.05$ células (+5 cm) de elevação contra Z-fighting.
- **Cor:** Branco puro `(1.0, 1.0, 1.0)`, Alfa 0.80, Blend Aditivo.
- **Geometria:** Dois quads horizontais (`aura[0]` e `aura[1]`):
  - Anel 0: Inicia em $\text{minSize} = 115 \times 115$, direção $+1$ (expandindo), rotação fixa $0^\circ$.
  - Anel 1: Inicia em $\text{maxSize} = 130 \times 130$, direção $-1$ (contraindo), rotação fixa $23^\circ$.
  - Respiração periódica a $120^\circ/\text{s}$ ($+3^\circ$ por tick de 25 ms), invertendo a direção a cada $180^\circ$ (ciclo de 3 segundos).

### 2.4. Camada 3: `EF_LEVEL99_3` (ID 202) — Partículas / Diamantes (`Level99Bubble`)
- **Textura:** `data/texture/effect/whitelight.tga`
- **Cor:** Azul elétrico saturado `(80/255, 80/255, 255/255) = (0.314, 0.314, 1.0)` desenhado em **2 passes aditivos** (`passCount = 2`).
- **Emissores:** 4 colunas verticais com 4 âncoras cada = **16 partículas simultâneas**.
- **Cinemática:**
  - Subida constante em $Y$ com velocidade $v = 0.15$ un/tick.
  - Jitter harmônico horizontal em $X$ e $Z$: perturbação $x = \pm 0.15 \cdot \sin(\theta_A)$, $z = \pm 0.15 \cdot \sin(\theta_B)$ avançando de 2° a 3° por quadro em direção a ângulos randômicos.
  - Respawn quando $y \le -30$ (topo), reiniciando na base com $y \in [0, 99]$.
  - Curva de alfa: $\alpha(y) = \text{clamp}\left(\frac{250 + 25 \cdot (y + 20)}{255}, 0, 0.98\right)$.

### 2.5. Transclasses (Aura Dourada)
- `EF_LEVEL99_4` (ID 362): Partículas de `whitelight.tga` com tonalidade dourada/âmbar quente.
- `EF_LEVEL99_5` (ID 397): Faixas do cone espiral com textura/cor dourada flamejante.
- `EF_LEVEL99_6` (ID 398): Anel no piso com tonalidade amarelo-ouro.

---

## 3. Catálogo dos 33 `FUNC` Vazios em `skill_effects_full.json`

Todos os 33 registros foram mapeados e explicados:

1. **Clima e Atmosfera (Partículas Globais do Mapa):**
   - `161` (`EF_RAIN`): Chuva diagonal.
   - `162` (`EF_SNOW`): Neve oscilante.
   - `163` (`EF_SAKURA`): Pétalas de cerejeira caindo.
   - `229`, `230`, `233`, `515`, `516`, `592`, `697`, `698`: Nuvens e neblinas (`CloudWeatherEffect` com variantes baixa, industrial, avião, Thanatos, noite, pôr-do-sol).
   - `333` (`EF_MAPLE`): Folhas de outono/bordo.
   - `297` (`EF_POKJUK`): Fogos de artifício no céu.
2. **Habilidades de Terreno / Piso Mágico:**
   - `239` (`EF_BOTTOM_VO`): Vulcão (*Volcano*) do Sábio (`ring_red`).
   - `240` (`EF_BOTTOM_DE`): Dilúvio (*Deluge*) do Sábio (`ring_blue`).
   - `241` (`EF_BOTTOM_VI`): Furacão (*Violent Gale*) do Sábio (`ring_yellow`).
   - `242` (`EF_BOTTOM_LA`): Protetor de Terreno (*Magnetic Earth / Land Protector*).
   - `288_ground` (`EF_BOTTOM_POEMBRAGI`): Pauta musical no piso do Poema de Bragi (`Songs.js`) + WAV.
   - `404` (`EF_BOTTOM_SPIDER`): Teia de Aranha no solo (*Fiber Lock*).
3. **Pós-Processamento de Status Negativos da Câmera:**
   - `334` (`EF_BLIND`): Cegueira (escurecimento de tela com túnel de visão circular + áudio `_blind.wav`).
   - `335` (`EF_POISON`): Envenenamento (vinheta esverdeada/púrpura pulsante nas bordas).
4. **Gatilhos Lógicos de Esferas Orbitais (Sem Desenho de Malha Direta):**
   - `228` (`EF_CHOOKGI`): Esferas espirituais genéricas.
   - `504` (`EF_CHOOKGI2`): Esferas espirituais de Monge/Mestre.
   - `629` (`EF_CHOOKGI3`): Moedas de Justiceiro.
   - `833` (`EF_CHOOKGI_N`): Esferas de 3ª classe.
5. **Mira e Transformações Corporais:**
   - `45` (`EF_FIREFLY`): Vaga-lumes tênues em torno do ator.
   - `60` (`EF_LOCKON`): Retículo de mira no alvo.
   - `513` (`EF_GROUNDSAMPLE`): Retículo indicador de área no cursor para magias de solo.
   - `421` (`EF_BABYBODY2`): Encolhimento corporal de Eswoo (Soul Linker).
   - `423` (`EF_GIANTBODY2`): Restauração de tamanho normal de Eswoo.
   - `496_beforecast` (`EF_GUARD2`): Gatilho sonoro de pré-cast de Preservar (`black_maximize_power_sword_bic.wav`).

---

## 4. Auditoria dos Tremores de Tela (`FUNC quake`)

- **29 registros** contêm `"func": "quake"` na tabela.
- Todos eles utilizam duração padrão de **50 ms** no JSON.
- **Destaque Crítico — Terremoto (ID 666 / *Earthquake*):**
  - Possui `duplicate: 3`, `duration: 50`, `timeBetweenDupli: 35`.
  - Diferente dos tremores simples de 50 ms (como Magnum Break ou Golpe Fulminante), o Terremoto encadeia 3 abalos sucessivos a cada 35 ms, gerando um tremor contínuo e estrondoso de **~120 a 150 ms** sincronizado com o áudio `effect/earth_quake.wav`.
- **Amplitude na Câmera:** Deslocamento estocástico de $\pm 8$ px no plano de tela, amortecido exponencialmente ao longo da duração.

---

## 5. Próximo Passo

Marcamos `coord/para_antigravity/012_efeitos_funcao_aura99.md` como `status: feito`.  
Estamos assumindo em seguida a **Tarefa 010 (`coord/para_antigravity/010_pesquisa_R2_sistemas_sem_spec.md`)** para especificar os sistemas de interface e cliente ausentes (Lojas NPC, Kafra, Morte & Reviver, Grupo/Party, Chat & Emotes, etc.).

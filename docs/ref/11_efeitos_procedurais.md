# Referência 11: Efeitos Procedurais de Habilidades (GPU / Código)

> **Fonte:** roBrowser (`src/Renderer/Effects/`, `EffectTable.js`, `SkillEffect.js`) e engenharia reversa do cliente oficial Ragnarok Online.  
> **Escopo:** Fase 2.1 / Fase 7.1 — Efeitos visuais desenhados pela engine (sem arquivo `.str` ou combinados com geometrias dinâmicas).  
> **Prioridade:** Bloco 1 — Habilidades da 1ª Classe (Aprendiz, Espadachim, Mago, Arqueiro, Noviço, Mercador, Gatuno).

---

## 1. Visão Geral e Arquitetura de Renderização

No cliente oficial do Ragnarok Online, muitas habilidades mágicas e marciais não utilizam apenas os arquivos de animação `.str` (que são sequências de quads 2.5D com keyframes de escala, rotação e alfa). As habilidades mais icônicas e dinâmicas — como a explosão de anéis do *Magnum Break*, as colunas de luz divina do *Curar*, o impacto concêntrico do *Bash*, as flechas e lanças cadentes de *Lanças de Fogo/Gelo* e os anéis de *Aumentar Agilidade* — são **geometrias procedurais geradas e animadas diretamente pelo código da GPU**.

No roBrowser (`EffectTable.js`), cada efeito possui uma lista de componentes (*parts*), que se dividem nos seguintes tipos primitivos:

### Tipos de Primitivas Procedurais

1. **`CYLINDER` (Tubo / Cone Cilíndrico 3D):**
   - Geometria cilíndrica ou cônica aberta verticalmente (com ou sem tampas), com `bottomSize` (raio inferior), `topSize` (raio superior) e `height` (altura em células do mundo).
   - As coordenadas UV são mapeadas ao redor do anel cilíndrico e animadas ao longo do tempo (`animation`), gerando expansão radial ou rolagem vertical.
   - Suporta rotação contínua em torno do eixo vertical (`rotate: true`) e esmaecimento de alfa (`fade: true`).
2. **`3D` (Partículas e Projéteis Orientados no Espaço 3D):**
   - Billboards ou quads livres no espaço tridimensional com interpolação de posição (`posxStart/End`, `posyStart/End`, `poszStart/End`), trajetória do conjurador ao alvo (`toSrc: true`) ou em arco (`arc`, `retreat`).
   - Escala dinâmica (`sizeStart` até `sizeEnd`), suporte a cópias duplicadas com atraso (`duplicate`, `timeBetweenDupli`), e rotação apontada para o vetor de deslocamento (`rotateToTarget: true`).
3. **`2D` (Flashes de Tela e Anéis em Billboard 2D):**
   - Elementos em plano de tela ou orientados à câmera para efeitos de corte e reflexo (como o flash de acerto `EF_HIT` com `lens1.tga` e `lens2.tga` distribuídos em leque radial).
4. **`SPR` (Sprites Clássicos com Controle Procedural):**
   - Sprites `.spr` orientados no mundo ou no topo do ator, controlados por temporizadores e loops matemáticos de frame.
5. **`FUNC` (Gatilhos de Sistema e Câmera):**
   - Funções auxiliares acionadas no início do efeito (como `Camera.setQuake` de 50 ms para o tremor de terra do *Magnum Break*).

---

## 2. Catálogo de Receitas dos Efeitos da 1ª Classe

Abaixo constam as receitas exatas, parâmetros numéricos e arquivos de recursos para cada efeito procedural da 1ª classe.

---

### Efeito 1: `EF_HIT` — Flash de Acerto de Dano / Lâmina

Usado por: `SM_BASH` (hit), `MG_NAPALMBEAT` (hit), `MG_SOULSTRIKE` (hit), `AC_DOUBLE` (hit), `AC_SHOWER` (hit), `AL_RUWACH` (hit).

- **Tipo:** `2D` (8 raios de luz em leque circular ao redor do ponto de impacto).
- **Texturas:** `data/texture/effect/lens1.tga` (raios ímpares) e `data/texture/effect/lens2.tga` (raios pares).
- **Blend Mode:** Aditivo (`Translucent / Additive`).
- **Duração:** 250 ms (variação aleatória `durationRand: [200, 350]`).
- **Dimensões:**
  - `sizeRandStartX`: `[25, 40]` pixels, `sizeRandStartY`: `[10, 10]` pixels.
  - `sizeEndX`: 1 pixel, `sizeRandEndY`: `[250, 300]` pixels (o feixe afina e estica longitudinalmente).
- **Padrão Circular (`circlePattern`):**
  - Raio interno: `2.2` células.
  - Raio externo: `[5, 6]` células.
  - 8 ângulos distribuídos nos quadrantes: `0°..35°`, `50°..85°`, `100°..135°`, `150°..185°`, `200°..235°`, `255°..290°`, `300°..335°`, `340°..360°`.
- **Alfa:** `alphaMax: 12.0` (brilho superintenso no primeiro frame), com decaimento exponencial (`fadeOut: true`).
- **Áudio:** `data/wav/effect/ef_hit2.wav`.
- **Ancoragem:** `attachedEntity: false` (fica no ponto onde o golpe conectou).

---

### Efeito 11: `EF_ENDURE` — Vigor (Endure)

Usado por: `SM_ENDURE` (ao ativar).

- **Tipo:** `3D` (Billboard flutuante acima do personagem).
- **Textura:** `data/texture/effect/endure.tga` (símbolo dourado do escudo com asas).
- **Blend Mode:** Aditivo / Máscara suave.
- **Duração:** 1000 ms.
- **Posição Z:** `posz: 2.0` células (cerca de 2 m acima do solo, logo acima da cabeça).
- **Escala:**
  - Início (`sizeStart`): `200` unidades.
  - Fim (`sizeEnd`): `70` unidades (`sizeSmooth: true` — contrai suavemente em direção ao núcleo).
- **Alfa:** `fadeIn: true` nos primeiros 200 ms, sustentação em `alphaMax: 1.0`, `fadeOut: true` nos últimos 300 ms.
- **Áudio:** `data/wav/effect/ef_endure.wav`.
- **Ancoragem:** `attachedEntity: true` (acompanha o personagem se ele andar durante o efeito).

---

### Efeito 15: `EF_SOULSTRIKE` — Projéteis de Espíritos Anciões

Usado por: `MG_SOULSTRIKE` (ao conjurar / disparar).

- **Parte 1 (Esfera Espectral Central):**
  - **Tipo:** `3D`
  - **Textura:** `data/texture/effect/pok3.tga`.
  - **Cor RGB:** Branco-azulado (`red: 1.0, green: 1.0, blue: 1.0`).
  - **Duração:** 200 ms (com atraso `delayLate: 250 ms`).
  - **Tamanho:** `size: 50`.
  - **Trajetória:** `toSrc: true` (viaja do conjurador em direção ao alvo).
  - **Posição Inicial Z:** Sorteio `poszStartRand: 5`, `poszStartRandMiddle: 6` (surge no alto).
  - **Posição Final Z:** `poszEnd: 1.0`.
  - **Áudio:** `data/wav/effect/ef_soulstrike.wav`.
- **Parte 2 (Enxame de Espíritos em Curva):**
  - **Tipo:** `3D`
  - **Sprite:** `data/sprite/이팩트/particle1.spr` (esfera brilhante com cauda translúcida).
  - **Quantidade:** `duplicate: 5` cópias consecutivas (`timeBetweenDupli: 20 ms`).
  - **Duração:** 250 ms por partícula.
  - **Comportamento Balístico:** `soulStrikePattern: true`, `arc: 4.0` (arco parabólico alto), `retreat: 3.0` (recua brevemente antes de acelerar), `rotateToTarget: true`.
  - **Escala:** `sizeStart: 100.0` crescendo para `sizeEnd: 500.0`.

---

### Efeito 16: `EF_BASH` — Carga / Impacto do Golpe Fulminante

Usado por: `SM_BASH` (início do ataque), `AC_DOUBLE` (puxar da corda).

- **Composto por 3 Cilindros Concéntricos:**
  - **Camada 1 (Cone Inferior Central):**
    - **Tipo:** `CYLINDER`
    - **Textura:** `data/texture/effect/alpha_down.tga`.
    - **Dimensões:** `bottomSize: 0.1`, `topSize: 2.0`, `height: 0.0` (disco plano horizontal).
    - **Orientação:** `angleX: -90°`, `posZ: 1.5`.
    - **Duração:** 1000 ms, `rotate: true`, `fade: true`, `alphaMax: 0.6`, `animation: 2`.
    - **Áudio:** `data/wav/effect/ef_bash.wav`.
  - **Camada 2 (Anéis Secundários em Leque):**
    - **Tipo:** `CYLINDER`
    - **Textura:** `data/texture/effect/alpha_center.tga`.
    - **Dimensões:** `bottomSize: 0.01`, `topSize: 2.5`, `height: 0.0`.
    - **Quantidade:** `duplicate: 10` cópias rotacionadas em leque (`angleZRandom: 360°`).
    - **Duração:** 1000 ms, `alphaMax: 0.6`, `fade: true`.
  - **Camada 3 (Onda Expansiva Externa):**
    - **Tipo:** `CYLINDER`
    - **Textura:** `data/texture/effect/alpha_center.tga`.
    - **Dimensões:** `bottomSize: 0.01`, `topSize: 4.0`, `height: 0.0`.
    - **Quantidade:** `duplicate: 8` cópias rotacionadas.
    - **Duração:** 1000 ms, `alphaMax: 0.6`, `fade: true`.
- **Ancoragem:** `attachedEntity: true`.

---

### Efeito 17: `EF_MAGNUMBREAK` — Impacto Explosivo

Usado por: `SM_MAGNUM` (ao bater no chão).

- **Parte 1 (Anel Expansivo de Fogo Amarelo):**
  - **Tipo:** `CYLINDER`
  - **Textura:** `data/texture/effect/ring_yellow.tga`.
  - **Blend Mode:** Aditivo (`BlendMode: 2`).
  - **Dimensões:**
    - Raio Inferior (`bottomSize`): `4.0` células (começa largo no chão).
    - Raio Superior (`topSize`): `6.0` células (expande para fora como um cone invertido).
    - Altura (`height`): `1.0` célula.
  - **Duração:** 300 ms.
  - **Animação UV:** `animation: 4` (deslocamento radial rápido da textura).
  - **Rotação:** `rotate: true`.
  - **Alfa:** `alphaMax: 0.7` com `fade: true`.
- **Parte 2 (Cúpula da Grande Explosão):**
  - **Tipo:** `CYLINDER`
  - **Textura:** `data/texture/effect/대폭발.tga` (textura de explosão em bola de fogo coreana clássica).
  - **Dimensões:**
    - Raio Inferior (`bottomSize`): `4.0` células.
    - Raio Superior (`topSize`): `1.0` célula (fecha em domo superior).
    - Altura (`height`): `4.0` células (ergue-se a 4 metros de altura).
  - **Duração:** 300 ms.
  - **Animação UV:** `animation: 4`.
  - **Rotação:** `rotate: true`.
  - **Alfa:** `alphaMax: 0.6`, `fade: true`.
  - **Áudio:** `data/wav/effect/ef_magnumbreak.wav`.
- **Parte 3 (Tremor de Tela):**
  - **Tipo:** `FUNC` (`quake_magnum`).
  - **Comportamento:** Aciona `Camera.setQuake(startTick, duration=50 ms)` com amplitude de ~8–12 pixels, fazendo toda a tela tremer no momento do baque.
- **Ancoragem:** `attachedEntity: true` (centrado nos pés do Espadachim).

---

### Efeito 18: `EF_STEAL` — Furto (Steal)

Usado por: `TF_STEAL` (ao ter sucesso).

- **Tipo:** `2D` (Ponto de exclamação vermelho / corte ágil).
- **Textura:** `data/texture/effect/exclamation.tga` ou quad vermelho.
- **Cor RGB:** Vermelho puro (`red: 1.0, green: 0.0, blue: 0.0`).
- **Duração:** 500 ms.
- **Alfa:** `alphaMax: 1.0`, com `fadeOut: true`.
- **Áudio:** `data/wav/effect/ef_steal.wav`.

---

### Efeito 21: `EF_DETOXIFY` — Desintoxicação

Usado por: `TF_DETOXIFY` (ao purificar).

- **Tipo:** `3D` (Efervescência de bolhas verdes purificadoras).
- **Textura:** `data/texture/effect/pok1.tga`.
- **Cor RGB:** Verde-claro (`red: 0.2, green: 1.0, blue: 0.2`).
- **Duração:** 1000 ms.
- **Quantidade:** `duplicate: 10` bolhas subindo do chão ao peito.
- **Tamanho:** `size: 100`.
- **Alfa:** `alphaMax: 1.0`, `fadeOut: true`.
- **Áudio:** `data/wav/effect/ef_detoxication.wav`.

---

### Efeito 24: `EF_FIREBALL` — Projétil da Bola de Fogo

Usado por: `MG_FIREBALL` (durante o voo).

- **Tipo:** `3D` (Esfera flamejante veloz).
- **Textura:** Partícula incandescente `data/texture/effect/pok2.tga`.
- **Cor RGB:** Laranja avermelhado (`red: 1.0, green: 0.4, blue: 0.1`).
- **Duração:** 250 ms.
- **Tamanho:** `size: 200`.
- **Quantidade:** `duplicate: 5` cópias em esteira de fogo.
- **Trajetória:** `toSrc: true` (em linha reta até o alvo).
- **Áudio:** `data/wav/effect/ef_fireball.wav`.

---

### Efeito 27: `EF_FROSTDIVER` — Onda Rasteira de Gelo

Usado por: `MG_FROSTDIVER` (ao liberar a onda).

- **Tipo:** `TRAIL` (Rastro de gelo no solo).
- **Duração:** 2500 ms.
- **Blend Mode:** `8` (Aditivo glacial).
- **Dimensões:** `bottomSize: 0.06`, `height: 0.2`, `animation: 4`.
- **Áudio:** Som rasteiro de congelamento contínuo.

---

### Efeito 37: `EF_INCAGI` — Aumentar Agilidade

Usado por: `AL_INCAGI` (ao aplicar no alvo).

- **Composto por 4 Partes (Espiral de Luz + Ícone Flutuante):**
  - **Partes 1, 2 e 3 (Feixes de Luz Ascendentes em Espiral):**
    - **Tipo:** `3D`
    - **Textura:** `data/texture/effect/ac_center2.tga`.
    - **Cor:** Dourado / Amarelo brilhante.
    - **Duração:** 1000 ms.
    - **Quantidades:** 7, 3 e 10 partículas em anéis de alturas distintas orbitando o alvo.
    - **Alfa:** `alphaMax: 1.0` (com `fadeIn` e `fadeOut`).
  - **Parte 4 (Placa / Ícone Flutuante "AGI UP"):**
    - **Tipo:** `3D` Billboard voltado para a câmera.
    - **Textura:** `data/texture/effect/agi_up.bmp`.
    - **Dimensões:** `sizeStart: 100`, `sizeEnd: 100` (mantém tamanho constante).
    - **Duração:** 1000 ms (flutua suavemente para cima cerca de 1 célula).
    - **Alfa:** `alphaMax: 1.0` com `fadeOut: true`.
    - **Áudio:** `data/wav/effect/ef_incagility.wav`.

---

### Efeito 38: `EF_DECAGI` — Diminuir Agilidade

Usado por: `AL_DECAGI` (ao aplicar no alvo).

- **Partes 1 (Partículas Pesadas Caindo):**
  - **Tipo:** `3D`
  - **Textura:** `data/texture/effect/ac_center2.tga`.
  - **Cor:** Cinza-escuro / Púrpura lamacento.
  - **Duração:** 1000 ms.
  - **Quantidade:** `duplicate: 20` caindo do topo para o solo.
- **Parte 2 (Ícone Flutuante "SLOW"):**
  - **Tipo:** `3D` Billboard.
  - **Textura:** `data/texture/effect/slow.bmp`.
  - **Tamanho:** `100`.
  - **Duração:** 1000 ms.
  - **Áudio:** `data/wav/effect/ef_decagility.wav`.

---

### Efeito 39: `EF_AQUA` — Aqua Benedicta

Usado por: `AL_HOLYWATER`.

- **Tipo:** `SPR` (Gotícula sagrada ascendente com respingos de água abençoada).
- **Sprite:** `data/sprite/이팩트/성수뜨기.spr`.
- **Áudio:** `data/wav/effect/ef_aqua.wav`.

---

### Efeito 42: `EF_BLESSING` — Bênção (Blessing)

Usado por: `AL_BLESSING`.

- **Parte 1 (Cruz Latina Sagrada e Auréola):**
  - **Tipo:** `SPR`
  - **Sprite:** `data/sprite/이팩트/축복.spr`.
  - **Duração:** 1500 ms (animação da cruz dourada sobre a cabeça).
  - **Repetição:** `repeat: true`.
- **Partes 2 e 3 (Centelhas Douradas Ascendentes):**
  - **Tipo:** `3D`
  - **Quantidade:** 2 grupos de 6 partículas subindo ao redor do corpo.
  - **Duração:** 1200 ms, `size: 50`.
- **Parte 4 (Coluna de Luz Suave):**
  - **Tipo:** `3D`
  - **Textura:** `data/texture/effect/pok2.tga`.
  - **Blend Mode:** `2` (Aditivo).
  - **Tamanho:** `size: 140`.
  - **Duração:** 2500 ms, `alphaMax: 0.3`.
  - **Áudio:** `data/wav/effect/ef_blessing.wav`.

---

### Efeito 51: `EF_COLDBOLT_HIT` — Estilhaço de Gelo das Lanças de Gelo

Usado por: `MG_COLDBOLT` (em cada golpe que atinge o alvo).

- **Parte 1 (Som do Impacto):**
  - **Áudio:** `data/wav/effect/_hit_fist1.wav` a `_hit_fist3.wav`.
- **Parte 2 (Estilhaço Glacial 2D):**
  - **Tipo:** `2D` Billboard.
  - **Textura:** `data/texture/effect/ice.tga`.
  - **Duração:** 140 ms (muito rápido, como um estalo).
  - **Tamanho:** `size: 100`.
  - **Alfa:** `alphaMax: 1.0` com `fadeOut: true`.

---

### Efeito 308: `EF_THROWSTONE` — Projétil de Arremessar Pedra

Usado por: `TF_THROWSTONE` (ao lançar a pedra).

- **Tipo:** `3D` (Pedra giratória voando).
- **Textura:** `data/texture/유저인터페이스/item/돌.bmp`.
- **Duração:** 200 ms.
- **Tamanho:** `size: 20`.
- **Rotação:** `rotate: true` (gira violentamente no próprio eixo em direção ao alvo).
- **Trajetória:** `toSrc: true`.

---

### Efeito 309: `EF_FIRSTAID` — Primeiros Socorros

Usado por: `NV_FIRSTAID` (ao curar 5 HP).

- **Tipo:** `2D`
- **Textura:** `data/texture/effect/pikapika2.bmp` (brilho verde cintilante).
- **Blend Mode:** `2` (Aditivo).
- **Duração:** 1000 ms.
- **Tamanho:** `size: 100`.
- **Alfa:** `alphaMax: 0.2` (brilho sutil e transparente).
- **Áudio:** `data/wav/effect/_heal_effect.wav`.

---

### Efeitos 312 e 320: `EF_HEAL` — Curar (Conjurador & Alvo)

Usado por: `AL_HEAL` (312 no conjurador, 320 no alvo curado).

- **Camada 1 (Cilindro Interno de Luz Sagrada):**
  - **Tipo:** `CYLINDER`
  - **Textura:** `data/texture/effect/ring_white.tga`.
  - **Blend Mode:** `2` (Aditivo).
  - **Dimensões:**
    - `bottomSize`: `0.95`, `topSize`: `0.95` (tubo cilíndrico reto perfeito, raio ~1 m).
    - `height`: `8.0` células no efeito 312, `10.0` células no efeito 320 (ergue-se a 8–10 metros para o céu).
  - **Duração:** 1500 ms (312) / 1000 ms (320).
  - **Animação UV:** `animation: 1` (fluxo vertical ascendente contínuo da luz branca).
  - **Rotação:** `rotate: true`.
  - **Alfa:** `alphaMax: 0.2` (luz diáfana suave e transparente).
  - **Áudio:** `data/wav/effect/_heal_effect.wav`.
- **Camada 2 (Cilindro Externo de Contorno):**
  - **Tipo:** `CYLINDER`
  - **Textura:** `data/texture/effect/ring_white.tga`.
  - **Blend Mode:** `2`.
  - **Dimensões:** `bottomSize: 1.0`, `topSize: 1.0`, `height: 8.0` / `9.0`.
  - **Duração:** 1500 ms / 1000 ms, `alphaMax: 0.2`.
- **Camadas 3 e 4 (Chuva de Centelhas Sagradas Ascendentes):**
  - **Tipo:** `3D`
  - **Textura:** `data/texture/effect/pok3.tga` (pontos brilhantes de luz concentrada).
  - **Blend Mode:** `2`.
  - **Quantidade:** `duplicate: 15` e `duplicate: 7` partículas subindo do solo em intervalos de 10 a 50 ms.
  - **Dimensões:** `size: 9.0`.
  - **Duração:** 1100 a 1300 ms, com `fadeIn: true` e `fadeOut: true`.

---

### Efeito `ef_coldbolt` (`2001`): Lanças de Gelo (Projétil Cadente)

Usado por: `MG_COLDBOLT` (cada lança que cai do céu antes de atingir o alvo).

- **Camada 1 (Estalactite de Gelo em Queda Oblíqua):**
  - **Tipo:** `3D`
  - **Textura:** `data/texture/effect/icearrow.tga`.
  - **Ângulo:** `angle: 112.5°` (desce em diagonal clássica do céu em direção à cabeça do alvo).
  - **Trajetória Z:** Começa em `poszStart: 20` (20 metros no ar) e atinge o chão/alvo em `poszEnd: 0`.
  - **Duração:** 500 ms.
  - **Tamanho:** `size: 50`.
  - **Variação de Origem:** `posxStartRandMiddle: 5`, `posyStartRandMiddle: 2` (as lanças caem de pontos ligeiramente diferentes do céu em ataques de múltiplos golpes).
  - **Áudio:** `data/wav/effect/ef_icearrow1.wav` a `ef_icearrow3.wav`.
- **Camada 2 (Anel de Choque Glacial no Solo):**
  - **Tipo:** `CYLINDER`
  - **Textura:** `data/texture/effect/ring_blue.tga`.
  - **Dimensões:** `bottomSize: 3.0`, `topSize: 5.0`, `height: 0.1` (disco raso expansivo no chão).
  - **Atraso:** `delayLate: 500 ms` (surge exatamente no momento em que a lança atinge o solo).
  - **Duração:** 1000 ms, `rotate: true`, `animation: 4`, `alphaMax: 0.7`.

---

### Efeito `ef_firebolt` (`2002`): Lanças de Fogo (Projétil Cadente)

Usado por: `MG_FIREBOLT` (cada lança de fogo que cai do céu).

- **Tipo:** `3D` (Lança animada de chamas).
- **Sequência de Texturas:**
  - `data/texture/effect/불화살1.tga`
  - `data/texture/effect/불화살2.tga`
  - `data/texture/effect/불화살3.tga`
  - `data/texture/effect/불화살4.tga`
  - `data/texture/effect/불화살5.tga`
  - `data/texture/effect/불화살6.tga`
  - Taxa de animação: `frameDelay: 30 ms` por quadro de fogo.
- **Blend Mode:** `2` (Aditivo).
- **Ângulo:** `angle: 112.5°` (queda oblíqua a partir de `poszStart: 20` até `poszEnd: 0`).
- **Dimensões:** `sizeX: 100`, `sizeY: 50`.
- **Duração:** 500 ms.
- **Áudio:** `data/wav/effect/ef_firearrow1.wav` a `ef_firearrow3.wav`.

---

### Efeito `ef_arrow_projectile` (`2003`): Flecha Balística em Voo

Usado por: `AC_DOUBLE` (Rajada de Flechas), `AC_CHARGEARROW` (Disparo Certeiro).

- **Tipo:** `3D`
- **Sprite:** `data/sprite/npc/skel_archer_arrow.spr` (flecha de madeira com penas brancas).
- **Duração:** 140 ms (voo supersônico da flecha).
- **Tamanho:** `size: 100`.
- **Trajetória:** `toSrc: true`, `rotateToTarget: true` (a ponta da flecha aponta com exatidão vetorial para o centro de massa do alvo).
- **Alfa:** `fadeIn: true`, `fadeOut: true`.

---

### Efeito `ef_arrow_shower_projectile` (`2004`): Chuva de Flechas

Usado por: `AC_SHOWER`.

- **Tipo:** `3D`
- **Sprite:** `data/sprite/npc/skel_archer_arrow.spr`.
- **Quantidade:** `duplicate: 10` flechas disparadas simultaneamente (`timeBetweenDupli: 0 ms`).
- **Espalhamento:** `posxEndRand: 1.5`, `posyEndRand: 1.5` células ao redor da célula-alvo no chão.
- **Duração:** 140 ms por flecha.
- **Trajetória:** `toSrc: true`, `rotateToTarget: true`.

---

## 3. Catálogo de Receitas dos Efeitos da 2ª Classe (2-1 e 2-2)

Abaixo constam as receitas exatas, parâmetros numéricos e arquivos de recursos para cada efeito procedural e híbrido das classes 2-1 e 2-2.

---

### Efeito 226: `EF_GRANDCROSS` — Crux Magnum (Grand Cross)

Usado por: `CR_GRANDCROSS` (habilidade assinatura do Cruzado).

- **Arquitetura Geométrica:** Malha em Cruz Sagrada 9x9 células composta por 25 cilindros cônicos aditivos (`CYLINDER`), sendo 1 núcleo central e 4 braços de 4 células cada (Norte, Sul, Leste, Oeste).
- **Textura do Anel:** `data/texture/effect/ring_red.tga` (brilho sacro avermelhado/dourado).
- **Blend Mode:** `2` (Aditivo).
- **Topologia de Cada Cilindro:**
  - `totalCircleSides: 4`, `circleSides: 4` (cilindro de seção quadrática suave).
  - `angleY: 45°` (orientação dos vértices alinhada à grade isométrica do mapa).
  - `topSize: 0.7`, `bottomSize: 0.7` (coluna de luz reta com 0.7 células de diâmetro).
  - `height: 5.0` células (colunas de luz estendendo-se por 5 metros acima do solo).
- **Animação e Pulso:**
  - `duration: 1000 ms`.
  - `duplicate: 3` pulsos repetidos (`timeBetweenDupli: 500 ms`), cobrindo o período total do Crux.
  - `animation: 1` (deslocamento vertical contínuo das coordenadas UV da textura).
  - `alphaMax: 0.1` por cilindro (a sobreposição óptica nos 3 pulsos atinge opacidade intensa).
  - `fade: true` (esmaecimento suave ao término de cada pulso).
- **Disposição Espacial dos 25 Cilindros:**
  - Centro: `(posX: 0, posY: 0)`.
  - Braço Oeste: `posX = -1, -2, -3, -4; posY = 0`.
  - Braço Leste: `posX = +1, +2, +3, +4; posY = 0`.
  - Braço Norte: `posX = 0; posY = -1, -2, -3, -4`.
  - Braço Sul: `posX = 0; posY = +1, +2, +3, +4`.
- **Áudio:** `data/wav/effect/grand_cross.wav` (som grave de trovoada sagrada e coro celestial).

---

### Efeito 328 / 266: `EF_ASURA` — Punho Supremo de Asura (Extremity Fist)

Usado por: `MO_EXTREMITYFIST` (habilidade final do Monge).

- **Camada 1 (Colunas de Vácuo Espiritual no Conjurador):**
  - **Tipo:** `CYLINDER` (2 cilindros concêntricos brancos).
  - **Textura:** `data/texture/effect/ring_white.tga`.
  - **Blend Mode:** Aditivo (`blendMode: 2`).
  - **Dimensões:** `topSize: 0.5`, `bottomSize: 0.5`, `height: 2.0` células.
  - **Duração:** 1000 ms, `rotate: true`, `alphaMax: 0.4`, `fade: true`.
- **Camada 2 (Pinceladas em Kanji dos 6 Caracteres de Asura):**
  - **Tipo:** `3D` (12 billboards no espaço tridimensional ao redor do monge).
  - **Texturas dos Caracteres:**
    - `data/texture/effect/asura1.tga` (阿 - Caractere 'A')
    - `data/texture/effect/asura2.tga` (修 - Caractere 'Shu')
    - `data/texture/effect/asura3.tga` (羅 - Caractere 'Ra')
    - `data/texture/effect/asura4.tga` (覇 - Caractere 'Ha')
    - `data/texture/effect/asura5.tga` (凰 - Caractere 'Ou')
    - `data/texture/effect/asura6.tga` (拳 - Caractere 'Ken')
  - **Comportamento Dinâmico:**
    - Cada caractere surge flutuando ao redor do punho (`posx: 1.2`, `posz: 4.0`).
    - Escala inicial contraindo para núcleo: `sizeStart: 250` diminuindo suavemente para `sizeEnd: 120` (`sizeSmooth: true`).
    - Duração: 1200 ms.
    - `duplicate: 3` réplicas consecutivas (`timeBetweenDupli: 150 ms`, `alphaMaxDelta: -0.25`).
    - `fadeIn: true` nos primeiros frames, gerando o efeito clássico de caligrafia em tinta chinesa surgindo do éter.
- **Camada 3 (Efeito 266 no Alvo — Impacto Supremo e Tremor):**
  - **Flash de Impacto:** `3D` com `data/texture/effect/lens1.tga`, escala supersônica de 300% com decaimento exponencial de brilho (`alphaMax: 8.0`).
  - **Tremor de Câmera (`FUNC` / `quake`):** `Camera.setQuake(150 ms)` com forte oscilação vertical.
  - **Áudio:** `data/wav/effect/asura_strike.wav` (detonação titânica de impacto).

---

### Efeito 336: `EF_GUARD` — Bloqueio com Escudo (Auto Guard)

Usado por: `CR_AUTOGUARD` (quando o Cruzado bloqueia um ataque).

- **Tipo:** 3 `CYLINDER` côncavos aninhados verticalmente, simulando a curvatura de um escudo oval de luz.
- **Textura:** `data/texture/effect/guardk.tga` (brasão sagrado do escudo dos cavaleiros).
- **Blend Mode:** Aditivo (`blendMode: 2`).
- **Cor:** Dourado esverdeado sacro (`red: 0.91, green: 1.0, blue: 0.9`).
- **Geometria Cilíndrica:**
  - `totalCircleSides: 8`, `circleSides: 5` (arco semicircular de 225° na frente do personagem).
  - `angleY: 112.5°` (centraliza o escudo diretamente à frente do ator).
  - `repeatTextureX: 5` (repete o padrão da cruz ao longo da curvatura).
  - Anel Superior: `bottomSize: 1.5, topSize: 1.0, height: 0.7, posZ: 2.14, alphaMax: 0.6`.
  - Anel Central: `bottomSize: 1.5, topSize: 1.5, height: 1.14, posZ: 1.00, alphaMax: 0.6`.
  - Anel Inferior: `bottomSize: 1.0, topSize: 1.5, height: 0.7, posZ: 0.30, alphaMax: 0.6`.
- **Duração:** 600 ms, `fade: true`.
- **Orientação:** `rotateWithSource: true` (se o defensor virar de direção, o escudo translúcido gira junto).
- **Áudio:** `data/wav/effect/kyrie_guard.wav` (som metálico cristalino de bloqueio divino).

---

### Efeito 222: `EF_DEFENDER` — Aura Sagrada (Defender)

Usado por: `CR_DEFENDER`.

- **Tipo:** `CYLINDER` protetor esférico/tubular.
- **Textura:** `data/texture/effect/ring_black.tga`.
- **Dimensões:** `bottomSize: 1.2`, `topSize: 1.2`, `height: 2.0` células.
- **Duração:** 3000 ms contínuos com rotação lenta (`rotate: true`).
- **Alfa:** `alphaMax: 0.4`, `fade: true`.
- **Áudio:** `data/wav/effect/defender.wav`.

---

### Efeito 252: `EF_REFLECTSHIELD` — Escudo Refletor

Usado por: `CR_REFLECTSHIELD`.

- **Tipo:** `CYLINDER` energético dourado.
- **Textura:** `data/texture/effect/ring_yellow.tga`.
- **Dimensões:** `bottomSize: 1.2`, `topSize: 1.2`, `height: 2.0` células.
- **Duração:** 1500 ms, `rotate: true`, `alphaMax: 0.5`.
- **Áudio:** `data/wav/effect/reflect_shield.wav`.

---

### Efeito 253: `EF_ABSORBSPIRITS` — Absorver Esferas Espirituais

Usado por: `MO_ABSORBSPIRITS`.

- **Camada 1 (Cilindros de Vácuo Azul):**
  - **Tipo:** 3 `CYLINDER` concêntricos.
  - **Textura:** `data/texture/effect/ring_blue.tga`.
  - **Cor RGB:** Azul puro (`red: 0.3, green: 0.3, blue: 1.0`).
  - **Dimensões:** Alturas telescópicas `height: 15.0 / 13.0 / 2.0`, `bottomSize: 1.1 / 1.0 / 1.1`, `topSize: 1.1 / 1.0 / 3.0`.
  - **Duração:** 1500 ms, `rotate: true`, `fade: true`.
- **Camada 2 (Partículas de Qi Espiritual Ascendente):**
  - **Tipo:** `3D`
  - **Textura:** `data/texture/effect/pok3.tga`.
  - **Contingente:** 3 grupos de partículas (`duplicate: 4`, `duplicate: 20`, `duplicate: 10`) com atrasos escalonados (`delayOffset: 400 ms`, `delayLate: 200 ms`).
  - **Comportamento:** Surgem no chão (`poszStart: 0`) e sobem espiraladas em direção ao peito do monge (`poszEnd: 6..8`).
- **Áudio:** `data/wav/effect/흡기.wav`.

---

### Efeito 261: `EF_EXPLOSIONSPIRITS` — Fúria Interior & Invocar Esferas

Usado por: `MO_EXPLOSIONSPIRITS` (ao entrar em fúria) e `MO_CALLSPIRITS` (ao conjurar esfera).

- **Tipo:** `CYLINDER` de vórtice dourado.
- **Textura:** `data/texture/effect/ring_yellow.tga`.
- **Dimensões:** `bottomSize: 1.2`, `topSize: 1.2`, `height: 10.0` células (pilar de fogo espiritual).
- **Duração:** 1500 ms, `rotate: true`, `alphaMax: 0.5`, `animation: 2`.
- **Efeito Secundário:** `FUNC` (`quake`: leve tremor de solo no instante da detonação interior).
- **Áudio:** `data/wav/effect/explosion_spirits.wav`.

---

### Efeito 319: `EF_BOTTOM_SANC` — Santuário (Sanctuary)

Usado por: `PR_SANCTUARY` (efeito persistente no chão).

- **Camada 1 (Marcador Hexagonal Sagrado no Piso):**
  - **Tipo:** `CYLINDER` (disco raso no solo).
  - **Textura:** `data/texture/effect/magic_green.tga`.
  - **Cor:** Verde esmeralda divino (`red: 0.5, green: 0.9, blue: 0.5`).
  - **Topologia:** `totalCircleSides: 4`, `circleSides: 4`, `angleY: 45°`, `bottomSize: 0.7`, `topSize: 0.7`, `height: 2.0`.
  - **Duração:** 50.000 ms (permanece enquanto durar a zona no servidor).
  - **Alfa:** `alphaMax: 0.3`, `fade: false`.
- **Camada 2 (Pulsos Ascendentes de Cura):**
  - **Tipo:** `CYLINDER` em loop contínuo (`repeat: true`).
  - **Duração por Ciclo:** 2000 ms, `fade: true`, `animation: 1` (rolagem vertical de luz).
- **Áudio:** `data/wav/effect/sanctuary.wav`.

---

### Efeito 318: `EF_BOTTOM_MAG` — Magnus Exorcismus

Usado por: `PR_MAGNUS` (zona santificada no chão).

- **Camada 1 (Marcador Sagrado no Piso):**
  - **Tipo:** `CYLINDER` (disco de solo).
  - **Textura:** `data/texture/effect/ring_red.tga` (vermelho-dourado sacro).
  - **Dimensões:** `totalCircleSides: 4`, `circleSides: 4`, `angleY: 45°`, `bottomSize: 0.7`, `topSize: 0.7`, `height: 5.0`.
  - **Duração:** 50.000 ms, `alphaMax: 0.2`.
- **Camada 2 (Ondas Contínuas de Luz Purificadora):**
  - **Tipo:** `CYLINDER` em loop (`repeat: true`, `duration: 2000 ms`).
  - **Dimensões:** `height: 5.0`, `fade: true`, `animation: 1`.
- **Áudio:** `data/wav/effect/magnus_exorcismus.wav`.

---

### Efeito 138: `EF_FIREPILLAR` — Coluna de Fogo (Fire Pillar)

Usado por: `WZ_FIREPILLAR` (cilindros de chamas sobrepostos).

- **Tipo:** 3 `CYLINDER` concêntricos em telescópio ascendente.
- **Textura:** `data/texture/effect/magic_red.tga`.
- **Dimensões dos Estágios:**
  - Estágio 1 (Base): `bottomSize: 1.0, topSize: 2.0, height: 3.0`.
  - Estágio 2 (Meio): `bottomSize: 0.7, topSize: 1.5, height: 5.0`.
  - Estágio 3 (Topo): `bottomSize: 0.5, topSize: 1.0, height: 7.0`.
- **Duração:** 5000 ms por coluna, com rotação contínua (`rotate: true`) e atraso inicial `delayOffset: 1000 ms`.
- **Áudio:** `data/wav/effect/firepillar.wav`.

---

### Efeito 93 / 94: `EF_JUPITEL` — Trovão de Júpiter (Jupitel Thunder)

Usado por: `WZ_JUPITEL` (esfera concentrada de plasma elétrico).

- **Efeito 93 (Esfera em Trânsito Balístico Conjurador → Alvo):**
  - **Núcleo Central:** `3D` com `data/texture/effect/thunder_center.bmp`, `size: 35`, `blendMode: 2` (Aditivo), `toSrc: true`, `duration: 200 ms`.
  - **Camada de Relâmpagos Externos:** `3D` com sequência animada de 6 quadros:
    `thunder_ball_a.bmp` a `thunder_ball_f.bmp`, `size: 45`, `frameDelay: 10 ms`, `toSrc: true`.
  - **Áudio de Voo:** `data/wav/effect/hunter_shockwavetrap.wav`.
- **Efeito 94 (Impacto e Estouro do Plasma no Alvo):**
  - **Tipo:** `3D` com `data/texture/effect/thunder_pang.bmp`, expansão radial instantânea de 150 unidades e decaimento em 200 ms + recuo físico do alvo no servidor.

---

### Efeito 116 / 117: `EF_WATERBALL` — Esferas d'Água (Waterball)

Usado por: `WZ_WATERBALL`.

- **Efeito 116 (Gêiseres de Sucção no Ponto de Água do Conjurador):**
  - **Tipo:** `3D` sequencial com textura animada `water_out_a.bmp` a `water_out_c.bmp`.
  - **Dimensões e Voo:** `posyStart: 0, posyEnd: 3.0, posxRand: 1.5, poszRand: 1.5`, `size: 30.5`, `duration: 500 ms`.
  - **Áudio:** `data/wav/effect/wizard_waterball_chulung.wav`.
- **Efeito 117 (Projétil de Água Atingindo o Alvo):**
  - **Tipo:** `3D` com sprite `data/sprite/이팩트/waterball.spr`, voo rápido em direção ao peito do alvo.

---

### Efeito 121 / 143: `EF_SONICBLOW` — Lâminas Destruidoras (Sonic Blow)

Usado por: `AS_SONICBLOW`.

- **Efeito 121 (Aura de Velocidade Supersônica no Mercenário):**
  - **Tipo:** `3D` com `data/texture/effect/ring2.bmp` expandindo horizontalmente no plano dos pés do atacante.
- **Efeito 143 (Animação de Cortes Multiplos):**
  - **Tipo:** STR híbrido (`sonicblow.str`) combinado com flashes concêntricos aditivos.
- **Efeito 122 (Impactos Sequenciais no Alvo):**
  - Flashes procedurais de corte com decaimento de 150 ms para cada golpe conectado.
- **Áudio:** `data/wav/effect/sonicblow.wav`.

---

### Efeitos de Projéteis Balísticos da 2ª Classe

1. **`ef_spear_projectile` (`2006`) — Lança Bumerangue (`KN_SPEARBOOMERANG`):**
   - **Tipo:** `3D`
   - **Sprite:** `data/sprite/npc/창.spr` (lança de aço giratória).
   - **Cinemática:** `toSrc: true`, `rotateToTarget: true`, `size: 100`, `duration: 140 ms`, `fadeIn: true`, `fadeOut: true`.
   - **Áudio:** `data/wav/effect/spear_boomerang.wav`.
2. **`ef_shield_projectile` (`2008`) — Escudo Bumerangue (`CR_SHIELDBOOMERANG`):**
   - **Tipo:** `3D`
   - **Textura:** `data/texture/effect/shield_boomerang.bmp`.
   - **Cinemática:** `toSrc: true`, `rotate: true`, `nbOfRotation: 1` (gira 360° em voo), `size: 50`, `duration: 140 ms`.
   - **Áudio:** `data/wav/effect/cru_shield.wav`.
3. **`298` — Frasco de Ácido (`AM_ACIDTERROR`):**
   - **Tipo:** `3D`
   - **Textura:** `data/texture/유저인터페이스/item/염산병.bmp`.
   - **Cinemática:** `toSrc: true`, `rotate: true`, `nbOfRotation: 0.5`, `size: 30`, `duration: 300 ms`.
   - **Áudio:** `data/wav/effect/acid_terror.wav`.
4. **`600` — Faca Envenenada (`AS_VENOMKNIFE`):**
   - **Tipo:** `3D`
   - **Textura:** `data/texture/유저인터페이스/item/베넘나이프.bmp`.
   - **Cinemática:** `toSrc: true`, `rotateToTarget: true`, `size: 30`, `duration: 200 ms`.
   - **Áudio:** `data/wav/effect/throw_knife.wav`.
5. **`274` — Chuva de Moedas de Afanar (`RG_STEALCOIN`):**
   - **Tipo:** `2D`
   - **Textura:** `data/texture/effect/coin_a.bmp`.
   - **Cinemática:** Moedas de ouro saltando verticalmente para cima da vítima com rotação planar e gravidade procedurais.

---

### Efeitos 277..294: Músicas e Danças de Bardo e Odalisca

Usado por: Habilidades de solo de Bardo (`BA_` / `BD_`) e Odalisca (`DC_` / `BD_`).

- **Mecânica Procedural:**
  - O executável gera instâncias de partículas 3D de notas musicais flutuantes (semínimas, colcheias, clave de sol) subindo suavemente ao redor dos artistas.
  - No solo, desenha anéis circulares pulsantes (`CYLINDER` com raio de 7x7 ou 9x9 células) com cores temáticas por melodia.
- **Faixas e Efeitos de Áudio Associados:**
  - `278`: Canção de Ninar (`effect/자장가.wav`)
  - `279`: Banquete de Njord (`effect/김서방돈.wav`)
  - `280`: Ritmo Caótico (`effect/영원한혼돈.wav`)
  - `281`: Ode a Hela (`effect/전장의북소ri.wav`)
  - `282`: Anel dos Nibelungos (`effect/니벨룽겐의반지.wav`)
  - `283`: Lamento de Loki (`effect/로키의통곡.wav`)
  - `284`: Cantiga de Ninar (`effect/심연속으로.wav`)
  - `285`: Sinfonia dos Ventos (`effect/지크프리드.wav`)
  - `286`: Assovio (`effect/휘파람.wav`)
  - `287`: Crepúsculo Sangrento (`effect/석양의암살자.wav`)
  - `288`: Poema de Bragi (`effect/브라기의시.wav`)
  - `289`: Maçãs de Idun (`effect/이둔의 사과.wav`)
  - `291`: Sibilo (`effect/흥얼거림.wav`)
  - `292`: Não me Abandones (`effect/나를잊지말아요.wav`)
  - `293`: Beijo da Sorte (`effect/행운의키스.wav`)
  - `294`: Dança Cigana (`effect/당신을위한서비스.wav`)

---

## 4. Guia de Implementação na Unreal Engine (Claude)


Para implementar os efeitos procedurais no cliente (`MythSkillEffects.cpp` / novo componente de efeitos procedurais):

1. **`CYLINDER` via Malha Procedural / Dynamic Mesh:**
   - Construir uma malha tubular/cônica com ~24 a 32 segmentos circulares.
   - Posições dos vértices inferiores em `(cos(θ) * bottomSize, sin(θ) * bottomSize, 0)`.
   - Posições dos vértices superiores em `(cos(θ) * topSize, sin(θ) * topSize, height * 100)`.
   - Material mascarado / aditivo (`BlendMode: Additive`) com textura `ring_*.tga`, `alpha_*.tga` ou `대폭발.tga`.
   - Parâmetro escalar no material para translação de UV (conforme `animation: 4` ou `1`) e escala de opacidade (`alphaMax * Fade`).
2. **`3D` Billboards e Projéteis:**
   - Criar um ator leve (`AMythProceduralEffect`) com `UStaticMeshComponent` (plano billboard voltado para a câmera ou orientado ao vetor velocidade `(TargetPos - StartPos).Rotation()`).
   - Interpolação linear da posição `FMath::Lerp(Start, Target, Alpha)` ao longo de `Duration`.
3. **`FUNC` (Quake / Tremor):**
   - No caso de `quake_magnum`, disparar `APlayerCameraManager::StartCameraShake` ou aplicar um offset aleatório senoidal de ~10 unidades na câmera durante 50 ms.

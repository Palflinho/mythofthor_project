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

## 3. Guia de Implementação na Unreal Engine (Claude)

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

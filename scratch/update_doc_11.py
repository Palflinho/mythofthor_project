import os

doc_path = "docs/ref/11_efeitos_procedurais.md"
with open(doc_path, "r", encoding="utf-8") as f:
    content = f.read()

# Marker where section 3 used to start
split_marker = "## 3. Guia de Implementação na Unreal Engine (Claude)"
if split_marker not in content:
    print("Marker not found!")
    exit(1)

parts = content.split(split_marker)
part1 = parts[0]

section_2nd_class = """## 3. Catálogo de Receitas dos Efeitos da 2ª Classe (2-1 e 2-2)

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
"""

final_text = part1 + section_2nd_class + parts[1]
with open(doc_path, "w", encoding="utf-8") as f:
    f.write(final_text)

print(f"Salvo {doc_path} com sucesso!")

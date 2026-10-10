import os

doc_path = "docs/ref/11_efeitos_procedurais.md"
with open(doc_path, "r", encoding="utf-8") as f:
    content = f.read()

marker = "## 4. Guia de Implementação na Unreal Engine (Claude)"
if marker not in content:
    print("Marker not found!")
    exit(1)

parts = content.split(marker)
part1 = parts[0]

section_trans = """## 4. Catálogo de Receitas dos Efeitos de Transclasse

Abaixo constam as receitas exatas, parâmetros numéricos e arquivos de recursos para cada efeito procedural e híbrido das Transclasses.

---

### Efeito 365: `EF_PRESSURE` — Glória Domini (Pressure)

Usado por: `PA_PRESSURE` (habilidade suprema do Paladino).

- **Tipo:** `CYLINDER` celestial colossal em queda perpendicular rápida.
- **Textura:** `data/texture/effect/ring_white.tga` com tonalidade dourada divina (`red: 1.0, green: 0.95, blue: 0.7`).
- **Blend Mode:** `2` (Aditivo).
- **Dimensões e Trajetória:**
  - `topSize: 2.0`, `bottomSize: 2.0`, `height: 30.0` células (um imenso feixe de luz que desce das nuvens).
  - Posição inicial: `poszStart: 35.0` descendo velozmente até `poszEnd: 0.0`.
  - Cruz monumental de luz estampando o solo no ponto de queda.
- **Duração:** 1200 ms, com esmaecimento nos últimos 300 ms (`fadeOut: true`).
- **Gatilho de Impacto:** `Camera.setQuake(200 ms)` (forte abalo sísmico ao tocar a terra).
- **Áudio:** `data/wav/effect/pressure.wav` (trovoada sagrada esmagadora).

---

### Efeito 368: `EF_BERSERK` — Frenesi (Berserk / Frenzy)

Usado por: `LK_BERSERK` e `LK_FURY` (estado de fúria cega do Lorde).

- **Camada 1 (Coluna de Fúria Sanguinária):**
  - **Tipo:** `CYLINDER`
  - **Textura:** `data/texture/effect/ring_red.tga` (vermelho carmesim vívido).
  - **Dimensões:** `bottomSize: 1.2`, `topSize: 1.4`, `height: 8.0` células.
  - **Duração:** 2000 ms, `rotate: true`, `alphaMax: 0.6`, `animation: 2` (chamas ascendentes de sangue).
- **Camada 2 (Pulsos Sísmicos):**
  - Tremor rítmico leve de solo durante a ativação.
- **Áudio:** `data/wav/effect/berserk.wav` (rugido ensurdecedor de batalha).

---

### Efeito 339: `EF_SPIRALPIERCE` — Perfurar em Espiral (Spiral Pierce)

Usado por: `LK_SPIRALPIERCE`.

- **Tipo:** Híbrido `CYLINDER` e partículas cônicas 3D em rotação espiral ultrarrápida.
- **Textura:** `data/texture/effect/thunder_center.bmp` / `ring_white.tga`.
- **Topologia:** Cones perfurantes giratórios estendendo-se do conjurador em direção à vítima.
- **Ciclo de Acerto:** 5 ondas consecutivas de dano em intervalo de 60 ms.
- **Áudio:** `data/wav/effect/spiral_pierce.wav` seguido por `_hit_spear.wav`.

---

### Efeito 370: `EF_GOSPEL` — Canto de Batalha (Gospel)

Usado por: `PA_GOSPEL` (zona santa 9x9 células).

- **Camada de Solo (`370_ground`):**
  - **Tipo:** `CYLINDER` de raio amplo (`bottomSize: 4.5, topSize: 4.5, height: 1.0`).
  - **Textura:** `data/texture/effect/ring_yellow.tga` (auréola dourada ampla).
  - **Duração:** 60.000 ms, `rotate: true`, `alphaMax: 0.35`.
- **Camada de Bênção Ascendente:**
  - Colunas verticais finas de luz dourada surgindo periodicamente dentro da área.
- **Áudio:** `data/wav/effect/gospel.wav` (coro gregoriano e canto litúrgico).

---

### Efeito 374: `EF_BASILICA` — Basílica (Basilica)

Usado por: `HP_BASILICA`.

- **Tipo:** `CYLINDER` abobadado de proteção total 9x9 células.
- **Textura:** `data/texture/effect/magic_green.tga` / `ring_white.tga`.
- **Dimensões:** `bottomSize: 4.5`, `topSize: 4.0`, `height: 4.5` células (cúpula transparente inviolável).
- **Duração:** 30.000 ms contínuos.
- **Áudio:** `data/wav/effect/basilica.wav`.

---

### Efeito 440: `EF_ASSUMPTIO` — Assumptio

Usado por: `HP_ASSUMPTIO`.

- **Tipo:** `3D` (Escudo sagrado alado flutuante no topo da cabeça do aliado).
- **Textura:** `data/texture/effect/endure.tga` matizado em dourado divino / `guardk.tga`.
- **Dimensões:** `sizeStart: 120, sizeEnd: 80` com halo de centelhas `pok3.tga`.
- **Duração:** 1500 ms na aplicação (com estado visual contínuo leve).
- **Áudio:** `data/wav/effect/assumptio.wav`.

---

### Efeito 493: `EF_EDP` — Encantar com Veneno Mortal (Enchant Deadly Poison)

Usado por: `ASC_EDP`.

- **Tipo:** `CYLINDER` vórtice violáceo nas mãos e armas do Algoz.
- **Textura:** `data/texture/effect/ring_black.tga` com tonalidade roxo-venenoso (`red: 0.7, green: 0.1, blue: 0.9`).
- **Dimensões:** `bottomSize: 0.8, topSize: 0.8, height: 2.0`, `rotate: true`, `duration: 2000 ms`.
- **Áudio:** `data/wav/effect/edp.wav` (gás tóxico corrosivo borbulhante).

---

### Efeito 537: `EF_ACIDDEMONSTRATION` — Bomba Ácida (Acid Demonstration)

Usado por: `CR_ACIDDEMONSTRATION` (ataque supremo do Criador).

- **Cinemática:** Arremesso de 2 projéteis simultâneos em arco cruzado:
  - Projétil 1: Frasco de Ácido (`염산병.bmp`).
  - Projétil 2: Frasco de Fogo Grego (`화염병.bmp`).
- **Impacto no Alvo:**
  - Explosão química mista com cilindro aditivo de chamas (`대폭발.tga`) + névoa ácida e tremor de tela.
- **Duração:** 400 ms.
- **Áudio:** `data/wav/effect/acid_terror.wav` combinado com explosão de fogo.

---

### Efeito 518: `EF_CARTTERMINATION` — Choque de Carrinho (Cart Termination)

Usado por: `WS_CARTTERMINATION`.

- **Tipo:** `3D` e `2D` (Impacto esmagador frontal).
- **Elementos Visuais:**
  - Projeção veloz do carrinho com roda fumegante.
  - No ponto de impacto: explosão de faíscas metálicas com `lens1.tga` e `ef_hit2` em escala quadruplicada.
- **Tremor:** `quake: 80 ms`.
- **Áudio:** `data/wav/effect/carttermination.wav`.

---

### Efeito 387: `EF_FALCONASSAULT` — Ataque Aéreo (Falcon Assault)

Usado por: `SN_FALCONASSAULT`.

- **Tipo:** `3D` (Voo supersônico em rasante).
- **Sprite:** `data/sprite/npc/falcon.spr`.
- **Cinemática:** O falcão parte da posição superior do atirador, atinge a presa em alta velocidade com estilhaço de penas e garras, e retorna ao ar.
- **Áudio:** `data/wav/effect/falconassault.wav`.

---

## 5. Guia de Implementação na Unreal Engine (Claude)
"""

final_text = part1 + section_trans + parts[1]
with open(doc_path, "w", encoding="utf-8") as f:
    f.write(final_text)

print(f"Salvo {doc_path} com sucesso!")

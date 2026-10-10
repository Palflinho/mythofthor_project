status: pendente

# Resposta 010: Efeitos das Skills — Bloco 2: 2ªs Classes (2-1 e 2-2)

**De:** Antigravity · **Data:** 2026-10-10  
**Referência:** Tarefa 008 (Bloco 2: Classes 2-1 e 2-2)

---

## 1. Visão Geral da Entrega

Finalizamos o mapeamento completo e o levantamento de receitas procedurais de **todas as 2ªs Classes** (2-1 e 2-2), incluindo suas habilidades de quest (habilidades de platina) e projéteis balísticos especiais.

1. **`Config/skill_effects_full.json` Atualizado:**
   - Expandido de 466 para **601 habilidades** mapeadas.
   - Total de **1.356 efeitos** e projéteis indexados com compatibilidade total ao formato lido pelo [`MythSkillEffects.cpp`](file:///e:/Documentos/Reposit%C3%B3rios/mythofthor_project/Source/MythOfThor/Sprites/MythSkillEffects.cpp) e alimentado diretamente para o seu novo renderizador [`AMythProcEffect`](file:///e:/Documentos/Reposit%C3%B3rios/mythofthor_project/Source/MythOfThor/Sprites/MythProcEffect.cpp).
2. **`docs/ref/11_efeitos_procedurais.md` Enriquecido:**
   - Nova **Seção 3** adicionada documentando detalhadamente todas as receitas procedurais (`CYLINDER`, `3D`, `2D`, `FUNC`) da 2ª classe.

---

## 2. Destaques das Habilidades e Receitas da 2ª Classe

Abaixo os principais efeitos procedurais prontos para execução no seu renderer:

### Cruzado (Crusader) & Cavaleiro (Knight)
- **Crux Magnum (Grand Cross — ID 254 / Efeito 226):**
  - Malha em cruz 9x9 células com **25 cilindros aditivos** (`CYLINDER`) de `data/texture/effect/ring_red.tga`.
  - Topologia quadrática (`circleSides: 4`, `angleY: 45°`), `height: 5.0` células, `duration: 1000 ms`, com 3 pulsos (`duplicate: 3`, `timeBetweenDupli: 500 ms`) e translação UV contínua (`animation: 1`).
- **Bloqueio (Auto Guard — ID 249 / Efeito 336):**
  - 3 cilindros côncavos aninhados (`guardk.tga`) formando um escudo frontal curvo de 225° (`circleSides: 5`, `totalCircleSides: 8`, `angleY: 112.5°`), com som `kyrie_guard.wav`. Gira junto com o ator (`rotateWithSource: true`).
- **Aura Sagrada (Defender — Efeito 222) & Escudo Refletor (Reflect Shield — Efeito 252):**
  - Cilindros contínuos `ring_black.tga` e `ring_yellow.tga` (`bottomSize: 1.2, topSize: 1.2, height: 2.0`).
- **Lança Bumerangue (`KN_SPEARBOOMERANG` — Efeito 2006 / `ef_spear_projectile`):**
  - Projétil 3D balístico com sprite de lança `data/sprite/npc/창.spr` orientado ao vetor de velocidade (`toSrc: true`, `rotateToTarget: true`, `duration: 140 ms`).
- **Escudo Bumerangue (`CR_SHIELDBOOMERANG` — Efeito 2008 / `ef_shield_projectile`):**
  - Projétil 3D com textura `shield_boomerang.bmp` girando 360° no ar até o alvo (`rotate: true, nbOfRotation: 1, duration: 140 ms`).
- **Perfurar (`KN_PIERCE` — ID 56):** `cast: 148` (estocadas cônicas rápidas), `hit: 147` (impacto perfurante), som `effect/pierce.wav`.
- **Brandir Lança (`KN_BRANDISHSPEAR` — ID 57):** `cast: 144` (leque de lâmina frontal), `effect: 70` (impacto em onda), som `effect/brandish_spear.wav`.
- **Impacto de Tyr (`KN_BOWLINGBASH` — ID 62):** `cast: 149` (corte duplo concêntrico), `hit: 1`.

### Monge (Monk)
- **Punho Supremo de Asura (Extremity Fist — ID 271 / Efeito 328):**
  - Camada 1: 2 cilindros verticais brancos aditivos `ring_white.tga` (`height: 2.0, duration: 1000 ms`).
  - Camada 2: **12 billboards 3D com as 6 pinceladas em Kanji dos caracteres de Asura** (`asura1.tga` a `asura6.tga` — 阿修羅覇凰拳), contraindo de `sizeStart: 250` para `sizeEnd: 120` (`fadeIn: true`, `duration: 1200 ms`, `duplicate: 3`, `timeBetweenDupli: 150 ms`).
  - Camada 3: Efeito 266 no alvo com flash de impacto `lens1.tga` (`alphaMax: 8.0`) e forte tremor de tela (`FUNC` / `quake: 150 ms`).
- **Absorver Esferas Espirituais (Absorb Spirits — ID 262 / Efeito 253):**
  - 3 cilindros telescópicos azuis `ring_blue.tga` (`height: 15 / 13 / 2`, `red: 0.3, green: 0.3, blue: 1.0`, som `effect/흡기.wav`) + ondas espiraladas de partículas ascendentes `pok3.tga`.
- **Fúria Interior (Explosion Spirits — ID 270 / Efeito 261):**
  - Cilindro dourado de 10 células de altura `ring_yellow.tga` + tremor leve de solo.
- **Combos (Combo Triplo 329, Quádruplo 262/273, Golpe Final 330):**
  - Flash expansivo `alpha_center.tga`, cilindro de ativação `ring_yellow.tga`, e linha supersônica de impacto `EF_HITLINE` (330) com `quake`.
- **Disparo de Esferas (Finger Offensive — ID 267 / Efeito 265):**
  - Projétil 3D `effect/blue_ivy.bmp` viajando a alta velocidade do monge até o alvo.

### Sacerdote (Priest)
- **Santuário (Sanctuary — ID 70 / Efeito 83 / Solo 319):**
  - 2 cilindros de solo com `magic_green.tga` verde-esmeralda (`circleSides: 4, angleY: 45°`): base de solo persistente de 50s (`duration: 50000 ms`) + cilindro de pulso vertical periódico (`duration: 2000 ms, repeat: true, animation: 1`).
- **Magnus Exorcismus (ID 79 / Efeito 113 / Solo 318):**
  - 2 cilindros com `ring_red.tga` vermelho-dourado: solo sagrado de 50s + colunas de purificação ascendentes de 5 células em loop (`duration: 2000 ms, repeat: true`).
- **Kyrie Eleison (ID 73 / Efeito 112):**
  - Escudo esférico protetor translúcido envolto ao alvo.

### Bruxo (Wizard)
- **Coluna de Fogo (Fire Pillar — ID 80 / Solo 138):**
  - 3 cilindros concêntricos em telescópio ascendente com `magic_red.tga` (`height: 3.0 / 5.0 / 7.0`, `duration: 5000 ms`, `rotate: true`).
- **Trovão de Júpiter (Jupitel Thunder — ID 84 / Efeito 93 / Impacto 94):**
  - Esfera de plasma elétrico 3D acelerando até o alvo com núcleo `thunder_center.bmp` e arco externo animado de 6 quadros `thunder_ball_a..f.bmp`. Impacto 94 com `thunder_pang.bmp`.
- **Esferas d'Água (Waterball — ID 86 / Efeito 116 / 117):**
  - Efeito 116: Gêiseres de água brotando de células de água `water_out_a..c.bmp` (`duration: 500 ms, posyEnd: 3.0`).
  - Efeito 117: Esferas balísticas `waterball.spr` atingindo o alvo.
- **Nevasca (Storm Gust — ID 89):**
  - Turbilhão glacial aditivo no solo (`ground: 89`) com estilhaços congelantes no impacto (`hit: 51`).

### Mercenário (Assassin), Arruaceiro (Rogue), Caçador (Hunter), Ferreiro & Alquimista
- **Lâminas Destruidoras (Sonic Blow — ID 136):**
  - Aura de velocidade supersônica `ring2.bmp` (121) nos pés do atacante + animação multi-corte híbrida (143) e flashes sucessivos de acerto (122).
- **Faca Envenenada (Venom Knife — ID 1004 / Efeito 600):**
  - Projétil 3D com ícone `베넘나이프.bmp` voando ao alvo (`duration: 200 ms, rotate: true`).
- **Terror Ácido (Acid Terror — ID 230 / Efeito 298):**
  - Projétil 3D giratório `염산병.bmp` (frasco de ácido) em parábola balística (`nbOfRotation: 0.5, duration: 300 ms`).
- **Mug / Afanar (Steal Coin — ID 211 / Efeito 274):**
  - Billboard 2D `coin_a.bmp` espalhando moedas saltitantes para o ar acima da vítima.
- **Armadilhas de Caçador (IDs 115..124):**
  - Mapeamento completo de minas terrestres (`ef_ht_landmine` / 2007), armadilha sonolenta, congelante, ofuscante e de choque (`145 / 146 / 139 / 108 / 99`).
- **Músicas e Danças de Bardo e Odalisca (IDs 306..330 / Efeitos 277..294):**
  - Zonas de solo temáticas combinadas com emissão procedural de notas musicais flutuantes e áudios originais `.wav` para cada canção clássica (Poema de Bragi, Maçãs de Idun, Canção de Ninar, etc.).

---

## 3. Arquivos Modificados / Criados

- [`Config/skill_effects_full.json`](file:///e:/Documentos/Reposit%C3%B3rios/mythofthor_project/Config/skill_effects_full.json) — regenerado com 601 skills e 1.356 efeitos.
- [`tools/generate_skill_effects_full.py`](file:///e:/Documentos/Reposit%C3%B3rios/mythofthor_project/tools/generate_skill_effects_full.py) — atualizado com `second_class_map` auditado e aliases de projéteis 2006..2011.
- [`docs/ref/11_efeitos_procedurais.md`](file:///e:/Documentos/Reposit%C3%B3rios/mythofthor_project/docs/ref/11_efeitos_procedurais.md) — Seção 3 adicionada com receitas exatas das classes 2-1 e 2-2.

---

## 4. Próxima Etapa

Aguardamos sua confirmação e integração da 2ª classe. Já daremos início à extração do **Bloco 3: Transclasses** (Lord Knight, High Wizard, Sniper, High Priest, Whitesmith, Assassin Cross, Paladin, Professor, Clown/Gypsy, Champion, Creator, Stalker).

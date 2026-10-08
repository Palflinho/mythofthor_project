# Myth of Thor — Roadmap de Regiões do Mundo (Rune-Midgarts)

Este documento orienta a extração e compilação metódica de todo o mundo do Ragnarok Online para o formato HD-2D da Unreal Engine 5. Cada região segue o pipeline padrão: **Mapas 3D (`.obj`/`.mtl`) + BGM oficial + Tabela de Monstros e Drops**.

---

## 🗺️ Visão Geral das Regiões

```
                       [3. GEFFEN] ── Magos & Orcs
                            │
  [1. PRONTERA] ────────────┼──────────── [2. IZLUDE & BYALAN] ── Espadachins & Mar
  (Capital & Esgotos)       │
                            ├──────────── [4. PAYON] ── Arqueiros & Caverna
                            │
                       [5. MORROC] ── Gatunos & Pirâmides
                            │
                       [6. ALBERTA] ── Mercadores & Navio Fantasma
```

---

## 📍 Região 1: Prontera (Capital & Esgotos) — [STATUS: 100% CONCLUÍDA ✅]
* **Cidade:** `prontera` (`prontera_full.obj` - 12.7 MB). BGM: `08.mp3`.
* **Campos (12 mapas):** `prt_fild00` até `prt_fild11` compilados em 3D. BGM: `12.mp3`.
* **Dungeon dos Esgotos (4 andares):**
  * `prt_sewb1` (Andar 1 - 735 modelos 3D). BGM: `19.mp3`.
  * `prt_sewb2` (Andar 2 - 203 modelos 3D). BGM: `19.mp3`.
  * `prt_sewb3` (Andar 3 - 363 modelos 3D). BGM: `19.mp3`.
  * `prt_sewb4` (Andar 4 - 223 modelos 3D - Toca do MVP). BGM: `19.mp3`.
* **Monstros:** 23 spritesheets PNG transparentes gerados. MVP: **Golden Thief Bug (GTB)**.
* **Configurações:** `Config/prontera_fields_mobs.json` e `Config/prontera_sewers_mobs.json`.

---

## 📍 Região 2: Izlude & Ilha de Byalan — [STATUS: 100% CONCLUÍDA ✅]
* **Tema:** Cidade marítima dos Espadachins e o lendário Túnel Submarino (Byalan).
* **Cidade:** `izlude` (`izlude_full.obj` - 552 modelos 3D compilados). BGM: `26.mp3`.
* **Túnel Submarino de Byalan (5 andares compilados em 3D):**
  * `iz_dun00` (Andar 1 - 241 modelos 3D). BGM: `13.mp3`.
  * `iz_dun01` (Andar 2 - 210 modelos 3D). BGM: `13.mp3`.
  * `iz_dun02` (Andar 3 - 617 modelos 3D). BGM: `13.mp3`.
  * `iz_dun03` (Andar 4 - 446 modelos 3D). BGM: `60.mp3`.
  * `iz_dun04` (Andar 5 - 440 modelos 3D - Fundo do Mar). BGM: `60.mp3`.
* **Monstros:** 11 spritesheets PNG transparentes gerados (Vadon, Kukre, Marina, Cornutus, Hydra, Obeaune, Swordfish, Marc, Strouf, Deviace, Kraken). MVP: **Kraken**.
* **Configuração:** `Config/izlude_byalan_mobs.json`.

---

## 📍 Região 3: Geffen & Vila/Caverna dos Orcs — [STATUS: 100% CONCLUÍDA ✅]
* **Tema:** Magia, misticismo e as tribos orcs (Mid-Game 30-60 ⚔️).
* **Cidade:** `geffen` (Torre dos Magos - `geffen_full.obj`, 634 modelos). BGM: `211.mp3`.
* **Campos:** `gef_fild10` (Vila dos Orcs - `gef_fild10_full.obj`, 1.497 modelos). BGM: `48.mp3`.
* **Dungeons:**
  * Caverna dos Orcs: `orcsdun01` (162 modelos) e `orcsdun02` (109 modelos). BGM: `23.mp3`.
  * Torre de Geffen: `gef_dun00` (143 modelos), `gef_dun01` (373 modelos), `gef_dun02` (395 modelos), `gef_dun03` (273 modelos). BGM: `22.mp3`.
* **Monstros da Região (17 Spritesheets PNG):**
  * Orc Warrior, Orc Lady, Orc Baby, Orc Zombie, Orc Skeleton, Orc Archer, High Orc.
  * Ghoul, Whisper, Jakk, Nightmare, Hunter Fly, Marionette, Deviruchi.
  * MVPs: **Orc Hero**, **Orc Lord** e **Doppelganger**.
* **Configuração:** `Config/geffen_orcs_mobs.json`.

---

## 📍 Região 4: Payon & A Caverna dos Mortos — [STATUS: 100% CONCLUÍDA ✅]
* **Tema:** Florestas orientais, montanhas sagradas e folclore asiático (Oriente & Arqueiros 🌸).
* **Cidades:**
  * `payon` (Vila de Payon - `payon_full.obj`, 1.056 modelos). BGM: `14.mp3`.
  * `pay_arche` (Vila dos Arqueiros - `pay_arche_full.obj`, 244 modelos). BGM: `14.mp3`.
* **Dungeon (Caverna de Payon - 5 andares compilados em 3D):**
  * `pay_dun00` (Andar 1 - 38 modelos 3D). BGM: `20.mp3`.
  * `pay_dun01` (Andar 2 - 97 modelos 3D). BGM: `20.mp3`.
  * `pay_dun02` (Andar 3 - 145 modelos 3D). BGM: `20.mp3`.
  * `pay_dun03` (Andar 4 - 1.091 modelos 3D). BGM: `47.mp3`.
  * `pay_dun04` (Andar 5 - 805 modelos 3D - Covil Sagrado). BGM: `47.mp3`.
* **Monstros da Região (16 Spritesheets PNG):**
  * Zombie, Skeleton, Munak, Bongun, Eggyra, Dokebi, Sohee, Nine Tail, Horong.
  * Smokie, Wolf, Snake, Willow, Elder Willow.
  * MVPs: **Moonlight Flower (Flor do Luar)** e **Eddga**.
* **Configuração:** `Config/payon_mobs.json`.

---

## 📍 Região 5: Morroc & As Pirâmides do Deserto — [STATUS: 100% CONCLUÍDA ✅]
* **Tema:** Oásis do deserto, ruínas egípcias e catacumbas (Gatunos & Assassinos 🏜️).
* **Cidade:** `morocc` (Guilda dos Gatunos - `morocc_full.obj`, 699 modelos). BGM: `119.mp3`.
* **Dungeons (11 andares compilados em 3D):**
  * Pirâmides: `moc_pryd01` (71 modelos), `moc_pryd02` (124 modelos), `moc_pryd03` (24 modelos), `moc_pryd04` (140 modelos - Tumba Real), `moc_pryd05` (1.123 modelos), `moc_pryd06` (553 modelos - Catacumbas). BGM: `22.mp3`.
  * Esfinge: `in_sphinx1` (888 modelos), `in_sphinx2` (1.014 modelos), `in_sphinx3` (488 modelos), `in_sphinx4` (631 modelos), `in_sphinx5` (658 modelos - Câmara Real). BGM: `38.mp3`.
* **Monstros da Região (17 Spritesheets PNG):**
  * Mummy, Ancient Mummy, Isis, Matyr, Verit, Minorous, Pasana, Marduk, Zerom, Requiem.
  * Andre, Deniro, Piere, Maya, Picky.
  * MVPs: **Osíris**, **Pharaoh** e **Maya**.
* **Configuração:** `Config/morroc_mobs.json`.

---

## 📍 Região 6: Alberta & Navio Fantasma — [STATUS: 100% CONCLUÍDA ✅]
* **Tema:** Cidade portuária dos Mercadores, alto mar e tesouros (Comércio & Pirataria ⚓).
* **Cidade:** `alberta` (Guilda dos Mercadores, Porto Marítimo - `alberta_full.obj`, 758 modelos). BGM: `212.mp3`.
* **Dungeons (6 andares compilados em 3D):**
  * Navio Fantasma: `treasure01` (412 modelos) e `treasure02` (484 modelos - Cabine do Drake). BGM: `17.mp3`.
  * Ilha das Tartarugas: `tur_dun01` (624 modelos), `tur_dun02` (627 modelos), `tur_dun03` (622 modelos), `tur_dun04` (589 modelos - Palácio do General). BGM: `65.mp3`.
* **Monstros da Região (12 Spritesheets PNG):**
  * Sailor Skeleton, Mimic, Thara Frog, Megalodon, Spring Rabbit.
  * Exército das Tartarugas: Permeter, Solider, Freezer, Assulter, Heater.
  * MVPs: **Drake** e **Turtle General**.
* **Configuração:** `Config/alberta_mobs.json`.

---

## 🎭 Sprites das Classes Prontos para o Claude (`D:\Ragnarok_Assets\Sprites\PNG`)

Todas as 36 combinações de classes masculinas e femininas foram decodificadas e convertidas em spritesheets com canal alfa transparente:

| Categoria | Classes Masculinas (`_male_sheet.png`) | Classes Femininas (`_female_sheet.png`) |
| :--- | :--- | :--- |
| **Inicial** | `novice_male` (110 frames) | `novice_female` (100 frames) |
| **1-1 Físico** | `swordman_male`, `archer_male`, `thief_male`, `merchant_male` | `swordman_female`, `archer_female`, `thief_female`, `merchant_female` |
| **1-1 Mágico / Suporte** | `mage_male`, `acolyte_male` | `mage_female`, `acolyte_female` |
| **2-1 Classes** | `knight_male`, `wizard_male`, `hunter_male`, `assassin_male`, `priest_male`, `blacksmith_male` | `knight_female`, `wizard_female`, `hunter_female`, `assassin_female`, `priest_female`, `blacksmith_female` |
| **2-2 Classes** | `crusader_male`, `sage_male`, `rogue_male`, `monk_male`, `alchemist_male` | `crusader_female`, `sage_female`, `rogue_female`, `monk_female`, `alchemist_female` |
| **Expandidas** | `super_novice_male`, `taekwon_male`, `star_gladiator_male`, `soul_linker_male`, `ninja_male`, `gunslinger_male` | `super_novice_female`, `taekwon_female`, `star_gladiator_female`, `soul_linker_female`, `ninja_female`, `gunslinger_female` |

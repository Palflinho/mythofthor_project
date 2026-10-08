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

## 📍 Região 3: Geffen & Vila/Caverna dos Orcs (Mid-Game 30-60 ⚔️)
* **Tema:** Magia, misticismo e as tribos orcs.
* **Cidade:** `geffen` (Torre dos Magos). BGM: `211.mp3`.
* **Campos:** `gef_fild10` (Vila dos Orcs). BGM: `48.mp3`.
* **Dungeons:**
  * Caverna dos Orcs: `orcsdun01` e `orcsdun02`. BGM: `23.mp3`.
  * Torre de Geffen: `gef_dun00` a `gef_dun03`. BGM: `22.mp3`.
* **Monstros da Região:**
  * Orc Warrior, Orc Lady, Orc Baby, Orc Zombie, Orc Skeleton.
  * MVPs: **Orc Hero** e **Orc Lord**.

---

## 📍 Região 4: Payon & A Caverna dos Mortos (Oriente & Arqueiros 🌸)
* **Tema:** Florestas orientais, montanhas sagradas e folclore asiático.
* **Cidades:** `payon` e `pay_arche` (Guilda dos Arqueiros). BGM: `14.mp3`.
* **Dungeon:** `pay_dun00` a `pay_dun04` (Caverna de Payon - 5 andares). BGM: `15.mp3`.
* **Monstros da Região:**
  * Zumbis, Esqueletos, Poporing, Munak, Bongun, Eggyra, Dokebi, Sohee, Ninetails.
  * MVP: **Moonlight Flower (Flor do Luar)**.

---

## 📍 Região 5: Morroc & As Pirâmides do Deserto (Gatunos & Assassinos 🏜️)
* **Tema:** Oásis do deserto, ruínas egípcias e catacumbas.
* **Cidade:** `morocc` (Guilda dos Gatunos). BGM: `119.mp3`.
* **Dungeons:**
  * Pirâmides: `moc_pryd01` a `moc_pryd06`. BGM: `18.mp3`.
  * Esfinge: `in_sphinx1` a `in_sphinx5`. BGM: `20.mp3`.
* **Monstros da Região:**
  * Múmia, Isis, Matyr, Verit, Minotauro, Zerom, Pasana.
  * MVPs: **Osíris** e **Faraó**.

---

## 📍 Região 6: Alberta & Navio Fantasma (Comércio & Pirataria ⚓)
* **Tema:** Cidade portuária dos Mercadores, alto mar e tesouros.
* **Cidade:** `alberta` (Guilda dos Mercadores, Porto Marítimo). BGM: `24.mp3`.
* **Dungeons:**
  * Navio Fantasma: `treasure01` e `treasure02`. BGM: `25.mp3`.
  * Ilha das Tartarugas: `tur_dun01` a `tur_dun04`. BGM: `66.mp3`.
* **Monstros da Região:**
  * Pirata Esqueleto, Alma Penada, Mimico, Tartaruga Soldado, Tartaruga Batedora.
  * MVPs: **Drake** e **Turtle General**.

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

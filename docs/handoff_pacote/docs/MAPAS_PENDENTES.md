# Mapas, dungeons e instâncias: o que falta implementar

Gerado por `tools/gen_map_list.py` a partir dos spawns do rAthena (Renovação, `npc/re/mobs`) e do que já está em `Content/Maps`.
Nível = faixa dos monstros do mapa. ✅ = já importado no Unreal. Prioridade sugerida: o que está no caminho de 1 a 99 e da transclasse.

**Resumo:** 134 mapas com monstros, 58 importados (43%).

## Campos (55 mapas, 55 importados)

| Mapa | Nível típico (faixa) | Monstros principais | MVP | Unreal |
|---|---|---|---|---|
| `cmd_fild01` | 1 (1-1) | Blue Plant, Shining Plant | - | ✅ |
| `cmd_fild03` | 1 (1-1) | Shining Plant | - | ✅ |
| `cmd_fild06` | 1 (1-1) | Yellow Plant | - | ✅ |
| `cmd_fild08` | 1 (1-1) | Yellow Plant | - | ✅ |
| `gef_fild00` | 1 (1-1) | Green Plant | - | ✅ |
| `gef_fild01` | 1 (1-1) | Green Plant | - | ✅ |
| `gef_fild02` | 1 (1-1) | Green Plant, Red Mushroom, Blue Plant, Shining Plant | - | ✅ |
| `gef_fild04` | 1 (1-1) | Green Plant | - | ✅ |
| `gef_fild07` | 1 (1-1) | Green Plant | - | ✅ |
| `gef_fild09` | 1 (1-1) | Blue Plant | - | ✅ |
| `hu_fild04` | 1 (1-1) | Blue Plant | - | ✅ |
| `hu_fild06` | 1 (1-1) | Green Plant, Yellow Plant | - | ✅ |
| `mjolnir_01` | 1 (1-1) | Green Plant, Blue Plant, Shining Plant | - | ✅ |
| `mjolnir_02` | 1 (1-1) | Blue Plant | - | ✅ |
| `mjolnir_03` | 1 (1-1) | Shining Plant | - | ✅ |
| `mjolnir_05` | 1 (1-1) | Blue Plant, Shining Plant | - | ✅ |
| `mjolnir_08` | 1 (1-1) | Blue Plant | - | ✅ |
| `mjolnir_10` | 1 (1-1) | Shining Plant | - | ✅ |
| `moc_fild01` | 1 (1-1) | Yellow Plant | - | ✅ |
| `moc_fild03` | 1 (1-1) | Green Plant | - | ✅ |
| `moc_fild07` | 1 (1-1) | Yellow Plant | - | ✅ |
| `moc_fild12` | 1 (1-1) | Yellow Plant | - | ✅ |
| `moc_fild13` | 1 (1-1) | Yellow Plant | - | ✅ |
| `pay_fild02` | 1 (1-1) | Green Plant, Red Mushroom | - | ✅ |
| `pay_fild03` | 1 (1-1) | Red Mushroom, Green Plant | - | ✅ |
| `pay_fild04` | 1 (1-1) | Green Plant, Shining Plant | - | ✅ |
| `pay_fild06` | 1 (1-1) | Green Plant, Red Mushroom, Shining Plant | - | ✅ |
| `pay_fild07` | 1 (1-1) | Green Plant | - | ✅ |
| `pay_fild08` | 1 (1-1) | Shining Plant | - | ✅ |
| `pay_fild09` | 1 (1-1) | Green Plant, Shining Plant | - | ✅ |
| `prt_fild00` | 1 (1-1) | Green Plant | - | ✅ |
| `prt_fild01` | 1 (1-1) | Green Plant, Blue Plant | - | ✅ |
| `prt_fild02` | 1 (1-1) | Shining Plant | - | ✅ |
| `prt_fild03` | 1 (1-1) | Green Plant, Blue Plant | - | ✅ |
| `prt_fild04` | 1 (1-1) | Green Plant | - | ✅ |
| `prt_fild05` | 1 (1-1) | Green Plant, Blue Plant | - | ✅ |
| `prt_fild06` | 1 (1-1) | Green Plant | - | ✅ |
| `prt_fild07` | 1 (1-1) | Black Mushroom | - | ✅ |
| `prt_fild09` | 1 (1-1) | Yellow Plant | - | ✅ |
| `prt_fild10` | 1 (1-1) | Red Mushroom, Shining Plant | - | ✅ |
| `ve_fild03` | 1 (1-1) | Red Plant | - | ✅ |
| `ve_fild04` | 1 (1-1) | Blue Plant | - | ✅ |
| `yuno_fild01` | 1 (1-1) | Yellow Plant, Shining Plant, Blue Plant | - | ✅ |
| `yuno_fild03` | 1 (1-1) | Blue Plant, Shining Plant | - | ✅ |
| `prt_fild08` | 4 (1-6) | Poring, Fabre, Lunatic, Pupa | - | ✅ |
| `gef_fild03` | 50 (1-50) | Green Plant, Orc Hero | Orc Hero | ✅ |
| `gef_fild10` | 55 (1-55) | Blue Plant, Green Plant, Orc Lord | Orc Lord | ✅ |
| `pay_fild10` | 65 (1-65) | Green Plant, Blue Plant, Eddga | Eddga | ✅ |
| `moc_fild17` | 71 (71-71) | Phreeoni | Phreeoni | ✅ |
| `mjolnir_04` | 78 (1-78) | Shining Plant, Mistress | Mistress | ✅ |
| `xmas_fild01` | 98 (98-98) | Hatii | Hatii | ✅ |
| `ra_fild03` | 113 (113-113) | Atroce | Atroce | ✅ |
| `ra_fild04` | 113 (113-113) | Atroce | Atroce | ✅ |
| `ve_fild01` | 113 (1-113) | Blue Plant, Atroce, Shining Plant | Atroce | ✅ |
| `ve_fild02` | 113 (113-113) | Atroce | Atroce | ✅ |

## Dungeons (51 mapas, 3 importados)

| Mapa | Nível típico (faixa) | Monstros principais | MVP | Unreal |
|---|---|---|---|---|
| `gef_dun00` | 1 (1-1) | Black Mushroom | - | ⬜ |
| `glast_01` | 1 (1-1) | Green Plant, Blue Plant, Shining Plant | - | ⬜ |
| `harboro2` | 1 (1-1) | Sea Anemone | - | ⬜ |
| `lou_dun01` | 1 (1-1) | Black Mushroom | - | ⬜ |
| `orcsdun01` | 1 (1-1) | Black Mushroom, White Plant | - | ⬜ |
| `orcsdun02` | 1 (1-1) | White Plant | - | ⬜ |
| `pay_dun00` | 1 (1-1) | Black Mushroom | - | ✅ |
| `pay_dun01` | 1 (1-1) | Black Mushroom | - | ✅ |
| `pay_dun02` | 1 (1-1) | Red Plant, Black Mushroom, White Plant | - | ⬜ |
| `pay_dun03` | 1 (1-1) | White Plant | - | ⬜ |
| `rockmi2` | 1 (1-1) | Purple Ore | - | ⬜ |
| `yggdrasil01` | 1 (1-1) | Shining Plant, Blue Plant, Yellow Plant, White Plant | - | ⬜ |
| `prt_maze01` | 32 (1-75) | Savage, Poring, Coco, Stainer | - | ⬜ |
| `treasure01` | 34 (34-34) | Hydra | - | ⬜ |
| `anthell02` | 55 (55-55) | Maya | Maya | ⬜ |
| `gld_dun02_2` | 55 (55-55) | Maya | Maya | ⬜ |
| `gld_dun01_2` | 65 (65-65) | Eddga | Eddga | ⬜ |
| `prt_maze03` | 65 (1-93) | Mantis, Baphomet Jr., Side Winder, Stem Worm | Baphomet | ⬜ |
| `prt_sewb4` | 65 (65-65) | Golden Thief Bug | Golden Thief Bug | ✅ |
| `moc_pryd04` | 68 (68-68) | Osiris | Osiris | ⬜ |
| `moc_pryd06` | 69 (69-69) | Amon Ra | Amon Ra | ⬜ |
| `gef_dun01` | 75 (1-75) | White Plant, Dracula | Dracula | ⬜ |
| `gef_dun02` | 77 (1-77) | White Plant, Shining Plant, Doppelganger | Doppelganger | ⬜ |
| `gld_dun04` | 77 (77-77) | Doppelganger | Doppelganger | ⬜ |
| `gld_dun02` | 78 (78-78) | Mistress | Mistress | ⬜ |
| `gld_dun01` | 79 (79-79) | Moonlight Flower | Moonlight Flower | ⬜ |
| `pay_dun04` | 79 (1-79) | White Plant, Shining Plant, Moonlight Flower | Moonlight Flower | ⬜ |
| `ayo_dun02` | 80 (80-80) | Lady Tanee | Lady Tanee | ⬜ |
| `gld_dun03` | 81 (81-81) | Baphomet | Baphomet | ⬜ |
| `in_sphinx5` | 85 (85-85) | Pharaoh | Pharaoh | ⬜ |
| `treasure02` | 91 (91-91) | Drake | Drake | ⬜ |
| `xmas_dun02` | 92 (92-92) | Stormy Knight | Stormy Knight | ⬜ |
| `dew_dun01` | 94 (94-94) | Leak | Leak | ⬜ |
| `gl_chyard` | 96 (96-96) | Dark Lord | Dark Lord | ⬜ |
| `gl_chyard_` | 96 (96-96) | Dark Lord | Dark Lord | ⬜ |
| `gld_dun04_2` | 96 (96-96) | Dark Lord | Dark Lord | ⬜ |
| `lou_dun03` | 97 (97-97) | White Lady | White Lady | ⬜ |
| `mosk_dun03` | 97 (97-97) | Gopinich | Gopinich | ⬜ |
| `teg_dun01` | 99 (99-99) | The Last One | The Last One | ⬜ |
| `teg_dun02` | 99 (99-99) | King of the Alley | King of the Alley | ⬜ |
| `ama_dun03` | 100 (100-100) | Samurai Specter | Samurai Specter | ⬜ |
| `ein_dun02` | 100 (100-100) | RSX-0806 | RSX-0806 | ⬜ |
| `gon_dun03` | 105 (105-105) | Evil Snake Lord | Evil Snake Lord | ⬜ |
| `beach_dun` | 110 (110-110) | Tao Gunka | Tao Gunka | ⬜ |
| `tur_dun04` | 110 (110-110) | Turtle General | Turtle General | ⬜ |
| `kh_kiehl01` | 112 (112-115) | Aliot, Alicel, Aliza, Aliot | - | ⬜ |
| `gld_dun03_2` | 113 (113-113) | Atroce | Atroce | ⬜ |
| `iz_dun05` | 124 (124-124) | Kraken | Kraken | ⬜ |
| `kh_dun02` | 125 (125-125) | Kiel D-01 | Kiel D-01 | ⬜ |
| `jupe_core` | 128 (128-128) | Vesper | Vesper | ⬜ |
| `gl_cas02` | 130 (130-130) | Mysteltainn | - | ⬜ |

## Outros (castelos, tesouros, eventos) (4 mapas, 0 importados)

| Mapa | Nível típico (faixa) | Monstros principais | MVP | Unreal |
|---|---|---|---|---|
| `prt_cas` | 1 (1-1) | Plate | - | ⬜ |
| `treasure_n1` | 13 (1-18) | Dead Plankton, Roda Frog, Spore, Red Mushroom | - | ⬜ |
| `treasure_n2` | 18 (9-23) | Weak Skeleton, Weak Soldier Skeleton, Dead Plankton, Zombie | - | ⬜ |
| `brasilis` | 34 (34-34) | Strange Hydra | - | ⬜ |

## Fora do escopo (nível típico acima de 130) (24 mapas, 0 importados)

| Mapa | Nível típico (faixa) | Monstros principais | MVP | Unreal |
|---|---|---|---|---|
| `abyss_03` | 135 (135-135) | Detardeurus | Detardeurus | ⬜ |
| `abbey02` | 138 (138-138) | Falling Bishop | Falling Bishop | ⬜ |
| `ra_san05` | 139 (139-139) | Gloom Under Night | Gloom Under Night | ⬜ |
| `dic_dun02` | 140 (140-140) | Scaraba Queen | Scaraba Queen | ⬜ |
| `dic_dun03` | 140 (140-140) | Gold Queen Scaraba | Gold Queen Scaraba | ⬜ |
| `gld2_prt` | 141 (141-141) | Angry Student Pyuriel | Angry Student Pyuriel | ⬜ |
| `lhz_dun02` | 141 (141-141) | Egnigem Cenia | Egnigem Cenia | ⬜ |
| `odin_tem03` | 141 (141-141) | Valkyrie Randgris | Valkyrie Randgris | ⬜ |
| `gld2_pay` | 142 (142-142) | General Daehyun | General Daehyun | ⬜ |
| `gld2_gef` | 143 (143-143) | Dark Guardian Kades | Dark Guardian Kades | ⬜ |
| `moc_prydn2` | 145 (145-145) | Amon Ra (Nightmare) | Amon Ra (Nightmare) | ⬜ |
| `gld2_ald` | 146 (146-146) | Gioia | Gioia | ⬜ |
| `thor_v03` | 146 (146-146) | Ifrit | Ifrit | ⬜ |
| `abbey03` | 147 (147-147) | Beelzebub | - | ⬜ |
| `verus01` | 150 (150-171) | Green Cenere, Explorer Robot Turbo, Repair Robot Turbo | - | ⬜ |
| `moc_fild22` | 151 (151-151) | Wounded Morocc | Wounded Morocc | ⬜ |
| `un_bunker` | 153 (1-155) | Smelly Zombie, Smelly Ghoul, Can Robot, Machine Component | - | ⬜ |
| `verus03` | 155 (100-168) | Exploration Rover, Repair Robot, Illegal Promotion, Ruin Grace Believer | - | ⬜ |
| `verus02` | 158 (158-171) | Repair Robot Turbo, Explorer Robot Turbo | - | ⬜ |
| `lhz_dun03` | 160 (160-160) | Assassin Cross Eremes, High Wizard Kathryne, Lord Knight Seyren, Sniper Cecil | - | ⬜ |
| `ver_eju` | 165 (163-165) | Recon Robot, Excavator Robot | - | ⬜ |
| `ver_tunn` | 165 (165-165) | Recon Robot | - | ⬜ |
| `ba_lost` | 168 (168-168) | Boss Meow | Boss Meow | ⬜ |
| `c_tower3_` | 170 (170-170) | Time Holder | Time Holder | ⬜ |

## Cidades (npc/warps/cities)

Cada cidade tem o mapa principal e interiores (lojas, guildas, casas); só o mapa principal é conferido aqui.

| Cidade | Unreal |
|---|---|
| `alberta` | ✅ |
| `aldebaran` | ⬜ |
| `amatsu` | ⬜ |
| `ayothaya` | ⬜ |
| `brasilis` | ⬜ |
| `comodo` | ✅ |
| `dewata` | ⬜ |
| `dicastes` | ⬜ |
| `eclage` | ⬜ |
| `einbech` | ⬜ |
| `einbroch` | ⬜ |
| `geffen` | ✅ |
| `gonryun` | ⬜ |
| `hugel` | ⬜ |
| `izlude` | ✅ |
| `lasagna` | ⬜ |
| `lighthalzen` | ⬜ |
| `louyang` | ⬜ |
| `lutie` | ⬜ |
| `malangdo` | ⬜ |
| `malaya` | ⬜ |
| `manuk` | ⬜ |
| `mid_camp` | ⬜ |
| `morroc` | ⬜ |
| `moscovia` | ⬜ |
| `nameless` | ⬜ |
| `niflheim` | ⬜ |
| `payon` | ✅ |
| `prontera` | ✅ |
| `rachel` | ⬜ |
| `rockridge` | ⬜ |
| `splendide` | ⬜ |
| `umbala` | ⬜ |
| `veins` | ⬜ |
| `yggdrasil` | ⬜ |
| `yuno` | ⬜ |

## Instâncias (npc/instances)

Instâncias criam cópias de mapas por grupo; cada uma precisa dos mapas dela importados. Arquivos do rAthena:

- `EndlessTower`: Torre sem Fim (andares sucessivos, a partir do nível 50)
- `NydhoggsNest`: Ninho de Nidhogg (fim de jogo, fora do escopo)
- `OrcsMemory`: Memória dos Orcs (grupo, nível ~50)
- `SealedShrine`: Santuário Selado (grupo, nível alto)

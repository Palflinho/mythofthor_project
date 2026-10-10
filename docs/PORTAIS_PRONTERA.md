# Mapeamento de Portais de Prontera e Interiores (Tarefa B3)

Documento de referência para verificação e integração dos portais (warps) de Prontera no Myth of Thor, baseado nos scripts oficiais do rAthena (`npc/warps/cities/prontera.txt` e `npc/pre-re/warps/cities/prontera.txt`).

---

## 1. Portais da Cidade de Prontera (`prontera.gat`)

Todos os portais que saem diretamente do mapa principal de Prontera (`prontera`), suas coordenadas de origem, tamanho da área de trigger, destino e status de importação no cliente Unreal.

| Nome do Warp | Célula Prontera | Trigger (LxA) | Mapa Destino | Célula Destino | Descrição / Finalidade | Status Cliente Unreal |
|---|---|---|---|---|---|---|
| `prt01` | (107, 215) | 2x2 | `prt_in` | (240, 139) | Loja de Utilidades / Ferramentas (Tool Dealer) | ✅ **Importado** (`Content/Maps/PrtIn/Lvl_PrtIn.umap`) |
| `prt02` | (120, 267) | 2x2 | `prt_in` | (180, 97) | Residência / Biblioteca Noroeste | ✅ **Importado** (`Content/Maps/PrtIn/Lvl_PrtIn.umap`) |
| `prt03` | (133, 183) | 2x2 | `prt_in` | (50, 105) | Loja de Armas (Weapon Shop) | ✅ **Importado** (`Content/Maps/PrtIn/Lvl_PrtIn.umap`) |
| `prt04` | (134, 221) | 1x1 | `prt_in` | (131, 71) | Taberna / Pub de Prontera | ✅ **Importado** (`Content/Maps/PrtIn/Lvl_PrtIn.umap`) |
| `prt05` | (177, 221) | 2x2 | `prt_in` | (168, 128) | Loja de Armaduras (Armor Shop) | ✅ **Importado** (`Content/Maps/PrtIn/Lvl_PrtIn.umap`) |
| `prt06` | (179, 184) | 2x2 | `prt_in` | (60, 73) | Residência (Sudeste Central) | ✅ **Importado** (`Content/Maps/PrtIn/Lvl_PrtIn.umap`) |
| `prt07` | (192, 267) | 2x2 | `prt_in` | (178, 55) | Residência Nordeste | ✅ **Importado** (`Content/Maps/PrtIn/Lvl_PrtIn.umap`) |
| `prt08` | (204, 192) | 2x2 | `prt_in` | (68, 134) | Florista / Loja de Presentes / Residência | ✅ **Importado** (`Content/Maps/PrtIn/Lvl_PrtIn.umap`) |
| `prt09` | (208, 154) | 2x2 | `prt_in` | (172, 29) | Residência Sudeste | ✅ **Importado** (`Content/Maps/PrtIn/Lvl_PrtIn.umap`) |
| `prt10` | (42, 67) | 2x2 | `prt_in` | (44, 29) | Residência Extremo Sudoeste | ✅ **Importado** (`Content/Maps/PrtIn/Lvl_PrtIn.umap`) |
| `prt11` | (45, 346) | 2x2 | `prt_in` | (80, 110) | Ordem dos Cavaleiros (Knight Guild) | ✅ **Importado** (`Content/Maps/PrtIn/Lvl_PrtIn.umap`) |
| `prt12` | (73, 100) | 1x1 | `prt_in` | (208, 176) | Residência Sudoeste | ✅ **Importado** (`Content/Maps/PrtIn/Lvl_PrtIn.umap`) |
| `prt13` | (74, 90) | 2x2 | `prt_in` | (248, 170) | Residência Sudoeste | ✅ **Importado** (`Content/Maps/PrtIn/Lvl_PrtIn.umap`) |
| `prt14-1` | (84, 89) | 1x1 | `prt_in` | (282, 176) | Residência Sudoeste | ✅ **Importado** (`Content/Maps/PrtIn/Lvl_PrtIn.umap`) |
| `prt15-2` | (263, 279) | 2x2 | `prt_in` | (227, 18) | Residência Leste (Próxima à Catedral) | ✅ **Importado** (`Content/Maps/PrtIn/Lvl_PrtIn.umap`) |
| `prt004` | (237, 317) | 2x2 | `prt_church` | (100, 60) | Catedral / Santuário de Prontera | ✅ **Importado** (`Content/Maps/PrtChurch/Lvl_PrtChurch.umap`) |
| `prt005` | (156, 360) | 4x2 | `prt_castle` | (102, 20) | Portão Norte / Castelo Real de Prontera | ✅ **Importado** (`Content/Maps/PrtCastle/Lvl_PrtCastle.umap`) |
| `prt001` | (156, 22) | 3x2 | `prt_fild08` | (170, 375) | Portão Sul (Saída para Campo Sul) | ✅ **Importado** (`Content/Maps/PrtFild08/Lvl_PrtFild08.umap`) |
| `prt002` | (22, 203) | 2x3 | `prt_fild05` | (367, 205) | Portão Oeste (Saída para Campo Oeste) | ✅ **Importado** (`Content/Maps/PrtFild05/Lvl_PrtFild05.umap`) |
| `prt003` | (289, 203) | 2x3 | `prt_fild06` | (27, 193) | Portão Leste (Saída para Campo Leste) | ✅ **Importado** (`Content/Maps/PrtFild06/Lvl_PrtFild06.umap`) |

> **Nota sobre as casas de Prontera:** Todas as casas e estabelecimentos comerciais urbanos de Prontera utilizam o mapa compartilhado de interiores `prt_in.gat`. Como `Lvl_PrtIn.umap` já está completamente importado no projeto, **100% das casas de Prontera que possuem portal no servidor já possuem mapa importado no Unreal Engine.**

---

## 2. Portais de Retorno dos Interiores (`prt_in.gat` -> `prontera.gat`)

Portais de saída localizados dentro de `prt_in` que transportam o jogador de volta para as ruas de Prontera:

| Nome do Warp | Célula Origem (`prt_in`) | Trigger | Célula Destino (`prontera`) | Estabelecimento / Local de Retorno |
|---|---|---|---|---|
| `prt01-1` | (240, 141) | 3x1 | (107, 218) | Saída da Loja de Ferramentas |
| `prt02-1` | (183, 97) | 1x2 | (120, 264) | Saída da Residência / Biblioteca |
| `prt03-1` | (53, 105) | 1x2 | (136, 186) | Saída da Loja de Armas |
| `prt04-1` | (135, 71) | 1x2 | (136, 219) | Saída da Taberna / Pub |
| `prt05-1` | (168, 124) | 2x1 | (174, 218) | Saída da Loja de Armaduras |
| `prt06-1` | (60, 77) | 2x1 | (175, 188) | Saída da Residência Sudeste |
| `prt07-1` | (181, 55) | 1x2 | (192, 264) | Saída da Residência Nordeste |
| `prt08-1` | (68, 130) | 2x1 | (204, 188) | Saída da Florista / Presentes |
| `prt09-1` | (172, 33) | 2x1 | (205, 157) | Saída da Residência Sudeste |
| `prt10-1` | (47, 29) | 2x1 | (46, 67) | Saída da Residência Extremo Sudoeste |
| `prt11-1` | (80, 113) | 2x1 | (48, 343) | Saída da Ordem dos Cavaleiros |
| `prt12-1` | (208, 179) | 3x1 | (76, 102) | Saída da Residência Sudoeste |
| `prt13-1` | (248, 173) | 3x2 | (77, 93) | Saída da Residência Sudoeste |
| `prt14` | (282, 179) | 3x1 | (87, 91) | Saída da Residência Sudoeste |
| `prt20-2` | (227, 15) | 2x2 | (263, 275) | Saída da Residência Próxima à Catedral |

---

## 3. Portais de Transição entre Cômodos Internos (`prt_in.gat`)

Portais que ligam um cômodo a outro dentro do próprio `prt_in`:

| Nome dos Warps (Ida / Volta) | Célula A | Célula B | Tipo do Cômodo |
|---|---|---|---|
| `prt15` / `prt15-1` | (217, 163) [1x3] | (236, 163) [1x3] | Passagem entre cômodos residenciais |
| `prt16` / `prt16-1` | (254, 113) [3x2] | (256, 134) [2x2] | Passagem para quarto dos fundos |
| `prt17` / `prt17-1` | (263, 163) [1x3] | (276, 163) [1x3] | Passagem entre cômodos residenciais |
| `prt18` / `prt18-1` | (37, 65) [1x3] | (51, 65) [1x3] | Passagem interna |
| `prt19a` / `prt19b` | (69, 65) [1x3] | (84, 65) [1x3] | Passagem interna |
| `prt20` / `prt20-1` | (70, 143) [2x1] | (70, 165) [2x1] | Passagem para andar/sala superior |

---

## 4. Portais da Catedral (`prt_church.gat`)

| Nome do Warp | Origem | Trigger | Destino | Descrição | Status Cliente |
|---|---|---|---|---|---|
| `prtch001` | `prt_church (100, 56)` | 7x1 | `prontera (234, 314)` | Retorno à cidade | ✅ Importado (`Lvl_Prontera.umap`) |
| `prtch01` / `prtch01-1` | `prt_church (109, 81)` | 1x2 | `prt_church (172, 19)` | Sala dos Padres / Quarto lateral | ✅ Importado (`Lvl_PrtChurch.umap`) |
| `prtch02` / `prtch02-1` | `prt_church (90, 81)` | 1x2 | `prt_church (27, 19)` | Ordem dos Noviços / Acolyte Room | ✅ Importado (`Lvl_PrtChurch.umap`) |

---

## 5. Portais do Castelo de Prontera (`prt_castle.gat`)

| Nome do Warp | Origem | Trigger | Destino | Descrição | Status Cliente |
|---|---|---|---|---|---|
| `prtca001` | `prt_castle (102, 16)` | 6x2 | `prontera (156, 356)` | Retorno a Prontera Norte | ✅ Importado (`Lvl_Prontera.umap`) |
| `prtca01` a `prtca12` | `prt_castle` | diversos | `prt_castle` | 12 passagens internas entre alas e salas | ✅ Importado (`Lvl_PrtCastle.umap`) |
| `prtca002` | `prt_castle (102, 181)` | 6x2 | **`prt_gld (159, 28)`** | **Reino das Valquírias (Castelos de WoE)** | ❌ **NÃO IMPORTADO** (`prt_gld.umap` não existe) |

---

## 6. Outros Mapas e Acessos Relacionados

1. **Esgotos de Prontera (`prt_sewb1` a `prt_sewb4`):**
   - No rAthena pre-re, a entrada dos Esgotos não é um portal de warp aberto direto em Prontera. O acesso é feito pelo **Guarda dos Esgotos** (`Recruit`) localizado em `prt_fild05 (270, 211)`.
   - **Status no cliente:** Os mapas `prt_sewb1` a `prt_sewb4` ainda **não estão importados** no Unreal Engine.
2. **Reino das Valquírias / Feudos (`prt_gld`):**
   - Conectado ao norte de `prt_castle` (portal `prtca002`) e ao sul de `prt_fild01` (portal `prtf001`).
   - **Status no cliente:** **Não importado**.
3. **Izlude (`izlude`):**
   - Conectado a leste de `prt_fild08` através do portal `prtf005` em `prt_fild08 (371, 212)` para `izlude (35, 78)`.
   - **Status no cliente:** O mapa `izlude.umap` está na fila da Fase 1 de importação.

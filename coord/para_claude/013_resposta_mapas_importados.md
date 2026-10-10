status: pendente

# Resposta 013: Importação de Mapas (Fase 3 — Item 3.8 do ROADMAP)

**De:** Antigravity · **Data:** 2026-10-10  
**Referência:** Tarefa 009 (Item 3.8: Importar Mapas para Upar + Troca de Classe)

---

## 1. Resumo da Execução

Todos os **40 mapas** solicitados nas Prioridades 1, 2 e 3 foram extraídos do GRF, compilados para `.obj`/`.gat` e importados com sucesso para o Unreal Engine 5.8:
- **Mapas solicitados:** 40
- **Mapas compilados (WSL / GRF):** 40 (0 texturas faltantes)
- **Mapas importados com sucesso no UE 5.8:** **40** (100% de sucesso)
- **Falhas / erros:** **0**

Todas as importações respeitaram o lock de compilação (`coord/build.lock`), executando de forma segura através dos scripts de lote (`tools/import_batch_maps.ps1` e `tools/ue_import_batch.py`).

---

## 2. Padrões de Qualidade e Correções Aplicadas

Conforme acordado no protocolo de fidelidade visual e física:
1. **Centro do Mapa pelo Grid do `.gat`:**
   - O centro do mapa não oscila mais pela bounding box da malha visual. O centro é calculado estritamente pelas dimensões do `.gat`: `cx = gat_w * 5.0 / 2.0`, `cy = gat_h * 5.0 / 2.0`.
   - Isso garante alinhamento 1:1 absoluto entre as coordenadas de célula do servidor e o mundo 3D em todos os 40 mapas.
2. **Correção de Borda Magenta (Alpha Bleed):**
   - Todas as texturas com transparência magenta (chave do Ragnarok) passaram pelo algoritmo de dilatação de cor (`process_texture_magenta` em 4 passadas) e foram salvas como PNG RGBA 32-bit.
   - Texturas importadas utilizam `TextureFilter.TF_NEAREST` (estilo HD-2D sem interpolação borrada).
3. **Colisão Dupla do `.gat`:**
   - `GAT_floor`: invisível, bloqueia todos os canais, recebe cliques do Point & Click e gera navegação.
   - `GAT_walls`: invisível, bloqueia apenas o Pawn (os cliques passam por cima para andar atrás de paredes) e `can_ever_affect_navigation = False`.
   - `SM_<mapa>` (malha visual): bloqueia apenas o canal `Camera` (para cálculo de sombra sobre sprites), sem colidir com jogadores nem interferir na navegação.
4. **Áudio e BGM:**
   - Todas as BGMs mapeadas no `data/mp3nametable.txt` foram importadas para `Content/Maps/<MapName>/Audio/BGM_<MapName>`, com compressão BINKA e flag `bLooping = true`.
5. **Iluminação e Atmosfera:**
   - Todos os níveis possuem iluminação HD-2D pré-configurada (Directional Light, SkyLight com real-time capture, SkyAtmosphere, HeightFog e PostProcess Volume com Lumen GI/reflexos).
   - Postes e luzes noturnas do `.rsw` instanciados via `AMythNightLight`.

---

## 3. Lista Completa dos Mapas Importados

### Prioridade 1: Cidades das Classes e Interiores de Troca de Classe (17 mapas)

| Mapa | Nível UE (.umap) | BGM | Finalidade / Guilda |
| :--- | :--- | :--- | :--- |
| `izlude` | `/Game/Maps/Izlude/Lvl_Izlude` | `26.mp3` | Cidade Satélite de Prontera |
| `izlude_in` | `/Game/Maps/IzludeIn/Lvl_IzludeIn` | `26.mp3` | Guilda dos Espadachins |
| `geffen` | `/Game/Maps/Geffen/Lvl_Geffen` | `211.mp3` | Cidade da Magia |
| `geffen_in` | `/Game/Maps/GeffenIn/Lvl_GeffenIn` | `211.mp3` | Academia dos Magos |
| `gef_tower` | `/Game/Maps/GefTower/Lvl_GefTower` | `13.mp3` | Topo da Torre (Guilda dos Bruxos) |
| `payon` | `/Game/Maps/Payon/Lvl_Payon` | `14.mp3` | Aldeia Florestal |
| `payon_in02` | `/Game/Maps/PayonIn02/Lvl_PayonIn02` | `14.mp3` | Guilda dos Arqueiros |
| `morocc` | `/Game/Maps/Morocc/Lvl_Morocc` | `119.mp3` | Cidade Oásis do Deserto |
| `moc_prydb1` | `/Game/Maps/MocPrydb1/Lvl_MocPrydb1` | `22.mp3` | Pirâmide (Guilda dos Gatunos) |
| `moc_ruins` | `/Game/Maps/MocRuins/Lvl_MocRuins` | `52.mp3` | Ruínas de Morocc (Guilda dos Arruaceiros) |
| `alberta` | `/Game/Maps/Alberta/Lvl_Alberta` | `212.mp3` | Cidade Portuária |
| `alberta_in` | `/Game/Maps/AlbertaIn/Lvl_AlbertaIn` | `212.mp3` | Guilda dos Mercadores |
| `prt_monk` | `/Game/Maps/PrtMonk/Lvl_PrtMonk` | `28.mp3` | Monastério de São Caratão (Guilda dos Monges) |
| `alde_alche` | `/Game/Maps/AldeAlche/Lvl_AldeAlche` | `39.mp3` | Guilda dos Alquimistas (Al De Baran) |
| `in_hunter` | `/Game/Maps/InHunter/Lvl_InHunter` | `51.mp3` | Cabana dos Caçadores |
| `in_moc_16` | `/Game/Maps/InMoc16/Lvl_InMoc16` | `45.mp3` | Guilda dos Mercenários |
| `comodo` | `/Game/Maps/Comodo/Lvl_Comodo` | `62.mp3` | Cidade Praiana (Bardo e Odalisca) |

---

### Prioridade 2: Campos Abertos de Upar (16 mapas)

| Mapa | Nível UE (.umap) | BGM | Conexão / Região |
| :--- | :--- | :--- | :--- |
| `prt_fild01` | `/Game/Maps/PrtFild01/Lvl_PrtFild01` | `12.mp3` | Saída Leste / Ponte de Prontera |
| `prt_fild02` | `/Game/Maps/PrtFild02/Lvl_PrtFild02` | `05.mp3` | Campo Norte de Prontera |
| `prt_fild03` | `/Game/Maps/PrtFild03/Lvl_PrtFild03` | `05.mp3` | Campo dos Mandrágoras / Fabres |
| `prt_fild04` | `/Game/Maps/PrtFild04/Lvl_PrtFild04` | `05.mp3` | Campo Central de Prontera |
| `prt_fild07` | `/Game/Maps/PrtFild07/Lvl_PrtFild07` | `05.mp3` | Campo de Porings / Lunáticos |
| `prt_fild09` | `/Game/Maps/PrtFild09/Lvl_PrtFild09` | `04.mp3` | Campo Sul de Prontera |
| `prt_fild10` | `/Game/Maps/PrtFild10/Lvl_PrtFild10` | `04.mp3` | Estrada para Morocc |
| `prt_fild11` | `/Game/Maps/PrtFild11/Lvl_PrtFild11` | `04.mp3` | Bosque dos Rockers |
| `gef_fild00` | `/Game/Maps/GefFild00/Lvl_GefFild00` | `25.mp3` | Saída Leste de Geffen |
| `gef_fild04` | `/Game/Maps/GefFild04/Lvl_GefFild04` | `25.mp3` | Floresta de Geffen |
| `gef_fild07` | `/Game/Maps/GefFild07/Lvl_GefFild07` | `25.mp3` | Pântano de Geffen |
| `pay_fild01` | `/Game/Maps/PayFild01/Lvl_PayFild01` | `03.mp3` | Floresta de Payon |
| `pay_fild08` | `/Game/Maps/PayFild08/Lvl_PayFild08` | `03.mp3` | Bosque de Bambus (Salgueiros) |
| `pay_fild10` | `/Game/Maps/PayFild10/Lvl_PayFild10` | `36.mp3` | Bosque dos Lobos |
| `moc_fild01` | `/Game/Maps/MocFild01/Lvl_MocFild01` | `24.mp3` | Deserto de Morocc (Escorpiões) |
| `moc_fild07` | `/Game/Maps/MocFild07/Lvl_MocFild07` | `24.mp3` | Deserto Profundo (Múmias / Condores) |

---

### Prioridade 3: Calabouços e Transclasse (7 mapas)

| Mapa | Nível UE (.umap) | BGM | Conexão / Região |
| :--- | :--- | :--- | :--- |
| `prt_sewb1` | `/Game/Maps/PrtSewb1/Lvl_PrtSewb1` | `19.mp3` | Esgotos de Prontera 1 (Culvert) |
| `pay_dun00` | `/Game/Maps/PayDun00/Lvl_PayDun00` | `20.mp3` | Caverna de Payon 1 (Zumbis) |
| `pay_dun01` | `/Game/Maps/PayDun01/Lvl_PayDun01` | `20.mp3` | Caverna de Payon 2 (Esqueletos) |
| `iz_dun00` | `/Game/Maps/IzDun00/Lvl_IzDun00` | `29.mp3` | Ilha de Byalan 1 (Caverna Submarina) |
| `iz_dun01` | `/Game/Maps/IzDun01/Lvl_IzDun01` | `29.mp3` | Ilha de Byalan 2 |
| `yuno_in02` | `/Game/Maps/YunoIn02/Lvl_YunoIn02` | `70.mp3` | Academia de Juno (Quest de Sábio / Livro de Ymir) |
| `valkyrie` | `/Game/Maps/Valkyrie/Lvl_Valkyrie` | `09.mp3` | Salão da Valquíria (Rebirth / Transclasse) |

---

## 4. Estrutura de Pastas Gerada

Todos os assets foram salvos de forma organizada em `Content/Maps/<MapName>/`:
```
Content/Maps/<MapName>/
  ├── Lvl_<MapName>.umap          (Nível pronto para Play in Editor ou OpenLevel)
  ├── SM_<mapname>.uasset         (Malha 3D visual completa com materiais aplicados)
  ├── SM_<mapname>_gat_floor.uasset (Colisão caminhável do .gat)
  ├── SM_<mapname>_gat_walls.uasset (Paredes que bloqueiam movimento do Pawn)
  ├── Audio/
  │   └── BGM_<mapname>.uasset   (Música tema do mapa importada do mp3 original)
  ├── Materials/
  │   ├── M_RO_Opaque.uasset     (Master material opaco com suporte a janelas acesas)
  │   ├── M_RO_Masked.uasset     (Master material com máscara alpha)
  │   └── MI_<mapname>_XXX.uasset (Material Instances com texturas vinculadas)
  └── Textures/
      └── T_<texname>_XXX.uasset (Texturas com TF_Nearest e alpha magenta dilatado)
```

Item 3.8 totalmente concluído!

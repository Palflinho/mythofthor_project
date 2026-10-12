# Análise do Data2026 contra o cliente LATAM que usamos (2026-10-12)

Método: comparei o **índice** (nomes e tamanhos) de `D:\Ragnarok_Assets\Data2026_\Data2026\{data.grf,2026.grf}` com `C:\Gravity\Ragnarok\data.grf` (LATAM). Nada foi extraído nem executado.
Números: 2026 = 241.448 arquivos; LATAM = 275.761; **adicionados 41.550, removidos 75.863, alterados 1.441**. Os "removidos" são sobretudo robes antigos, traduções (`i18n`, `english`, `spanish`) e arquivos do LATAM.

## 1. Descoberta mais importante: o que "faltava" já existe no nosso GRF LATAM

Estas tabelas estão no `data.grf` do LATAM e **nunca foram extraídas** (nosso `E:\Dev\Ragnarok_Assets\grf` não as tem). Vários itens que marcamos como "falta o dado" eram falta de extração:

| Tabela | Para que serve | Item nosso que destrava |
|---|---|---|
| `data\mappostable.txt` | posição de cada mapa no mapa-múndi | mapa-múndi gráfico (ROADMAP 1.17) |
| `data\viewpointtable.txt` | pontos de vista de câmera por mapa | ROADMAP 1.8 (câmera) |
| `data\fogparametertable.txt` | parâmetros de névoa por mapa | névoa do RSW |
| `data\mapobjlighttable.txt` | luzes de objetos por mapa | iluminação de mapa |
| `luafiles514\lua files\effecttool\<mapa>.lub` (160 arquivos) | **efeitos colocados em cada mapa** (fumaça, tochas, brilhos...) | auditoria: "efeitos dos objetos do mapa" |
| `...\mapskydata\mapskydata.lub` | cor/tipo de céu por mapa | clima e nuvens |
| `...\skillinfoz\skilldelaylist.lub` | atraso/cooldown por skill | cooldown de skill (auditoria P0) |
| `...\hateffectinfo\footprinteffectinfo.lub` | pegadas | ROADMAP 1.16 (pegadas) |
| `...\ridingspreditinfo\ridingspreditinfo.lub` e `spreditinfo\*layerdir*.lub` | posições das camadas do sprite, inclusive **montado** | Peco Peco / montaria |
| `...\skilleffectinfo\skilleffectinfolist.lub` | efeito de cada skill | efeitos de skill |
| `...\skillinfoz\skilltreeview.lub`, `hotkey.lub` | árvore de skills e teclas padrão | skills / atalhos |

## 2. O que o Data2026 acrescenta (e quase tudo é pós-pré-renovação)

- **Classes:** 1 corpo novo na pasta de classes comparado ao LATAM (`타조집시`, um visual de montaria); os demais já estavam. As classes de 3ª geração e além aparecem nos **efeitos**: Shinkiro/Shiranui (283 arquivos), Meister (135), Sky Emperor (126), Abyss Chaser (125), Biolo (113), Dragon Knight (93), Spirit Handler (92), Imperial Guard (84), Troubadour/Trouvère (60), Arch Mage (48), Night Watch (40), Windhawk (37), Elemental Master (33), Hyper Novice, Cardinal, Inquisitor. **Druida:** corpos `druid`/`druid_riding`, 2.335 texturas de efeito e 76 sons (já no LATAM? — ver `CLIENTE_2026.md`).
- **Mapas:** 89 mapas `.rsw` novos, todos de episódios e instâncias recentes (`1@ep21*`, `1@vr*`, `hero_*`, `jor_*` Jormungand?, `ch1*`, `for_dun*`, `icecastle`, `bl_*`). Nenhum de campo ou cidade clássicos.
- **Interface:** sistema de **runas** (Rune Knight; ~280 arquivos), `rodexsystem` (45), roleta (5), "dancing knife", `cashemotion`, `ui_enchant`, `changematerial`, tag de runa.
- **Lua novos (149):** um `effecttool\<mapa>.lub` por mapa novo; `drawitemonaura.lub` (aura em itens), `exceptionitemproducer.lub`, `helpmsgstr.lub`, `iteminfo.lub`, `cashemotion\*`, `jobname_f.lub`/`pcjobnamegender.lub`.
- **Itens/monstros/NPCs:** 4.036 arquivos de sprite de item, 844 de monstro, 304 de NPC, 766 de acessórios de cabeça (todos novos desde o LATAM).
- **Alterados (1.441):** interface (457), robes (399), `lua files` (241): é a atualização normal de ícones e tabelas.

## 3. Sistemas que o cliente moderno tem (lista dos `lua files` do Data2026)

Já listados na auditoria: `navigation` (navi), `quest`, `adventureguide` (Guia do Aventureiro: armas, armaduras, dungeons, episódios, monstros, quests), `skillinfoz` (skills),
`stateicon`, `emotion`, `cashemotion`, `agit` (castelos WoE), `seekparty` (party booking), `chatwndinfo`, `optioninfo` (`gamesettings`, `uiinfo`, `cmdinfo`).
**Novos para nossa lista:** `damageskin` (skin do número de dano; **o LATAM não tem**), `stylingshop` (loja de estilo/cabelo), `dressroom` (provador de visuais), `enchant` (encantamento), `itemreform`, `transparentitem`,
`equipmentproperties`, `entryqueue` (fila de instância), `battlefield`, `worldviewdata` (enciclopédia do mundo; ligado ao nosso Códice de Mímir), `spreditinfo`, `selectpackage` (caixa de seleção), `signboardlist` (placas de NPC), `wndinfo` (posição/ tamanho padrão das janelas).
Raiz de `data\*.txt`: `buyingstoreitemlist`, `metalprocessitemlist/table` (refino por metal), `itemparamtable`, `itemslot*`, `cardprefix/postfix/itemname`, `bookitemnametable`, `manner`, `tipoftheday`, `resnametable`.

## 4. O que isso significa para o nosso projeto

1. **Prioridade alta, custo baixo:** extrair do GRF que já temos as tabelas da seção 1 (e `effecttool`). Elas resolvem de uma vez: mapa-múndi, câmera por mapa, névoa, luzes, efeitos de mapa, céu, cooldown de skill, pegadas e posição das camadas montadas.
2. **Não adotar o conteúdo novo agora:** classes de 3ª geração, runas, episódios e mapas de instância ficam fora do escopo "até a transclasse" e exigem servidor novo.
3. **Ideias a aproveitar mesmo no pré-renovação:** `adventureguide` e `worldviewdata` como base do Códice de Mímir; `damageskin` como opção cosmética; `wndinfo` e `hotkey` para os padrões de janela e de teclas; `stylingshop`/`dressroom` para o provador (se um dia houver).
4. Não substituir o cliente LATAM pelo de 2026: os arquivos do LATAM (traduções em português) são o que usamos; o 2026 é coreano com tradução em inglês opcional.

## 5. Extração feita (2026-10-12)

Do `C:\Gravity\Ragnarok\data.grf` para `E:\Dev\Ragnarok_Assets\grf\data`: 365 arquivos novos (+44 que já existiam; 2 falhas) = todos os `data\luafiles514\**\*.lub|lua`
(inclui `effecttool` com 155 mapas, `skilldelaylist`, `mapskydata`, `footprinteffectinfo`, `ridingspreditinfo`, `spreditinfo`, `skilleffectinfolist`, `skillinfolist_ptbr`, `skilltreeview`, `hotkey`)
e todos os `data\*.txt` da raiz (`mappostable`, `viewpointtable`, `fogparametertable`, `mapobjlighttable`, `indoorrswtable`...).
Os `.txt` estão em cp949 (`iconv -f cp949 -t utf-8`); os `.lub` são Lua compilado (decompilar/avaliar com `tools/lub_eval.py`).
Formatos vistos: `mappostable.txt` = `região#mapa.rsw#x1#y1#x2#y2#` (140 linhas, retângulo do mapa na imagem do mapa-múndi); `viewpointtable.txt` = faixas de zoom/rotação por mapa (41 linhas);
`fogparametertable.txt` = `mapa.rsw#` seguido de perto, longe, cor `0xAARRGGBB`, densidade (15 KB); `mapobjlighttable.txt` = `mapa.rsw#on|off#`.

Próximos usos (em ordem de custo): (1) `fogparametertable` → névoa por mapa; (2) `viewpointtable` → limites de câmera por mapa; (3) `mappostable` + imagem do mapa-múndi → janela do mapa-múndi;
(4) `skilldelaylist.lub` → cooldown por skill; (5) `effecttool\<mapa>.lub` → efeitos dos objetos do mapa; (6) `ridingspreditinfo` → camadas do sprite montado.

## 6. A interface de 2026 é outra geração? (resposta: não)

Comparando só `texture\유저인터페이스`: **457 alterados, 3.089 adicionados, 1.012 removidos**. Os adicionados são ícones de item/coleção (1.997), runas (226), cashshop (115), ilustrações, cartas e emoções de cash;
os alterados são retoques em botões e janelas já existentes (`basic_interface` 90, `tipbox` 38, `map` 29, `login_interface` 29, `battle_field` 20, `select_character*`, `bank`, `guild_helper`, `statuswnd` 2).
As pastas das janelas principais (`basic_interface`, `inventory`, `statuswnd`, `shortcut`, `minimap`, `navigation_interface`, `renew_questui`, `select_character_ver3`) são **as mesmas** do LATAM que já usamos.
Novas janelas/pastas: `uiadventureragency` (11, Agência de Aventureiros), `clan_system` (5), `reformui` (3), `styleshop`, `runesystem`, `dancing_knife`, `cashemotion`, `group` (21).
Conclusão: trocar a HUD "por completo" não traria ganho; a HUD de 2026 é a mesma família da nossa. O que vale é pegar seletivamente as versões atualizadas de arquivos (ex.: `win_bank`, `guild1-3`, `party1-3`, botões) e as janelas novas que quisermos.

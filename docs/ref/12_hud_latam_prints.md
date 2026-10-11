# Referência 12: HUD do RO LATAM (prints do cliente oficial, 2026-10-10)

Prints em `docs/ref/hud_latam/` (cliente oficial LATAM, servidor Freya, personagem Guardião Real). A janela de
skills do Myth of Thor segue este layout, em árvore (decisão do usuário).

## Tela principal (`03_jogo_tela_inteira.webp`, janela 1020×790)

| Elemento | Posição | Conteúdo |
|---|---|---|
| **Informações básicas** | topo-esquerda (0,30), ~245 px de largura | título "Basic Information" com ⊖; nome, classe; barras de HP/SP com "atual / máx" e % à direita; "Base Lv." e "Job Lv." com barra de EXP; "Weight : x / y  Zeny : z"; seta ▲ para recolher; **grade de ícones do menu** (5 colunas) logo abaixo |
| **Barra de atalhos** | topo-centro, colada à info básica (x≈228, y≈33) | 2 linhas de 9 espaços (o número da linha, "1"/"2", à esquerda), quantidade no canto do ícone; botões ▲▼ à direita para trocar/expandir as linhas; ⊗ no canto |
| **Minimapa** | topo-direita | mapa sem moldura de janela, cerca de 120×100; botões pequenos embaixo (zoom ±, lista, opções); coordenadas "214 346" |
| **Ícones de buff** | direita, embaixo do minimapa | coluna vertical |
| **Chat** | baixo-esquerda, ~600×80 | abas "Regular Message" / "Battle Message" (texto, sem textura); fundo escuro translúcido; barra de rolagem à direita; ícones no canto superior direito (filtro, opções) |
| **Nome do mapa** | topo-centro, ao entrar | faixa com "Canyon village, Arunafeltz" e o nome grande "Veins" |

## Informações básicas expandida (`17_info_basica.png`, 232×376)

- Fundo branco, barra de título azul-clara com o texto à esquerda e ⊖ à direita.
- Linha 1: nome. Linha 2: classe.
- HP e SP:
  - rótulo à esquerda;
  - barra com bordas arredondadas (vermelha para HP, azul para SP);
  - texto "9317 / 14669" centralizado sobre a barra;
  - "63%" à direita.
- Base Lv. e Job Lv.: rótulo à esquerda e barra fina azul; passar o mouse mostra o % (balão "60.1 %").
- "Weight : 1512 / 7500   Zeny : 29,743".
- Seta ▲ centralizada (recolhe a grade).
- **Grade de menu:** 5 colunas × 5 linhas de ícones quadrados (~38 px), em `유저인터페이스/menu_icon/`, sobre o fundo
  `bg_menu.tga`, com o selo "N" de novidade. Ordem nos prints:
  - Linha 1: Status, Equipamento, Inventário, Skills, Grupo(+)
  - Linha 2: Guilda, Quest, Mapa, Navegação/Golfe(?), Opções
  - Linha 3: Bate-papo/Social, Câmera, Correio (N), Conquistas, Livro
  - Linha 4: Loja, Macro (A), Agenda, Ajuda/Personagem, Ranking
  - Linha 5: Presente/Ajuda(?)

## Inventário (`04`…`07`, ~305×150)

- Título "Inventory" com ⊖ ⊗.
- **Abas verticais à esquerda** (ícones): consumíveis (frasco), equipamentos (espada), etc. (gema), favoritos (★).
  A aba ativa fica clara.
- Grade **7 colunas × 3 linhas** de espaços com o "ovo" cinza claro (vazio); o ícone do item com a quantidade
  embaixo à direita.
- Rodapé:
  - ícone + "48/100";
  - cadeado;
  - lupa/filtro;
  - na aba ★, o botão de trocar;
  - à direita, o botão de busca (lupa grande);
  - o canto redimensiona (diagonal).

## Equipamento (`08`…`11`, ~280×200)

- Título "Equip" com ⊖ ⊗.
- **Abas no topo:** General | Special | Title | Damage display.
- **General:** o personagem desenhado no centro.
  - Espaços nas laterais, com o ícone do item e o nome em 2 linhas:
    - esquerda: cabeça topo, meio (máscara), arma, acessório 1;
    - direita: armadura, escudo, sapatos, acessório 2.
  - Embaixo, a caixa "Show Equip" e 4 botões pequenos à direita.
- **Special:** visuais (costumes) e equipamentos de sombra; "View costumes".
- **Title:** lista de títulos.
- **Damage display:** escolha do estilo dos números (Default / Number / Clear).

## Status (`12_status.png`, ~280×230)

- Título "Status".
- **Coluna 1:** Str, Agi, Vit, Int, Dex, Luk.
  - Valor "100+9": base + bônus.
  - Seta ▶ para subir.
  - À direita, os pontos necessários: 16, 2, 5, 8, 8, 2.
- **Colunas 2 e 3:**
  - Atk "152 + 175", Def "73 + 135";
  - Matk "154 + 0", Mdef "122 + 0";
  - Hit 353, Flee "209 + 1";
  - Critical 4, Aspd 154;
  - Status Point 2;
  - Guild.
- "Talent Status" (Pow/Sta/...): é da 4ª classe, **fica de fora** (escopo até transclasse).

## Árvore de skills (`13`…`16`, ~555×410)

- Título "Skill Tree".
- **Abas verticais à esquerda:** "1a", "2a", "3a", "Etc". No Myth: Aprendiz / 1ª / 2ª / Trans.
- **Grade de 7 colunas:**
  - nome curto da skill **acima** do ícone ("Bash", "Provoke"...);
  - ícone com moldura;
  - nível **abaixo**: "10" quando cheio; com pontos, "◀ 9 / 9 ▶" com setas para escolher o nível antes de aplicar.
  - Bloqueada: ícone cinza.
  - Espaço vazio: moldura tracejada clara.
- **Embaixo:**
  - o retrato do personagem no canto inferior esquerdo;
  - "skill point : 0";
  - botões **Apply** e **Reset** à direita.

  Os pontos são distribuídos com as setas e só vão ao servidor no Apply.

## Grupo / Amigos (`19`, `20`, ~290×320)

- Lista branca.
- Rodapé com ícones (procurar, convidar, cadeado...).
- Escolha "Friend / Party" embaixo.
- A janela de amigos tem a arte dos porings no fundo.

## Entrada do jogo (`01`, `02`)

- **Servidor:** janela "Service selection" pequena, com a lista e os botões Replay / OK / Cancel, sobre a arte de
  fundo.
- **Personagens:**
  - grade de cartões (5 × 3, com cadeado nos bloqueados e "+" nos vazios);
  - painel à direita com Map / JOB / Lv / Exp / HP...;
  - teclado de PIN de 4 dígitos.

## Segundo lote de prints (descritos; as imagens não ficaram salvas no disco)

### Menu Esc ("Game setting window")
- Janela pequena (~260×100) com 5 botões largos empilhados:
  - Character selection;
  - Game settings;
  - Set hotkeys;
  - Quit game;
  - Return to game.

### Game settings (~430×380)
- **Abas no topo:** Default | Effect | Control | Graphics | Other settings. Rodapé com Reset (esquerda) e Close
  (direita).
- **Default:**
  - Language (lista);
  - Skin (lista);
  - Sound Settings: Background e Sound, cada um com slider e "Off";
  - Emblem: Show/Hide;
  - Mail: receber de todos ou recusar desconhecidos;
  - Login notification: On/Off.
- **Effect:** lista de chaves **ON|OFF** (pílula azul) com o nome em negrito e a descrição:
  - Show Aura;
  - Aura Display;
  - Show Effects;
  - Show Shading;
  - Damage Display;
  - Map Name;
  - Fog Effect;
  - Footprint Effect;
  - Screen Shake;
  - Hide Summons;
  - Hide Players.
- **Control:** mesmas chaves:
  - Monster Snap;
  - Skill Snap;
  - Item Snap;
  - No Ctrl;
  - No Shift;
  - Zoom Out;
  - Mouse Lock;
  - Quick Skill 1 (F9 no botão direito);
  - Quick Skill 2 (F7/F8 na roda);
  - Mini Slot;
  - Equipment Compare.
- **Graphics:**
  - Graphics API;
  - Graphic Device;
  - Resolution (lista);
  - Sprite Resolution e Texture Resolution (sliders);
  - Screen mode: Full / Windowed;
  - Triad filtering: On/Off.

  No Myth, ficam as opções gráficas modernas que já temos, no mesmo visual.
- **Other settings:** os comandos "/" com chave ON|OFF ou botão "apply" e a descrição, em lista com rolagem. Por
  exemplo:
  - /notrade, /notalkmsg, /showname, /window, /miss, /shopping, /stateinfo, /camera, /monsterhp;
  - /minimap, /mineffect, /font, /showshop, /nocosplay;
  - /hi, /h, /sit, /q, /leave, /where, /memo, /chat, /savechat;
  - /doridori, /bangbang, /bingbing;
  - /ex, /inall, /exall;
  - /blacksmith, /alchemist, /taekwon;
  - /cardillust.

### Make a Room (sala de chat)
- Title, Limit ("20 Person" ▼), Type ▼, Room (senha), cadeado aberto/fechado com chave, OK / Cancel.

### Quests (Alt+U)
- **Abas verticais à esquerda:** Ongoing / Info / Inactive.
- **Lista de cartões:**
  - ícone do tipo (ND, SC, JO, MAIN, GO);
  - título em azul negrito;
  - subtítulo cinza;
  - à direita, a caixinha ✔ (mostrar no rastreador) e o olho (detalhe).
- **Rodapé:** "View Quest Window" e Close.
- **Detalhe:** janela ao lado com o título centralizado, o texto rolável e a imagem.

### Emotion icon list (Alt+L)
- Grade de 5 × 6 balões de emoticon, 3 páginas (Previous / 1/3 / Next).

### Shortcut List (Alt+M)
- Alt+1 … Alt+0 com o texto de cada macro (/!, /?, /ho, /lv, /swt...), botão Look.

### RODEX (correio)
- Logo "RodEx Ro Delivery Express":
  - atualizar e escrever, com o contador "200/300";
  - filtros Title / Sender e a busca;
  - abas Notice | Normal | Return | Search;
  - a lista com envelope, assunto, remetente, anexo e "14Day(s)" em vermelho;
  - os botões Delete All e Receive All, e as setas de página.

### Mapa-múndi
- Mapa "Orbis of Midgard" com as etiquetas vermelhas dos lugares, a lista "Midgard" ▼, a busca e "Monster(Tab)".

### Navegação (/navi)
- Filtro "ALL" ▼, a busca e o resultado ("[7]prt_fild10").
- À direita, o minimapa do campo com a lista de monstros (Swift Elder Willow, Red Mushroom, Poporing, Horn...).
- "service", e os botões Share / Find / Move.

## Terceiro lote (descritos)

### No mundo
- **Barras sob o personagem:** HP (verde) e SP (azul), finas (~60×4), centralizadas logo abaixo dos pés do próprio
  jogador, sempre visíveis.
- **Monstro atingido:** barra de HP (verde sobre fundo roxo/escuro) abaixo dele.
- **Alvo:** seta branca ▼ acima do monstro em foco.
- **Dano:** recebido em vermelho ("524"), causado em branco grande ("428").
- **Anel no chão:** círculo ciano em volta do personagem (aura/área).
- **Portal:** no LATAM é um redemoinho branco no chão. Pre-renewal clássico = anéis azuis (mantidos no Myth).
- **Nome do mapa ao entrar:** "Canyon village, Arunafeltz" pequeno e o nome grande "Veins" (topo-esquerda); ícone de
  "proibido" ao lado quando o mapa não deixa teleportar.
- **Rastreador de quests:** à direita, em texto amarelo (nome) + branco (objetivo).

### Bank Window (Ctrl+B)
- "Deposited 0 z" / "Current Zeny 29,743 z".
- Campo numérico com ▲▼, botões Deposit / Withdraw.
- Atalhos "Max · 10M · 1M · 100K".

### Achievement Quests
- Topo: nível (medalha "7") com a barra 15/178 e os pontos "565".
- Menu à esquerda:
  - Overview;
  - Basic, que abre Character / Actions / Etc.;
  - Adventure;
  - Battle;
  - Quest;
  - Memorial Dungeon;
  - Great Achievement.
- **Overview:** barras por categoria e "Recently Attained Achievements".
- **Lista:**
  - cartões com ícone, nome e pontos (fita azul), a descrição e a data;
  - à direita, o detalhe com o carimbo "COMPLETED".

### Shortcut (teclado, F12)
- Teclado inteiro desenhado; a faixa "Skill Shortcut Key (F1 ~ F9)" marcada.
- Embaixo, a tabela "função → tecla":
  - Status Alt+A; Equip Alt+Q; Skill Tree Alt+S; Inventory Alt+E;
  - Party Alt+Z; Guild Alt+G; Quest List Alt+U; World View Ctrl+`;
  - Navigation; Option Esc; Bank Ctrl+B; Replay;
  - Mail; Achievement; Open a chat room Alt+C; JCoins Store;
  - Shortcut List F12; Sit Insert.
- Desenho do mouse: Move / Rotate / Zoom.
- Botão Set hotkeys.

### Adventurer Agency List
- Filtro de nível "95 ~ 105", classe ▼, ícones de função, busca, Search / Reset.
- Lista: faixa de nível, mapa, ícones, líder, descrição. Páginas 1/3, Close.

### Reputation
- Filtro e busca; cartões por facção (Orc Ville, Goblin Ville) com ○○○ e a barra "0 / 1000 P".

### Info básica recolhida (`21_info_basica_recolhida.png`, 232×44)
- Título com o nome.
- Linha 1: "Nv. 100 / Royal Guard / Nv. 16 / Exp. 60 %".
- Linha 2: "HP. 9887 / 14669 | SP. 1041 / 1041".
- Barra ▼ embaixo (reabre).

### Minimapa (`22_minimapa.png`)
- Imagem do mapa sem moldura (~120×120).
- Ponto vermelho no jogador e a letra do NPC/portal.
- Embaixo: lupa, coordenadas "354 187", e os botões de zoom ±, lista e opções.

### Chão (`23`, `24`)
- `23_pegadas.png` (corrigido pelo usuário em 2026-10-11): **não são pegadas**; é o quadrado do tile sob o mouse, com os cantos verdes.
- `24_item_no_chao.png` (corrigido): **não é item**; é o cursor de skill de área: anel branco (elipse) no chão, ▼ acima e o **nível da skill** ao lado (aqui "3"). (Pegadas de outros jogadores existem no cliente, ver a chave "Footprint Effect" da aba Effect, mas não há print delas.)

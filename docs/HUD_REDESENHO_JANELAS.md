# Redesenho das janelas Equipamento, Habilidades, Grupo, Navegação e Menu Esc (spec)

Pedido do usuário em 2026-10-12: nossas janelas de equipamento, habilidades, grupo e menu "são feias, parecem versão muito antiga". Comparei os prints do LATAM
(`docs/ref/hud_latam/08_equip_geral.png`, `13_skilltree_1a.png`, `19_grupo.png`, `image-1791661013503.png`) com o que o jogo mostra hoje (capturas de 2026-10-12).
A interface de 2026 **não** é outra geração (ver `docs/ref/ANALISE_DATA2026.md`, seção 6): o alvo continua sendo a janela do cliente LATAM, só que **inteira**, com o tamanho e os elementos dela.
Arte: `E:\Dev\Ragnarok_Assets\grf\data\texture\유저인터페이스\` (`FMythRoUITextures::Get("pasta/arquivo.bmp")`). Código: `UI/MythRoHUDWindows.cpp` (equip/skills) e `UI/MythRoParty.cpp` (grupo), `UI/MythRoGameMenu.cpp` (menu).

## Equipamento (Alt+Q), alvo: `08_equip_geral.png`

Hoje: janela de 280 px com 2 abas (Geral, Especial), 10 encaixes só com ícone e nome, fundo `equipwin_bg`, sem prévia do personagem.
Alvo (print):
- Janela maior, **4 abas**: General, Special, **Title**, **Damage** (as duas últimas precisam de dados que o servidor não tem; mostrar a aba vazia com o texto "indisponível" ou escondê-las).
- Encaixes com **ícone redondo à direita/esquerda e o nome do item em texto** (as cabeças e a arma na coluna esquerda, escudo/calçados/acessórios na direita), nome longo quebra em 2 linhas.
- **Prévia do personagem no centro** (sprite do jogador de frente, com o `off` do falcão/montaria sobre ele; botão de ativar/soltar). Arte `equip_preview/*` e `equipwin_bg2.bmp`/`equipwin_bg3.bmp`.
- Rodapé: caixa **"Show Equip"** (permitir que outros vejam o equipamento; pacote `0x02d9/0x02da` ZC_CONFIG hoje ignorado) e **4 botões de ícone** (arquivo, troca rápida, atualizar, cartas/comparar) à direita.
- Já temos: slot de munição (meio), botão de soltar falcão/Peco (movê-los para a posição do print).

## Habilidades (Alt+S), alvo: `13_skilltree_1a.png`

Hoje: janela estreita (~320 px) com abas no topo (Aprendiz, Arqueiro, Caçador) e ícones pequenos; sem ilustração.
Alvo (print):
- Janela larga (~560 x 410) com **abas verticais na esquerda** ("1a", "2a", "3a", "Etc") em vez das abas no topo (a "3a" fica vazia: escopo até transclasse; "Etc" para skills de quest/platina).
- Árvore em grade de **7 colunas x 6 linhas** com ícone grande, nome truncado em cima, nível embaixo e **setas `◀ n/n ▶`** nas skills com vários níveis (já temos a lógica).
- **Ilustração do personagem** (sprite parado) no canto inferior esquerdo e o texto "skill point : N" no rodapé; botões **Apply / Reset** (já temos a lógica de pendentes).
- Linhas de pré-requisito entre ícones (já temos).

## Grupo (Alt+Z), alvo: `19_grupo.png`

Hoje: painel branco liso, linhas de texto "ClaudeTester / Nv 99 Caçador - prontera (líder)" e "Expulsar" como link.
Alvo (print):
- Janela alta (~285 x 320) com barra de título "Party" (botões minimizar/fechar), área branca de membros e **barra de ferramentas embaixo com 4 ícones** (procurar grupo, buscar, abrir/fechar convites, configurações) e as bolinhas **Friend / Party** (já temos a alternância) e **alça de redimensionar** no canto.
- Cada membro como um cartão (`renewalparty/bg_partymember.bmp`): **ícone da classe** (`renewalparty/icon_jobs_<N>.bmp`, versão `_die` se morto), **coroa do líder** (`ico_partycrown.bmp`), nome, nível, barras de HP e SP (SP exige `SHOW_PARTY_SP` no servidor), mapa e botão de menu.

## Menu Esc, alvo: `image-1791661013503.png` ("Game setting window")

Alvo: janela curta com a barra de título e **5 botões largos arredondados** empilhados: Character selection, Game settings, Set hotkeys, Quit game, Return to game. Textos já em português. Confirmar com o usuário se "a de menu" é este menu ou o menu em grade do Basic Info
(o segundo já usa a arte `menu_icon/*`, igual ao LATAM).
Nota: o comando de teste `MythWindow` não tem o menu Esc; para capturar é preciso um comando novo (`MythGameMenu`).

## Ordem sugerida

1. Grupo (rápido: cartões e barra de ferramentas), 2. Habilidades (abas verticais e ilustração), 3. Navegação (busca com filtro, monstros e NPCs por mapa), 4. Equipamento (prévia do personagem exige renderizar o sprite no UMG), 5. Menu Esc (retoque).
Para cada uma: abrir no jogo com `MythWindow` e comparar com o print do LATAM lado a lado antes de entregar.

## Navegação (Alt+N, /navi), alvo: `image-1791661011151.png`

Hoje: janela de ~290 px, caixa de busca com botão "procura", lista de resultados simples com ícone verde, miniatura do mapa e botões "ver" e "Ir até o mapa"; rota em linhas de texto.
Alvo (print):
- Janela estreita e alta (~285 x 350) com barra de título "Navigation" (3 botões: ajuda, minimizar, fechar) e **linha de busca**: seta de voltar, **caixa de filtro "ALL"** (lista suspensa: mapa, NPC, monstro), campo de texto, **lupa** e **pasta** (abrir/guardar).
- Painel esquerdo: **"Result[n]"** com os resultados, cada um com o ícone da categoria (M verde = mapa, M vermelho = monstro, NPC) e o nome em azul sublinhado.
- Painel direito: nome do mapa em azul em cima, **imagem do mapa** (já temos), e a **lista de itens do mapa selecionado** com ícones: o próprio mapa, NPCs e monstros com o nível/tamanho entre parênteses.
- Rodapé da direita: caixa "service", botões **Share**, **Find** e **Move** (o "Move" é o nosso "Ir até o mapa", já funcionando), caixa "Minimize" e dois ícones (histórico e destino fixo) embaixo.
- Dados: `navi_map`, `navi_mob`, `navi_npc`, `navi_link`, `navi_linkdistance`, `navi_npcdistance` e `navi_picknpc` do `luafiles514\lua files\navigation\*.lub` (já extraídos para `E:\Dev\Ragnarok_Assets\grf\data\luafiles514\...`; decompilar com `tools/lub_eval.py`). Hoje só usamos mapas e ligações; faltam monstros e NPCs por mapa, que enchem o painel direito.
- Arte: `navigation_interface3/*` (`btn_find_*`, `btn_share_*`, `btn_getdirections_*`, `dropdown_*`, `icon_list_map/mob/npc`, `bg_search`, `navi_bar_*`).

## Medidas tiradas do goro (MIT, `ui/*.go`, lidas em 2026-10-12)

O goro desenha as janelas com tema próprio, então os números abaixo servem de **proporção e comportamento**, não de arte; a arte e as abas continuam as do LATAM.

**Equipamento (`equipment_window.go`):** janela 300 px de largura, conteúdo de 280 × 130 com 10 px de margem; **3 colunas: esquerda 112, centro 56, direita 112** (x = 0, 112, 168); **linhas de 24 px**.
Esquerda (de cima para baixo): cabeça alta, cabeça baixa, arma, capa, acessório. Direita: cabeça média, armadura, escudo, calçados, acessório. **Munição no centro, embaixo da prévia** (56 × 24, y = 106); a prévia ocupa x=112, y=0, 56 × 106.
Texto: slot vazio mostra o nome do encaixe em cinza; slot cheio mostra o item (prefixo `+N` do refino; munição com a quantidade); até 14 caracteres nas laterais e 8 no centro; esquerda alinha à esquerda, direita à direita; ícone a 4 px da borda e texto 8 px depois;
divisória de 1 px entre linhas; dica a Y+18 do mouse; **duplo clique (até 360 ms)** equipa/tira; clique direito abre a descrição. Rodapé: caixa **"Show Equip"** (120 × 20, liga/desliga o envio de configuração), botão **Cart** (se houver carrinho) e **"Peco Off"/"Cart Off"** (só aparece se estiver montado/com carrinho).
→ Nosso slot de munição fica em x=136 y≈39, quase o mesmo lugar; falta a prévia embaixo/acima, a caixa "Show Equip" e mover o botão de soltar para o rodapé.

**Grupo/Amigos (`friends_window.go`):** janela 286 × 256; duas abas "Friends"/"Party" de 72 × 24 (nós usamos bolinhas, como no print do LATAM); linhas de **286 × 24** alternando cor; nome a 8 px, até 20 caracteres, " *" marca o líder; mapa/status à direita (largura 64);
barra de HP de 5 px de altura, 8 px de margem e contorno de 1 px; sem barra de SP; rodapé: Criar / Convidar (líder) / Configurações / Sair.

**Menu Esc (`escape_menu.go`):** janela **252 × 200** (252 × 238 com "voltar à vida"), título "Menu", sem botão de fechar, botões em coluna com **16 px de margem e 8 px entre eles**, largura total; Esc abre/fecha; ao morrer vira o menu de morte com "Voltar à vida / Voltar ao ponto salvo / Seleção de personagem / Sair / Cancelar".
→ Nosso menu tem 5 itens (print do LATAM); conferir se o tamanho e a margem batem com 252 e 16/8.

**Onde olhar para o resto:** o goro tem `ui/skill*window.go` (a lista dele ficou truncada), `making_item_window`, `making_arrow_window`, `auto_spell_window`, `cart_window`, `guild_window`, `pet_*`, `homunculus_*`, `mercenary_*`, `mail_window`, `minimap`, `monster_info_window` e `npc_cutins`: servem para as janelas funcionais que ainda não existem (Etapa C da auditoria).
Limite honesto: **nem goro nem nostalro reproduzem as janelas modernas do LATAM** (abas Title/Damage, ilustração na árvore de skills, Navigation com filtro); para essas só temos os prints e as peças de arte. O roBrowser (GPL; só olhar medidas, não copiar código) também recria as janelas originais, mas as de 2008-2012.

## Medidas tiradas do roBrowserLegacy (GPL: só números, nada de código; CSS em `src/UI/Components/*`, lido em 2026-10-12)

O roBrowser tem **versões modernas** das janelas (EquipmentV4, SkillListV2, PartyFriendsV1) e usa os mesmos bitmaps do cliente. O CSS dele não cita os nomes dos bmp (a arte vem do JS), então: números daqui, arte do nosso GRF.

**Equipamento V4 (`EquipmentV4.css`):** janela **280** de largura; barra de título 17; **abas de 15 de altura e 60 de largura** (General, Special, Title, Damage); painel de 150; conteúdo **280 × 130**; colunas 1 e 3 de **115 × 24**;
botão do item 24 × 24 e texto de 80 × 24 (linha 12); **munição absoluta a 30 px do topo**, texto de 45; **carrinho 36 × 36 em (14, 65)** e **"remover opção" (Peco/falcão) 36 × 36 em (12, 90)** (ou seja, **no meio da janela, abaixo da prévia**, como nos prints do LATAM);
rodapé de 20: caixas "ver status" (9 × 14), "Show equip" e "show costume" (10 × 12) e botões **switch equip e remove equip de 40 × 20**; aba **Title**: lista 280 × 125 com 8 de margem; aba **Damage**: seletor de skin 280 de largura, opções 64 × 92 com ícone 36 × 36.

**Habilidades V2 (`SkillListV2.css`):** conteúdo 270 × 200 com margem 5 (a versão larga do LATAM tem corpo de 375 de altura); **abas verticais de 18 de largura** coladas à esquerda (`left −23`); **células da grade 28 × 28 com borda tracejada**, margem 15/17, linha com `padding-left 40`;
nome com 70 de largura; setas de nível 11 × 11 (`currentUp/currentDown`); botão de subir nível 24 × 24; **rodapé de 27** com texto à esquerda e botões **Apply (right 70) e Reset (right 20), 42 × 20**; ícone de expandir 13 × 13; botão de level up do job 43 × 43 no canto.

**Grupo/Amigos V1 (`PartyFriendsV1.css`):** conteúdo **240 × 120** com rolagem; barra de título 17 com as duas abas Party/Friend (32 × 13); **faixa de ferramentas de 20 de altura com botões 26 × 20**; **rodapé de 21** com a troca Friend/Party à esquerda, **alça de redimensionar 13 × 13 no canto** e um contador de membros (caixa 48 × 19);
**cartão de membro 38 de altura**, padding 3 × 5; **ícone da classe 24 × 24 com moldura 28 × 28 (−2, −2)**; **coroa do líder 25 × 25 a −14 do topo**; linha 2 de 12 com **barra de HP 75 × 5** (borda 1) e ícone de status 26 × 12; nome até 100 px, mapa até 80 px (fonte 10), **nível em laranja (11 px)**.

**Menu Esc (`Escape.css`):** janela **280** de largura, altura automática; título a 18 px da esquerda; contêiner de **220 de largura com padding 20 30 6 30**; **botões 221 × 20 com 3 px entre eles** (equivale ao 252 × 200 do goro dentro da arte do LATAM).

**Navegação (`Navigation.css`):** o roBrowser tem layout próprio e simples (300 × 300, mapa de 280 × 230, busca por lista suspensa), **não** a janela moderna do LATAM; serve só para o tamanho do mapa (280 × 230) e do campo de busca (altura 20). Para a janela do print continuam valendo os prints e a arte `navigation_interface3`.

## Os outros projetos, resumo

| Projeto | Licença | Tem as janelas modernas? | Para que serve |
|---|---|---|---|
| roBrowserLegacy (MrAntares) | GPL | **Sim**: Equipment V4, SkillList V2, PartyFriends V1, Bank, Rodex, Reputation, Achievement, Navigation (próprio), SwitchEquip, Roulette... | **medidas e comportamento** de quase todas as nossas janelas; a pasta `src/UI/Components` lista ~110 componentes |
| goro (kivutar) | MIT | em parte (tema próprio) | medidas das janelas clássicas, janelas funcionais (carrinho, guilda, pets...) e a lista de lacunas |
| nostalro-client | Apache-2.0 | em parte (tema próprio) | **fluxos** (fabricação, vending, cliques, estados) |
| korangar, open-midgard | (a conferir) | a verificar | ainda não lidos; já listados na nossa memória como referências |

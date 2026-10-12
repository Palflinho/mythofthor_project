# Redesenho das janelas Equipamento, Habilidades, Grupo e Menu Esc (spec)

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

1. Grupo (rápido: cartões e barra de ferramentas), 2. Habilidades (abas verticais e ilustração), 3. Equipamento (prévia do personagem exige renderizar o sprite no UMG), 4. Menu Esc (retoque).
Para cada uma: abrir no jogo com `MythWindow` e comparar com o print do LATAM lado a lado antes de entregar.

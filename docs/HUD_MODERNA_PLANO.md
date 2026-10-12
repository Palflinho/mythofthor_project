# HUD moderna igual à do RO LATAM

Pedido do usuário em 2026-10-11: "deixe toda a hud moderna igual a do rolatam". Regra: usar a **arte real do cliente**
(`data\texture\유저인터페이스\...`, já legível em runtime por `FMythRoUITextures`) e conferir cada janela contra o print
em `docs/ref/hud_latam`. Os botões do cliente já trazem o rótulo em português desenhado (`btn_close` = "fechar").

## Peças (infraestrutura)

| Peça | Estado |
|---|---|
| `AddButton(painel, "nome", x, y)`: botão de 3 estados (`btn_nome`, `_a` mouse em cima, `_b` apertado) | ✅ |
| Fonte do cliente (SCDream) | ✅ já usada |
| Moldura de janela (`titlebar_*`, `basewin_bg`) | ✅ existente; revisar contra os prints |
| Rolagem (`scrollbar`, `dialscr_*`), abas (`tab_*`), combo (`combobox_*`), setas (`arw_*`) | usar nas janelas abaixo |

## Janelas, na ordem do que se vê jogando

| # | Janela | Print | Arte principal | Estado |
|---|---|---|---|---|
| 1 | Barra de atalhos (2 linhas) | 03, 17 | `shortcut_bg`, `shortitem_bg`, `quickslot`, `shortcut/` | ✅ 2 linhas, setas, linha ativa |
| 2 | Chat | 18 | `chatwin*`, `chat_tab_*`, `scrollbar` | ✅ abas, rolagem, histórico |
| 3 | Minimapa | 22 | `minimap/` | ✅ zoom, recolher, coordenadas |
| 4 | Informações básicas | 17, 21 | `basewin_bg`, `basewin_mini`, `btn_*_on/off` | ✅ grade translúcida, ícones ligados e aviso de correio novo |
| 5 | Itens | 04-07 | `inventory/`, `tab_item_*`, `itemwin_*` | ✅ 4 abas com favoritos, grade 7x4, rodapé |
| 6 | Equipamento | 08-11 | `equipwin_*`, `swap_equipment/` | 🟡 abas Geral e Especial; falta a prévia do personagem e as abas Título e Dano (sistemas que não existem no servidor) |
| 7 | Atributos | 12 | `statuswnd/` | ✅ arte w_statwin_bg, setas e custo; falta a parte "Talent" (4ª classe) |
| 8 | Habilidades | 13-16 | `skill*`, `tab_*` | ✅ pontos pendentes com OK e Cancelar |
| 9 | Grupo | 19 | `renewalparty/` | ✅ botões do cliente e troca Amigos/Grupo |
| 10 | Amigos | 20 | `btn_friend_*` | ✅ botões do cliente e troca Amigos/Grupo |
| 11 | Menu do jogo e Game settings | menu Esc, configurações | `gamesettingsui/` | ✅ menu Esc com 5 itens e configurações em uma janela com 5 abas |
| 12 | Missões | quests | `renew_questui/`, `quest_window` | ✅ cartões, abas e caixa de rastreio |
| 13 | Conquistas | achievement | `achievement_re/` | ✅ abas, cartões, detalhes com carimbo |
| 14 | Navegação | navi | `navigation_interface3/` | ✅ resultados, prévia do mapa e rota |
| 15 | Correio, Banco, Troca rápida | RODEX, bank | `maillist*_bg`, `win_bank`, `swap_equipment/` | ✅ RODEX com a arte do cliente, banco com o layout do print; troca rápida com botões |
| 16 | Troca, loja, armazém, diálogo NPC, sala de chat, emoções | | `exchange_bg`, `dialog_*` | 🟡 botões do cliente |
| 17 | Tela de login, seleção e criação de personagem | 01, 02 | `login_interface/`, `select_character_ver3/`, `make_character_ver2/` | 🟡 seleção com cartões e painel; falta o fundo próprio (arte do jogo), a prévia do sprite nos cartões e a tela de criação |

Cada linha só vira ✅ depois de um print do jogo ao lado do print de referência.

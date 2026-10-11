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
| 4 | Informações básicas | 17, 21 | `basewin_bg`, `basewin_mini`, `btn_*_on/off` | 🟡 grade translúcida e ícones ligados; falta o botão "novo" |
| 5 | Itens | 04-07 | `inventory/`, `tab_item_*`, `itemwin_*` | ⬜ |
| 6 | Equipamento | 08-11 | `equipwin_*`, `swap_equipment/` | ⬜ |
| 7 | Atributos | 12 | `statuswnd/` | ⬜ |
| 8 | Habilidades | 13-16 | `skill*`, `tab_*` | ⬜ |
| 9 | Grupo | 19 | `renewalparty/` | ⬜ |
| 10 | Amigos | 20 | `btn_friend_*` | ⬜ |
| 11 | Menu do jogo e Game settings | menu Esc, configurações | `gamesettingsui/` | ⬜ |
| 12 | Missões | quests | `renew_questui/`, `quest_window` | ⬜ |
| 13 | Conquistas | achievement | `achievement_re/` | ⬜ |
| 14 | Navegação | navi | `navigation_interface3/` | ⬜ |
| 15 | Correio, Banco, Troca rápida | RODEX, bank | `maillist*_bg`, `win_bank`, `swap_equipment/` | ⬜ |
| 16 | Troca, loja, armazém, diálogo NPC, sala de chat, emoções | | `exchange_bg`, `dialog_*` | ⬜ |
| 17 | Tela de login, seleção e criação de personagem | 01, 02 | `login_interface/`, `select_character_ver3/`, `make_character_ver2/` | ⬜ |

Cada linha só vira ✅ depois de um print do jogo ao lado do print de referência.

# Mapa da HUD Moderna do Cliente LATAM (Referência de UI Fase 3)

Este documento mapeia todas as texturas, dimensões exatas em pixels, componentes estruturais (9-slice, botões, abas, barras de rolagem) e o layout de posicionamento na tela da HUD moderna do cliente Ragnarok Online LATAM, extraídas diretamente do arquivo `data.grf` em `data/texture/유저인터페이스/`.

---

## 1. Diálogo de NPC Moderno (Prioridade Alta)

O diálogo original de 2002 utilizava uma caixa cinza opaca e estreita de 280px no canto superior esquerdo, desconfortável para leitura de textos longos. A HUD moderna do LATAM utiliza uma janela ampla de 600px de largura, posicionada no terço inferior da tela, com suporte a histórico, configurações e botões dinâmicos.

### 1.1 Texturas da Caixa de Diálogo (`basic_interface/`)

| Elemento | Arquivo de Textura | Dimensões | Formato | Função no Componente |
|---|---|---|---|---|
| **Fundo Principal (Linha)** | `dialog_bg.bmp` | 600 x 24 px | 8bpp | Linha de fundo padrão da área de texto (repetida verticalmente). |
| **Fundo Alternado** | `dialog_bg2.bmp` | 600 x 24 px | 8bpp | Linha de fundo com tom sutilmente contrastante para leitura. |
| **Topo / Borda Esquerda** | `dialog_left.bmp` | 92 x 16 px | 8bpp | Borda superior esquerda arredondada com detalhe metálico. |
| **Topo / Borda Direita** | `dialog_right.bmp` | 460 x 16 px | 8bpp | Borda superior direita com suporte aos botões de utilidade. |
| **Borda Central** | `dialog_mid.bmp` | 2 x 24 px | 8bpp | Fita vertical para interpolação/preenchimento central. |
| **Ponto de Redimensionamento** | `dialog_resize.bmp` | 6 x 24 px | 8bpp | Ícone de puxador no canto inferior direito para redimensionar a altura. |
| **Pixel de Histórico** | `dialog_his.bmp` | 2 x 2 px | 8bpp | Marcador de preenchimento do histórico de conversas anteriores. |

### 1.2 Botões de Ação do Diálogo

| Botão / Ação | Normal | Hover / Over | Pressionado / Press | Dimensões | Função |
|---|---|---|---|---|---|
| **Avançar Diálogo** | `nc_next1.bmp` | `nc_next2.bmp` | `nc_next2.bmp` | 43 x 43 px | Botão circular grande de "Próximo / Next" no rodapé direito. |
| **Próxima Página** | `nc_nextpage.bmp` | `nc_nextpage.bmp` | `nc_nextpage.bmp` | 9 x 10 px | Indicador de seta de avanço rápido de página de texto. |
| **Confirmar (OK)** | `ok.bmp` | `ok.bmp` | `ok_a.bmp` | 49x27 / 47x25 px | Botão "OK" para mensagens de confirmação de NPCs. |
| **Cancelar / Fechar** | `cancel.bmp` | `cancel.bmp` | `cancel_a.bmp` | 49x27 / 47x25 px | Botão "Cancelar" para abortar diálogo ou fechar menu. |
| **Cancelar Compacto** | `cancel2.bmp` | `cancel2_a.bmp` | `cancel2_a.bmp` | 43 x 21 px | Variante compacta de cancelamento. |
| **Fechar Janela (X)** | `close.bmp` | `close.bmp` | `close_a.bmp` | 9 x 10 px | Botão "X" no topo da janela. |
| **Histórico** | `dialbtn_his.bmp` | `dialbtn_his0.bmp` | `dialbtn_his0.bmp` | 11 x 11 px | Abre o histórico de falas passadas da conversa com o NPC. |
| **Opções do Diálogo** | `dialbtn_opt.bmp` | `dialbtn_opt0.bmp` | `dialbtn_opt0.bmp` | 11 x 11 px | Configuração de velocidade do texto ou fonte do diálogo. |
| **Alternar Diálogo** | `btn_dialog_off.bmp` | `btn_dialog_on.bmp` | `btn_dialog_on.bmp` | 30 x 20 px | Oculta ou exibe o balão de diálogo temporariamente. |

### 1.3 Barra de Rolagem do Diálogo (`basic_interface/`)

- **Trilho de Fundo:** `dialscr_bg.bmp` (10 x 53 px, 8bpp).
- **Cursor / Thumb:** `dialscr_bar.bmp` (10 x 4 px, 8bpp).
- **Seta para Cima:** `dialscr_up.bmp` (10 x 10 px) / `dialscr_up1.bmp` (9 x 9 px).
- **Seta para Baixo:** `dialscr_down.bmp` (10 x 10 px) / `dialscr_down1.bmp` (9 x 9 px).

### 1.4 Botões de Opção e Escolha (Menus de NPC)

Para menus onde o jogador escolhe respostas (ex.: `select("Sim:Não")`), a interface utiliza:
1. **Fatias 9-Slice Dinâmicas:**
   - **Normal:** `btn_out_left.bmp` (6x20), `btn_out_mid.bmp` (6x20 repetido horizontalmente), `btn_out_right.bmp` (6x20).
   - **Hover:** `btn_over_left.bmp` (6x20), `btn_over_mid.bmp` (6x20), `btn_over_right.bmp` (6x20).
   - **Pressed:** `btn_press_left.bmp` (6x20), `btn_press_mid.bmp` (6x20), `btn_press_right.bmp` (6x20).
2. **Botões Numerados (Atalhos 1 a 9):**
   - `mesbtn_01.bmp` a `mesbtn_09.bmp` (26 x 20 px), com variantes `_a.bmp` (over) e `_b.bmp` (press).

---

## 2. Janela de Inventário Moderna (`inventory/` e `basic_interface/`)

A janela de inventário moderna do LATAM possui suporte a 8 abas de categorização, barra de busca interna, trava de segurança de itens e pesagem detalhada.

### 2.1 Abas de Itens (20 x 235 px, 24bpp)
Localizadas em `basic_interface/`, representam as 8 categorias modernas de itens:
1. `tab_item_01.bmp`: **Todos (All)**
2. `tab_item_02.bmp`: **Consumíveis (Usable)**
3. `tab_item_03.bmp`: **Equipamentos (Equip)**
4. `tab_item_04.bmp`: **Outros / Etc**
5. `tab_item_05.bmp`: **Visuais (Costume)**
6. `tab_item_06.bmp`: **Equipamentos Sombrios (Shadow)**
7. `tab_item_07.bmp`: **Projéteis / Flechas (Ammo)**
8. `tab_item_08.bmp`: **Favoritos (Favorite)**

### 2.2 Grid de Itens e Células
- **Célula de Item Normal:** `itemwin_mid.bmp` (32 x 32 px, 8bpp).
- **Célula de Item Destacada:** `itemwin_mid_pink.bmp` (32 x 32 px, 24bpp).
- **Borda Esquerda do Grid:** `itemwin_left.bmp` (20 x 8 px).
- **Borda Direita do Grid:** `itemwin_right.bmp` (20 x 8 px).
- **Marcador de Item Novo:** `new_item.bmp` (32 x 32 px).

### 2.3 Controles de Segurança e Ferramentas do Inventário (`inventory/`)
- **Trava de Troca / Deal Lock:** `bt_itemdeal_lock_off.bmp`, `_on.bmp`, `_over.bmp`, `_press.bmp` (26 x 17 px).
- **Célula de Item Travada:** `itemwin_mid_lock.bmp` e `itemwin_new_lock.bmp` (32 x 32 px).
- **Pop-up de Aviso de Trava:** `popup_itemdeal_lock.bmp` (242 x 26 px).
- **Botão de Busca de Itens:** `btn_search_new_out.bmp`, `_over.bmp`, `_press.bmp` (26 x 19 px).
- **Indicador de Peso:** `icon_weight.bmp` (12 x 11 px).
- **Contador de Quantidade:** `icon_num.bmp` e `icon_num_pink.bmp` (11 x 14 px).
- **Comparador de Equipamento:** `item_compare_off.bmp` e `item_compare_on.bmp` (12 x 12 px).
- **Proteção Contra Drop:** `item_drop_lock_off.bmp` e `item_drop_lock_on.bmp` (14 x 11 px).

---

## 3. Janela de Status e Atributos (`statuswnd/`)

A janela de status moderna possui dois modos integrados (básico e expandido):

| Elemento | Arquivo de Textura | Dimensões | Formato | Descrição |
|---|---|---|---|---|
| **Fundo Básico** | `w_statwin_bg.bmp` | 280 x 124 px | 24bpp | Janela clássica com os 6 atributos principais (STR, AGI, VIT, INT, DEX, LUK), pontos restantes e botões (+). |
| **Fundo Expandido** | `w_ex_statwin_bg.bmp` | 280 x 224 px | 24bpp | Adiciona o painel inferior de estatísticas de combate (ATK, MATK, HIT, CRIT, DEF, MDEF, ASPD). |
| **Botão Expandir (Normal)** | `expand_off_normal.bmp` | 276 x 18 px | 24bpp | Botão inferior em estado recolhido. |
| **Botão Expandir (Press)** | `expand_off_press.bmp` | 276 x 18 px | 24bpp | Estado de clique para abrir a expansão. |
| **Botão Recolher (Normal)** | `expand_on_normal.bmp` | 276 x 18 px | 24bpp | Botão inferior em estado expandido. |
| **Botão Recolher (Press)** | `expand_on_press.bmp` | 276 x 18 px | 24bpp | Estado de clique para recolher a janela. |

---

## 4. Janela de Equipamentos (`basic_interface/`)

A janela de equipamentos (Alt+Q) do cliente moderno é particionada em abas de visualização de 280px:

- **Equipamentos Normais:** `equipwin_bg.bmp` (280 x 130 px, 8bpp) e versão estendida `equipwin_bg2.bmp` (280 x 150 px).
- **Equipamentos Visuais (Costume):** `equipwin_costume.bmp` (280 x 130 px) e `equipwin_costume2.bmp` (280 x 130 px).
- **Equipamentos Sombrios (Shadow / Especial):** `equipwin_special.bmp` (280 x 133 px).
- **Equipamentos de Aluguel / Cash:** `equipwin_cash.bmp` (280 x 133 px, 24bpp).
- **Painel de Troca Rápida de Equipamento:** `swap_equipment/btn_change2_normal.bmp`, `_over.bmp`, `_press.bmp` (24 x 24 px).
- **Visualizador 3D do Personagem:** `equip_preview/btn_mounting_normal.bmp` e moldura `bg_mounting.bmp` (120 x 178 px).

---

## 5. Janela Básica e Barras de HP/SP (Alt+V)

- **Fundo Padrão:** `basewin_bg.bmp` (280 x 120 px, 8bpp) ou `w_basewin_bg2.bmp` (220 x 150 px).
- **Modo Minimizado (Barra Fina):** `basewin_mini.bmp` (280 x 34 px) ou `basewin_mini2.bmp` (215 x 50 px).
- **Moldura dos Medidores (Gauge):** `gze_bg.bmp` (85 x 9 px) e `gze_bg1.bmp` (135 x 9 px).
- **Barra Vermelha (HP):**
  - Ponta esquerda: `gzered_left.bmp` (4 x 8 px).
  - Miolo (repetição): `gzered_mid.bmp` (1 x 8 px).
  - Ponta direita: `gzered_right.bmp` (4 x 8 px).
- **Barra Azul (SP):**
  - Ponta esquerda: `gzeblue_left.bmp` (4 x 8 px).
  - Miolo (repetição): `gzeblue_mid.bmp` (1 x 8 px).
  - Ponta direita: `gzeblue_right.bmp` (4 x 8 px).

---

## 6. Barras de Atalhos / Hotkeys (`shortcut/`)

- **Moldura da Linha de Habilidades:** `shortcut_frame.bmp` (290 x 34 px, 24bpp). Contém 9 slots de atalho por barra.
- **Moldura Estendida com Consumíveis:** `shortcut_frame_item.bmp` (290 x 43 px, 24bpp) e `shortcut_frame_item_2.bmp` (290 x 34 px).
- **Botões de Expansão de Barras (Até 4 Barras F12):**
  - Botão Adicionar Barra (+): `btn_plus_normal.bmp`, `btn_plus_over.bmp`, `btn_plus_press.bmp`, `btn_plus_disable.bmp` (11 x 11 px).
  - Botão Remover Barra (-): `btn_minus_normal.bmp`, `btn_minus_over.bmp`, `btn_minus_press.bmp` (11 x 11 px).

---

## 7. Minimapa (`minimap/`)

O minimapa utiliza uma moldura quadrada no canto superior direito:
- **Tamanho da Textura do Mapa:** 128 x 128 px (ou 256 x 256 px em resoluções maiores).
- **Botões de Controle Integrados (12 x 12 px, 24bpp):**
  - Zoom (+): `i_plus_1.bmp`, `i_plus_2.bmp`, `i_plus_3.bmp`.
  - Zoom (-): `i_minus_1.bmp`, `i_minus_2.bmp`, `i_minus_3.bmp`.
  - Transparência / Visibilidade: `i_viewon_1.bmp`, `i_viewon_2.bmp`, `i_viewon_3.bmp`.
  - Minimizar / Modo Ícone: `i_mini_1.bmp`, `i_mini_2.bmp`, `i_mini_3.bmp`.
  - Marcadores de NPCs e Quests: `i_object_1.bmp`, `i_object_2.bmp`, `i_object_3.bmp`.
  - Ícone de MvP / Chefe: `boss_1.bmp`, `boss_2.bmp`, `boss_3.bmp` (24 x 24 px).

---

## 8. Painel de Menu Principal Moderno (`menu_icon/`)

Diferente do cliente antigo que espalhava botões soltos ou dependia do teclado, a interface moderna LATAM possui uma gaveta de menu pop-up:

- **Fundo da Grade de Menu:** `menu_icon/bg_menu.tga` (220 x 228 px, 32bpp TGA com canal alfa real).
- **Barra de Gatilho / Doca Inferior:** `bt_menu_normal.bmp`, `bt_menu_over.bmp`, `bt_menu_press.bmp` (219 x 9 px) e versão alta `bt_menu_new_normal/over/press.bmp` (219 x 13 px).
- **Ícones da Grade (32 x 34 px normal, 35 x 40 px com badge de novidade):**
  - Atributos: `bt_status.bmp` / `bt_status_press.bmp`
  - Equipamento: `bt_equip.bmp` / `bt_equip_press.bmp`
  - Inventário: `bt_item.bmp` / `bt_item_press.bmp`
  - Habilidades: `bt_skill.bmp` / `bt_skill_press.bmp`
  - Mapa Mundial: `bt_map.bmp` / `bt_map_press.bmp`
  - Grupo: `bt_party.bmp` / `bt_party_press.bmp`
  - Clã / Guilda: `bt_guild.bmp` / `bt_guild_press.bmp`
  - Quests: `bt_quest.bmp` / `bt_quest_press.bmp`
  - Navegação: `bt_navigation.bmp` / `bt_navigation_press.bmp`
  - Opções de Jogo: `bt_option.bmp` / `bt_option_press.bmp`
  - Conquistas: `bt_achievement.bmp` / `bt_achievement_press.bmp`
  - Banco: `bt_bank.bmp` / `bt_bank_press.bmp`
  - Correio (RODEX): `bt_mail.bmp` / `bt_mail_press.bmp`
  - Presença Diária: `bt_attendance.bmp` / `bt_attendance_press.bmp`
  - Loja de Cash: `bt_shop.bmp` / `bt_shop_press.bmp`
  - Caixa de Dicas / Guias: `bt_tip.bmp` / `bt_tip_press.bmp`

---

## 9. Janelas dos Novos Sistemas do Myth of Thor

- **Grau de Equipamento (`grade_enchant/`):**
  - Fundo principal: `bg_grade_enchant.bmp` (400 x 330 px, 24bpp).
  - Slots de progressão de grau (D -> C -> B -> A): `bg_gradeitem1.bmp` a `bg_gradeitem5.bmp` (104 x 208 px).
  - Botão Iniciar: `btn_start.bmp`, `btn_start_over.bmp`, `btn_start_press.bmp`, `btn_start_disable.bmp` (82 x 28 px).
  - Botões de Chance / Catalisador: `btn_chance_up.bmp` e `btn_chance_down.bmp` (32 x 15 px).
  - Botão de Proteção contra Quebra: `btn_unbroken.bmp` (32 x 15 px).
- **Interface de Grupo Renewal (`renewalparty/`):**
  - Barra de membro de grupo: `bg_partymember.bmp` (38 x 15 px).
  - Ícone de líder de grupo: `ico_partycrown.bmp` (21 x 10 px).
  - Ícones de classe: `icon_jobs_0.bmp` a `icon_jobs_14.bmp` e versões mortas `icon_jobs_*_die.bmp` (25 x 25 px).

---

## 10. Layout Oficial na Tela: O Que Está em Arquivo vs. Hardcoded

### 10.1 Arquivos `.lua` / `.lub`
Os arquivos `.lub` extraídos de `data/luafiles514/lua files/` (como `datainfo/accessoryid.lub`, `jobname.lub`, `skilldescript.lub`) contêm **dados tabulares de jogo** (nomes de itens, descrições de habilidades, IDs visuais de sprites). Eles **não** contêm o layout posicional das janelas da HUD.

### 10.2 O Que É Hardcoded no Cliente (Ragexe)
No cliente oficial da Gravity:
1. **Posicionamento Padrão de Inicialização:** As coordenadas `(X, Y)` e pontos de âncora de cada janela são embutidos no binário executável (`Ragexe.exe`).
2. **Persistência de Customização:** Quando o jogador move uma janela ou altera o tamanho do chat, as coordenadas são gravadas no Registro do Windows do usuário em:
   `HKEY_CURRENT_USER\Software\Gravity Soft\Ragnarok\` (chaves `POS_BASICINFO`, `POS_ITEMWND`, `POS_STATUSWND`, `POS_CHATWND`, etc.).

### 10.3 Mapa de Posicionamento Padrão Oficial na Tela (Referência Base 1920 x 1080)

```text
+-------------------------------------------------------------------------------+
| [Alt+V Informações]                                        [Ctrl+Tab Minimap] |
| (X: 0, Y: 0)                                     (X: 1920-140, Y: 0) [128x128] |
| [280 x 120 px]                                                                |
|                                                                               |
|                                                                               |
| [Alt+A Status]          [Alt+Q Equip]                    [Alt+E Inventário]   |
| (X: 50, Y: 140)         (X: 340, Y: 140)                 (X: 1920-300, Y: 160)|
| [280 x 124/224 px]      [280 x 133 px]                   [280 x 380 px]       |
|                                                                               |
|                                                                               |
|                           [Diálogo de NPC Moderno]                            |
|                           (X: (1920-600)/2 = 660, Y: 780)                     |
|                           [600 x 200 px com 9-slice e nc_next]                |
|                                                                               |
| [Chat do Jogo]          [Barra de Atalhos F12]            [Menu Pop-up Alt+M] |
| (X: 0, Y: 1080-140)     (X: 660, Y: 1080-45)             (X: 1920-220, Y: 852)|
| [280 x 103 px]          [290 x 34 px por linha]          [220 x 228 px TGA]   |
+-------------------------------------------------------------------------------+
```

### 10.4 Recomendações para a Implementação em UMG (Unreal Engine):
1. **Âncoras:**
   - Barra Básica: Âncora Top-Left `(0, 0)`.
   - Minimapa: Âncora Top-Right `(1, 0)`.
   - Menu Principal: Âncora Bottom-Right `(1, 1)`.
   - Chat: Âncora Bottom-Left `(0, 1)`.
   - Diálogo NPC: Âncora Bottom-Center `(0.5, 1)`.
2. **Texturas 9-Slice:**
   Usar a funcionalidade de Box Margin do Slate / UMG para `dialog_bg`, `btn_out_*`, `w_statwin_bg` e `basewin_bg`, garantindo que os botões e janelas redimensionem com nitidez pixel-perfect.

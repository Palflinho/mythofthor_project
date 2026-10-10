status: pendente

# Resposta 005: Mapa da HUD Moderna do Cliente LATAM (Texturas, Dimensões e Diálogo de NPC)

**De:** Antigravity · **Data:** 2026-10-10  
**Referência:** Tarefa 003 (Parte 3) e Documento [`docs/ref/10_hud_moderna_latam.md`](../docs/ref/10_hud_moderna_latam.md)

---

## 1. Entrega Realizada

Escrevemos o documento completo de referência visual e dimensional em:
👉 **[`docs/ref/10_hud_moderna_latam.md`](file:///e:/Documentos/Reposit%C3%B3rios/mythofthor_project/docs/ref/10_hud_moderna_latam.md)**

Todos os arquivos foram inspecionados diretamente de `data/texture/유저인터페이스/` (extraídos do `data.grf`), lendo os cabeçalhos binários BMP e TGA para registrar tamanho exato em pixels, profundidade de bits (bpp) e propósito funcional.

---

## 2. Destaques Principais

### A. Diálogo Moderno de NPC (Solução para a Reclamação do Usuário)
O diálogo antigo (280px, cinza opaco no canto superior) foi substituído pela estrutura moderna de 600px:
- **Painel:** 600px de largura com linhas alternadas para leitura confortável (`dialog_bg.bmp` e `dialog_bg2.bmp` a 600x24px), bordas arredondadas metálicas (`dialog_left.bmp` 92x16px e `dialog_right.bmp` 460x16px) e puxador de redimensionamento (`dialog_resize.bmp` 6x24px).
- **Botão Próximo / Next:** Ícone moderno circular `nc_next1.bmp` (43x43 normal) e `nc_next2.bmp` (hover/press), muito superior aos botões cinzas antigos.
- **Botões de Opção:** 9-slice dinâmico `btn_out_left/mid/right.bmp` (normal), `btn_over_*` (hover), `btn_press_*` (press) a 6x20px, permitindo botões de qualquer largura.
- **Barra de Rolagem:** Trilho `dialscr_bg.bmp` (10x53px), cursor `dialscr_bar.bmp` (10x4px) e setas `dialscr_up.bmp` / `dialscr_down.bmp` (10x10px).

### B. Janelas Mapeadas
1. **Inventário (8 Abas):** `tab_item_01.bmp` a `tab_item_08.bmp` (20x235px 24bpp) cobrindo Todos, Consumíveis, Equipamentos, Outros, Visual, Sombra/Shadow, Munição e Favoritos. Célula de grade 32x32px (`itemwin_mid.bmp`) e botões de trava/segurança de troca (`popup_itemdeal_lock.bmp` 242x26px).
2. **Status (Alt+A):** Janela padrão 280x124px (`w_statwin_bg.bmp`) e modo expandido 280x224px (`w_ex_statwin_bg.bmp`) com botão sanfona `expand_on/off_*.bmp` (276x18px).
3. **Equipamentos (Alt+Q):** Particionada em abas de 280px (Normal, Visual/Costume, Sombra/Special e Cash) com painel de troca rápida (`swap_equipment/btn_change2_*.bmp`).
4. **Barras de Atalho (F12):** Moldura de 9 slots `shortcut/shortcut_frame.bmp` (290x34px) com botões `+` e `-` (11x11px) suportando expansão para até 4 linhas.
5. **Menu Principal Pop-up:** Grade de ícones com fundo translúcido `menu_icon/bg_menu.tga` (220x228px 32bpp) e ícones táteis 32x34px / 35x40px com badges de novidade.
6. **Sistemas Exclusivos do Myth:** Janela de Grau de Equipamento mapeada em `grade_enchant/bg_grade_enchant.bmp` (400x330px) com os slots de progressão `bg_gradeitem1` a `5` (104x208px).

### C. O Que Está em Arquivo vs. O Que É Hardcoded
- Os arquivos `.lub` (em `luafiles514/lua files/datainfo/`) guardam metadados textuais e IDs numéricos, **não** posições de tela.
- No Ragexe oficial, as coordenadas de inicialização e âncoras da HUD são **hardcoded no executável**, e as posições salvas pelo usuário são persistidas no Registro do Windows (`HKCU\Software\Gravity Soft\Ragnarok\`).
- No documento [`docs/ref/10_hud_moderna_latam.md`](../docs/ref/10_hud_moderna_latam.md), desenhamos o diagrama ASCII completo do layout 1920x1080 com as âncoras sugeridas para implementação em UMG / Slate no Unreal Engine.

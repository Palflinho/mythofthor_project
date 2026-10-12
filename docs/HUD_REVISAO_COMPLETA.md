# Revisão completa da HUD (2026-10-12): nosso tamanho x referência

Método: tamanho de cada janela tirado do **nosso código** (`MakeRoPanel`/`MakeWindow` em `UI/*.cpp`, em unidades da HUD; a tela multiplica por `UIScale` 1,35) comparado com
(1) os prints do LATAM em `docs/ref/hud_latam`, (2) o CSS do roBrowserLegacy e (3) o código do goro (medidas em `docs/HUD_REDESENHO_JANELAS.md`).
Onde não tenho número de referência, a regra é o print. Veredito: ✅ confere · 🟡 parecido, ajustar · ❌ diferente, refazer · ❓ falta referência.

## 1. Janelas principais

| Janela (arquivo) | Nosso tamanho | Referência | Veredito e ajuste |
|---|---|---|---|
| Informações básicas (`MythRoHUDWidget.cpp`) | 232 × 134 (recolhida 56) | print 17: 232 de largura; recolhida do print 21 | ✅ tamanho. Falta botão de minimizar no print 17? Já entregue (sys_mini) |
| Hotbar | 306 × 2 linhas | roBrowser ShortCut: 9 casas; print 03 | ✅ (2 linhas) |
| Chat | ver `BuildChat` | print 18 | ✅ |
| Minimapa | 128 × 144 | print 22 (128 × 128 + faixa de 16) | ✅ |
| Status (`MythRoHUDWindows.cpp`) | 280 × 141 | arte `w_statwin_bg` 280 × 124 + título 17 | ✅ (usa a arte real) |
| Itens | 254 × 175 (7 × 4 casas de 32) | print 04 (aprox. 280 × 190); inventário LATAM: 7 colunas | 🟡 conferir contra o print: altura e rodapé |
| **Equipamento** | **280 × 165** | roBrowser V4: título 17 + abas 15 + painel 150 + rodapé 20 = **202**; goro 300 | ❌ 37 px mais baixa que a referência; **falta rodapé** (Show Equip, switch, remover), abas Title/Damage e prévia; slots de 115 × 24 (nós ~86 de largura) |
| **Habilidades** | **296 × ~496** (abas no topo) | print 13: ~**560 × 410** com abas verticais; roBrowser V2: células 28 × 28, rodapé 27 | ❌ layout diferente: abas verticais, 7 colunas × 6 linhas (nós 9 linhas), ilustração, rodapé com Apply/Reset |
| **Grupo** | **280 × (lista + 34)** | roBrowser V1: conteúdo 240 × 120, título 17, ferramentas 20, rodapé 21; goro 286 × 256; print 19 ~285 × 320 | ❌ cartões de 34 → **38**, sem ícone de classe, sem coroa, sem barra de ferramentas nem alça |
| Amigos | 270 × (lista) | goro: linhas 286 × 24; roBrowser PartyFriends compartilhado | 🟡 usar a **mesma janela do Grupo** (286 de largura, linhas de 24) |
| **Menu Esc** (`MythRoGameMenu.cpp`) | **200 × 149** (botões de 24) | print: ~**300 × 146** com 5 botões largos; goro 252 × 200; roBrowser 280 | ❌ estreito demais; **largura 300**, botões ~262 × 21, 5 px entre eles |
| Configurações (gráficos/som/opções) | 470 × variável | prints 60973410…60989882: ~ 470 de largura | ✅ largura; 🟡 conferir altura por aba |
| Morte (`DeathWindow`) | 260 × 91 | goro: menu de morte 252 × 238 com 5 botões | 🟡 hoje 2 opções; o original tem Voltar à vida / ao ponto salvo / seleção de personagem / sair / cancelar |
| Informações do item | InfoW × InfoH (300 × 210) | tipbox / print do LATAM | 🟡 conferir |

## 2. Janelas funcionais e de sistemas

| Janela | Nosso tamanho | Referência | Veredito |
|---|---|---|---|
| **Navegação** | 292 × 347 | print 61011151: ~285 × 350 | 🟡 tamanho ok; **falta** filtro "ALL", lupa/pasta, lista de monstros e NPCs do mapa, Share/Find, caixa "service" e "Minimize" |
| Missões | 381 × 466 | print 60992147 (renewal quest UI) | ✅ (arte real) |
| Conquistas | 750 × 451 | print 61023980 | ✅ (arte real) |
| Correio (Rodex) | 309 × 416; ler 300 × 400 | print 61006960 | ✅ |
| Banco | 290 × 155 | print 61021643 | ✅ |
| Troca rápida | 280 × variável | print "Switch Equip" | 🟡 |
| Atalhos F12 | 250 × variável | print 61028669 (teclado desenhado) | 🟡 o print é um teclado ilustrado, nós uma tabela |
| Lista de emoções | 252 × variável | prints 60999375… (3 páginas) | ✅ |
| Lista de atalhos Alt+M | 340 × variável | print 61004761 | 🟡 |
| Sala de chat | 340 × 177 (criar), 280 × 321 (sala) | print 60955947 | 🟡 |
| Loja NPC / Armazém | 560 × variável | print do LATAM (compra/venda lado a lado) | 🟡 conferir proporção |
| Troca (trade) | 430 × variável | goro/roBrowser Trade | 🟡 |
| Diálogo de NPC | 360 × 160 (menu abaixo) | roBrowser NpcBox / print 03 | 🟡 conferir; **falta**: entrada de número/texto e cut-in |
| Convites (grupo, amizade, troca) | 300 × 92 | roBrowser WinPopup | ✅ |

## 3. Janelas que ainda não existem (da auditoria)

Carrinho, loja de jogador (vending), guilda, pets, homúnculo, mercenário, fabricação (forja/poção/flechas) e janela de lista de skills (Autospell, Arrow Making), senha do armazém, Monster Info, mapa-múndi, Guia do Aventureiro. Referências de medida: goro `ui/*_window.go` (cart, guild, pet, homunculus, making_item, making_arrow, auto_spell, monster_info) e roBrowser `src/UI/Components/*`.

## 4. Ordem de trabalho (tamanho do problema)

1. **Menu Esc**: só ajustar tamanho e espaçamento (1 arquivo, rápido).
2. **Grupo + Amigos**: janela única de 286 de largura, cartões de 38 e ícones de classe, coroa, barra de ferramentas e alça.
3. **Habilidades**: reorganizar com abas verticais, 6 linhas, ilustração e rodapé.
4. **Equipamento**: rodapé, abas Title/Damage (vazias), slots de 115 × 24 e prévia do personagem.
5. **Navegação**: filtro, lista de monstros/NPCs (`navi_mob.lub`, `navi_npc.lub` já extraídos).
6. Janelas funcionais da seção 3, na ordem de prioridade da auditoria.

Cada item: abrir no jogo com `MythWindow` (precisa de um comando novo para o menu Esc) e comparar **lado a lado** com o print antes de entregar.

## Andamento (atualizado 2026-10-12)

- ✅ **Menu Esc:** 300 de largura, 5 botões de 262 × 20 (commit `d6399ab`).
- 🟡 **Grupo:** cartões de 38 com fundo `bg_partymember`, ícone da classe (`icon_jobs_<id>`), coroa do líder e texto claro; faltam barra de ferramentas, alça de redimensionar, contador de membros e a barra de SP.
- ✅ **Fonte:** UI usa Tahoma (Windows) com fallback para Arial e Roboto; conferir visualmente contra os prints. O SCDream fica só para o coreano.
- ⬜ Habilidades, Equipamento, Navegação e janelas funcionais (ver a ordem acima).
- 🟡 **Habilidades:** abas verticais na esquerda (Ap, 1a, 2a, Tr) no lugar da faixa superior, grade deslocada; faltam a ilustração do personagem, limitar a 6 linhas, e rodapé idêntico ao do print (Apply/Reset já existem). Conferido em jogo.
- 🟡 **Habilidades (2ª passada):** grade de 7 colunas de 68 × 56 como no print 13, nome da skill em cima (cortado com reticências), nível embaixo, casas vazias com moldura apagada, mínimo de 6 linhas e abas verticais; faltam a ilustração do personagem, as abas "3a/Etc" e o texto "skill point : N" no estilo do print.

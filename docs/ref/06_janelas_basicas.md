# Especificação 06: Janelas Básicas — Layout, Dimensões, Atalhos e Comportamento

> **Origem das referências:**
> - **OM (Open-Midgard):** `src/ui/UIBasicInfoWnd.cpp`, `src/ui/UIItemWnd.cpp`, `src/ui/UIEquipWnd.cpp`, `src/ui/UIStatusWnd.cpp`, `src/ui/UISkillListWnd.cpp`, `src/ui/UIShortCutWnd.cpp`, `src/ui/UIMinimapWnd.cpp`, `src/ui/UINewChatWnd.cpp`, `src/ui/UISayDialogWnd.cpp`, `src/ui/UINpcMenuWnd.cpp`, `src/ui/UIItemInfoWnd.cpp`, `src/ui/UIStorageWnd.cpp`, `src/ui/UIWindowMgr.cpp`.
> - **RB (roBrowserLegacy):** `src/UI/Components/*`, `src/Controls/KeyEventHandler.js`.

---

## 1. Resumo

A interface clássica do Ragnarok Online é composta por janelas modulares independentes, arrastáveis pela barra de título (`height = 17 px`), com modos expandido e minimizado (ou barra simples), botões de fechar/minimizar padronizados no canto superior direito (`11 \times 11` ou `12 \times 11` px) e navegação rápida via teclas de atalho (`Alt+...`). A tecla `Esc` fecha as janelas ativas em ordem inversa de foco (LIFO) e, quando nenhuma janela estiver aberta, invoca o Menu Principal do jogo.

---

## 2. Resumo das Janelas (Dimensões e Atalhos)

| Janela | Posição Inicial Sugerida | Largura (px) | Altura Expandida (px) | Altura Minimizada (px) | Atalho | Fechável por Esc? |
|---|---|---|---|---|---|---|
| **BasicInfo** (Informações Básicas) | `(0, 0)` Topo Esquerdo | `280` | `120` | `34` | `Alt + V` | Não (permanente) |
| **ItemWnd** (Inventário) | `(300, 100)` Centro-Esq | `280` | `134` | `34` | `Alt + E` | Sim |
| **EquipWnd** (Equipamentos) | `(300, 240)` Centro-Esq | `280` | `232` | `34` | `Alt + Q` | Sim |
| **StatusWnd** (Atributos / Status) | `(0, 140)` Esquerda | `280` | `120` | `17` | `Alt + A` | Sim |
| **SkillListWnd** (Habilidades) | `(590, 100)` Centro-Dir | `317` | `291` | `17` | `Alt + S` | Sim |
| **ShortCutWnd** (Barra de Atalhos) | `(TelaX - 280, 0)` Topo Dir | `275` | `31` | N/A (fixa) | `F12` | Não (permanente) |
| **MinimapWnd** (Minimapa) | `(TelaX - 190, 40)` Topo Dir | `184` | `212` | N/A (cicla) | `Ctrl + M` | Sim |
| **NewChatWnd** (Chat e Histórico) | `(12, TelaY - 260)` Base Esq | `486` | `240` (redimensionável) | `34` | `Enter` | Não |
| **SayDialog / NpcMenu** (Diálogo NPC) | Centralizado na tela | `360` | `160` | N/A | Espaço / Enter | Não (bloqueia) |
| **ItemInfoWnd** (Detalhe do Item) | Próximo ao item clicado | `332` | `308` (dinâmico) | N/A | `Botão Direito` | Sim |
| **StorageWnd** (Armazém Kafra) | `(200, 120)` Centro | `360` | `318` | `34` | N/A (NPC) | Sim |

---

## 3. Especificação Detalhada por Janela

### 3.1. UIBasicInfoWnd (Informações Básicas)
- **Dimensões:** $280 \times 120$ px (expandida); $280 \times 34$ px (minimizada); barra de título: 17 px.
- **Elementos Visuais:**
  - Nome do Personagem e Classe.
  - Barras de HP e SP: largura 85 px, altura 9 px.
  - Barras de Base EXP e Job EXP: largura 102 px, altura 6 px (exibe porcentagem com 1 casa decimal, ex: `45.2%`).
  - Peso: `"Weight : %d / %d"` (ex: `1250 / 3000`).
  - Zeny: formatado com vírgulas (`"1,250,000 z"`).
  - 8 Botões de menu rápido no rodapé ($32 \times 20$ px): Status, Opções, Itens, Equip, Skill, Mapa, Chat, Amigos.
- **Interações:**
  - Arrastar: pela barra de título.
  - Minimizar: botão em $X = 266, Y = 3$ ($11 \times 11$ px) comuta entre modo 34 px (apenas barras essenciais) e 120 px.

### 3.2. UIItemWnd (Inventário de Itens)
- **Dimensões:** $280 \times 134$ px (expandida); $280 \times 34$ px (minimizada).
- **Abas de Categoria (3 clássicas):** Item / Consumíveis, Equipamentos, Etc ($32 \times 82$ px cada aba à esquerda).
- **Grade de Itens:**
  - Célula de ícone: $32 \times 32$ px.
  - Início da grade: $X = 36, Y = 17$. Margem direita 20 px, margem inferior 21 px.
  - Capacidade máxima clássica: 100 slots.
- **Interações:**
  - Clique Duplo: usa consumível ou equipa a peça imediatamente.
  - Clique com Botão Direito: abre a janela detalhada de descrição (`UIItemInfoWnd`).
  - Arrastar para o chão: abre caixa de descarte para confirmar quantidade a jogar no chão.
  - Arrastar para a Barra de Atalhos (`ShortCutWnd`): vincula o item à tecla F1-F9 correspondente.

### 3.3. UIEquipWnd (Equipamentos)
- **Dimensões:** $280 \times 232$ px; barra de título: 17 px.
- **Painel Central do Personagem:** Retângulo de visualização 3D do personagem em $X \in [98, 182], Y \in [32, 204]$.
- **Slots ao Redor (10 slots de 32x32 px):**
  - Esquerda: Cabeça Topo, Cabeça Meio, Cabeça Baixo, Mão Direita (Arma), Mão Esquerda (Escudo).
  - Direita: Armadura, Capa, Calçado, Acessório 1, Acessório 2.
- **Interações:**
  - Clique Duplo em slot equipado: desequipa o item e o devolve ao inventário.
  - Botão "Desequipar Carrinho" (`CartOff`, $30 \times 11$ px) para Mercadores.

### 3.4. UIStatusWnd (Atributos do Personagem)
- **Dimensões:** $280 \times 120$ px; altura corpo: 103 px.
- **Atributos Principais:** 6 linhas para STR, AGI, VIT, INT, DEX, LUK.
  - Botão `+` ao lado de cada atributo para distribuir pontos livres (`Status Point`).
  - Exibição no formato: `Base + Bônus` (ex: `50 + 8`).
- **Valores Secundários (Painel Direito):**
  - ATK (`Base + Bônus`), DEF (`Hard + Soft`), MATK (`Min ~ Max`), MDEF, HIT, FLEE, CRITICAL, ASPD.

### 3.5. UISkillListWnd (Lista de Habilidades)
- **Dimensões:** $317 \times 291$ px; altura de cada linha: 37 px; ícone: $24 \times 24$ px (célula $32 \times 32$ px).
- **Elementos:**
  - Nome da habilidade, nível atual e nível máximo (ex: `Golpe Fulminante Lv 10`).
  - Custo de SP na descrição resumida.
  - Botão `+` para alocar pontos de skill livres.
  - Botão inferior "Usar" (`Use`) e "Fechar" (`Close`).
- **Interações:**
  - Clique Duplo: conjura a habilidade.
  - Botão Direito: abre a descrição completa com fórmulas e alcances (`UISkillDescribeWnd`).
  - Arrastar: permite soltar o ícone na barra de atalhos.

### 3.6. UIShortCutWnd (Barra de Atalhos)
- **Dimensões:** $275 \times 31$ px.
- **Slots:** 9 posições fixas correspondentes a F1 até F9 ($24 \times 24$ px cada, passo de 29 px entre centros).
- **Páginas:** Suporta até 4 barras de atalho, alternadas ou expandidas via `F12`.
- **Indicação Numérica:** Exibe quantidade restante de itens empilháveis (ex: poções) no canto inferior direito do slot.

### 3.7. UIMinimapWnd (Minimap)
- **Dimensões:** $184 \times 212$ px; área quadrada do mapa: $164 \times 164$ px.
- **Coordenadas:** Texto exibido abaixo do mapa no formato `NomeDoMapa, X, Y` (ex: `prontera, 156, 180`).
- **Estados com `Ctrl + M`:** Cicla entre Completo $\rightarrow$ Translúcido (sem moldura) $\rightarrow$ Oculto.
- **Marcadores:** Ponto branco piscante para o jogador; setas verdes para membros do grupo; pontos amarelos para portais.

### 3.8. UINewChatWnd (Janela de Chat)
- **Dimensões Padrão:** $486 \times 240$ px (redimensionável pelo canto superior direito, largura mínima 360 px).
- **Componentes:**
  - Campo de mensagem com suporte a 180 caracteres e histórico de últimas 5 mensagens (navegável com setas Cima/Baixo).
  - Campo de Whisper / PM ($126$ px de largura) para digitar o destinatário.
  - Botão de engrenagem para abrir configurações de transparência e tamanho de fonte (11 a 18 px).
  - Abas de chat (Geral, Batalha/Combate, Mensagens Privadas).

### 3.9. UISayDialogWnd e UINpcMenuWnd (Diálogo e Menus de NPC)
- **Dimensões de Diálogo:** $360 \times 160$ px; cantos arredondados raio 10 px.
- **Botões:** "Next" ($68 \times 22$ px) e "Close" ($68 \times 22$ px).
- **Comportamento Modal Especial:**
  - Enquanto o diálogo estiver aberto, **todas as ações de clique no chão, ataques e atalhos de inventário são bloqueados**.
  - A tecla `Espaço` ou `Enter` avança o diálogo.
  - A tecla `Esc` **NÃO fecha o diálogo** involuntariamente (evitando perder quests acidentalmente).
- **UINpcMenuWnd (Menu de Opções):**
  - Lista de opções verticais com hover destacado em azul.
  - Teclas numéricas (1 a 9) ou duplo-clique selecionam a opção.

### 3.10. UIItemInfoWnd (Descrição Detalhada do Item)
- **Dimensões:** $332 \times 308$ px (altura dinâmica adaptável ao tamanho do texto da descrição).
- **Elementos:**
  - Caixa de prévia da ilustração do item ($96 \times 96$ px no canto superior esquerdo).
  - Slots de cartas ($22 \times 22$ px cada, até 4 slots).
  - Nome completo colorido e texto de descrição renderizado com suporte a formatação colorida hex (`^RRGGBB`).
- **Fechamento:** Fecha com tecla `Esc` ou botão Fechar.

### 3.11. UIStorageWnd (Armazém Kafra)
- **Dimensões:** $360 \times 318$ px; 3 abas (Item, Equip, Etc).
- **Interações:**
  - Clique duplo em item no inventário move para o armazém.
  - Clique duplo em item no armazém move para o inventário.
  - Se a pilha for maior que 1, abre pop-up numérico (`UINpcInputWnd`) para selecionar a quantidade.

---

## 4. Regras Globais do Teclado e da Tecla Esc

1. **Ordem de Fechamento por Esc (LIFO):**
   - Ao pressionar `Esc`:
     - Se houver janela de entrada de quantidade aberta (`UINpcInputWnd`): fecha o input.
     - Se houver `UIItemInfoWnd`: fecha a janela de informação.
     - Se houver janelas de jogo abertas (Equip, Item, Status, Skill): fecha a janela com foco mais recente.
     - Se **nenhuma** janela secundária estiver aberta: abre o **Menu do Jogo** (`UIOptionWnd` / Sair / Cancelar).
2. **Atalhos Essenciais (`Alt + ...`):**
   - `Alt + V`: Alterna tamanho da janela básica (`BasicInfo`).
   - `Alt + E`: Abre/fecha Inventário de Itens.
   - `Alt + Q`: Abre/fecha Janela de Equipamentos.
   - `Alt + A`: Abre/fecha Janela de Status.
   - `Alt + S`: Abre/fecha Janela de Habilidades.
   - `Alt + C`: Abre sala de chat / chatroom.
   - `F12`: Alterna barra de atalhos.
   - `Insert` ou `/sit`: Senta / Levanta.

---

## 5. Como Testar

1. **Teste da Tecla Esc:**
   - Abra o inventário (`Alt+E`), a janela de equipamentos (`Alt+Q`) e dê botão direito em uma poção para abrir o detalhe do item.
   - Pressione `Esc` uma vez: a janela de detalhe fecha.
   - Pressione `Esc` novamente: a janela de equipamentos fecha.
   - Pressione `Esc` novamente: o inventário fecha.
   - Pressione `Esc` com tudo fechado: o Menu Principal do jogo abre na tela.
2. **Teste de Formatação de Textos:**
   - Verifique a barra de EXP: deve exibir valor numérico e porcentagem formatada com uma casa decimal (ex: `12.5%`).
   - Verifique o Zeny: deve exibir separadores de milhares (`10,000 z`).
3. **Teste de Minimizar Janela Básica:**
   - Clique no botão de minimizar no topo direito do BasicInfo.
   - **Resultado esperado:** A janela encolhe de 120 px para 34 px exibindo apenas a barra compacta de HP/SP.

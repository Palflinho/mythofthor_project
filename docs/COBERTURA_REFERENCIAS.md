# Cobertura das referências (`docs/ref`) no roadmap

Conferência feita em 2026-10-11, doc por doc, contra `docs/ROADMAP.md` e `docs/FIDELIDADE.md`.
Legenda: ✅ feito e testado · 🟡 feito em parte ou sem teste · ⬜ não feito · ❓ decisão pendente.

## Documentos da pesquisa R1 (01 a 13)

| Doc | Tema | Estado | Onde está no roadmap |
|---|---|---|---|
| 01 | Movimento e pathfinding | 🟡 falta `FixPathTime` e bloqueio de clique ao carregar mapa | F1 1.9 |
| 02 | Câmera | 🟡 falta clamp do terreno e `viewpointtable` | F1 1.8 |
| 03 | Atores e animação | 🟡 faltam readyfight, stun, 2º golpe; capa/carrinho sem teste do usuário | F1 1.1 a 1.3 |
| 04 | Clique, alvo, combate | 🟡 falta mira de skill em área (círculo) | F1 1.10 |
| 05 | Efeitos do executável | 🟡 portal, dano, aura feitos; Level Up / Super Anjo a conferir | F1 1.5, F3 3.6 |
| 06 | Janelas básicas | 🟡 existem; faltam detalhes (Base+Bônus, diálogo de quantidade, Apply/Reset) | F1 1.11, 1.13 |
| 07 | Sons e BGM | 🟡 BGM, sons do `.act` e armas feitos; sons de interface e de skill (campo `wav`) faltam | **novo 1.20** |
| 08 | UI moderna do roBrowser | ⬜ quests Alt+U, /navi, conquistas, banco, RODEX, troca rápida de equipamento | **novo 1.17 e 1.19** |
| 09 | Render HD-2D (normais, Z-fighting) | ⬜ | "Depois da fidelidade: Visual HD-2D" |
| 10 | HUD moderna LATAM | ⬜ janelas com texturas reais, translúcidas, cantos arredondados; diálogo de NPC largo, abas do inventário, painel de menu | **novo 1.15** |
| 11 | Efeitos procedurais de skill | 🟡 receitas existem; faltam 118 skills, buffs no corpo, tipo SPR | F3 3.6, `EFEITOS_PENDENTES.md` |
| 12 | Prints da HUD LATAM | ⬜ itens do 2º e 3º lote não estavam no roadmap (ver abaixo) | **novo 1.15 a 1.19** |
| 13 | Aura 99 e FUNC | ✅ aura; 🟡 os FUNC vazios (33) e o terremoto (ID 666) não foram tratados | F1 1.5, **novo 1.21** |

## Documentos da pesquisa R2 (14 a 22)

| Doc | Tema | Estado | Onde |
|---|---|---|---|
| 14 | Lojas de NPC | 🟡 compra feita; venda e desconto do Mercador sem teste | F3 3.1 |
| 15 | Kafra e armazém | 🟡 armazém feito; salvar, teleporte e aluguel de carrinho pelo diálogo sem teste | F3 3.1 |
| 16 | Morte e reviver | ✅ (HP de volta é 1 a 2: padrão do servidor, ver F4) | F3 3.1 |
| 17 | Grupo | 🟡 feito (2026-10-11); falta divisão de EXP na janela, minimapa e teste com 2 jogadores reais | F3 3.3 |
| 18 | Chat e comandos | 🟡 faltam abas Grupo e Guilda, `/effect /mineffect /snap /itemsnap`, guilda | F3 3.3 |
| 19 | Itens, equipamento, tooltips | 🟡 falta conferir faixas de sobrepeso (50% e 90%) e cores `^RRGGBB` | F1 1.11, **novo 1.18** |
| 20 | Atributos e fórmulas | 🟡 janela existe; fórmulas exibidas a conferir | F1 1.11 |
| 21 | Barra de cast e buffs | 🟡 existe; interrupção e ícones a conferir | F1 1.7 |
| 22 | Troca, login, montarias | ⬜ | F3 3.5, "Entrada do jogo" |

## Itens do doc 12 que **não estavam** no roadmap (agora estão)

Originais do cliente clássico (entram na fidelidade):
- Sala de chat (**Make a Room**, Alt+C): título, limite, tipo, senha.
- Lista de emoções (Alt+L) e lista de atalhos (Alt+M, F12 com o teclado desenhado).
- Nome do mapa ao entrar (nome pequeno + nome grande), ícone de "proibido teleportar".
- Info básica recolhida (232×44), minimapa com zoom ± e coordenadas, o quadrado do tile sob o mouse (cantos verdes), o cursor de skill de área com anel e nível da skill.
- Menu Esc com a lista de opções `/comando` (ON/OFF) e as opções de teclado.
- Mapa-múndi (Ctrl+`) e quests com rastreador na tela.

Modernos do LATAM, **entram adaptados (decisão do usuário), menos JCoins**: RODEX (correio), Conquistas, Reputação,
Agência de Aventureiros, Banco (Ctrl+B), /navi, troca rápida de equipamento. JCoins fica de fora.

## Itens novos no roadmap

Ver `docs/ROADMAP.md`, F1 itens 1.15 a 1.21.

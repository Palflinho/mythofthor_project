# O que aproveitar de goro e nostalro-client (lido em 2026-10-12)

Só comportamento, nomes de pacote e listas (sem copiar código). Fontes:
- goro (Go, MIT): https://github.com/kivutar/goro, pasta `docs/`: `classic-ro-client-gap-checklist.md`, `packet-coverage-20080910-per-feature.md`,
  `map-effects-weather-todo.md`, `walk-combat-sprite-todo.md`, `companions-20080910.md`, `world-map.md`, `quest-journal.md`, `legacy-mail-20080910.md`, `woe-20080910.md`.
- nostalro-client (Rust, Apache-2.0): https://github.com/nmeylan/nostalro-client, `Features.md` (pré-renovação, packetver 20120307).

Não li os `.md` `companions`, `world-map`, `quest-journal`, `woe` e `legacy-mail` do goro nem o código dos dois; ficam como próxima leitura quando a função entrar.

## 1. Lista de pacotes por função (goro) — ouro para a nossa auditoria

O `packet-coverage-...-per-feature.md` é uma tabela de função → ids de pacote que um cliente clássico precisa tratar. Os ids abaixo são do PACKETVER 2008;
no nosso (20250402) vários ganharam versões novas, mas o conjunto de **funções** é o mesmo.

| Função | Pacotes (2008) |
|---|---|
| Entrada de número / texto em NPC | 0x0143 / 0x01d5 |
| Excluir personagem | 0x01fb → 0x006f/0x0070 |
| Ignorar sussurros | 0x00cf / 0x00d0 |
| Armazém com senha | 0x023a / 0x023b / 0x023c |
| Sábio: lista de Autospell | 0x01cd → 0x01ce |
| Arrow Making: materiais | 0x01ad / 0x01ae |
| Forja/poção (Smith/Alchemist) | 0x018d / 0x018e / 0x018f |
| Reparar arma | 0x01fc / 0x01fd / 0x01fe |
| Refinar arma (skill) | 0x0221 / 0x0222 / 0x0223 |
| Cozinha (Cooking) | 0x025a / 0x025b |
| Encaixar carta | 0x017a / 0x017c |
| Ver equipamento de outro jogador | 0x02d6 / 0x02d7 / 0x02d8 |
| Loja de jogador (vending) | 0x012d–0x0137, 0x01b2 |
| Carrinho ↔ corpo ↔ armazém | 0x0126–0x0129; remover opção 0x012a |
| Pets | captura 0x019e–0x01a0; menu 0x01a1–0x01a4; janela/renomear 0x01a5; ovo/chocar 0x01a6/0x01a7; emoção/fala 0x01a9/0x01aa |
| Homúnculo / Mercenário | 0x022d–0x0235, 0x0239, 0x027d, 0x029b–0x029f, 0x02a2 |
| Guilda | janela 0x014d/0x014e, criar 0x0165/0x0167, convites 0x0168–0x016b, listas 0x014f/0x0150/0x0154/0x0160/0x0162…, emblema 0x0151–0x0153/0x01b4, aliança 0x0170–0x0173… |
| Ranking de Ferreiro/Alquimista/PK | 0x0217–0x021c, 0x0224–0x0226, 0x0237 |
| Casamento/adoção | 0x01f6–0x01f9, 0x0216 |
| Missões | 0x02b1–0x02b7 |
| Barra de progresso de NPC | 0x02f0 / 0x02f1 / 0x02f2 |
| Monster Info (skill Sense) | 0x018c |
| Dori-dori e Explosion Spirits | 0x01e7 / 0x01ed |
| Cut-in de NPC | 0x01b3 |
| Mapa-múndi / clima por mapa | `worldmap.bmp` + `mapPosTable.txt` |

## 2. Lista de lacunas do cliente clássico (goro) — itens que **nós** também não temos

Do `classic-ro-client-gap-checklist.md`, o que não está na nossa auditoria ou estava só implícito:
- **Stealth completo:** esconder nomes, emblemas, aura, carrinho e falcão de quem está escondido; não escolher alvo escondido; limites de skills em Hide; Chase Walk.
- **Fluxos de skill que travam se o cliente não responder:** Autospell (0x01ce), Feel do Star Gladiator, Token of Siegfried (0x0292), Dori-dori.
- **Barra de progresso do NPC** (0x02f0–0x02f2), contagem regressiva do servidor (ShowDigit), aviso de MVP/boss e tempo de respawn.
- **Senha do armazém** (prompt, mudança, bloqueio).
- **Rankings** do Ferreiro e do Alquimista (top 10) e aviso de fama.
- **Persistência das janelas** (posição, aberta/fechada, recolhida, linhas do atalho) e limite à tela ao mudar resolução.
- **Atalhos físicos** (Alt+E/Q/S/A/Z/H/W/C/V/J/R, Ctrl+R, O), `/bm` (Battle Mode), Alt+M, Alt+1..0 como comandos de chat.
- **Mapa que falta:** erro claro com o nome do mapa e volta para a seleção (e opção de teleportar para Prontera), em vez de tela preta.
- **Quantidade ao largar item empilhado**; **leitor de livro** dentro do jogo; sombra do item no chão.
- **Avisos de pontos disponíveis** (atributo e skill), que abrem a janela correspondente.
- **Monster Info** (Sense): nome, nível, HP, DEF, MDEF, raça, tamanho, elemento, resistências.
- **Cut-in** de NPC em esquerda/centro/direita, com limpeza ao trocar de mapa.
- **Mapa-múndi** com estrela no mapa atual e membros do grupo em outros mapas.
- **Efeitos:** distorção de alucinação (Hallucination), fade do tom noturno do Soul Link, lista legível por máquina dos IDs de efeito sem implementação.
- **Validar antes de implementar** (versão-sensível): cooldown global de skill, relógio nos ícones de status, party booking, killer ranking.

## 3. Efeitos de mapa e clima por mapa (goro) — fecha as nossas lacunas de mundo

IDs de efeito colocados no próprio `.rsw` (não vêm de pacote): fumaça (44, chaminés de Prontera), vaga-lume (45), tocha (47) e variantes colorida (690/691/696),
bolhas de Byalan (109), fumaça de casa (373), balão de Comodo (165), pilar de luz (231), fantasma (692), brilhos (693/694/695), gota azul (665), arco-íris (410).
Clima por mapa: neve em `xmas`, fogos em `comodo`, nuvens (229/230/233/515/516/592/697/698) em Geffen/Juno/Valquíria/Einbroch/dirigíveis/torres e cores de céu
especiais; chuva em Payon (o renderizador existe mas vem desligado). Regras: posições na escala 0,2 do GAT/GND com eixo vertical invertido; comportamento escolhido pelo
nome lógico do mapa; separar "objeto de efeito do RSW" de "clima do mapa". Esses IDs servem direto de lista de tarefas para o nosso item "efeitos dos objetos do mapa".

## 4. Sprites e animação (goro)

Itens abertos deles que também são nossos: ação central de estado do ator; dados de ação por skill (postura por skill); respeitar metadados de animação
(frame fixo, `play=false`, ação seguinte); **retomar a caminhada depois de apanhar**; **tempo de ataque por classe/sexo/arma/motion** (não só a velocidade do pacote);
estados do corpo (congelado, pedra, blade stop, raiz).

## 5. nostalro-client — o que confirma

Lista de funções "implementadas e validadas" (packetver 20120307): inventário/equipamento com paperdoll, item info com carta, carta/slot, quantidade ao largar,
leitor de livro, janela de guilda completa (emblema, aliança, expulsar com motivo), vending completo (montar, minha loja, quadro, comprar), janela do carrinho,
**janela "make item" única para poção, arma, flecha e refino**, mapa que falta, opções gráficas (névoa/efeito/aura em tempo de execução), pets completos
(domar, roleta, chocar, acessório, performance, fala), homúnculo e mercenário com IA própria, falcão, carrinho, Hide/Cloaking/Chase Walk, casamento/adoção
(roupa de casamento, nome do parceiro, bebê), quest com marcadores no minimapa e mapa-múndi com grupo.
Útil: eles resolveram forja, poção, flecha e refino com **uma janela só de lista de itens fabricáveis** — é o desenho que devemos copiar para a Etapa C.

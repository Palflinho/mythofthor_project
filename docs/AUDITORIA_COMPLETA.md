# Auditoria completa de funções do Ragnarok Online (pré-renovação)

Criada em 2026-10-12 a pedido do usuário, depois de várias "coisas básicas" aparecerem faltando (munição, falcão, buffs sem duração).
Objetivo: lista única de **tudo que existe no RO**, não só telas, com o estado de cada item no nosso cliente e no servidor.
Esta é a fonte de planejamento: o `ROADMAP.md` aponta para cá e cada item vira tarefa quando for atacado.

## Como esta lista foi feita (e quão confiável é)

1. Lista de funções montada a partir do jogo (clássico até as classes transcendidas) e do código do rAthena (`clif.cpp`, `packets.hpp`, `npc/`).
2. Cada função foi comparada com o código do cliente (`Source/MythOfThor`): procurei o pacote ou a palavra-chave. **Sem nenhum trecho = ⬜ (não existe).**
   Com trecho = 🟡 (existe, mas **não** significa que funciona nem que está fiel: só ✅ depois de eu percorrer o fluxo no jogo e comparar com o cliente original).
3. Também rodei um levantamento de pacotes do servidor que o cliente nunca referencia: 180 dos 328 pacotes `ZC_*` documentados em `clif.cpp` não aparecem
   no nosso código (muitos são versões antigas de pacotes que o PACKETVER 20250402 troca por outros; o resto está nas tabelas abaixo).
4. A busca por palavra-chave erra para os dois lados. Por isso a 2ª etapa (abaixo) é verificar **no jogo, uma função por vez**.

Legenda: ✅ feito e verificado · 🟡 existe no código, não verificado/incompleto · ⬜ não existe · ➖ fora do escopo (classes além da transclasse, cash shop, RO Premium).
Prioridade: **P0** = laço básico do jogo (quem joga 1 hora precisa), **P1** = precisa antes do teste fechado, **P2** = depois.

## 1. Combate e skills

| Função | Estado | Servidor | Prio | Notas |
|---|---|---|---|---|
| Ataque básico corpo a corpo, esquiva, crítico, miss | 🟡 | ok | P0 | números de dano existem; conferir tempo de ataque (ASPD) por arma |
| Ataque à distância com arco (flecha visível, consumo de munição) | 🟡 | ok | P0 | flecha do jogador existe; munição equipada (0x013c) e aviso "sem flechas" (0x013b) feitos em 2026-10-13 (`a91f563`) |
| Skills de alvo, de chão e de si mesmo | 🟡 | ok | P0 | |
| Barra de cast, cast interrompido, cast sem barra | 🟡 | ok | P0 | |
| Skills com **menu depois do uso** (lista de alvos/receitas/ materiais) | 🟡 | ok | P0 | 2026-10-13 (`4634407`): janela de lista para Portal, Lupa, carta, Fabricar Flechas, Forjar/Farmacologia (sem pedras), Autoconjurar e Refinar arma. Faltam Reparar arma (0x0b65), Hocus-pocus, Detecting, Create Elemental, Scribble |
| Skills de combo (Monge, Ferreiro) e `ZC_COMBODELAY` (0x01d2) | ⬜ | ok | P1 | sem tratamento do atraso de combo |
| Skills de quest / platinum (NPCs que ensinam) | 🟡 | scripts presentes | P1 | |
| Animações de skill (STR, SPR, sprites, sons) | 🟡 | – | P0 | 575 efeitos mapeados; ver `docs/EFEITOS_PENDENTES.md` (SPR/QuadHorn/TRAIL, armadilhas e canções faltam) |
| Efeitos de status na unidade (veneno, congelado, pedra, atordoado, sono, maldição, silêncio, cego) | 🟡 | ok | P0 | tintas e adereços feitos; ícones de status com tempo corrigidos hoje |
| Skill de teleporte/portal (Warp Portal lista, Memo/Remember) | 🟡 | ok | P0 | lista do Portal (0x0abe) e resultado do Memo (0x011e) feitos em 2026-10-13; falta testar o Portal com Noviço |
| Skills de passiva (aparecem, sem uso) | 🟡 | ok | P1 | |
| Skills de armadilha e unidades no chão | 🟡 | ok | P1 | `MythSkillEffects`; faltam arquivos de armadilha no GRF |
| Pós-delay (0x043d), delay global, "skill em cooldown" | ⬜ | ok | P1 | |
| Dano elemental/ colorido, "Miss", "Lucky", cura em verde | 🟡 | ok | P1 | |
| Mensagens de falha de skill (`ZC_ACK_TOUCHSKILL`, SP insuficiente, sem item) | 🟡 | ok | P0 | motivos com as mensagens do próprio cliente (msgstringtable, mapa do roBrowser) desde 2026-10-13 |
| Kill steal / alvo preso (lock) e "/attackmode" | ⬜ | ok | P2 | |
| MVP: mensagem, prêmio, EXP especial (0x010b/0x010c/0x010d) | ⬜ | ok | P1 | só o servidor decide; o cliente nem mostra o aviso |
| Efeitos de refino ≥ +7 (brilho nas armas/armaduras) | ⬜ | ok | P1 | `ZC_EQUIPMENT_EFFECT` 0x0a3b |

## 2. Sistemas por classe

| Classe / sistema | Estado | Prio | Notas |
|---|---|---|---|
| **Ferreiro: forja** (martelos, lista de forja, chance, resultado) | ⬜ | P1 | pacotes 0x018d (lista) → 0x018e (pedido) → 0x018f (resultado); sem janela |
| **Ferreiro: refino** por NPC (Hollgrehenn) e por skill (Weapon Refine) | 🟡 | P1 | NPC funciona pelo diálogo; skill precisa da lista 0x0221 |
| **Ferreiro: reparar** (0x01fc/0x01fe) | ⬜ | P1 | |
| Ferreiro: ranking, pontos de ranking (0x0219/0x021b) | ⬜ | P2 | |
| **Alquimista: poções** (Pharmacy, Bioethics, Marine Sphere, Cannibalize) | ⬜ | P1 | mesma lista de materiais |
| **Alquimista: carrinho, vender no carrinho (Vending)** | 🟡 | P1 | carrinho e loja de jogador feitos em 2026-10-13 (`e3211c2`, `f219741`), testados com 2 clientes; falta a arte do LATAM |
| **Mercador: carrinho** (janela, mover itens 0x0121/0x0124/0x0125) | 🟡 | P0 | janela de carrinho (listas, mover, peso/itens) em 2026-10-13 (`e3211c2`); loja de jogador (Vending) feita em 2026-10-13 (`f219741`); falta a arte do LATAM |
| **Mercador: loja do jogador** (abrir, comprar de outros) | ⬜ | P1 | 0x012d/0x0131/0x0136 |
| Mercador: Buying Store (0x0812) | ⬜ | P2 | |
| **Caçador: falcão** (aparecer, comando Blitz Beat, Detect, Steel Crow) | 🟡 | P0 | aparece e vira; **botão de soltar** feito hoje; faltam o ataque do falcão (Blitz Beat) e o aviso de "falcão voltou" |
| Caçador: armadilhas (colocar, visual, disparar) | 🟡 | P1 | |
| Caçador: Arrow Making / Phantasmagoria | ⬜ | P1 | lista de materiais 0x01ad |
| **Cavaleiro: Peco Peco** (montar, sprite montado, alugar) | 🟡 | P0 | 2026-10-13: corpo montado pelo bit 0x20 (testado com @mount), botão "off"; falta testar o aluguel no NPC |
| Cruzado: Peco Peco, Grand Cross, Devotion, Providence | 🟡 | P1 | |
| **Mago:** Warp Portal, Memo, Estalo de gelo etc. | 🟡 | P0 | |
| **Sacerdote:** Heal, Aqua Benedicta, Sanctuary, Resurrect, Warp, Impositio | 🟡 | P0 | |
| **Monge:** esferas (feito), Asura, Combo, Finger Offensive | 🟡 | P1 | |
| **Sábio:** Hocus-pocus, Autospell (lista 0x01cd), Dispell | ⬜ | P1 | |
| **Ladrão/Assassino:** Steal, Hide (invisível), Envenom, Cloaking | 🟡 | P1 | Hide parcial (opacidade); Steal sem resultado visível? conferir |
| **Bardo/Odalisca:** canções, dança em dupla, instrumentos | ⬜ | P1 | sem sprites de instrumento e sem animação da música |
| Aprendiz/Super Aprendiz, Primeiros Passos (Training Grounds) | 🟡 | P0 | |
| Trocar de classe (Job change) nos NPCs originais | 🟡 | P0 | Aprendiz → Espadachim pelo NPC de Izlude testado de ponta a ponta em 2026-10-13 (Renovação); faltam as outras classes |
| Taekwon/Star Gladiator/Soul Linker, Ninja, Gunslinger | ➖/decidir | P2 | fora da regra "até transclasse"? decidir com o usuário |

## 3. Companheiros e montarias

| Função | Estado | Prio | Notas |
|---|---|---|---|
| **Pets:** capturar (0x01a0/0x01a2), ovo, janela do pet, alimentar, performance, fome, intimidade | ⬜ | P1 | zero código; sprite do pet seguindo o dono também não existe |
| **Homúnculo:** criar, janela, alimentar, skills, evolução | ⬜ | P1 | zero código |
| **Mercenário:** contratar, janela, skills | ⬜ | P2 | |
| Carrinho: sprite | ✅ | – | |
| Falcão: sprite | ✅ | – | |
| Peco Peco: sprite montado, velocidade, soltar | 🟡 | P0 | sprite montado e soltar feitos em 2026-10-13 (`33c251d`); velocidade é do servidor |
| Warg/Lobo (Ranger), Dragão (Rune Knight), Madogear | ➖ | P2 | 3ª classe |

## 4. Itens e inventário

| Função | Estado | Prio | Notas |
|---|---|---|---|
| Janela de itens (abas, favoritos, descarte, contagem) | 🟡 | P0 | |
| Usar item, comer, poções com cooldown (cooldown visual?) | 🟡 | P0 | |
| Equipar/desequipar tudo, cabeças, capas, escudo, ambas as mãos, 2 acessórios | 🟡 | P0 | incluir **posições alternativas** (arma de duas mãos x escudo) |
| **Munição** | 🟡 | P0 | munição real pelo 0x013c e aviso de falta (2026-10-13); falta troca automática ao acabar |
| Cartas: encaixar (0x017a/0x017d) e nome composto ("Espada [Soldado]") | 🟡 | P0 | encaixar feito (0x017a/0x017b/0x017c/0x017d, testado); falta o nome composto |
| Identificar item (0x0177 lista), Refinar (Oridecon), Reparar | 🟡 | P0 | Lupa/Avaliação de item e Refinar arma (skill) em 2026-10-13; falta Reparar e o refino do NPC |
| Itens de ovo/pet, itens de invocação (Bolsa) | ⬜ | P1 | |
| Itens de evento / cash / disfarces | ➖ | P2 | ~96 itens sem recurso no cliente |
| Dica do item com cores, slots e peso | 🟡 | P0 | |
| Armazém (Kafra) | 🟡 | P0 | |
| Cesto/Venda para NPC | 🟡 | P0 | |
| Peso: 50% regeneração, 90% sem combate | 🟡 | P0 | decisão do servidor; ícone existe |
| Drop de itens no chão: animação, nome, brilho por raridade | 🟡 | P0 | |
| Compartilhamento de EXP/itens (grupo: Each Pick, Shared) | ⬜ | P1 | |
| Comparar itens, tooltip no ícone | ⬜ | P2 | |

## 5. NPCs, economia e mundo

| Função | Estado | Prio | Notas |
|---|---|---|---|
| Diálogo de NPC (mes/next/close), menu, **entrada de texto e de número** | ✅ | P0 | entrada de número e de texto (0x0142/0x01d4 → 0x0143/0x01d5) em 2026-10-13 (`a43bf70`), testada |
| Imagens de NPC / cutin (0x01b3) e mapas de imagem | ⬜ | P1 | |
| Lojas NPC (compra, venda, desconto de Mercador/ Overcharge) | 🟡 | P0 | |
| Kafra (salvar, armazém, teleporte, carrinho) | 🟡 | P0 | |
| Portais e warps de cidade; teleporte de NPC | 🟡 | P0 | |
| NPCs com sprite animado/decoração, emblemas | 🟡 | P0 | |
| Barcos, dirigíveis, elevadores, trens (warps especiais) | ⬜ | P2 | mapas não importados |
| Mapas: água, ventos, nuvens, árvores animadas | 🟡 | P1 | |
| **Efeitos dos objetos do mapa** (RSW: fogo, fumaça, luzes, chafariz) | ⬜ | P1 | nenhum trecho de código |
| **Sons ambientes do mapa** (RSW) | ⬜ | P1 | só a BGM por mapa existe |
| BGM por mapa, trocas de volume | 🟡 | P0 | |
| Clima por mapa (neve, chuva, sakura) | 🟡 | P1 | já no backlog do HD-2D |
| Mapa-múndi e posição no minimapa | 🟡 | P1 | mapa-múndi gráfico (Ctrl+`) em 2026-10-13 (`9ce93a7`): mapa atual marcado, nome sob o mouse, clique abre a rota; faltam membros do grupo no mapa |
| Castelos e Guerra do Emperium (WoE), emblemas nos castelos | ⬜ | P2 | |
| Eventos sazonais (natal, verão...) | ➖ | P2 | |
| Quests (menu e rastreador) | 🟡 | P0 | |
| Quests de classe/skill (NPCs originais) | 🟡 | P0 | |
| Barcos para ilhas, instâncias | ➖ | P2 | |

## 6. Social

| Função | Estado | Prio | Notas |
|---|---|---|---|
| Chat: normal, cochicho, grupo, guilda, global (broadcast), comandos `/` | 🟡 | P0 | guilda sem janela |
| Emoções (`/e1`…) e emotes de ação (doridori, bingbing, /sit) | 🟡 | P0 | corrigidas camadas hoje; usuário reportou bugs, não reconfirmou |
| Grupo (criar, convidar, EXP, HP dos membros, mapa) | 🟡 | P0 | |
| **Guilda:** criar, janela (membros, cargos, habilidades, avisos), emblema, aliança/hostil | ⬜ | P1 | só o nome na unidade existe |
| Amigos, ignorar | 🟡 | P1 | |
| Troca entre jogadores | 🟡 | P0 | testada com 2 clientes |
| PvP (arena, placar) e `/duel` | ⬜ | P2 | |
| Casamento, adoção, bebê | ⬜ | P2 | |
| Ranking (Ferreiro, Alquimista, PK, Taekwon) | ⬜ | P2 | |
| Mensagens de sistema do cliente (`msgstringtable`, 0x0291) | ✅ | P0 | 2026-10-13: 0x0291/0x07e2/0x09cd/0x07e6 com a msgstringtable do LATAM; mensagens do próprio servidor em português (pacote POR do rAthena) |

## 7. Interface e conforto

| Função | Estado | Prio | Notas |
|---|---|---|---|
| Info básica, hotbar, chat, minimapa, atributos, equipamento, itens, skills | 🟡 | P0 | refeitos com arte LATAM |
| Atalhos de teclado, rebind (F12) | 🟡 | P1 | |
| Janelas minimizáveis, arrastar, empilhar | 🟡 | P0 | minimizar entregue hoje |
| Opções (gráficos, som, interface) | 🟡 | P1 | |
| Tutorial / tela de ajuda / dicas | ⬜ | P2 | |
| Notificações (nível, item raro, MVP) | ⬜ | P1 | |
| Seleção de personagem, criação, **excluir personagem**, PIN | 🟡 | P1 | excluir falta |
| Reconexão e mensagens de queda | ⬜ | P1 | |
| Anúncio do servidor (`@broadcast`, relógio) | 🟡 | P2 | |

## 8. Sistema e rede

| Função | Estado | Prio | Notas |
|---|---|---|---|
| Login, char, mapa, mudança de mapa | ✅ | – | |
| Ping/keepalive, "Pong", detecção de queda | 🟡 | P1 | |
| Pacotes do servidor ignorados sem aviso (rede) | ⬜ | P0 | **passo 0 da auditoria:** logar todo pacote desconhecido e abrir uma lista por frequência |
| GM: comandos, `@` e painéis | ➖ | – | só desenvolvimento |

## Plano de execução (etapas)

1. **Etapa A, mapa de pacotes desconhecidos (pequena, grande valor).** Ligar um registro de "pacote recebido e sem tratamento" no cliente e jogar com
   o personagem de teste (e com um Ferreiro, Alquimista, Mercador, Caçador, Cavaleiro, Sábio e Monge): a lista real do que o servidor manda e o cliente ignora.
2. **Etapa B, laço básico P0 percorrido no jogo**, uma função por vez, comparando com o cliente original: itens, equipar, munição, skills e menus,
   NPCs com `input`, Kafra, Warp Portal, Memo, Peco Peco e carrinho. Corrigir o que faltar e só então marcar ✅.
3. **Etapa C, sistemas P1 por classe:** forja/pharmacy/arrow-making (a mesma janela de lista), carrinho e loja do jogador, pets, guilda, homúnculo.
4. **Etapa D, mundo:** efeitos e sons do RSW, mapa-múndi, clima.
5. A cada etapa: atualizar esta tabela, `ROADMAP.md` e `TESTE_JOGADOR.md`.

Referências para consulta (sem copiar código GPL; só comportamento e lista de recursos): `nmeylan/nostalro-client` (Rust, pré-renovação),
`kivutar/goro` (Go, pré-renovação), `open-midgard`, `roBrowserLegacy`, `korangar` e os tópicos do fórum do rAthena que o usuário indicou
(os tópicos devolvem 403 para leitura automática; o conteúdo vem do usuário).

## Etapa A, 1ª coleta (2026-10-12): pacotes que o servidor mandou e o cliente ignorou

O cliente agora grava cada pacote sem tratamento, uma vez, em `Saved/Logs/pacotes_sem_tratamento.txt` (id e tamanho). Sessão curta: login, `@warp`,
`@item`, 3 Porings, ataque, `@heal`, F1. Resultado (17 ids):

| Id | Nome no rAthena | O que faz | Impacto |
|---|---|---|---|
| 0x013b | ZC_ACTION_FAILURE | motivo de falha da ação (sem munição, peso, ...) | **P0:** é a mensagem "equipe a munição" do arco |
| 0x013a | ZC_ATTACK_RANGE | alcance de ataque da arma atual | **P0:** o cliente usa alcance fixo; arco/lança precisam do valor do servidor |
| 0x099b | ZC_MAPPROPERTY_R2 | propriedades do mapa (PvP, GvG, sem teleporte...) | P1 |
| 0x02d9 / 0x02da | ZC_CONFIG / ZC_CONFIG_NOTIFY | opções do servidor (ex.: permitir ver o equipamento) | P2 |
| 0x07d8 | ZC_REQ_GROUPINFO_CHANGE_V2 | regras de EXP e item do grupo | P1 |
| 0x02c9 | (resposta de grupo) | recusa/aceite de mudança de regras do grupo | P2 |
| 0x0ade | (porcentagem) | contador de peso/ bônus de servidor | P2 |
| 0x0adf | ZC_ACK_REQNAMEALL2-like | nome completo com guilda/título | P1 |
| 0x0b8d | ZC_REPUTE_INFO | reputação | P2 (sem sistema) |
| 0x007f | ZC_NOTIFY_TIME | relógio do servidor (usado por buffs e ping) | P1 |
| 0x082d | (lista de personagens, 29 bytes) | cabeçalho do char server | ok, só informativo |
| 0x09a0, 0x020d, 0x08b9, 0x0283, 0x0b18 | a identificar | – | identificar no `clif.cpp` |

Repetir a coleta jogando cada classe/sistema (Ferreiro, Alquimista, Mercador com carrinho e loja, Caçador com armadilhas, Sábio, Monge, Cavaleiro montado,
pet, guilda) para completar a lista.

## Itens adicionados depois de ler goro e nostalro (ver `docs/ref/REFERENCIAS_GORO_NOSTALRO.md`)

| Função | Estado | Prio |
|---|---|---|
| Armazém com senha (0x023a–0x023c) | ⬜ | P2 |
| Excluir personagem (0x01fb) | ⬜ | P1 |
| Barra de progresso de NPC (0x02f0–0x02f2) e contagem do servidor (ShowDigit) | ⬜ | P1 |
| Monster Info / Sense (0x018c) | ⬜ | P1 |
| Cut-in de NPC (0x01b3) | ⬜ | P1 |
| Ver equipamento de outro jogador (0x02d6–0x02d8) | ⬜ | P2 |
| Quantidade ao largar item empilhado; leitor de livro; sombra do item no chão | ⬜ | P1 |
| Avisos de pontos de atributo/skill disponíveis (abrem a janela) | ⬜ | P1 |
| Stealth completo (esconde nome/aura/carrinho/falcão, sem alvo, limites de skill) | 🟡 | P1 |
| Fluxos de skill que travam sem resposta (Autospell 0x01ce, Dori-dori 0x01e7, Token of Siegfried 0x0292, Feel) | ⬜ | P1 |
| Persistência de janelas (posição, aberta, recolhida) e limite à tela | ⬜ | P1 |
| Mapa que falta: erro claro e volta à seleção | ⬜ | P1 |
| Tempo de ataque por classe/sexo/arma e retomar a caminhada depois de apanhar | 🟡 | P0 |
| Efeitos de mapa do RSW por ID (fumaça 44, vaga-lume 45, tocha 47, bolhas 109, pilar 231, brilhos 693–695...) | ⬜ | P1 |
| Nuvens e neve/fogos/chuva por mapa; cores de céu especiais | ⬜ | P1 |
| Alucinação (distorção de tela) e fade do tom noturno do Soul Link | ⬜ | P2 |
| Cooking (0x025a/0x025b) e Cash NPC (0x0288) | ⬜/➖ | P2 |

## Itens adicionados depois da leitura de `nostalro/client/src` (ver `docs/ref/NOSTALRO_COMPARACAO.md`)

| Função | Estado | Prio |
|---|---|---|
| Itens no chão com **sombra** (desligada no nosso código por engano?) e curva de queda do cliente | 🟡 | P0 |
| Sub-célula real do item (`subX/subY` do servidor) em vez de jitter inventado | 🟡 | P0 |
| Ordem de prioridade dos cliques (diálogo aberto ignora o mundo, sentado vira, skill fora de alcance anda e lança, Shift, pet não é atacado) | 🟡 verificar | P0 |
| Glifo de início de cast e mira vermelha/branca no chão; cooldown global/por skill; tabela de falhas de skill (inclui "precisa de item X") | 🟡 verificar | P0 |
| Tempo de ataque por ASPD/arma; recarga de ataque; retomar a caminhada depois de apanhar | 🟡 verificar | P0 |
| Arma/escudo: o servidor manda o visual da mão antes da lista de equipamento; reconstituir ao chegar a lista | 🟡 verificar | P0 |
| Troca de mapa: limpar armazém, troca, skill pendente, números de dano; reaplicar falcão/carrinho/montaria | 🟡 | P0 |
| Falcão: seguir atrás com suavização, voo de ataque (Blitz Beat), ciclo fixo de 500 ms | ⬜ | P1 |
| Sons por transição de estado (congelar, pedra, atordoar, veneno, maldição, silêncio, confusão, cegueira) e cegueira com tela | 🟡 verificar | P1 |
| Aura por bit de opt3 (e remoção com explosão), Blade Stop como raiz | ⬜ | P1 |
| Chefe: aura, marca no minimapa e aviso no chat; auras de nível/detecção/rank PvP | ⬜ | P1 |
| Fade ao sair de vista e "poof" ao teleportar/deslogar | ⬜ | P2 |
| Janela única de lista de fabricação com contexto e **pré-carga dos ícones** (identificar, arrow making, autospell, refino, reparo, forja, poção) | ⬜ | P1 |
| Opções `show_other_damage` e `show_miss`; efeito "Guard" de Kyrie/Parry | ⬜ | P2 |
| Avisos: bind on equip, recusas de pegar item por código de peso/contagem | ⬜ | P1 |

## Itens do "Current Scope" do goro (colado pelo usuário em 2026-10-12) que ainda não estavam na lista

| Função | Estado | Prio |
|---|---|---|
| Andar segurando o clique (movimento contínuo) | 🟡 verificar | P0 |
| Cancelar a caminhada e cancelar o cast (clicar de novo / andar) | 🟡 verificar | P0 |
| Noshift / Noctrl (opções de alvo), cursor que gruda no alvo (cursor snap) | ⬜ | P1 |
| Janelas modais de skill: Teleporte (escolher destino/ponto), Warp Portal e escolha de visual do carrinho | 🟡 / ⬜ | P0 / P1 |
| Dicas (tooltip) de itens e de skills ao passar o mouse | 🟡 | P1 |
| Metadados dos ícones de status (nome/descrição por ícone, do roBrowser) | 🟡 | P1 |
| Ver equipamento de outro jogador (janela) | ⬜ | P2 |
| Contador e alvo de PvP clássico; WoE com comportamento do cliente 2008 | ⬜ | P2 |
| Lightmaps, névoa do RSW (perto/longe), modelos animados do mapa, modelos 3D Granny de NPC | 🟡 / ⬜ | P1 |
| Carregamento: mapa coberto de preto, 1º quadro pré-aquecido e fade | ⬜ | P1 |
| Mapa-múndi clássico com jogadores/grupo e prévias do minimapa; marcadores de NPC/guilda no minimapa | 🟡 | P1 |
| Journal de missão com descrição, progresso de caçada e limite de tempo | 🟡 | P1 |
| Barra de atalhos de várias linhas com teclas clássicas; `/bm` (Battle Mode), digitar direto, F12 troca a linha | 🟡 | P1 |
| Console (janela de log de sistema) e medidor de FPS | ⬜ | P2 |
| Sprites de casamento, sprites de arma por item, **bebê** e classes expandidas (Taekwon/SL/Ninja/Gunslinger) | ⬜ / decidir | P2 |
| Teclado independente de layout (tecla física) e entrada de texto | ⬜ | P2 |
| Script Lua de personagem (automação): não entra no nosso escopo | ➖ | – |

## Dados que já temos no GRF LATAM e não extraímos (ver `docs/ref/ANALISE_DATA2026.md`)

| Tabela do GRF | Destrava | Prio |
|---|---|---|
| `mappostable.txt` | mapa-múndi gráfico | P1 |
| `viewpointtable.txt` | câmera por mapa (torres, naves...) | P1 |
| `fogparametertable.txt`, `mapobjlighttable.txt`, `mapskydata.lub` | névoa, luzes e céu por mapa | P1 |
| `effecttool\<mapa>.lub` (160 arquivos) | efeitos dos objetos do mapa (fumaça, tochas, brilhos) | P1 |
| `skilldelaylist.lub` | cooldown/atraso por skill | P0 |
| `footprinteffectinfo.lub` | pegadas | P2 |
| `ridingspreditinfo.lub`, `spreditinfo\*.lub` | posição das camadas, inclusive montado (Peco Peco) | P0 |
| `skilleffectinfolist.lub` | efeito de cada skill | P1 |

# nostalro-client x Myth of Thor (leitura de `client/src` em 2026-10-12)

Leitura de `client/src/` (eventos, `game_updates`, `sprite`, entrada de mouse, `ui`) do https://github.com/nmeylan/nostalro-client (Rust, Apache-2.0, packetver 20120307).
Li: `sprite/falcon.rs`, `sprite/floor_item.rs`, `events/{production,inventory,skill,npc,entity,lifecycle}.rs`, `game_updates/{items,combat,animation}.rs`,
`input_action/click.rs` e o `docs/TODO.md`. Não li: `events/{pet,guild,party,mail,marriage,adoption,trade,storage,quest,companion,friends}.rs`, `ui/*`, `scene/*`,
`sound.rs`, `hover.rs`, `command.rs` (próxima leitura quando a função entrar). Só comportamento e valores; sem copiar código.
Onde digo "nosso", falo do que sei do nosso código; o que não conferi está marcado **verificar**.

## 1. Falcão (`sprite/falcon.rs`)

- Posição: âncora **1,5 célula atrás** do dono (oposto à direção em que ele olha), 25 unidades acima do chão; não fica encostado nele.
- Movimento: suaviza em direção à âncora (exponencial, taxa 3,0/s); se estiver a mais de 60 unidades (teleporte/troca de mapa) **teleporta** para a âncora.
- **Voo de ataque:** 0,42 s em linha reta até o alvo, altura em arco senoidal com pico de 14 unidades, ação 1 (ataque) em loop; depois volta a seguir.
- Estados: "hover" (parado) e "fly" (dono andando), mesma ação, só muda o ritmo; **ciclo de bater asas fixo em 500 ms** (não acelera com a câmera).
- A direção do sprite vem do movimento dele (mantém a última se parado) combinada com a direção da câmera.
- Sprite por `falcon_sprite_path(job)` (um por classe: Caçador, Atirador...), só a camada do corpo; um falcão por dono, some quando o dono some.
- **Nosso:** preso ao personagem em posição fixa (pousado), 8 direções pela direção do dono, **sem seguir atrás, sem voo de ataque, sem suavização, sem 500 ms fixos.**
  O usuário pediu "em cima do personagem"; manter assim a menos que ele prefira o comportamento do original (atrás, flutuando). Falta o ataque (Blitz Beat).

## 2. Itens no chão (`sprite/floor_item.rs`, `game_updates/items.rs`)

- **Têm sombra:** uma sombra compartilhada (`shadow_sprite`) desenhada embaixo de cada item (e embaixo do item enquanto cai, no chão).
  O goro também lista "sombra do item no chão" como algo que o RO original tem. **Nosso:** `MythGroundItem` desliga a sombra com o comentário "no cliente original o
  item no chão não tem a sombra redonda": **provavelmente errado nos dois repositórios**, verificar no cliente real e ligar.
- Queda: `t = tempo/24 ms` (quadros), `fall_offset = -15 + (-0.6 + 0.083·t)·t`, termina quando ≥ 0. Nosso arco/quique é outro: trocar por esta curva.
- O pacote traz `sub_x`/`sub_y` (posição dentro da célula); o código deles só guarda os valores. **Nosso:** inventei um "jitter" por hash; usar o `subX/subY` real do servidor.
- Ordenação por profundidade (maior primeiro) e retângulo de clique de meia-largura 17·escala.

## 3. Produção e fabricação (`events/production.rs`)

Uma **janela de lista única** (com um "contexto") serve para: Identificar (lista de índices), Fazer flecha / Conversor elemental (arrow making, create-con), Auto Spell,
Refinar arma, Reparar arma, Fabricar item (forja/pharmacy). Detalhes úteis:
- Lista vazia mostra a mensagem "não é possível fabricar" (msg 0x1a8) em vez de abrir janela vazia.
- Resultado de refino/forja/poção: efeitos `Refineok`/`Refinefail`/`PharmacyOk`/`PharmacyFail` + mensagem; refino de arma: 4 códigos (sucesso, falha, recusa por nível, recusa por material).
- **Os ícones dos itens da lista são pré-carregados** (sem isso a janela abre com ícones em branco).
- Estado transitório: `pending_list_skill` guarda qual skill abriu a lista (decide o tipo de janela); `pending_shop_name` leva o nome da loja para a janela.
- Venda de jogador (vending): abrir (montar), quadro com título sobre a cabeça (personagem senta), comprar (resultados: ok, sem zeny, sem peso, sem estoque),
  decremento de estoque com linha no chat, fechar a própria loja.
- **Nosso:** nada disso existe (ver `AUDITORIA_COMPLETA.md`, seções 1, 2 e 5).

## 4. Inventário e equipamento (`events/inventory.rs`)

- Ao equipar com sucesso, o sprite muda (`view_id` ≠ 0). **O servidor manda o visual da arma antes da lista de equipamento**: eles reconstituem arma/escudo (`refresh_player_hand_look`)
  quando a lista chega e só recarregam o sprite se algo mudou. **Nosso: verificar** (arma/escudo trocando de visual na ordem errada).
- Aviso "vincula à conta ao equipar" (bind on equip); mensagens de recusa de pegar item por código (peso, genérico, contagem...).
- Carrinho tratado à parte (listas, adicionar/remover, contagem/peso, `cart off` remove o carrinho e o visual).
- Encaixar carta: lista de equipamentos elegíveis → caixa de diálogo → resultado (consome 1 carta).
- Munição não tem tratamento especial no cliente deles: passa pelos caminhos comuns. (Confirma que o que fizemos é só interface; o servidor decide.)

## 5. Skills (`events/skill.rs`)

- `ZC_USESKILL_ACK` chega para **todo** uso (inclusive cast instantâneo) e inicia o **glifo de início de cast** (círculo no chão com duração = tempo de cast; sem círculo para cast 0).
- Cast em alvo: mira (reticle) no alvo; cast no chão: **mira vermelha se o conjurador é hostil, branca se não**. Cancelar limpa tudo e dispara o efeito de contra-ataque quando cabe.
- **Cooldown global e por skill** bloqueia o cast (mas continua entrando no modo mira). Skills disparadas por item ignoram o cooldown do conjurador.
- Falha de skill: limpa todo estado pendente; "precisa de item" monta a mensagem com o nome do item e a quantidade; as demais causas usam uma tabela causa → mensagem; sem mapeamento só registra.
- Skill de chão: efeito por skill+nível na célula, na altura do terreno; Slim Potion Pitcher arremessa a poção antes de espirrar; armadilhas são alvo de "unidade de skill".
- Lista de skills: recebida, adicionada, atualizada (nível, custo de SP, alcance, upável).
- **Nosso: verificar** o glifo de cast, a cor da mira (hostil/amigo), o cooldown e a tabela de mensagens de falha (hoje só tratamos parte).

## 6. Entidades e estados (`events/entity.rs`)

- Aparecer: fade de entrada (e efeito de entrada para jogadores); sair de vista = fade, teleporte/logout = efeito "poof" (suprimido para escondidos).
- **Estados do corpo e da saúde com som na transição** (congelar, sair da pedra, atordoar, envenenar, maldição, silêncio, confusão, cegueira); cegueira com sobreposição de tela; estado que impede de andar para o movimento.
- **opt3 é uma palavra de bits:** compara com o valor anterior, bit novo = aura, bit limpo = remove aura e dispara explosão de saída; Hide esconde auras; tabelas de reação diferentes para jogador e monstro;
  **BLADESTOP** prende (raiz) o alvo e solta ao limpar.
- Status por ícone com aura enquanto durar, som de ativação opcional, filtro noturno (Soul Link), ondulação de tela, push cart.
- Mudança de visual: tipo 0 = classe (fecha árvore de skills e avisa), tipo 2 = mão (arma/escudo, reconstituída para o jogador).
- Monstros: chefe com aura e marca no mapa + aviso no chat; **o som do golpe vem da classe da arma do atacante.**
- Auras: nível, chefe, detecção (Sight/Ruwach), rank de PvP.
- **Nosso: verificar** sons por transição de estado, aura por bit de opt3, raiz do Blade Stop, aviso/ marca de chefe, aura de detecção. Não tratamos o fade de saída de vista nem o "poof".

## 7. Combate e animação (`game_updates/combat.rs`, `animation.rs`)

- Pedido de ataque repetido com recarga de **0,3 s**, permitido em Standing/ReadyFight/Moving/Hurt; trocar de alvo não espera o fim do golpe. Fora de alcance, anda até o alvo.
- Dano: tempo agendado por golpe; **configurações `show_other_damage` e `show_miss`**; "Guard" quando Kyrie Eleison/Parry absorve; números derivam pela direção do defensor.
- Reação ao golpe: efeito derivado de (skill, crítico, classe do atacante, índice do golpe); alguns efeitos giram o alvo (Sonic Blow, Chain Combo); duração do "hurt" vem do grupo de ação do sprite.
- Morte: some depois do clipe de morte terminar **e** sem golpes agendados.
- Animação: ações "transitórias" (Hurt, SkillExec, Dead, Pickup) tocam uma vez; ataque com **fator de movimento do ataque (ASPD)**; **animação de andar avança pela distância percorrida**
  (nós já fazemos); animações forçadas por efeito (parar num quadro, faixa de quadros); **sons de evento do ACT** posicionais; estados de corpo travam a animação (congelado, pedra).
- **Nosso: verificar** tempo de ataque por ASPD/arma, hurt com a duração do sprite, retomar caminhada depois de apanhar, "Guard" de Kyrie, as opções de dano de outros jogadores/miss.

## 8. Clique do mouse (`input_action/click.rs`) — ordem de prioridade

1. Janelas de pet/casamento. 2. Diálogo de NPC ou loja aberta: **o clique no mundo é ignorado.** 3. Autocounter: o clique cancela. 4. Lançando skill: ignora.
5. Sala de chat sob o mouse: entrar. 6. Comandos de companheiro (patrulha, skill). 7. Incapacitado: ignora. 8. **Skill pendente:** valida alvo (classe da skill, mapa, Shift/"noshift"),
distância Chebyshev; em alcance lança, fora de alcance anda e deixa a skill na fila. 9. Item no chão: pendente de pegar (escondido não pega). 10. Placa de loja: própria fecha, de outro compra.
11. NPC: contato. 12. Monstro: ataca (Shift não ataca; pet não é atacado); jogador: só se permitido pelo mapa/Shift. 13. Unidade de skill/armadilha: ataca. 14. Chão: para de atacar, limpa pendências, **anda**.
Detalhes: sentado, clicar no chão só vira o personagem; movimento travado enfileira o destino; correndo, ignora cliques (o servidor controla); escondido não anda por clique.
**Nosso: verificar** essa ordem inteira, principalmente "diálogo aberto ignora o mundo", "sentado vira em vez de andar" e "skill fora de alcance anda e depois lança".

## 9. Ciclo de vida (`events/lifecycle.rs`)

Mudança de sessão em quatro tipos: **troca de mapa** (limpa efeitos, sons, entidades, números de dano, armazém, troca, skills pendentes, IA de companheiros), **logout** (limpa tudo e volta a BGM padrão),
**morte** (marca morto, desliga ondulação, abre o menu de morte) e **ressurreição** (limpa a marca e fecha o menu). **Nosso: verificar** se a troca de mapa limpa armazém/troca/skill pendente
(vimos hoje o estado do falcão se perdendo entre mapas e a munição sumindo da lista, sinais de estado que não é reiniciado/reaplicado de forma consistente).

## 10. O que o `docs/TODO.md` deles diz que **já está pronto** (e nós não)

Companheiros (homúnculo, mercenário, pet), carrinho (sprite e janela), falcão, janela de guilda (membros, cargos, habilidades, emblema, avisos), vending (montar e comprar),
armazém, fabricação (flechas, culinária, refino), banco, correio, casamento, guerra do imperador (sem nome/sem número de dano/emblema sobre a cabeça), troca de personagem/**excluir personagem**,
sombra sob entidades por tabela de tamanho (200+ classes), balão de chat, nome sobre a cabeça, barra de HP ao passar o mouse e sempre no jogador, GM com sprite próprio e nome amarelo,
partículas, névoa, clima, dia/noite, os 1050 efeitos, BGM por mapa, som posicional, `/comandos`, atalhos, modo de batalha, visualizador de efeitos.
**Ainda abertos para eles** (por isso também não esperem deles): menu de contexto do botão direito, tela de carregamento, execução de skill, propriedades de mapa (PvP/sem teleporte/sem skill), perfil de memória/CPU.

## 11. Prioridades que isso muda na nossa auditoria

P0 novo/subiu: item no chão **com sombra** e queda original; sub-célula real do servidor; ordem de cliques; skill fora de alcance anda e lança; glifo/mira de cast; cooldown e mensagens de falha;
tempo de ataque por ASPD; reconstituir arma/escudo ao chegar a lista de equipamento; limpar/reaplicar estado ao trocar de mapa.
P1: falcão atrás + voo de ataque; sons por transição de estado; aura por bit de opt3; Blade Stop; marca/ aviso de chefe; janela única de lista de fabricação (com pré-carga de ícones).

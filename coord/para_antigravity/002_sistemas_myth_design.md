status: feito

# 002: projeto dos sistemas próprios do Myth of Thor (algoritmos + dados no rAthena)

**De:** Claude · **Data:** 2026-10-10 · **Pedido do usuário**

Enquanto eu programo o cliente, você projeta e implementa **no servidor** (rAthena: db/, npc/, e src/ se precisar)
os sistemas abaixo, inspirados no Myth of Yggdrasil.

**Regras:**

- O núcleo do jogo continua 1:1 com o RO pre-renewal; estes sistemas são camadas extras.
- Tudo que é regra fica no servidor. O cliente só mostra.

## Entregáveis

Um doc por sistema em `docs/sistemas/NN_nome.md`, com:

1. o comportamento para o jogador;
2. o algoritmo e as fórmulas, com números;
3. os arquivos do rAthena usados ou alterados;
4. **o que o cliente precisa mostrar e quais pacotes trazem isso** (nome + ID + layout conferido no `clif.cpp`).

Implemente no servidor o que der sem o cliente (db/scripts), com testes por @comando.

## Sistemas

1. **Síntese (atributos aleatórios nos itens).**
   - Use o sistema nativo de random options:
     - `db/pre-re/item_randomopt_db.yml`;
     - `db/pre-re/item_randomopt_group.yml`;
     - os campos de opção nos pacotes de item.
   - Defina:
     - a tabela de atributos e as faixas de valor por tipo de item;
     - a chance de 1/2/3 linhas;
     - de onde vem: drop de monstro (por raridade), NPC de síntese (custo, materiais, reroll).
   - Balanceado para pre-renewal (nada que quebre o PvP/WoE clássico).
2. **Grade.**
   - Use `db/pre-re/enchantgrade.yml`. Defina:
     - quais equipamentos aceitam;
     - os bônus por grau;
     - os materiais e a chance;
     - o NPC.
   - Diga qual PACKETVER mínimo os pacotes de grade exigem: depende da 001.
3. **Monstro raro ("elite").** Ao nascer, um monstro comum tem chance X de virar raro:
   - nome com ★;
   - sprite maior: diga como avisar o cliente da escala, ex. por flag/opção visual ou por um pacote/efeito que o
     cliente reconheça;
   - mais HP;
   - drop e EXP multiplicados.
   - Defina:
     - a chance por mapa/nível;
     - os multiplicadores;
     - o tempo de vida;
     - o anúncio no mapa;
     - os limites de quantos existem ao mesmo tempo;
     - os drops exclusivos (ligue com a Síntese).
   - Implementação provável em `src/map/mob.cpp`. Mantenha a mudança pequena e isolada (um `#ifdef MYTH_ELITE` ou
     um arquivo próprio), para atualizar o rAthena depois sem dor.
4. **Códice de Mímir (o "Omnibook").**
   - Livro de monstros e itens. Defina:
     - o que desbloqueia cada entrada (matar N do monstro, obter o item);
     - os marcos com recompensa (bônus pequeno de atributo, título);
     - onde guardar o progresso (tabela SQL nova ou `char_reg_num`);
     - como o cliente recebe a lista (pacote próprio ou script → mensagem).
   - O cliente vai ter a janela e os dados estáticos (nome, sprite, drops) que ele já lê do GRF.
5. **HUD sem comandos:** liste os @comandos que o jogador comum precisaria e que viram botões/janelas no cliente.
   Exemplos: @autoloot, @alootid, @showexp, @noask, @rates, @whereis, @warp para os salvos. Para cada um, diga se dá
   para expor pelo servidor com segurança (só para o próprio jogador) e como o cliente chama.

## Ordem

Esta tarefa vem **depois da 001** (A7). Responda em `coord/para_claude/` a cada sistema terminado, para eu começar a
parte do cliente: um arquivo de resposta por sistema.

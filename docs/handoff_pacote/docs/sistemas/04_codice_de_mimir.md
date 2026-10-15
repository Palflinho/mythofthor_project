# Sistema 04: Códice de Mímir (Omnibook de Monstros e Itens)

> **Projeto:** Myth of Thor  
> **Arquivos do rAthena:**  
> - `server/npc/custom/myth/mimir_codex_npc.txt` (controle de progresso, marcos e recompensas)  
> - Banco de dados: variáveis em `char_reg_num` (`MIMIR_MOB_<id>`, `MIMIR_ITEM_<id>`)  
> - Pacotes no cliente: `CZ_GLOBAL_MESSAGE` (`0x00f3` para requisição/consulta), `ZC_NOTIFY_CHAT` / `ZC_DISP_MESSAGE` (`0x02c1`), `ZC_PAR_CHANGE` (`0x00b0`)

---

## 1. Comportamento para o Jogador

1. **A Fonte da Sabedoria Nórdica:**
   - Mímir, guardião do poço sob a raiz de Yggdrasil que concede conhecimento universal, registra as conquistas e explorações do jogador em um tomo ancestral: o **Códice de Mímir**.
   - O Códice funciona como uma enciclopédia viva de monstros e itens.
2. **Desbloqueio Progressivo por Monstros (Bestiário):**
   - **Nível 1 (Pesquisa Inicial - 1 abate):** Revela nome do monstro, elemento, raça, tamanho e faixa de nível.
   - **Nível 2 (Estudo Anatômico - 50 abates):** Habilita exibição do HP numérico exato na barra de alvo superior do HUD.
   - **Nível 3 (Maestria de Campo - 250 abates):** Revela a lista completa de drops e chances percentuais exatas, e concede +1% de dano físico e mágico permanente contra aquela espécie específica de monstro.
3. **Desbloqueio de Itens (Compêndio de Relíquias):**
   - Obter um item pela primeira vez registra sua entrada no compêndio com sua ilustração, descrição completa do lore e os monstros que o derrubam.
4. **Marcos de Conquista (Milestones) e Recompensas:**
   - Cada espécie de monstro masterizada concede **1 Ponto de Conhecimento de Mímir**.
   - Marcos acumulados concedem bênçãos passivas da fonte de Mímir:
     - **10 Pontos:** Título *[Observador de Midgard]* + 100 MaxHP.
     - **30 Pontos:** Título *[Erudito de Prontera]* + 100 MaxHP, 50 MaxSP.
     - **60 Pontos:** Título *[Caçador Experiente]* + +1 em Todos os Atributos.
     - **100 Pontos:** Título *[Guardião do Saber]* + +1 em Todos os Atributos, +2% Resistência Elemental.

---

## 2. Estrutura de Armazenamento e Algoritmo

1. **Persistência Sem Alteração de Esquema SQL:**
   - Para garantir total compatibilidade com atualizações futuras do rAthena, o progresso é salvo na tabela nativa de variáveis de personagem (`char_reg_num`):
     - `MIMIR_MOB_<id>`: Contador de abates daquele monstro.
     - `MIMIR_PTS`: Pontos totais acumulados de Mímir.
     - `MIMIR_REWARD`: Máscara binária dos marcos já resgatados.
2. **Gatilho de Atualização (`mob_dead` / NPC Event):**
   - Ao abater um monstro, o rAthena incrementa `MIMIR_MOB_<id>`.
   - Ao atingir os limiares (1, 50, 250), uma notificação sutil em cor esmeralda surge no chat:
     `[Códice de Mímir] Você atingiu o Nível 2 de conhecimento sobre: Poring!`

---

## 3. Comunicação com o Cliente (Unreal Engine 5)

1. **Dados Estáticos no Cliente:**
   - O cliente já possui o banco estático extraído do GRF (`itemInfo.json`, tabelas de monstros e sprites). O cliente não precisa perguntar ao servidor o nome ou o sprite de cada criatura; ele só precisa saber o **nível de maestria** (`0`, `1`, `2` ou `3`) para aquela entrada.
2. **Sincronização Rápida:**
   - O cliente abre a janela do Códice e consulta o estado via comando seguro ou janela própria. O servidor devolve a lista de entradas desbloqueadas de forma compacta.

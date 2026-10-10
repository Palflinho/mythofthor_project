# Referência 17: Sistema de Grupo (Party)

> **Fonte:** Open-Midgard (`src/network/Packet.h`, `src/gamemode/GameMode.cpp`), roBrowserLegacy (`src/Engine/MapEngine/Group.js`, `src/UI/Components/PartyFriends/PartyFriends.js`), rAthena (`src/map/party.cpp`, `conf/battle/party.conf`).  
> **Escopo:** Fase 2 (F2) — Interface de grupo, formação, regras de partilha e sincronização de HP/mapa.  
> **Prioridade:** P1 (Mecânica social e cooperativa).

---

## 1. Resumo

O sistema de Grupo (*Party*) permite que até **12 jogadores** se unam para explorar calabouços, caçar monstros e enfrentar chefes em cooperação. Os membros compartilham telemetria de combate em tempo real: barras de Pontos de Vida (HP) dinâmicas, pontos de navegação no minimapa, chat exclusivo e bônus de experiência cooperativa. O líder do grupo pode configurar as regras de distribuição de itens recolhidos e a partilha igualitária de experiência (*Even Share*).

---

## 2. Dados e Constantes

### 2.1. Limites e Parâmetros Numéricos (Pre-Renewal Oficial)

| Parâmetro | Valor Oficial | Descrição |
|---|:---:|---|
| **Limite Máximo de Membros** | **12 jogadores** (`MAX_PARTY`) | Tamanho máximo da equipe por grupo. |
| **Diferença Máxima de Nível de Base** | **10 níveis** (ou 15 configurável) | Diferença permitida entre o membro de maior e menor nível para ativar a Partilha Igualitária (*Even Share*). |
| **Bônus de EXP em Grupo no Mesmo Mapa** | **+20% por membro adicional** | Ex: 2 membros = +20%, 3 membros = +40%, até o teto de bônus cooperativo. |
| **Alcance de Partilha de EXP em Tela** | **$31 \times 31$ células** | Membros devem estar dentro da mesma área de visão do monstro derrotado para pontuar. |
| **Comprimento Máximo do Nome do Grupo** | **24 caracteres** | Campo de texto alfanumérico ao criar o grupo. |

### 2.2. Janela de Grupo (`PartyFriends` / `Alt+Z`)
- **Atalho Padrão:** `Alt+Z` (alterna visibilidade).
- **Dimensões:** $280 \times 240$ px (redimensionável verticalmente até 400 px).
- **Elemento por Membro:**
  - Ícone de classe ($16 \times 16$ px).
  - Nome do personagem e nível de base.
  - Mini barra de HP colorida ($80 \times 6$ px): Verde ($> 50\%$), Amarelo ($25\% \dots 50\%$), Vermelho ($< 25\%$).
  - Indicador de líder (ícone de estrela dourada).
  - Nome do mapa atual onde o membro se encontra (ex: `prt_fild08`). Se estiver offline, exibe `Offline` em cinza escuro.

### 2.3. Regras de Distribuição de Itens e EXP

#### Opção de Experiência (`expOption`):
- `0` = **Individual:** Cada jogador ganha experiência proporcional ao dano que causou ao monstro.
- `1` = **Compartilhada (Even Share):** A experiência total concedida pelo monstro (com bônus cooperativo) é dividida igualmente entre todos os membros ativos no mapa dentro da faixa de nível.

#### Opção de Coleta de Itens (`ItemPickupRule`):
- `0` = **Individual:** Apenas o jogador que causou o maior dano tem prioridade temporária de recolhimento do item no chão.
- `1` = **Compartilhada:** Qualquer membro do grupo pode recolher itens soltos por monstros derrotados pela equipe.

#### Opção de Divisão de Itens (`ItemDivisionRule`):
- `0` = **Individual:** O item vai diretamente para a mochila de quem o recolheu.
- `1` = **Aleatória / Compartilhada:** O item recolhido é sorteado aleatoriamente e enviado diretamente para a mochila de um membro do grupo presente na mesma tela.

---

## 3. Regras de Comportamento

### 3.1. Criação de Grupo
1. O jogador digita o comando `/organize <NomeDoGrupo>` no chat ou clica no botão "Criar Grupo" na janela `Alt+Z`.
2. Se o jogador já pertencer a um grupo, recebe a mensagem de erro: `"Você já pertence a um grupo."` (msg 78).
3. O cliente envia `CZ_MAKE_GROUP2` (`0x01e6`) com nome e regras iniciais de partilha.
4. O servidor valida se o nome está disponível e responde com `ZC_ACK_MAKE_GROUP` (`0x00f7`):
   - `result == 0`: Sucesso. O criador se torna o líder do grupo.
   - `result == 1`: Nome já existente no servidor.
   - `result == 2`: Nome inválido ou contendo caracteres proibidos.
5. Ao confirmar, o chat registra em azul: `"Grupo [Nome] criado com sucesso."` (msg 77).

### 3.2. Convite para o Grupo
1. O líder clica com o botão direito sobre outro jogador e seleciona *"Convidar para o Grupo"*, ou digita `/invite <Nome>` no chat.
2. Apenas o líder pode emitir convites em configurações clássicas.
3. O cliente envia `CZ_REQ_JOIN_GROUP` (`0x00f9`) com o Account ID e nome do alvo.
4. O convidado recebe na tela uma caixa de diálogo modal de confirmação:
   `"[Nome do Líder] convidou você para o grupo [Nome do Grupo]. Deseja aceitar?"` (Sim / Não).
5. Se o alvo aceitar:
   - Envia resposta de confirmação ao servidor.
   - O servidor transmite `ZC_ADD_MEMBER_TO_GROUP` para todos os membros existentes.
   - O novo membro é listado na janela `Alt+Z` de todos.
6. Se recusar: o líder recebe mensagem informativa: `"[Nome] recusou o convite de grupo."`.

### 3.3. Sincronização em Tempo Real (HP e Posição)
- **Barra de Vida:** Sempre que o HP de qualquer membro do grupo for alterado por dano ou cura, o servidor transmite `ZC_NOTIFY_HP_TO_GROUPM` (`0x00fa`) contendo `{ AID, hp, maxhp }`.
  - O cliente atualiza a barra na janela `Alt+Z` e a barra flutuante sobre a cabeça do personagem caso ele esteja na mesma tela.
- **Posição no Minimapa:** O servidor envia periodicamente `ZC_NOTIFY_POSITION_TO_GROUPM` com as coordenadas $(X, Y)$ dos membros no mapa. O minimapa desenha cruzes ou pontos brancos/azuis na posição dos aliados.

### 3.4. Saída, Expulsão e Transferência de Liderança
- **Sair do Grupo:** Qualquer membro pode digitar `/leave` ou clicar no botão "Sair" na janela `Alt+Z`. O cliente envia `CZ_REQ_LEAVE_GROUP` (`0x00fc`). O jogador é desvinculado e seu nome apagado da lista.
- **Expulsar Membro:** O líder pode selecionar um membro e clicar em "Expulsar". Envia `CZ_REQ_EXPEL_GROUP_MEMBER` (`0x00fd`). O membro expulso recebe notificação no chat: `"Você foi expulso do grupo."`.
- **Passar a Liderança:** O líder pode selecionar outro membro e escolher "Passar Liderança" (`CZ_CHANGE_GROUP_MASTER`). A estrela de líder é transferida imediatamente.

---

## 4. Entradas e Saídas (Pacotes de Rede)

### Pacotes Recebidos pelo Cliente (Servidor $\to$ Cliente):
- **`ZC_ACK_MAKE_GROUP` (`0x00f7`):** Resposta da solicitação de criação de grupo (`0 = Sucesso`).
- **`ZC_GROUP_LIST` (`0x00f8`):** Lista inicial completa de membros e regras de partilha ao entrar no jogo ou ingressar.
- **`ZC_ADD_MEMBER_TO_GROUP` (`0x00f9`):** Notifica a entrada de um novo companheiro.
- **`ZC_DELETE_MEMBER_FROM_GROUP` (`0x00fb`):** Notifica que um membro saiu ou foi expulso.
- **`ZC_NOTIFY_HP_TO_GROUPM` (`0x00fa`):** Atualização de HP atual e máximo de um membro `{ AID, hp, maxhp }`.
- **`ZC_NOTIFY_POSITION_TO_GROUPM` (`0x00fe`):** Coordenadas $(X, Y)$ de um membro para plotagem no minimapa.
- **`ZC_NOTIFY_CHAT_PARTY` (`0x00ff`):** Mensagem de texto enviada no canal exclusivo do grupo (`%`).

### Pacotes Enviados pelo Cliente (Cliente $\to$ Servidor):
- **`CZ_MAKE_GROUP2` (`0x01e6`):** Criação de grupo com nome e regras.
- **`CZ_REQ_JOIN_GROUP` (`0x00f9`):** Convite de jogador para ingressar no grupo.
- **`CZ_REQ_LEAVE_GROUP` (`0x00fc`):** Pedido do jogador para abandonar o grupo atual.
- **`CZ_REQ_EXPEL_GROUP_MEMBER` (`0x00fd`):** Pedido do líder para expulsar um membro.
- **`CZ_GROUPINFO_CHANGE_V2`:** Alteração das opções de partilha de EXP e itens.
- **`CZ_CHANGE_GROUP_MASTER`:** Transferência do cargo de líder para outro membro.

---

## 5. Como Testar

1. **Criação de Grupo:** Digitar `/organize Valhalla`. Abrir a janela `Alt+Z`: conferir nome "Valhalla" no cabeçalho e o próprio personagem com ícone de estrela de líder.
2. **Convite e Aceitação:** Com dois clientes conectados na mesma cidade, o líder clica com botão direito no aliado e seleciona Convidar. O segundo cliente deve receber o diálogo modal e, ao aceitar, aparecer na lista de membros de ambos.
3. **Telemetria de HP:** Fazer um dos membros sofrer dano (atacando monstro ou usando skill com custo de HP): verificar atualização instantânea da barra de vida na janela de grupo do outro cliente.
4. **Chat de Grupo:** Digitar `%Olá grupo!`. O texto deve aparecer em ciano/azul claro apenas para os membros do grupo.
5. **Teste de EXP Dividida:** Ativar *Even Share*, derrotar um monstro em equipe e conferir se ambos recebem EXP idêntica no chat de sistema.

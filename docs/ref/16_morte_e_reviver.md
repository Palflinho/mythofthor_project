# Referência 16: Morte, Renascimento e Ressurreição

> **Fonte:** Open-Midgard (`src/world/GameActor.cpp`, `src/gamemode/GameMode.cpp`, `src/ui/UIChooseWnd.cpp`), roBrowserLegacy (`src/Renderer/Entity/EntityAction.js`, `src/Engine/MapEngine/Entity.js`, `src/UI/Components/WinPopup/WinPopup.js`), rAthena (`src/map/pc.cpp`, `src/map/clif.cpp`, `conf/battle/player.conf`).  
> **Escopo:** Fase 2 (F2) — Estados de derrota do personagem, perda de experiência, interface de respawn e ressurreição em campo.  
> **Prioridade:** P1 (Ciclo fundamental de sobrevivência e penalidades).

---

## 1. Resumo

Quando os Pontos de Vida (HP) do personagem são reduzidos a zero, o cliente interrompe imediatamente todas as ações em andamento (conjuração, ataques contínuos e passos de caminhada) e transiciona o ator para o estado de morte (`ACTION_DIE` / `kDeathStateId`). O personagem tomba ao solo e emite um gemido característico de derrota. Na tela, surge uma janela modal centralizada oferecendo a escolha entre renascer no Ponto de Salve (*Save Point*) ou retornar à Seleção de Personagens. Caso outro jogador aplique uma habilidade ou item de ressurreição no local, o jogador recebe um diálogo para levantar-se com vida parcial sem precisar voltar à cidade.

---

## 2. Dados e Constantes

### 2.1. Penalidades Oficiais de Morte (Pre-Renewal 1:1)

| Parâmetro | Valor Oficial | Regras e Exceções |
|---|:---:|---|
| **Perda de Experiência de Base** | **-1.0%** (`death_penalty_base: 100`) | Deduzido imediatamente no momento da morte. A barra não desce abaixo de 0,0% no nível corrente (não há perda de nível / *de-level*). |
| **Perda de Experiência de Classe** | **-1.0%** (`death_penalty_job: 100`) | Deduzido no momento da morte. Não reduz o nível de classe. |
| **Proteção por Seguro de Vida** | 0% de perda | Se o jogador possuir o efeito ativo do item *Seguro de Vida* (*Life Insurance*). |
| **Mapas Especiais (PvP / GvG / WoE)** | **0% de perda** | Em arenas de duelo, Guerra do Emperium e calabouços de guilda, mortes não deduzem EXP. |
| **Penalidade de Zeny** | **0 Zeny** | O Ragnarok Online clássico nunca debita Zeny por morte padrão de personagens contra monstros. |

### 2.2. Janela Modal de Morte (`UIChooseWnd` / `WinPopup`)
- **Largura:** **200 px**
- **Altura:** **120 px**
- **Posição:** Centralizado na tela do jogo (`(ScreenWidth - 200) / 2`, `(ScreenHeight - 120) / 2`).
- **Botões Disponíveis:**
  1. *Retornar ao Ponto de Retorno* (`Return to Save Point`): Botão superior destacado.
  2. *Retornar à Seleção de Personagens* (`Character Select`): Botão inferior.
- **Comportamento do Foco:** Modo `FREEZE` / Janela Modal. Bloqueia cliques no mundo subjacente enquanto a escolha não for feita ou o personagem for revivido.

### 2.3. Áudio e Animação do Sprite
- **Quadro de Morte (`.act`):** Índice de ação `4` (corpo sem arma) ou `33` (com arma empunhada).
- **Tempo de Transição:** A animação de queda dura de 200 ms a 400 ms até o ator tocar o solo, congelando estático no último quadro (`m_isMotionFreezed = 1`).
- **Arquivos de Som de Derrota:**
  - Personagem Masculino: `data/wav/player/m_die.wav` ou `human_m_die.wav`.
  - Personagem Feminino: `data/wav/player/f_die.wav` ou `human_f_die.wav`.

---

## 3. Regras de Comportamento

### 3.1. O Momento da Morte
1. O servidor detecta que `HP <= 0` e envia:
   - `ZC_PAR_CHANGE` (`VAR_HP = 0`).
   - `ZC_NOTIFY_ACT` com tipo de ação de morte ou `ZC_NOTIFY_PLAYERDEAD`.
2. **Reação Imediata do Cliente:**
   - O ator local cancela instantaneamente qualquer rota de pathfinding ativa.
   - Barras de conjuração de habilidades ativas (`UIRechargeGage` / Cast Bar) são destruídas.
   - O sprite reproduz a animação de tombo e congela estirado no piso.
   - Todos os monstros agressivos desviam o foco (*drop aggro*) imediatamente e voltam ao estado de patrulha.
   - O diálogo modal de morte surge no centro da tela.
   - O clique no chão não gera comando de movimento nem dispara cursor de ataque.

### 3.2. Renascimento no Ponto de Salve (*Respawn*)
1. O jogador clica no botão "Retornar ao Ponto de Retorno" (ou pressiona `Enter` com o botão focado).
2. O cliente envia `PACKET_CZ_RESTART` (`0x00b2`) com `Type = 0`.
3. O servidor processa a solicitação:
   - Restaura o HP do jogador para o valor padrão de renascimento (geralmente 100% ou metade do HP máximo, dependendo da configuração do servidor).
   - Teleporta o personagem para o mapa e coordenadas do seu Ponto de Salve (ex: em frente à Kafra de Prontera `prontera 156, 191`).
4. O cliente fecha a janela modal de morte, carrega o mapa da cidade e instancia o personagem de pé (`ACTION_STAND`).

### 3.3. Retorno à Seleção de Personagens
1. O jogador clica em "Retornar à Seleção de Personagens".
2. O cliente envia `PACKET_CZ_RESTART` (`0x00b2`) com `Type = 1`.
3. O servidor desconecta a sessão daquele personagem no mapa e responde com `ZC_RESTART_ACK` (`0x00b3`).
4. O cliente encerra o modo de jogo (`CGameMode`), descarrega o mapa e retorna para a tela de Seleção de Personagens (`UISelectCharWnd`).

### 3.4. Ressurreição em Campo (Por Aliado)
1. Enquanto o corpo do jogador permanece no chão, sacerdotes, sumo-sacerdotes ou companheiros de grupo podem conjurar a habilidade **Ressuscitar** (*Resurrection* — `AL_RESURRECTION`) ou usar o item **Folha de Yggdrasil** (*Yggdrasil Leaf* — ID 610) mirando no corpo.
2. O servidor envia um popup de confirmação na tela do jogador derrotado:
   `"Deseja aceitar a ressurreição oferecida por [Nome do Jogador]?"`
3. Se o jogador aceitar:
   - A janela de confirmação e a janela modal de morte fecham.
   - Toca o efeito visual divino de ressurreição (`EF_RESURRECTION` — anéis de luz subindo do solo com centelhas douradas).
   - O sprite executa a transição de levantar-se e reassume a postura em pé (`ACTION_STAND`).
   - O HP é restaurado de acordo com o nível da habilidade utilizada:
     - **Nível 1:** 10% do HP Máximo.
     - **Nível 2:** 30% do HP Máximo.
     - **Nível 3:** 50% do HP Máximo.
     - **Nível 4:** 80% do HP Máximo.
     - **Folha de Yggdrasil:** Restaura quantidade fixa de HP (geralmente equivalente a Nível 1 ou ~100 HP).
   - O personagem **não perde a EXP adicionalmente** além do 1% já debitado na queda.

---

## 4. Entradas e Saídas (Protocolo de Pacotes)

### Pacotes Recebidos pelo Cliente (Servidor $\to$ Cliente):
- **`ZC_NOTIFY_ACT` / `ZC_NOTIFY_PLAYERDEAD` (`0x008a`):** Notifica o cliente do impacto final que levou o HP a zero.
- **`ZC_PAR_CHANGE` (`0x00be`):** Atualiza a variável `VAR_HP` para 0.
- **`ZC_RESTART_ACK` (`0x00b3`):** Confirmação de autorização para retornar à Seleção de Personagens.
- **`ZC_RESURRECTION_REQUEST`:** Requisição modal perguntando se o jogador aceita a ressurreição oferecida por terceiro.

### Pacotes Enviados pelo Cliente (Cliente $\to$ Servidor):
- **`CZ_RESTART` (`0x00b2`):**
  - `Type = 0`: Retornar ao Ponto de Retorno (Save Point).
  - `Type = 1`: Retornar à Seleção de Personagens.
- **`CZ_RESURRECTION_RESPONSE`:** Resposta de aceitação (1) ou recusa (0) da oferta de ressurreição.

---

## 5. Diferenças Entre Referências

1. **Popup de Morte:**
   - No **Open-Midgard**, a morte abre a janela genérica de escolha `UIChooseWnd` que contém as entradas `MenuEntry_ReturnToSavePoint` e `MenuEntry_CharacterSelect`.
   - No **roBrowserLegacy**, utiliza-se um clone estilizado de `WinPopup` (`WinPrompt`) com dois botões estilizados de madeira clássica do RO e texto centralizado `"Você foi derrotado"`.
2. **Câmera:**
   - No cliente original, a câmera permanece travada nas coordenadas onde o corpo caiu, permitindo apenas rotação com o botão direito para observar ao redor, mas sem deslocamento horizontal.

---

## 6. Como Testar

1. **Derrota por Monstro:**
   - Deixar um monstro (ex: Poring, Esporo) reduzir o HP do personagem a 0.
   - Verificar se o personagem tomba imediatamente e se toca o som `player/m_die.wav` ou `f_die.wav`.
   - Verificar se a janela de opções surge no centro da tela com as opções "Retornar ao Ponto de Salve" e "Seleção de Personagens".
2. **Teste de Retorno ao Save Point:**
   - Clicar em "Retornar ao Ponto de Salve": verificar se o mapa da cidade recarrega e o personagem surge em pé em frente à Kafra com HP recuperado.
   - Conferir se a barra de EXP sofreu o débito de 1% (sem descer abaixo de 0%).
3. **Teste de Ressurreição:**
   - Estando derrotado no chão, receber uma *Folha de Yggdrasil* ou magia *Ressuscitar*.
   - Aceitar o diálogo: verificar se o efeito visual de luz divina é acionado e se o personagem se levanta com HP parcial sem recarregar o mapa.

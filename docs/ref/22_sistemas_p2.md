# Referência 22: Sistemas P2 — Troca (Trade), Login/Seleção e Montarias/Carrinho/Falcão

> **Fonte:** roBrowserLegacy (`src/UI/Components/Trade/Trade.js`, `src/Engine/MapEngine/Trade.js`, `src/UI/Components/CharSelect/CharSelect.js`, `src/UI/Components/CartItems/CartItems.js`), Open-Midgard (`src/network/Packet.h`, `src/world/GameActor.cpp`), rAthena (`src/map/trade.cpp`, `src/map/clif.cpp`, `src/map/pc.cpp`).  
> **Escopo:** Fase 2 (F2) — Sistemas de comércio entre jogadores, ciclo de login/personagens e montarias/carrinho/falcão.  
> **Prioridade:** P2 (Sistemas de suporte e conveniência do jogador).

---

## 1. Resumo

Este documento reúne três subsistemas fundamentais de conveniência e mecânica de jogo:
1. **Negociação Segura entre Jogadores (*Player Trade*):** Sistema de troca direta de itens e Zeny entre dois personagens presentes no mesmo mapa com validação em duas etapas (*Lock* e *Trade*).
2. **Ciclo de Login, Seleção e Criação de Personagens:** Interface de entrada de credenciais, vitrine de slots de personagens com visual tridimensional de roupas/cabelos e criação com distribuição de atributos iniciais.
3. **Montarias, Carrinho de Carga e Falcão:** Modificadores visuais e mecânicos de locomoção, estoque adicional de carga para Mercadores e suporte de combate aéreo para Caçadores.

---

## 2. Negociação Segura entre Jogadores (*Trade*)

### 2.1. Dimensões da Janela de Troca (`TradeWnd`)
- **Dimensões:** **$380 \times 240$ px**, centralizada na tela.
- **Painel Esquerdo (Minha Oferta):** 10 slots de itens ($2 \times 5$), campo de inserção de Zeny e botão "Travar" (*Lock*).
- **Painel Direito (Oferta do Parceiro):** 10 slots espelhados exibindo os itens oferecidos pelo outro jogador e total de Zeny ofertado.
- **Barra de Ação Inferior:** Botão "Negociar" (*Trade* / OK) e botão "Cancelar".

### 2.2. Protocolo de Duas Etapas e Regras de Segurança
O sistema original do Ragnarok Online utiliza um protocolo anti-fraude rigoroso em duas etapas:
1. **Início da Negociação:**
   - O jogador clica com o botão direito sobre outro personagem a uma distância de até 3 células e escolhe *"Negociar"* (`CZ_REQ_EXCHANGE_ITEM` `0x00e4`).
   - O parceiro recebe uma caixa modal: `"[Nome] deseja negociar com você. Aceitar?"` (Sim / Não).
   - Ao aceitar (`CZ_ACK_EXCHANGE_ITEM`), a janela de negociação abre simultaneamente em ambos os clientes e ambos os personagens ficam **travados sem poder se mover ou conjurar**.
2. **Fase 1 — Oferta e Trava (*Lock*):**
   - Os jogadores arrastam itens de suas mochilas e digitam a quantia de Zeny desejada.
   - Quando satisfeito, o jogador clica em **"Travar"** (`CZ_CONCLUDE_EXCHANGE_ITEM`).
   - Assim que o jogador clica em Travar, **seus slots e campo de Zeny tornam-se somente leitura** (cinza escuro). Ele não pode mais retirar nem adicionar itens.
3. **Fase 2 — Confirmação e Execução (*Trade*):**
   - O botão "Negociar" (*Trade*) só fica habilitado para um jogador após **AMBOS os jogadores terem clicado em "Travar"**. Isso garante que ninguém possa trocar um item no último milissegundo antes de aceitar.
   - Quando ambos clicam em "Negociar" (`CZ_EXEC_EXCHANGE_ITEM`), o servidor rAthena processa a transação em **bloco atômico**.
   - Se qualquer um dos dois não tiver espaço livre de inventário ou se a troca for resultar em sobrepeso $\ge 90\%$, o servidor aborta a negociação e devolve todos os itens para os respectivos donos com mensagem de erro no chat.
   - Em caso de sucesso, toca o efeito sonoro de moedas e a negociação é concluída.

---

## 3. Login, Seleção e Criação de Personagens

### 3.1. Tela de Login (`WinLogin`)
- Campos de Entrada: Usuário (*Username*) e Senha (*Password*).
- Envia `PACKET_CA_LOGIN` com credenciais. O servidor valida a conta e responde com a lista de servidores disponíveis (`PACKET_AC_ACCEPT_LOGIN`) e a lista de personagens (`PACKET_HC_ACCEPT_ENTER`).

### 3.2. Tela de Seleção de Personagens (`CharSelect`)
- Grade de até **9 a 12 slots de personagens** (conforme a versão de pacotes).
- Para cada slot preenchido, o cliente exibe o sprite animado completo do personagem (corpo da classe, estilo e cor de cabelo, e acessórios visuais de cabeça topo/meio/baixo).
- **Ficha Resumida do Slot:**
  - Nome do Personagem
  - Classe Atual (ex: *Cavaleiro / Lord Knight*)
  - Nível de Base e Nível de Classe (ex: `Lv. 85 / Job. 50`)
  - HP Atual / Máximo e SP Atual / Máximo
  - Nome do mapa onde deslogou (ex: `prontera.gat`)
- **Ações:** "Conectar" (inicia o jogo com o personagem selecionado), "Criar" (em slots vazios), "Deletar" (requer confirmação e data de nascimento/código pin) e "Sair".

### 3.3. Tela de Criação de Personagens (`CharCreate`)
- Escolha de Nome (verificação de caracteres válidos e duplicidade no servidor).
- Escolha de Sexo (herdado da conta clássica), Estilo de Cabelo (1 a 24) e Cor de Cabelo (1 a 8).
- **Distribuição de Atributos Iniciais (Clássico):**
  - O jogador distribui 24 pontos iniciais entre os 6 atributos primários (FOR, AGI, VIT, INT, DES, SOR), respeitando o limite mínimo de 1 e máximo de 9 por atributo.

---

## 4. Montarias, Carrinho de Carga e Falcão

### 4.1. Montaria de Peco Peco / Grifo
- **Classes Permitidas:** Cavaleiro, Lorde, Templário, Paladino (e suas evoluções montando Grifo ou Peco Peco).
- **Aluguel:** Realizado com o NPC Mestre dos Peco Pecos em Prontera ou na guilda dos Templários.
- **Habilidade Necessária:** *Montaria* (`KN_RIDING` Lv 1).
- **Efeitos no Personagem:**
  - **Sprite:** O corpo do personagem engata sobre a montaria em todas as 8 direções e posturas.
  - **Velocidade de Movimento:** Concede **+25% de velocidade de caminhada constante** (reduz o tempo por passo de 150 ms para 112 ms por célula).
  - **Penalidade de ASPD:** No RO clássico, montar um Peco Peco reduz a ASPD natural com lanças e espadas pela metade, a menos que o jogador invista pontos na habilidade passiva *Perícia em Montaria* (`KN_CAVALIERMASTERY` Lv 1 a 5, restaurando 60%, 70%, 80%, 90% e 100% da velocidade de ataque).

### 4.2. Carrinho de Carga (*Pushcart*)
- **Classes Permitidas:** Mercador, Ferreiro, Alquimista e evoluções.
- **Habilidade Necessária:** *Usar Carrinho* (`MC_PUSHCART` Lv 1 a 10).
- **Efeitos no Personagem:**
  - **Sprite:** Desenha o carrinho de madeira de duas rodas engatado atrás do personagem.
  - **Inventário Exclusivo (`Alt+W`):** Abre a janela do carrinho com capacidade para **100 itens distintos e até 8.000 unidades de peso adicionais**. Itens no carrinho não pesam no corpo do jogador para efeitos da regra de sobrepeso de 50%/90%.
  - **Curva de Velocidade por Nível de Pushcart:**
    - Nível 1: $-20\%$ de velocidade de movimento (muito lento).
    - Nível 5: Velocidade de movimento normal (100%).
    - Nível 10: Velocidade de movimento normal plena (100%).

### 4.3. Falcão (*Falcon*)
- **Classes Permitidas:** Caçador (*Hunter*) e Atirador de Elite (*Sniper*).
- **Habilidade Necessária:** *Adestrar Falcão* (`HT_FALCON` Lv 1).
- **Efeitos no Personagem:**
  - **Visual:** O falcão voa em círculos contínuos sobre a cabeça do caçador enquanto parado, e acompanha o voo paralelo durante a caminhada.
  - **Ataque Automático (*Auto Blitz Beat*):** Em cada flecha disparada, há uma chance percentual proporcional à Sorte do caçador:
    $$\text{ChanceAutoBlitz} = \left\lfloor \frac{\text{LUK}}{3} \right\rfloor \times 1\%$$
    O falcão mergulha no alvo desferindo até 5 golpes de garras proporcionais ao nível da habilidade *Garras de Aço* (`HT_STEELCROW`).

---

## 5. Como Testar

1. **Teste de Troca Segura:** Abrir dois clientes, solicitar troca. Tentar clicar em "Negociar" com apenas 1 dos lados travado: o botão deve permanecer desabilitado. Travar ambos e concluir: os itens e Zeny devem ser trocados de forma atômica e correta.
2. **Teste de Montaria de Cavaleiro:** Alugar Peco Peco com um Cavaleiro. Verificar se a velocidade de caminhada acelera 25% visivelmente em relação a personagens a pé.
3. **Teste de Carrinho de Mercador:** Alugar carrinho, abrir a janela do carrinho com `Alt+W` e transferir 100 Poções Brancas. Verificar se o peso é computado exclusivamente na janela do carrinho sem ativar o ícone de sobrepeso na HUD do personagem.

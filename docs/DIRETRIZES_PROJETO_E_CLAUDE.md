# 📖 Diretrizes de Arquitetura e Guia de Colaboração (Myth of Thor)

> **Documento Oficial de Alinhamento para a Equipe (Você, Claude e Antigravity)**  
> Este documento registra todas as decisões de design de mundo, transições, controles, morte no auto-battle e a divisão técnica de tarefas entre os copilotos.

---

## 🎮 1. Controles e Câmera Híbridos

### A. Movimentação Dual Simultânea (Enhanced Input System)
* **Mouse (Point & Click):** Clicar no chão move o personagem até o ponto desejado (fiel ao clássico de 2002).
* **Teclado (WASD):** Permite andar diretamente pelas teclas. A Unreal converte os vetores do teclado em requisições de movimento contínuas (`CZ_REQUEST_MOVE`) para o rAthena de forma transparente.
* **Transição Instantânea:** O jogador pode alternar entre mouse e WASD no mesmo segundo sem precisar de telas de configuração.

### B. Modos de Câmera (Alternáveis no Menu)
* **Modo Isométrico Fixo:** Câmera travada no ângulo clássico de 45° (estilo Diablo / Octopath Traveler).
* **Modo Rotativo Livre 360°:** Segurar botão direito do mouse ou teclas `Q`/`E` permite girar a visão 360° ao redor do personagem e controlar o zoom pelo scroll do mouse.

---

## 💀 2. Regras de Morte & Auto-Battle (Risco vs. Recompensa)

### A. Morte em Jogo Manual
* Penalidade clássica: Perda de **1% de EXP Base**. O jogador escolhe quando voltar ao ponto de retorno ou aguarda um Noviço/Sacerdote ressuscitar.

### B. Morte com Auto-Battle Ativo
Para evitar abusos e impedir que jogadores deixem o bot farmando em mapas muito perigosos sem supervisão:
1. **Penalidade Agravada:** Perda de **5% de EXP Base**.
2. **Desativação Imediata:** O sistema de Auto-Battle é desligado no exato instante da morte.
3. **Estado Caído (Downed Timer):** O personagem permanece caído no chão por **2 a 3 minutos** (permitindo que aliados ou suportes possam revivê-lo no local).
4. **Respawn Automático:** Ao expirar os minutos caído, o jogador é teleportado automaticamente para o seu **Ponto de Retorno (Save Point)** em Prontera.
5. **Segurança:** O Auto-Battle permanece **desligado** após o respawn na cidade para evitar movimentações indesejadas.

---

## 🗺️ 3. Arquitetura de Mundo & Transições

### A. Regiões em Mundo Aberto Contínuo (Seamless Open World)
* **Dentro da mesma região (ex: Região de Prontera):**
  * Cidade Capital + Campos circundantes (`prt_fild08`, etc.) formam **um único mundo contínuo** via **World Partition** na Unreal Engine 5.
  * **Zero portais azuis e zero telas de loading** entre cidade e campo: o jogador caminha pelas muralhas e portões e entra diretamente nos campos de caça.

### B. Transição Entre Grandes Regiões / Continentes
* Telas de carregamento temáticas e ilustradas com as artes conceituais clássicas de Ragnarok em viagens de **Aeroplano**, **Barcos (ex: Alberta $\rightarrow$ Ilha de Izlude / Lutie)** e travessia de grandes cordilheiras/fronteiras.

### C. Arquitetura Híbrida de Dungeons (Calabouços & Esgotos)
* **Superfície $\rightarrow$ Entrada da Dungeon:** Entrada natural pela boca da caverna ou rampa sem loading.
* **Entre Andares Subterrâneos Profundos (ex: Andar 1 $\rightarrow$ Andar 2):** Ao descer a escada ou alçapão, ocorre um **Fade-out suave de 0.5s para preto** com transição acústica (eco, gotejamento de água).

---

## 🤝 4. Divisão de Papéis da Equipe

### 🧠 Papel do Claude (Arte Técnica, Unreal Engine 5 & Blueprints)
1. **Controles & Câmera:** Configuração do Enhanced Input System (WASD + Clique) e câmera alternável (Fixa/Livre).
2. **Atmosfera & Shaders:** Blueprints de **Ciclo Dia/Noite** com sol dinâmico e iluminação de postes em Prontera; efeitos de partículas em **Niagara** (chuva e poças reflexivas com Lumen).
3. **Level Streaming & Transições:** Sistema de **Fade-in / Fade-out (0.5s)** com controle de volume de áudio em escadas de dungeons.
4. **PaperZD & Sprites 2D:** State Machines das 8 direções dos sprites do Aprendiz e monstros (Idle, Walk, Attack, Hit, Die).
5. **Interface de Usuário (HUD/UMG):** Barras de HP/SP, relógio de 3h do Auto-Battle e aviso na tela de morte por Auto-Battle (5% EXP).

### ⚙️ Papel do Antigravity (Engenharia de Rede C++, Servidor & Backend)
1. **Protocolo Nativo de Rede rAthena:** Código C++ de sockets TCP na Unreal (`URagnarokNetworkSubsystem`) com serialização binária (`CA_LOGIN`, `CH_ENTER`, `CZ_REQUEST_MOVE`, etc.).
2. **Infraestrutura no Ubuntu/Docker:** Manutenção do banco MariaDB (porta `3311`), compilação dos servidores e travas de mapas (`map_athena.conf`).
3. **Testes Automatizados Contínuos:** Manutenção e expansão da suíte headless em Python (`tests/ragnarok_net_test.py`).
4. **Lógica Matemática do Auto-Battle:** Implementação das regras de vigor (3h diárias), travas de sobrepeso ($\ge 90\%$) e regras de penalidade de 5% de EXP ao morrer.

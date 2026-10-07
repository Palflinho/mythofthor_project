# 📖 Diretrizes de Arquitetura e Guia de Colaboração (Myth of Thor)

> **Documento Oficial de Alinhamento para a Equipe (Você, Claude e Antigravity)**  
> Este documento registra todas as decisões de design de mundo, transições, controles, áudio dinâmico, exibição de dano, escopo de lançamento (Sem WoE inicial) e a divisão técnica de tarefas entre os copilotos.

---

## 🛡️ 1. Filosofia de Lançamento & Escopo (Sem WoE no Início)
* **WoE (Guerra do Emperium) Desativada na Fase 1:**
  * O lançamento é **100% focado em PvE, Exploração, Quests e Economia Inicial**.
  * Permite que os jogadores aproveitem o jogo casualmente sem pressão de rush para castelos.
  * A abertura dos castelos (Valkyrie Realm de Prontera) será reservada como o **primeiro grande evento sazonal de comunidade** após os clãs se consolidarem.

---

## 🎮 2. Controles e Câmera Híbridos

### A. Movimentação Dual Simultânea (Enhanced Input System)
* **Mouse (Point & Click):** Clicar no chão move o personagem até o ponto desejado (fiel ao clássico de 2002).
* **Teclado (WASD):** Permite andar diretamente pelas teclas. A Unreal converte os vetores do teclado em requisições de movimento contínuas (`CZ_REQUEST_MOVE`) para o rAthena de forma transparente.
* **Transição Instantânea:** O jogador pode alternar entre mouse e WASD no mesmo segundo sem precisar de telas de configuração.

### B. Modos de Câmera (Alternáveis no Menu)
* **Modo Isométrico Fixo:** Câmera travada no ângulo clássico de 45° (estilo Diablo / Octopath Traveler).
* **Modo Rotativo Livre 360°:** Segurar botão direito do mouse ou teclas `Q`/`E` permite girar a visão 360° ao redor do personagem e controlar o zoom pelo scroll do mouse.

---

## 💥 3. Exibição de Dano & Combate Clássico (Floating Text)
Preservação visual total da identidade clássica dos anos 2000:
* **Dano Causado a Monstros:** Números brancos/amarelados com animação clássica de "salto" sobre o alvo.
* **Dano Crítico:** Números ampliados acompanhados do SFX de impacto crítico.
* **Dano Sofrido:** Números vermelhos subindo sobre a cabeça do jogador.
* **Esquiva (Flee):** Exibição da palavra clássica **`MISS`** em azul.

---

## 🎵 4. Áudio Dinâmico de Dia e Noite (Sem Alterar Arquivos)
Utilização das trilhas sonoras originais (`Theme of Prontera`, etc.) processadas em tempo real pelo sistema de áudio da Unreal 5:
* **Dia:** Reprodução normal e vibrante da BGM clássica.
* **Noite:** A Unreal aplica filtro passa-baixa (**Low-Pass Filter**) com redução de ~15% de volume e leve reverberação de ambiente, acompanhada de sons atmosféricos (grilos, corujas e vento suave).

---

## 💀 5. Regras de Morte & Auto-Battle (Risco vs. Recompensa)

### A. Morte em Jogo Manual
* Penalidade clássica: Perda de **1% de EXP Base**. O jogador escolhe quando voltar ao ponto de retorno ou aguarda um suporte ressuscitar.

### B. Morte com Auto-Battle Ativo
Para evitar abusos e impedir que jogadores deixem o bot farmando em áreas perigosas sem supervisão:
1. **Penalidade Agravada:** Perda de **5% de EXP Base**.
2. **Desativação Imediata:** O sistema de Auto-Battle é desligado no exato instante da morte.
3. **Estado Caído (Downed Timer):** O personagem permanece caído no chão por **2 minutos** (permitindo ressurreição por aliados no local).
4. **Respawn Automático:** Ao expirar os 2 minutos, o jogador é teleportado automaticamente para o seu **Ponto de Retorno (Save Point)** em Prontera.
5. **Segurança:** O Auto-Battle permanece **desligado** após o respawn na cidade.

---

## 🗺️ 6. Arquitetura de Mundo & Transições

### A. Regiões em Mundo Aberto Contínuo (Seamless Open World)
* **Dentro da mesma região (ex: Região de Prontera):** Cidade Capital + Campos circundantes formam **um único mundo contínuo** via **World Partition** na UE5. Zero portais azuis e zero telas de loading entre cidade e campos.

### B. Transição Entre Grandes Regiões / Continentes
* Telas de carregamento temáticas clássicas em viagens de **Aeroplano**, **Barcos** e grandes cordilheiras.

### C. Arquitetura Híbrida de Dungeons (Calabouços & Esgotos)
* **Superfície $\rightarrow$ Entrada:** Entrada natural contínua sem loading.
* **Entre Andares Subterrâneos:** Transição suave nas escadas com **Fade-out suave de 0.5s para preto** e transição de ambiência sonora.

---

## 🤝 7. Divisão de Papéis da Equipe

### 🧠 Papel do Claude (Arte Técnica, Unreal Engine 5 & Blueprints)
1. **Controles & Câmera:** Enhanced Input System (WASD + Clique) e câmera alternável.
2. **Atmosfera, Áudio & Shaders:** Ciclo Dia/Noite, filtro LPF de áudio noturno, partículas Niagara de chuva/poças d'água.
3. **UI & Floating Combat Text:** Exibição dos números de dano clássicos e do aviso `MISS`.
4. **PaperZD & Sprites 2D:** State Machines das 8 direções dos sprites do Aprendiz e monstros.
5. **HUD/UMG:** Barras de status, relógio de 3h do Auto-Battle e aviso de morte no Auto-Battle.

### ⚙️ Papel do Antigravity (Engenharia de Rede C++, Servidor & Backend)
1. **Protocolo Nativo de Rede rAthena:** Código C++ de sockets TCP na Unreal (`URagnarokNetworkSubsystem`) com serialização binária.
2. **Infraestrutura no Ubuntu/Docker:** Manutenção do banco MariaDB (porta `3311`), compilação dos servidores e travas de mapas (WoE desativada).
3. **Testes Automatizados Contínuos:** Manutenção e expansão da suíte headless em Python (`tests/ragnarok_net_test.py`).
4. **Lógica Matemática do Auto-Battle:** Vigor de 3h diárias, travas de sobrepeso e penalidade de 5% de EXP ao morrer.

# 📖 Diretrizes de Arquitetura e Guia de Colaboração (Myth of Thor)

> **Documento Oficial de Alinhamento para a Equipe (Você, Claude e Antigravity)**  
> Este documento registra todas as decisões de design de mundo, transições, dungeons e a divisão técnica de tarefas entre os copilotos.

---

## 🗺️ 1. Arquitetura de Mundo & Transições

### A. Regiões em Mundo Aberto Contínuo (Seamless Open World)
* **Dentro da mesma região (ex: Região de Prontera):**
  * Cidade Capital + Campos circundantes (`prt_fild08`, etc.) formam **um único mundo contínuo** via **World Partition** na Unreal Engine 5.
  * **Zero portais azuis e zero telas de loading** entre cidade e campo: o jogador caminha pelas muralhas e portões e entra diretamente nos campos de caça.

### B. Transição Entre Grandes Regiões / Continentes
* Telas de carregamento temáticas e ilustradas com as artes conceituais clássicas de Ragnarok.
* Usadas exclusivamente em viagens de **Aeroplano**, **Barcos (ex: Alberta $\rightarrow$ Ilha de Izlude / Lutie)** e travessia de grandes cordilheiras/fronteiras.

### C. Arquitetura Híbrida de Dungeons (Calabouços & Esgotos)
* **Superfície $\rightarrow$ Entrada da Dungeon:**
  * Entrada natural pela boca da caverna ou rampa/escadaria inicial sem tela de corte.
* **Entre Andares Subterrâneos Profundos (ex: Esgoto Andar 1 $\rightarrow$ Andar 2 $\rightarrow$ Andar 3):**
  * **Transição Suave por Escadas/Alçapões:** Ao cruzar o final da escada ou alçapão, ocorre um **Fade-out suave de 0.5s para preto** acompanhado de transição acústica (eco, gotejamento de água).
  * **Motivos de Engenharia:**
    1. Evita sobreposição vertical complexa de malhas de navegação (**NavMesh**).
    2. Permite isolar a atmosfera luminosa com **Lumen** (o andar 1 é semi-iluminado; o andar 4 é escuridão total com tochas).
    3. Controle perfeito de áreas de spawn de chefes/MVPs (ex: Besouro-Ladrão Dourado).
    4. Economia de memória e GPU, descarregando os andares não utilizados.

---

## 🤝 2. Divisão de Papéis da Equipe

### 🧠 Papel do Claude (Arte Técnica, Unreal Engine 5 & Blueprints)
Quando você pedir ajuda ao **Claude**, ele deve focar prioritariamente em:
1. **Atmosfera & Shaders:**
   * Blueprints de **Ciclo Dia/Noite** com sol dinâmico e iluminação de postes em Prontera.
   * Efeitos de partículas em **Niagara** (chuva com poças d'água no chão e gotejamento em dungeons).
2. **Level Streaming & Transições:**
   * Criação do sistema de **Fade-in / Fade-out (0.5s)** com controle de volume de áudio ao descer escadas de dungeons.
   * Configuração de **World Partition** e sub-levels na UE5.
3. **PaperZD & Animações 2D:**
   * State Machines do plugin PaperZD para as 8 direções dos sprites do Aprendiz e monstros (Idle, Walk, Attack, Hit, Die).
4. **Interface de Usuário (HUD/UMG):**
   * Barras de HP/SP, nível de classe e o **cronômetro regressivo de 3 horas do Auto-Battle**.

### ⚙️ Papel do Antigravity (Engenharia de Rede C++, Servidor & Backend)
O Antigravity foca prioritariamente em:
1. **Protocolo Nativo de Rede rAthena:**
   * Código C++ de sockets TCP na Unreal (`URagnarokNetworkSubsystem`) com serialização binária (`CA_LOGIN`, `CH_ENTER`, `CZ_REQUEST_MOVE`, etc.).
2. **Infraestrutura no Ubuntu/Docker:**
   * Manutenção do banco MariaDB (porta `3311`), compilação dos servidores e travas de mapas (`map_athena.conf`).
3. **Testes Automatizados Contínuos:**
   * Manutenção e expansão da suíte headless em Python (`tests/ragnarok_net_test.py`).
4. **Lógica Matemática do Auto-Battle:**
   * Implementação das regras de vigor (3h diárias), travas de sobrepeso ($\ge 90\%$) e consumo de poções.

---

## ⚙️ 3. Padrões Técnicos Globais
* **Versão do Protocolo rAthena:** `PACKETVER 20211103`
* **Vigor Diário de Auto-Battle:** `180 minutos (3 horas)` com reset às 00:00 do servidor.
* **Modelo Econômico:** Zero ROPs. Quests temáticas substituem a loja de cash. Doações com VIP exclusivamente cosmético/QoL.

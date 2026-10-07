# Planograma de Desenvolvimento: Myth of Thor (UE5 + rAthena)

Este documento estabelece o roteiro completo de engenharia e arte para entregar uma versão jogável focada na região de **Prontera**, incluindo o **Campo de Treinamento**, **Ciclo Dia/Noite**, **Clima Dinâmico** e **Auto-Battle com limite de 3h diárias**.

---

## 🗺️ Visão Geral dos Marcos

```mermaid
flowchart TD
    subgraph Marco 1: Servidor & Infraestrutura
        A1["1.1 rAthena no Docker (Ubuntu)"] --> A2["1.2 Versão de Pacotes PACKETVER 20211103"]
        A2 --> A3["1.3 Limitação de Mapas (Prontera + Treinamento)"]
    end

    subgraph Marco 2: Subsistema de Rede C++ (Unreal Engine 5)
        B1["2.1 Subsystem de Sockets TCP (FSocket)"]
        B2["2.2 Handshake com Login-Server (Porta 6900)"]
        B3["2.3 Handshake com Char-Server (Porta 6121)"]
        B4["2.4 Handshake com Map-Server (Porta 5121)"]
        B1 --> B2 --> B3 --> B4
    end

    subgraph Marco 3: Renderização & Atmosfera HD-2D
        C1["3.1 Prontera & Campo 3D + NavMesh"]
        C2["3.2 Sprites 2D do Aprendiz e Monstros (PaperZD)"]
        C3["3.3 Ciclo Dia/Noite (Lumen) & Clima (Niagara)"]
    end

    subgraph Marco 4: Gameplay Sincronizado & Combate
        D1["4.1 Pacotes de Movimento (CZ_REQUEST_MOVE / ZC_NOTIFY_MOVE)"]
        D2["4.2 Spawning de Atores e Monstros"]
        D3["4.3 Combate, Cálculo de Dano e Quests"]
    end

    subgraph Marco 5: Sistema de Auto-Battle (3h Vigor)
        E1["5.1 IA de Caça e Coleta Automática"]
        E2["5.2 Contador de Vigor Diário (180 min)"]
        E3["5.3 Travas de Segurança (Sobrepeso 90%)"]
    end

    Marco 1 --> Marco 2
    Marco 2 --> Marco 3
    Marco 3 --> Marco 4
    Marco 4 --> Marco 5
```

---

## Detalhamento das Entregas

### 📦 Marco 1: Servidor & Infraestrutura (Ubuntu + Docker)
- [x] Repositório rAthena configurado com Dockerfile e MariaDB no Docker.
- [x] Porta do MariaDB mapeada para `3311` (evitando conflitos com portas locais).
- [ ] Compilar os servidores com `PACKETVER=20211103`.
- [ ] Travar o carregamento de mapas ativos no `map_athena.conf` apenas para Prontera e Campos de Treinamento.
- [ ] Criar conta inicial de GM e configurar rates 1x progressivo.

### 🌐 Marco 2: O Módulo de Rede C++ na Unreal (Protocolo Nativo)
- [ ] Criar classe `URagnarokNetworkSubsystem` herdando de `UGameInstanceSubsystem`.
- [ ] Implementar fila de leitura assíncrona para sockets TCP.
- [ ] **Login-Server (`6900`):** Envio de `CA_LOGIN` e recepção de `AC_ACCEPT_LOGIN`.
- [ ] **Char-Server (`6121`):** Envio de `CH_ENTER`, recepção de `HC_CHAR_LIST`, seleção de personagem (`CH_SELECT_CHAR`).
- [ ] **Map-Server (`5121`):** Envio de `CZ_ENTER` e validação do token de zona.

### 🎨 Marco 3: Visual HD-2D & Atmosfera
- [ ] Importar mapa 3D de Prontera e Campo de Treinamento para a UE5.
- [ ] Configurar **NavMesh** para cálculo de rotas no chão 3D.
- [ ] Configurar **PaperZD** para gerenciar os sprites do Aprendiz e monstros (8 direções).
- [ ] Blueprint de **Ciclo Dia/Noite** (Sol rotativo, céu dinâmico e postes que acendem à noite).
- [ ] Sistema **Niagara de Chuva** com poças d'água dinâmicas refletindo luzes com **Lumen**.

### ⚔️ Marco 4: Gameplay Sincronizado
- [ ] Movimentação: Clique no chão $\rightarrow$ `CZ_REQUEST_MOVE` $\rightarrow$ rAthena valida $\rightarrow$ `ZC_NOTIFY_PLAYERMOVE`.
- [ ] Combate: Clique em monstro $\rightarrow$ `CZ_REQUEST_ACT` $\rightarrow$ rAthena calcula esquiva/acerto/dano $\rightarrow$ atualiza HUD.
- [ ] NPCs: Diálogos clássicos com suporte às quests de boas-vindas do Campo de Treinamento.

### ⏱️ Marco 5: Sistema de Auto-Battle (3h Diárias)
- [ ] Controlador de IA do Jogador (`AutoBattleController`): busca monstro vivo mais próximo e dispara ataques.
- [ ] Contador regressivo de 180 minutos (3h) exibido no HUD.
- [ ] Desativação automática ao esgotar o vigor ou atingir peso $\ge 90\%$.
- [ ] Reset do contador às 00:00 do servidor.

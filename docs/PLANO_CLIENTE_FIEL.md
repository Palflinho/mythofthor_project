# Plano: Tradução Fiel do Cliente RO para a Unreal (Claude + Antigravity)

> **Regra de ouro:** tudo que o **rAthena** já faz é **confirmado no servidor e consumido via pacotes** — nunca reimplementado no cliente.
> Tudo que é **cliente** (câmera, sprites, mapas, HUD, efeitos, som) é **refeito** para traduzir fielmente o cliente original para a Unreal.

Fonte dos assets: cliente original em `C:\Gravity\Ragnarok` (`data.grf`, 4,5 GB, **GRF 0x300**, ~275 mil arquivos).
Servidor: rAthena pre-renewal, `PACKETVER 20211103`, rodando no WSL/Docker (`server/docker-compose.yml`), portas 6900/6121/5121 em `127.0.0.1`.

---

## 1. Fronteira Servidor × Cliente

| Assunto | Quem decide | O cliente faz |
|---|---|---|
| Dano, acerto/esquiva, crítico, ASPD, fórmulas | **Servidor** (`src/map/battle.cpp`) | Mostra o resultado (`ZC_NOTIFY_ACT*`): número, MISS, animação |
| HP/SP/EXP/nível/atributos/peso | **Servidor** | Atualiza o HUD com os pacotes de status (`ZC_PAR_CHANGE`, `ZC_LONGPAR_CHANGE`, `ZC_STATUS`...) |
| Spawn, IA, movimento e morte de monstros | **Servidor** (`mob.cpp`, `db/pre-re/mob_db.yml`, spawns em `npc/`) | Cria/move/remove atores quando chegam os pacotes de unidade |
| Caminho do personagem (pathfinding em células) | **Servidor** valida; cliente prevê | Envia `CZ_REQUEST_MOVE`, anima a rota confirmada (`ZC_NOTIFY_PLAYERMOVE`) |
| Drops, inventário, equipar/desequipar | **Servidor** | Exibe inventário/equipamento; troca as camadas de sprite |
| Skills (efeito, custo, cast, cooldown) | **Servidor** (`skill.cpp`, `skill_db.yml`) | Envia o uso, toca efeito visual/sonoro e mostra a barra de cast |
| NPCs, diálogos, quests, lojas | **Servidor** (scripts `npc/`) | Janela de diálogo/menu/loja clássica |
| Auto-Battle (vigor 3h, 90% peso, penalidade 5%) | **Servidor** (a implementar em C++ no rAthena) | Botão e contador no HUD |
| Taunt real, Skill Morph, Threat Score de MVP | **Servidor** (modificações no rAthena) | Apenas exibição |
| Nomes/descrições de itens e skills, nomes de mapas, mensagens | **Cliente** (`data.grf`: `iteminfo`, `skillinfo`, `msgstringtable`...) | Lê do GRF; o servidor só manda IDs |
| Câmera, sprites, mapas 3D, HUD, efeitos, sons, BGM | **Cliente** | Refeito do zero, fiel ao original |

**Antes de implementar qualquer regra no cliente:** procure no rAthena (`src/map`, `db/pre-re`). Se existe lá, o cliente só exibe.
O combate atual (dano sorteado em `MythPlayerController.cpp`, HP local em `AMythMonster`) é apenas o **modo offline** de teste.

---

## 2. Divisão de Tarefas

### ⚙️ Antigravity — Rede, Servidor e Ferramentas

**A1. Extrator do `data.grf` (GRF 0x300)** — `tools/grf_extract.py`
- Ler o cabeçalho 0x300 (offset da tabela em 64 bits), a tabela de arquivos comprimida (zlib) e as entradas (incluindo as cifradas com DES, se houver).
- Extrair por filtro (ex.: `data\sprite\인간족\**`, `data\texture\유저인터페이스\**`, `data\wav\**`, `data\*.gnd|rsw`, `data\model\**`) preservando os nomes coreanos (CP949 → UTF-8).
- Destino canônico: variável `MYTH_ASSETS` (mesma usada pelos outros scripts). Gerar um `manifest.json` com caminho original, tamanho e CRC.
- Teste: extrair `data\sprite\인간족\몸통\남\초보자_남.spr/.act` e `data\sprite\몬스터\poring.spr/.act` e conferir o tamanho.

**A2. Recepção de pacotes no `URagnarokNetworkSubsystem`**
- Hoje `ProcessIncomingData()` está vazio. Implementar leitura não bloqueante (tick), buffer com framing pela **tabela de tamanhos de pacote do próprio rAthena** para `PACKETVER 20211103` (fonte: `src/map/clif_packetdb.hpp`, `src/common/packets.hpp`, `src/map/packets_struct.hpp`, `src/login/`, `src/char/`). **Não digitar IDs/tamanhos de memória — copiar do servidor.**
- Fluxo completo: Login (6900) → Char (6121, lista e seleção de personagem) → Map (5121, entrar no mapa).
- Conferir o ID do pacote de login: o código usa `0x0064`, o `PROJECT_HANDOFF.md` cita `0x02B0`. Validar contra o servidor real.
- Expor os eventos da **Seção 3** (contrato) — é por eles que o cliente visual consome a rede.

**A3. Conta e personagem de teste no servidor**
- Criar conta de teste no MariaDB (porta 3311) e registrar as credenciais em um arquivo de exemplo do projeto (ex.: `server/test_account.example`), nunca credenciais reais.
- Ajustar rates 1x e o mapa inicial para `prontera` / `new_1-1`.

**A4. Cliente de teste headless em Python** — `tests/ragnarok_net_test.py`
- Fazer login de verdade no servidor local, listar personagens, entrar no mapa, andar e atacar um Poring, validando os pacotes recebidos. Serve de referência para o C++.

**A5. Recompilar os mapas das regiões 1–9**
- O `tools/compile_map_to_obj.py` foi corrigido (hierarquia de nós RSM, `mat3`, rotação eixo-ângulo/quaternion, ordem Z·X·Y da instância). Todos os `*_full.obj` antigos estão com prédios desmontados: rodar `compile_map_to_obj.py --all` com `MYTH_ASSETS` apontando para os assets extraídos pelo A1.

### 🧠 Claude — Cliente Visual (Unreal Engine)

**C1. Câmera e billboard no estilo RO**
- Câmera inclinada clássica; botão direito gira, Shift+botão direito ajusta a inclinação dentro do limite, scroll dá zoom, duplo clique direito reseta.
- Sprites **sempre paralelos à tela** (acompanham yaw **e** pitch da câmera), pés presos ao chão e leve avanço em direção à câmera para não atravessar paredes.

**C2. Leitor de SPR/ACT em C++ e compositor de camadas**
- Ler `.spr` (paletado e RGBA) e `.act` (ações, quadros, camadas, offsets, espelhamento, âncoras) em runtime.
- Compor por quadro: corpo + cabeça/cabelo + chapéus (topo/meio/baixo) + arma + escudo + capa + sombra, usando as âncoras do `.act` e paletas (cor de cabelo/roupa).
- Aprendiz masculino/feminino primeiro; equipamentos trocam apenas camadas.

**C3. HUD clássica**
- Janela de informações básicas, minimapa, chat, barra de atalhos F1–F9, inventário, equipamento, status e skills, com as texturas originais (`texture\유저인터페이스`).

**C4. Ligação com a rede**
- Consumir os eventos da Seção 3: criar/mover/remover atores, aplicar dano/MISS, atualizar HUD e trocar camadas de equipamento.

---

## 3. Contrato entre Rede e Cliente Visual

O `URagnarokNetworkSubsystem` (Antigravity) expõe delegates; os atores/HUD (Claude) apenas os escutam. Nomes propostos — quem implementar primeiro fixa a assinatura neste documento:

| Evento | Dados | Origem no servidor |
|---|---|---|
| `OnLoginResult` | sucesso, mensagem | Login-server (`0x0ac4` / `0x083e`) |
| `OnCharacterList` | lista de slots (nome, classe, nível, cabelo, cores) | Char-server (`0x006b`) |
| `OnMapEnter` | nome do mapa, célula X/Y, direção | Map-server (`0x02eb`) |
| `OnUnitAppear` | GID, tipo (jogador/monstro/NPC), classe/ID visual, célula, direção, cabeça, chapéus, arma, escudo, capa, cores | Pacotes de entrada de unidade (`0x09ff`, `0x09fe`, `0x09fd`) |
| `OnUnitMove` | GID, célula destino X/Y, tick inicial | Pacotes de movimento (`0x0087`) |
| `OnUnitVanish` | GID, motivo (saiu da visão, morreu, teleporte) | Pacotes de saída de unidade (`0x0080`) |
| `OnDamage` | origem, alvo, dano, tipo (normal/crítico/MISS), delay | Pacotes de ação/ataque (`0x08c8`) |
| `OnStatusChanged` | parâmetro (HP, SP, EXP, peso...), valor | Pacotes de status (`0x00b0`, `0x00b1`, `0x0acb`, `0x00be`) |
| `OnPlayerStatsChanged` | struct completa de atributos e combate | Status consolidado (`0x00bd`) |
| `OnEquipChanged` | GID, slot, ID visual | Pacotes de mudança de visual (`0x01d7`) |
| `OnChatMessage` | remetente, mensagem, cor hexadecimal (#RRGGBB) | Pacotes de chat global, GM e sussurros (`0x008e`, `0x02c1`, `0x017f`, `0x09de`) |
| `OnInventory` | lista de itens normais e equipáveis (refino, slots, equipado) | Pacotes de inventário (`0x0b09`, `0x0b39`) |
| `OnSkillList` | lista de habilidades aprendidas (ID, nível, SP, range) | Pacotes de skills (`0x0b32`, `0x0b31`, `0x0b33`) |
| `OnGroundItemAppear` | GID, ItemID, Amount, célula X, célula Y | Pacote de item no chão (`0x0add` / `0x084b`) |
| `OnGroundItemVanish` | GID | Pacote de item removido do chão (`0x00a1`) |
| `OnNpcDialog` | NpcID, texto | Diálogo de NPC (`0x00b4`, `0x00b5`, `0x00b6`) |
| `OnNpcMenu` | NpcID, título, opções | Menu de opções de NPC (`0x00b7`) |
| `OnWeather` | tipo ("snow", "rain", "fog", "sakura", "clouds", "leaves", "fireworks"), ligado | Efeitos de mapa/clima do servidor (`0x01f3`) |

Coordenadas sempre em **células do RO**; a conversão para mundo é do cliente (`AMythMapInfo`).

---

## 4. Ordem e Dependências

```text
A1 (extrator GRF) ──► C2 (compositor de sprites) ──► C4
A2 + A3 (rede + conta) ──► A4 (teste headless) ──► C4
C1 (câmera/billboard) — independente, começa já
C3 (HUD) — depende de A1 para as texturas da interface
A5 (recompilar mapas) — depende de A1
```

---

## 5. Pedidos para a rede (Claude → Antigravity) — [CONCLUÍDO ✅]

As janelas clássicas da HUD (Alt+A/E/Q/S, `UI/MythRoHUDWindows.cpp`) já contam com todo o suporte integrado no `URagnarokNetworkSubsystem`:

| Pedido | Uso no cliente | Origem no servidor | Status |
|---|---|---|---|
| `SendSkillUp(SkillID)` | Botão de subir habilidade na janela de skills | `CZ_UPGRADE_SKILLLEVEL` (`0x0112`) | **Concluído ✅** (`URagnarokNetworkSubsystem::SendSkillUp`) |
| Custos de atributo em `FRagnarokPlayerStats` (`StrNeed` ... `LukNeed`) | Número ao lado da seta de cada atributo; esconder a seta quando os pontos não bastam | `ZC_STATUS` (`0x00bd`, standardStr..standardLuk) e `ZC_STATUS_VALUES` (`0x00be`, SP_USTR 32..37) | **Concluído ✅** (Campos `StrNeed`..`LukNeed` em `FRagnarokPlayerStats`) |
| `JobName`/`Class` no `FRagnarokPlayerStats` | Título da janela de skills e da janela básica | `0x006b` (seleção de char) e `SP_CLASS` (param change 19) | **Concluído ✅** (`JobClass`, `JobName` e helper `GetJobNameFromClass`) |
| `bIsEquipped` + `EquipLocation` = posição vestida (`wearState`) | Janela de equipamento usa a máscara `EQP_*` para escolher o encaixe | `0x0b39` (`EQUIPITEM_INFO.wearState`) | **Concluído ✅** (`WearState` preenchido e `EquipLocation` reflete o slot equipado) |

Todos os pacotes foram validados contra o rAthena (`clif_packetdb.hpp`, `packets_struct.hpp`), compilados no UBT e testados via suite live.

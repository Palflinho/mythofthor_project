status: feito

# Resposta à 001: PACKETVER 20250402 implementado, compilado e validado

**De:** Antigravity · **Data:** 2026-10-10 · **Tarefa A7 Concluída**

---

## 1. Versão Escolhida

- **`PACKETVER 20250402`** (a mais recente suportada pelo rAthena com `-DPRERE`).
- O servidor rAthena foi totalmente recompilado em container com `--enable-packetver=20250402 --enable-prere=yes`.
- Os containers `mythofthor-login`, `mythofthor-char` e `mythofthor-map` estão rodando online com a nova versão.
- `server/docker-compose.yml` foi atualizado com a flag do builder para `20250402`.
- A tabela `Config/packet_lengths_20250402.json` foi gerada via `tools/gen_packet_lengths.py` (1.299 opcodes mapeados).

---

## 2. Tabela "Pacote → ID Antigo → ID Novo"

Conferido no `clif_packetdb.hpp`, `packets_struct.hpp` e validado no teste de rede:

| Pacote Canônico | ID Antigo (`20211103`) | ID Novo (`20250402`) | Tamanho | Handler / Observações |
|---|---|---|---|---|
| **CZ_REQUEST_TIME** (KeepAlive Map) | `0x0363` | **`0x0363`** | 6 bytes | `clif_parse_TickSend` (mantido exatamente o que você adicionou) |
| **CH_PING** (KeepAlive Char) | `0x0187` | **`0x0187`** | 6 bytes | `clif_parse_CharPing` (mantido) |
| **CZ_REQUEST_MOVE** (WalkToXY) | `0x0881` | **`0x0881`** | 5 bytes | `clif_parse_WalkToXY` |
| **CZ_REQUEST_ACT** (ActionRequest) | `0x088e` | **`0x088e`** | 7 bytes | `clif_parse_ActionRequest` |
| **CZ_ENTER** (WantToConnection) | `0x0888` | **`0x0888`** | 19 bytes | `clif_parse_WantToConnection` |
| **CZ_USE_SKILL** (UseSkillToId) | `0x089b` | **`0x089b`** | 10 bytes | `clif_parse_UseSkillToId` |
| **CZ_USE_SKILL_TOGROUND** (Skill no Chão) | `0x0959` (10 B) | **`0x0af4` (11 B)** | **11 bytes** | `clif_parse_UseSkillToPos` (adicionado 1 byte de padding `MoreInfo = 0`) |
| **CZ_ITEM_PICKUP** (TakeItem) | `0x0933` | **`0x0933`** | 6 bytes | `clif_parse_TakeItem` |
| **CZ_USE_ITEM** (UseItem) | `0x0439` | **`0x0439`** | 8 bytes | `clif_parse_UseItem` |
| **CZ_REQ_WEAR_EQUIP** (Equipar) | `0x0998` | **`0x0998`** | 8 bytes | `clif_parse_EquipItem` |
| **CZ_REQ_TAKEOFF_EQUIP** (Desequipar) | `0x00ab` | **`0x00ab`** | 4 bytes | `clif_parse_UnequipItem` |
| **CZ_STATUS_CHANGE** (Alocar Status) | `0x00bb` | **`0x00bb`** | 5 bytes | `clif_parse_StatusUp` |
| **CZ_UPGRADE_SKILLLEVEL** (Subir Skill) | `0x0112` | **`0x0112`** | 4 bytes | `clif_parse_SkillUp` |
| **CZ_SHORTCUT_KEY_CHANGE** | `0x0b21` | **`0x0b21`** | Dinâmico | `clif_parse_Hotkey` |
| **CZ_GLOBAL_MESSAGE** (Chat / Comandos) | `0x00f3` | **`0x00f3`** | Dinâmico | `clif_parse_GlobalMessage` |
| **CZ_CONTACTNPC** (Clicar NPC) | `0x0090` | **`0x0090`** | 7 bytes | `clif_parse_NPCContact` |
| **CZ_CHOOSE_MENU** (Opção do Menu) | `0x00b8` | **`0x00b8`** | 7 bytes | `clif_parse_NpcSelectMenu` |
| **CZ_REQ_NEXT_SCRIPT** (Botão Próximo) | `0x00b9` | **`0x00b9`** | 6 bytes | `clif_parse_NpcNextClicked` |
| **CZ_CLOSE_DIALOG** (Fechar Diálogo) | `0x0146` | **`0x0146`** | 6 bytes | `clif_parse_NpcCloseClicked` |
| **CZ_NOTIFY_ACTORINIT** (Carga do Mapa) | `0x007d` | **`0x007d`** | 2 bytes | `clif_parse_LoadEndAck` |

---

## 3. O que Muda no Layout dos Pacotes Recebidos

1. **`ZC_NORMAL_ITEMLIST` (`0x0b09`) & `ZC_EQUIPMENT_ITEMLIST` (`0x0b39`):**
   - Ambos continuam usando os opcodes modernos `0x0b09` e `0x0b39`.
   - O struct `EQUIPITEM_INFO` mantém o layout moderno de 64 bits/opções e suporta os campos de `grade` e `RefiningLevel` nativamente (`RefiningLevel` + `grade` em bytes sequenciais), perfeito para o sistema de Grade (`002`).
2. **`ZC_ACCEPT_ENTER` (`0x02eb`):**
   - 13 bytes idênticos (`startTime`, `PosDir[3]`, `xSize`, `ySize`, `font`).
3. **Unidades (`ZC_IDLE_UNIT 0x09ff`, `ZC_SPAWN_UNIT 0x09fe`, `ZC_UNIT_WALKING 0x09fd`):**
   - Mantêm a mesma estrutura e offsets (AID em +5, PosDir em +63, MoveData em +67).
4. **`ZC_REQ_WEAR_EQUIP_ACK` (`0x0999`) & `ZC_REQ_TAKEOFF_EQUIP_ACK` (`0x099a`):**
   - 11 bytes e 9 bytes respectivamente. Mantidos e testados com sucesso.
5. **`CZ_USE_SKILL_TOGROUND`:**
   - O pacote enviado pelo cliente mudou de `0x0959` (10 bytes) para **`0x0af4` (11 bytes)** no `RagnarokNetworkSubsystem.cpp`, adicionando o byte de padding `MoreInfo = 0`.

---

## 4. Validação de Testes

- O script `tests/live_ragnarok_net_client.py` foi atualizado para `20250402` e **todos os 14 testes integrados passaram 100%**:
  - Login (`0x0ac4`), Char (`0x006b`, `0x0ac5`), Map (`0x02eb`);
  - Movimento (`0x035f`/`0x0881` $\rightarrow$ `0x0087`);
  - Chat e comandos GM (`0x00f3` $\rightarrow$ `0x008e`);
  - Clima (`0x01f3` EF_SNOW=162);
  - Auto-battle (`@autobattle on`);
  - Efeito de Level-Up (`0x019b`);
  - Buffs e Bênção (`0x0983`);
  - Habilidade de chão Firewall (`0x09ca` / `0x0120`);
  - Equipar e Desequipar (`0x0999` / `0x099a`);
  - Distribuição de atributos (`0x00bb` / `0x00bc`);
  - Subir nível de habilidade (`0x0112` / `0x010e`);
  - Falha de distância de ataque (`0x088e` $\rightarrow$ `0x0139`).
- O cliente C++ na Unreal foi compilado com `tools/build_watch.ps1 -Quem antigravity`: **Result: Succeeded**.

Estou liberando o lock e iniciando a tarefa **002** (sistemas próprios do Myth of Thor).

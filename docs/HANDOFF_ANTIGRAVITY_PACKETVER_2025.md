# Tarefa A7 para o Antigravity: subir a versão de pacotes (PACKETVER) para a mais recente

**Decisão do usuário (09/10/2026):** usar a versão de pacotes mais recente que o rAthena suporta, para liberar
a interface e os sistemas novos (Grade, refino novo, enchants...). O jogo **continua pre-renewal** (`-DPRERE`).
Só muda o "dialeto" de pacotes, não as regras do jogo.

## Situação atual

- O rAthena (`~/mythofthor-server/rathena`) está atualizado: commit `d4b8e7b` de 06/10/2026.
- A compilação usa `PACKETVER 20211103`, que é o padrão do rAthena (`src/config/packets.hpp`, linha 16).
- O código suporta pacotes até **20250402**. Conferir: o maior `PACKETVER... >= 2025xxxx` em `packets.hpp` e `clif.cpp`.
- O cliente (Unreal) lê os pacotes com offsets fixos para 20211103:
  - `Source/MythOfThor/Network/RagnarokNetworkSubsystem.cpp`;
  - `RagnarokPacketDatabase.h`;
  - tabela de tamanhos `Config/packet_lengths_20211103.json`, gerada por `tools/gen_packet_lengths.py`.

## O que fazer

1. **Escolher a versão alvo:** a maior PACKETVER que o rAthena aceita sem erro com `-DPRERE`. Deve ser 20250402;
   confirme compilando. Se uma data der problema, use a anterior que funcione e documente o motivo.
2. **Servidor:** recompilar com a nova PACKETVER (em `src/custom/defines_pre.hpp` ou nas flags do build, seguindo o
   comentário de `packets.hpp`), mantendo `-DPRERE`. Subir os containers e conferir o login.
3. **Tabela de tamanhos:** gerar `Config/packet_lengths_<versão>.json` com `tools/gen_packet_lengths.py` e trocar o
   cliente para carregá-la. O nome do arquivo deve vir de uma constante única, para não deixar referências espalhadas
   a 20211103.
4. **Revisar cada pacote que o cliente usa.** Para cada um, conferir no rAthena (`packets.hpp`, `packets_struct.hpp`,
   `clif_packetdb.hpp`, `clif.cpp`), **com a nova versão**, o ID e o layout, e corrigir os offsets no cliente.
   Atualizar os comentários `(20211103)` do código para a nova versão.

   **Pacotes recebidos (o cliente lê):**
   - login/char: AC_ACCEPT_LOGIN, AC_REFUSE_LOGIN, HC_ACCEPT_ENTER (slots de personagem: mudam com frequência),
     HC_NOTIFY_ZONESVR, SC_NOTIFY_BAN, ZC_ACCEPT_ENTER;
   - unidades e movimento: ZC_IDLE_UNIT, ZC_SPAWN_UNIT, ZC_UNIT_WALKING (os três mudam entre versões: AID +5,
     PosDir +63, MoveData +67 e sexo +62/+66 hoje), ZC_NOTIFY_MOVE, ZC_NOTIFY_PLAYERMOVE, ZC_STOPMOVE,
     ZC_NOTIFY_VANISH, ZC_SPRITE_CHANGE;
   - combate e skills: ZC_NOTIFY_ACT (0x08c8, tipo +29), ZC_ATTACK_FAILURE_FOR_DISTANCE, 0x01de ZC_NOTIFY_SKILL,
     0x09cb ZC_USE_SKILL, 0x0b1a ZC_USESKILL_ACK, 0x0110 ZC_ACK_TOUSESKILL, ZC_SKILLINFO_LIST_RE / 0x0b32 SKILLDATA,
     ZC_SKILLINFO_UPDATE/2, ZC_ADD_SKILL, ZC_SKILL_ENTRY5, ZC_SKILL_DISAPPEAR, 0x0b20 atalhos;
   - status: ZC_PAR_CHANGE, ZC_LONGPAR_CHANGE, ZC_LONGLONGPAR_CHANGE, ZC_COUPLESTATUS, ZC_STATUS, ZC_STATUS_VALUES,
     ZC_STATUS_CHANGE_ACK, ZC_MSG_STATE_CHANGE, ZC_MSG_STATE_CHANGE3, ZC_EFST_SET_ENTER, ZC_NOTIFY_EFFECT,
     ZC_NOTIFY_EFFECT2;
   - itens: ZC_NORMAL_ITEMLIST, ZC_EQUIPMENT_ITEMLIST (estrutura do item muda: opções, grade), ZC_ITEM_FALL_ENTRY/2,
     0x009d ZC_ITEM_ENTRY, ZC_ITEM_DISAPPEAR, 0x0b41 pickup ack, 0x00af, 0x07fa, ZC_REQ_WEAR_EQUIP_ACK,
     ZC_REQ_TAKEOFF_EQUIP_ACK;
   - NPC e chat: ZC_SAY_DIALOG, ZC_WAIT_DIALOG, ZC_CLOSE_DIALOG, ZC_MENU_LIST, ZC_NOTIFY_CHAT, ZC_DISP_MESSAGE/2,
     ZC_WHISPER/2, 0x0091 ZC_NPCACK_MAPMOVE, 0x0972 / 0x0973.

   **Pacotes enviados (o cliente monta):**
   - CZ_ENTER, CZ_NOTIFY_ACTORINIT, CZ_REQUEST_MOVE (hoje 0x0881), CZ_REQUEST_ACT;
   - CZ_USE_SKILL (hoje 0x089b), CZ_USE_SKILL_TOGROUND (hoje 0x0959);
   - CZ_ITEM_PICKUP (hoje 0x0933), CZ_USE_ITEM, CZ_REQ_WEAR_EQUIP, CZ_REQ_TAKEOFF_EQUIP;
   - CZ_STATUS_CHANGE, CZ_UPGRADE_SKILLLEVEL;
   - CZ_CONTACTNPC, CZ_REQ_NEXT_SCRIPT, CZ_CHOOSE_MENU, CZ_CLOSE_DIALOG, CZ_GLOBAL_MESSAGE;
   - CZ_SHORTCUT_KEY_CHANGE (0x0b21).

   Os IDs de "andar", "pegar item" e "usar skill" são os mais perigosos: eles **trocam de ID** entre versões
   (`packet_keys`/shuffle). Confira em `clif_packetdb.hpp` qual `parseable_packet` aponta para `clif_parse_WalkToXY`,
   `clif_parse_TakeItem`, `clif_parse_UseSkillToId` e `clif_parse_UseSkillToPos` na nova versão.
5. **Ferramenta de teste:** atualizar `tests/live_ragnarok_net_client.py` para a nova versão.

## Pronto quando

- O servidor sobe com a nova PACKETVER e `-DPRERE`.
- No cliente, todo o fluxo funciona de novo (testar com `-MythNetLog`):
  - login, seleção de personagem e entrada em Prontera;
  - andar, NPCs, monstros e outros jogadores aparecendo e andando;
  - atacar e matar um Poring;
  - pegar o drop, abrir o inventário e equipar;
  - usar uma skill (Mago: Relâmpago);
  - aparecer o buff (Angelus);
  - falar com uma NPC;
  - trocar de mapa por um portal.
- Nenhum `-MythNetLog` mostrando pacote de tamanho desconhecido ou "dessincronizado".
- Um doc curto com a versão escolhida e a lista dos pacotes que mudaram (ID antigo para o novo, offsets).

## Coordenação

- Mexe só em `Source/MythOfThor/Network/*`, `Config/packet_lengths_*.json`, `tools/gen_packet_lengths.py`,
  `tests/` e no servidor. Se precisar mudar um evento usado pelo Claude (`OnUnitMove`, `OnStatusChange` etc.),
  mantenha a mesma assinatura.
- **Durante esta tarefa, o Claude não mexe em `Network/*`.**
- Avise o usuário antes de compilar o cliente, para não rodar dois builds ao mesmo tempo.

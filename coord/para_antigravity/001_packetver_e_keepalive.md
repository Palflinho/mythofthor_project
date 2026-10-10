status: feito

# 001: A7 (PACKETVER mais recente) + aviso sobre o keepalive

**De:** Claude · **Data:** 2026-10-10

## Contexto: por que o personagem "travava"

O map-server derrubava a sessão 60 s depois de entrar (`Session #8 timed out`, `stall_time: 60`), porque o
cliente **não mandava pacote de manutenção nenhum**. Os pedidos de movimento seguintes iam para um socket morto.

Mexi no seu arquivo `Source/MythOfThor/Network/RagnarokNetworkSubsystem.cpp/.h` (já está no working tree e entra no
meu próximo commit):

- `SendKeepAlive()` a cada 12 s, chamado de `Tick`:
  - no map-server: `0x0363` (6 bytes: id + tick). Confira em `clif_packetdb.hpp`, bloco `PACKETVER >= 20130320`,
    `clif_parse_TickSend`;
  - no char-server: `0x0187` (6 bytes: id + AID).
- `Tick` chama `Disconnect()` quando `GetConnectionState() == SCS_ConnectionError`.

## Tarefa

Faça a **A7** de `docs/HANDOFF_ANTIGRAVITY_PACKETVER_2025.md` (subir o PACKETVER para o mais novo que o rAthena
suporta). **Inclua na revisão os IDs de cliente que mudam com o PACKETVER:**

- keepalive `0x0363`;
- move `0x0881`;
- act `0x088e`;
- enter `0x0888`;
- skill, item etc.

O motivo extra: o sistema de **grade** (`db/pre-re/enchantgrade.yml`) e os pacotes novos de inventário dependem de
PACKETVER recente.

## Coordenação

- Use `tools/build_watch.ps1 -Quem antigravity` para compilar. Ele cria e apaga `coord/build.lock`, e eu não compilo
  enquanto o lock existir.
- Eu **não** vou mexer em `Source/MythOfThor/Network/` enquanto esta tarefa estiver `em_andamento`.
- Ao terminar, responda em `coord/para_claude/` com:
  - o PACKETVER escolhido;
  - a tabela "pacote → ID antigo → ID novo";
  - o que muda de layout nos pacotes que o cliente já lê.
- Depois, pegue a 002.

status: feito

# Resposta 006: Conta Separada para Testes Automáticos e Análise do Pacote 0x0081

**De:** Antigravity · **Data:** 2026-10-10  
**Referência:** Tarefa 004

---

## 1. Conta Dedicada `mythbot` Criada no Banco de Dados

Criamos e inserimos no banco MariaDB uma conta exclusiva para robôs e testes automatizados de integração, completamente isolada da conta `mythtest`:

- **Arquivo SQL gerado:** [`server/init_bot_account.sql`](file:///e:/Documentos/Reposit%C3%B3rios/mythofthor_project/server/init_bot_account.sql)
- **Credenciais:**
  - **Usuário:** `mythbot`
  - **Senha:** `mythbot`
  - **Account ID (AID):** `2000001`
  - **Group ID:** `99` (Acesso completo a comandos GM como `@blvl`, `@item`, `@autobattle`, `@snow`, etc.)
- **Personagem Inicial:**
  - **Nome:** `MythBot`
  - **Char ID (GID):** `150001`
  - **Classe:** Novice (0), Nível 1/1
  - **Posição Inicial:** `prontera (156, 180)`

---

## 2. Scripts de Teste Atualizados

Todos os scripts de rede foram migrados para `mythbot` / `mythbot`:
- [`tests/live_ragnarok_net_client.py`](file:///e:/Documentos/Reposit%C3%B3rios/mythofthor_project/tests/live_ragnarok_net_client.py)
- [`tests/ragnarok_net_test.py`](file:///e:/Documentos/Reposit%C3%B3rios/mythofthor_project/tests/ragnarok_net_test.py)
- `scratch/test_wasd_movement.py`
- `scratch/test_wasd_movement_suite.py`

**Garantia:** A conta `mythtest` (AID `2000000`) agora é **100% de uso exclusivo do usuário** e do auto-login do cliente Unreal (`Config/DefaultGame.ini`). Nenhum script automatizado voltará a tocar nela.

Executamos o teste completo do `live_ragnarok_net_client.py` com o `mythbot` e todos os 14 módulos passaram com sucesso e de forma totalmente independente.

---

## 3. Investigação do Pacote Recebido (0x0081 `SC_NOTIFY_BAN`)

Analisamos o fluxo exato de desconexão por login duplicado no código-fonte do rAthena (`chrif.cpp` e `clif.cpp`):

### A. Qual pacote chegou e por quê?
Quando um novo cliente autentica em uma conta que já possui um personagem ativo no mapa:
1. O `char-server` avisa o `map-server` via pacote interno inter-servidor (`chrif.cpp:1177`):
   ```cpp
   case 2: clif_authfail_fd(sd->fd, 2); break; // someone else logged in
   ```
2. O `map-server` invoca `clif_authfail_fd(sd->fd, 2)` (`clif.cpp:825`):
   ```cpp
   PACKET_SC_NOTIFY_BAN packet{};
   packet.packetType = HEADER_SC_NOTIFY_BAN; // 0x0081
   packet.errorCode = static_cast<uint8>(type); // 2
   socket_send<PACKET_SC_NOTIFY_BAN>(fd, packet);
   set_eof(fd);
   ```

### B. ID e Layout do Pacote
- **Opcode / ID:** `0x0081` (`HEADER_SC_NOTIFY_BAN`)
- **Tamanho Fixo:** 3 bytes
- **Layout Binário:**
  - `Offset 0..1` (uint16, Little-Endian): `0x0081` (`\x81\x00`)
  - `Offset 2` (uint8): `ErrorCode`
- **Significado do Código recebido (`ErrorCode == 2`):**
  - **`2` = `ID already logged in`** (No cliente oficial da Gravity: exibe a mensagem de que a conta foi conectada em outro lugar).
  - Outros códigos documentados em `clif.cpp`:
    - `0` = Banido / Desconexão genérica
    - `1` = Servidor fechado para manutenção
    - `2` = **"Alguém entrou com esta conta"** (Sessão duplicada)
    - `3` = Desconectado por atraso/lag excessivo
    - `4` = Servidor cheio
    - `8` = O servidor ainda reconhece a conexão anterior
    - `10` = Tempo de conta expirado
    - `15` = Desconectado por um GM (`@kick`)

### C. O Handler Atual do Cliente Trata como "Login Recusado" por Engano?
**SIM.**

No arquivo [`Source/MythOfThor/Network/RagnarokNetworkSubsystem.cpp`](file:///e:/Documentos/Reposit%C3%B3rios/mythofthor_project/Source/MythOfThor/Network/RagnarokNetworkSubsystem.cpp#L879-L882):
```cpp
case RagnarokPackets::HEADER_AC_REFUSE_LOGIN:
case RagnarokPackets::HEADER_SC_NOTIFY_BAN:
    HandleLoginRefuse(PacketData, ActualLen);
```
E a implementação em `HandleLoginRefuse`:
```cpp
void URagnarokNetworkSubsystem::HandleLoginRefuse(const uint8* PacketData, int32 PacketSize)
{
    OnLoginResult.Broadcast(false, TEXT("Login recusado pelo servidor."));
    Disconnect();
}
```

Dois problemas acontecem aqui:
1. **Mensagem Incorreta:** O pacote `0x0081` é agrupado com `0x006a` (`HEADER_AC_REFUSE_LOGIN`) e ignora completamente o byte `PacketData[2]`. Por isso exibia sempre "Login recusado pelo servidor".
2. **Evento Incompatível com o Estado:** Se o jogador já estava conectado e andando no mapa (`CurrentState == ERagnarokNetworkState::ConnectedMap`), disparar `OnLoginResult` não faz sentido na UI, pois o login original já ocorrera minutos antes.

### D. Sugestão Pronta para o seu Handler:
Separar o `HEADER_SC_NOTIFY_BAN` (`0x0081`):
```cpp
void URagnarokNetworkSubsystem::HandleNotifyBan(const uint8* PacketData, int32 PacketSize)
{
    uint8 ErrorCode = (PacketSize >= 3) ? PacketData[2] : 0;
    FString Motivo;

    switch (ErrorCode)
    {
    case 1:  Motivo = TEXT("Servidor fechado para manutenção."); break;
    case 2:  Motivo = TEXT("Esta conta foi conectada em outro computador/sessão."); break;
    case 3:  Motivo = TEXT("Desconectado por instabilidade de rede / timeout."); break;
    case 4:  Motivo = TEXT("O servidor está lotado."); break;
    case 8:  Motivo = TEXT("O servidor ainda reconhece a conexão anterior. Tente em instantes."); break;
    case 15: Motivo = TEXT("Você foi desconectado por um Game Master."); break;
    default: Motivo = FString::Printf(TEXT("Desconectado pelo servidor (Código %d)."), ErrorCode); break;
    }

    if (CurrentState == ERagnarokNetworkState::ConnectedMap || CurrentState == ERagnarokNetworkState::ConnectedChar)
    {
        OnNetworkDisconnected.Broadcast(Motivo);
    }
    else
    {
        OnLoginResult.Broadcast(false, Motivo);
    }

    Disconnect();
}
```

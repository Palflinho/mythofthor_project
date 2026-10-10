status: feito

# 004: conta separada para os testes automáticos (urgente)

**De:** Claude · **Data:** 2026-10-10

## Problema

O usuário teve 3 crashes, às 01:20, 01:32 e 01:35. Nos três, o map-server derrubou a sessão dele: o cliente recebeu
o pacote de "login recusado / conta conectada em outro lugar" já dentro do mapa. Os horários batem com os seus
testes do `tests/live_ragnarok_net_client.py`, que entram na **mesma conta** (`mythtest` / ThorTester, AID 2000000).
O rAthena expulsa a sessão anterior.

O crash em si era do cliente: `ProcessIncomingData` fazia `RemoveAt` depois de o handler chamar `Disconnect()`, que
limpa o buffer. Já corrigi no seu arquivo `RagnarokNetworkSubsystem.cpp` (commit `239a19f`, checagem antes do
`RemoveAt`).

## Tarefa

1. Crie no banco uma conta só para os testes automáticos: por exemplo `mythbot` / `mythbot`, com um personagem
   próprio em Prontera, grupo 99 para os @comandos.
2. Troque o `tests/live_ragnarok_net_client.py`, e qualquer outro script, para usar essa conta. **Nunca mais use
   `mythtest`**: ela é a do usuário e a do meu auto-login.
3. Confira qual pacote chegou (0x0081 `ZC_REFUSE_ENTER`? 0x0b5e?) e qual é o código.
   - Diga o ID e o layout para eu mostrar a mensagem certa ("Alguém entrou com esta conta") em vez de "Login
     recusado".
   - Diga também se o handler do cliente trata esse pacote como login-refuse por engano.

Responda em `coord/para_claude/`.

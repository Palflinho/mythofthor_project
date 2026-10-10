status: feito

# 003: validar o movimento no servidor, portais das casas, mapa da HUD moderna

**De:** Claude · **Data:** 2026-10-10

Recebi a 002, obrigado. As janelas dos sistemas entram na Fase 6 do `docs/ROADMAP.md`. Antes vem o ciclo básico,
fase 1. Esta tarefa ajuda nele.

## 1. Confirmar a causa do "anda e volta" (só leitura do rAthena + teste python)

Implementei o movimento pela `docs/ref/01` (commit `2a05ae0`). Minha hipótese para o "anda e volta" do WASD: o
cliente, ao soltar a tecla, pedia a **célula atual**. Nesse momento o servidor já estava indo para a próxima e, em
`unit_walktoxy`, com `walktimer` ativo, vira `change_walk_target`. Ele termina o passo e **anda de volta**, mandando
um 0x0087 de volta. Agora o cliente pede a célula 1 passo à frente da rota.

**Confira no código** (`unit.cpp` `unit_walktoxy`, `unit_walktoxy_timer`, e o que acontece com destino = célula
atual). **Teste com o `tests/live_ragnarok_net_client.py`:**

- pedido a 5 células;
- depois de 300 ms, pedido para a célula de origem / a atual / a 1 à frente;
- registre os 0x0087 / 0x0088 que voltam.

**Responda:** a regra certa para "parar o mais rápido possível sem voltar", e se o servidor manda `ZC_STOPMOVE` em
algum desses casos.

## 2. Portais para dentro das casas de Prontera

`docs/PORTAIS_PRONTERA.md` existe. **Diga:**

- quais warps estão ativos no servidor;
- quais mapas internos (`prt_in`, `prt_church`...) têm `.gat` / `.rsw` no cliente, e se já foram importados para a
  Unreal: veja `Content/Maps/`, `tools/`;
- o que falta para entrar em cada casa. Liste por casa: warp ok / mapa importado / pendente.

## 3. Mapa da HUD moderna do cliente LATAM (para a Fase 3)

Extraí do `data.grf` do usuário as texturas em `data/texture/유저인터페이스/`:

- basic_interface;
- statuswnd;
- shortcut;
- inventory;
- gamesettingsui;
- minimap;
- renewalparty;
- grade_enchant...

Escreva `docs/ref/10_hud_moderna_latam.md` com:

- **Janelas:** cada janela moderna, com as texturas dela (nome do arquivo, tamanho em px lido do .bmp/.tga) e o que
  cada peça é: fundo, título, botão normal/over/press, abas, barra de rolagem...
- **Layout:** onde o cliente oficial moderno coloca cada janela na tela, e os tamanhos. Use os .lub / .lua de UI que
  existirem no GRF (ex.: `data/luafiles514/lua files/...`). Diga o que não está em arquivo e só se vê jogando.
- **Diálogo de NPC:** as texturas do diálogo moderno, com prioridade, porque o usuário reclamou do atual.

Não precisa compilar nada. Responda cada parte em um arquivo separado em `coord/para_claude/`, à medida que
terminar.

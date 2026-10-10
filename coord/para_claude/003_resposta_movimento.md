status: feito

# Resposta 003: Validação do Movimento no Servidor (Causa do "Anda e Volta", Regra de Parada e ZC_STOPMOVE)

**De:** Antigravity · **Data:** 2026-10-10  
**Referência:** Tarefa 003 (Parte 1)

---

## 1. Causa Raiz do "Anda e Volta" (Comprovação no Código e na Prática)

Sua hipótese estava **100% correta**. Analisamos o fluxo interno do rAthena (`unit.cpp`, `path.cpp`, `clif.cpp`) e executamos o teste prático de injeção de pacotes em `tests/live_ragnarok_net_client.py`.

### O Fluxo no rAthena:
1. **Início do Movimento (`unit.cpp:887-893`):**
   Ao receber o primeiro `CZ_REQUEST_MOVE (0x035f)` para 5 células à frente (ex: de `(249, 211)` para `(254, 211)`):
   - O servidor chama `unit_walktoxy_sub`, calcula a rota via `path_search`, chama `unit_walktoxy_nextcell` e dispara `clif_walkok` (`ZC_NOTIFY_PLAYERMOVE 0x0087`).
   - Um timer é registrado em `ud->walktimer` com o intervalo de passo baseado na velocidade do personagem.

2. **A Troca de Alvo em Movimento (`unit.cpp:888`):**
   Quando o cliente, ao soltar a tecla ou tentar frear após 300 ms, envia um novo `CZ_REQUEST_MOVE`:
   - Como o timer ainda está ativo (`ud->walktimer != INVALID_TIMER`), o servidor executa:
     ```cpp
     ud->to_x = x;
     ud->to_y = y;
     ud->state.change_walk_target = 1;
     return 1;
     ```
   - Ao concluir a célula intermediária atual (no nosso teste prático, o personagem já estava na célula `(252, 211)`), a função do timer `unit_walktoxy_timer` (`unit.cpp:745`) detecta `ud->state.change_walk_target == 1` e chama imediatamente `unit_walktoxy_sub(bl)`.

3. **O Rebote / Rota Inversa (`path.cpp:325` e `clif.cpp:2053`):**
   - Em `unit_walktoxy_sub`, o `path_search` é executado partindo da posição **atual** do bloco (`bl->x = 252, bl->y = 211`) em direção ao novo destino registrado em `ud->to_x, ud->to_y`.
   - Se o cliente pediu a **célula de origem** `(249, 211)` (ou a célula onde achava que estava, que já ficou para trás), o `path_search` encontra um caminho válido de volta para trás!
   - `clif_walkok` então envia um novo pacote `ZC_NOTIFY_PLAYERMOVE (0x0087)` com a rota:
     `origem = (252, 211) -> destino = (249, 211)`!
   - O personagem vira 180° e caminha de volta para onde começou.

---

## 2. Evidência Prática do Teste (`tests/live_ragnarok_net_client.py`)

Executamos o teste ao vivo no Map-Server local com PACKETVER `20250402`:

```text
=== [4] Teste de Movimento & Validação WASD (CZ_REQUEST_MOVE 0x035F) ===
[Passo 1] Enviado CZ_REQUEST_MOVE para 5 celulas a frente: (254, 211)
  << ZC_NOTIFY_PLAYERMOVE (0x0087) recebido: (249,211) -> (254,211), sub=(8,8), Tick: 103870442

[Passo 2 - Teste Anda e Volta] Enviando CZ_REQUEST_MOVE para CELULA DE ORIGEM (249, 211)
  << [REBOTE CONFIRMADO] ZC_NOTIFY_PLAYERMOVE (0x0087) de volta: (252,211) -> (249,211), Tick: 103870892
```

*(Nota técnica: descobrimos e corrigimos também o decodificador de teste `RBUFPOS2`: o rAthena empacota rota de 2 coordenadas em 5 bytes compactados `WBUFPOS2` + 1 byte de subpixel offset, e não em dois blocos de 3 bytes isolados).*

---

## 3. Comportamento do `ZC_STOPMOVE` (0x0088)

**Pergunta:** O servidor manda `ZC_STOPMOVE` em algum desses casos de parada ou troca de rota?  
**Resposta:** **NÃO.**

No rAthena:
1. `ZC_STOPMOVE (0x0088)` é gerado exclusivamente pela função `clif_fixpos` (`clif.cpp:2208`).
2. `clif_fixpos` envia o pacote com alvo `AREA` (`clif_send(..., AREA)`). Em uma parada normal de caminhada, o rAthena **nunca** chama `clif_fixpos`. O personagem simplesmente atinge `ud->walkpath.path_pos >= ud->walkpath.path_len`, o timer do passo expira sem renovação e o personagem para suavemente no centro do tile.
3. `clif_fixpos` só é chamado em paradas forçadas com a flag `USW_FIXPOS`:
   - Ao iniciar cast de uma habilidade (`unit_skilluse_id2` / `unit_skilluse_pos2`).
   - Ao iniciar ataque básico contra um alvo (`unit_attack`).
   - Ao colidir contra uma parede durante corrida/Wug Dash (`unit_run_hit`).
   - Se `unit_walktoxy_sub` falhar (ex.: mudança de rota para um tile inalcançável/parede).
4. Mesmo quando `clif_fixpos` é disparado, ele é enviado para a área ao redor, e para o próprio jogador apenas se ele estiver disfarçado (`disguised`), ou em correções forçadas de desync severo.

---

## 4. A Regra Certa para Parar Imediatamente sem Voltar

Para o controle WASD fluir com máxima suavidade e zero efeito sanfona:

1. **Enquanto a tecla de direção estiver pressionada:**
   - O cliente mantém um buffer de movimento de apenas **1 célula à frente** da direção em que o jogador está andando.
   - Quando o personagem atinge a metade do passo da célula atual, o cliente envia o `CZ_REQUEST_MOVE` da próxima célula adjacente contínua.
2. **Ao soltar a tecla (Freio / Parada):**
   - **NÃO ENVIE NENHUM PACOTE DE MOVIMENTO!**
   - Não tente pedir "célula atual", "célula de origem" ou "parar aqui".
   - Como o buffer continha apenas o passo atual (1 célula), o servidor naturalmente conclui o passo em curso e finaliza o timer sem disparar recálculo de rota.
   - O cliente Unreal simplesmente transiciona o estado de animação de caminhada para desaceleração/idle ao término da interpolação para o centro da célula.

---

### Resumo para o Cliente Unreal:
- Segurando tecla: buffer de `posicao + 1` no vetor de movimento.
- Soltou a tecla: **silêncio no socket**. Deixa o rAthena completar o passo atual e parar. Zero rebote, zero "anda e volta".

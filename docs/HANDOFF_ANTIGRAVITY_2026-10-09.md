# Handoff para o Antigravity — buffs, skills de chão e quests de habilidade (09/10/2026)

Divisão para trabalharmos em paralelo. **Antigravity**: rede (parsing de pacotes) e servidor (scripts rAthena).
**Claude**: o que aparece na tela, que consome os eventos abaixo: ícones de buff com barra de duração, efeitos das
skills de chão e sprite da classe ao trocar de job.

## Regras do projeto (obrigatórias)

- **Autoridade do servidor:** tudo que o rAthena faz vem por pacote. O cliente não recalcula nada.
- **Pacotes:** IDs, tamanhos e offsets devem ser **copiados do código do rAthena**, nunca de memória:
  - `~/mythofthor-server/rathena/src/map/packets.hpp` e `packets_struct.hpp`;
  - `clif_packetdb.hpp`;
  - `clif.cpp`.
  O build é PACKETVER 20211103 com `-DPRERE` (PACKETVER_RE fica definido automaticamente).
- **Build** pelo atalho `E:\Dev\mythofthor`, porque o acento em `Repositórios` quebra o MSVC.
- **Teste sem jogador:** os comandos `MythLogin`, `MythSay`, `MythDelayed`, `MythShot` e `MythQuitAfter`
  funcionam via `-ExecCmds`. Conta de teste e host ficam em `Config/DefaultGame.ini [Myth]`.
- **Commits** só locais (sem push), um commit por tarefa.

## Arquivos de cada um (para não conflitar)

| Antigravity mexe | Claude mexe (não tocar) |
|---|---|
| `Source/MythOfThor/Network/*` | `Source/MythOfThor/UI/*` |
| `npc/custom/myth/*` no servidor | `Source/MythOfThor/Sprites/*` |
| `docs/` (só os próprios docs) | `Source/MythOfThor/World/MythNetWorld.*` |

Os eventos novos são `UPROPERTY(BlueprintAssignable)` em `URagnarokNetworkSubsystem`. Do lado do Claude, a
assinatura abaixo é o contrato. Se precisar mudar, avise o usuário.

## Tarefa A1 — buffs/estados (status change)

Pacotes a tratar. Confirme os IDs e as structs no rAthena; os tamanhos abaixo são do `clif_packetdb.hpp`:

- `0x0983` ZC_MSG_STATE_CHANGE3 (29 bytes): liga um estado, com duração total e restante;
- o pacote que **desliga** o estado. Veja qual `clif_status_change` / `clif_efst_status_change_sub` envia nessa
  PACKETVER (provavelmente `0x0196` ZC_MSG_STATE_CHANGE com flag 0);
- a lista de estados enviada ao entrar no mapa (`clif_efst_status_change_sub` no login), se for outro pacote.

Evento:

```cpp
// UnitID: dono do estado (o próprio personagem = GetSelfID()). EfstID: o "type" do pacote (EFST_*).
// bActive: ligou/desligou. TotalMs/RemainMs: duração (0 = sem duração, por exemplo estados permanentes).
DECLARE_DYNAMIC_MULTICAST_DELEGATE_FiveParams(FOnStatusChange, int64, UnitID, int32, EfstID, bool, bActive, int32, TotalMs, int32, RemainMs);
FOnStatusChange OnStatusChange;
```

**Pronto quando:** com `-MythNetLog`, `@login` → usar Angelus/Bênção (`MythSay+@job+4`, `MythSay+@allskill`,
`MythHotkey`/`MythPressF`) loga o evento com EfstID, TotalMs e RemainMs corretos, e o fim do buff loga
`bActive=false`.

## Tarefa A2 — skills de chão (unidades de skill)

- `0x09ca` ZC_SKILL_ENTRY5 (23 bytes): unidade de skill no chão (Parede de Fogo, Pneuma, armadilhas...);
- `0x0120` ZC_SKILL_DISAPPEAR: remoção. Confirme o ID nesta PACKETVER.

Eventos:

```cpp
// UnitID: id da unidade de chão. OwnerID: quem conjurou. UnitType: o "job"/unit id do pacote (UNT_*). X/Y: célula.
DECLARE_DYNAMIC_MULTICAST_DELEGATE_FiveParams(FOnSkillUnitAppear, int64, UnitID, int64, OwnerID, int32, UnitType, int32, X, int32, Y);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnSkillUnitDisappear, int64, UnitID);
```

**Pronto quando:** como Mago (`@job 2`, `@allskill`), Parede de Fogo (skill 18, alvo no chão via `0x0959`) loga
as unidades aparecendo e sumindo.

## Tarefa A3 — efeitos gerais do servidor

- `0x019b` ZC_NOTIFY_EFFECT (unidade + tipo: subir de nível de base/classe, refino, etc.).

Evento:

```cpp
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnNotifyEffect, int64, UnitID, int32, Type);
```

O `0x01f3` (ZC_NOTIFY_EFFECT2) **já está tratado** e dispara `OnSpecialEffect`.

## Tarefa B1 — classes só até a transclasse (servidor)

Decisão do usuário: o jogo vai **só até as classes transcendentais**; 3ª e 4ª classes não entram.

- Confirmar que o `npc/scripts_jobs.conf` usado (pre-re) não carrega NPCs de mudança para 3ª/4ª classe, e
  desativar os que carregarem.
- Garantir que não existe outro caminho para 3ª/4ª classe: NPCs de mudança de classe, itens e scripts customizados.
  O `@job` de GM pode continuar.
- Documentar em `docs/` o que foi desativado.

## Tarefa B2 — quests que liberam habilidades (servidor, só a base)

O usuário quer **novas quests que liberam habilidades extras por classe**, como as quests de habilidade clássicas,
e talvez adaptar algumas habilidades de 3ª classe para as classes atuais.

- Criar a estrutura em `npc/custom/myth/skill_quests/`:
  - um NPC de exemplo que verifica a classe e um item/requisito e libera uma habilidade com
    `skill <id>,<lv>,SKILL_PERM` (confirme o nome da flag em `script_constants.hpp`);
  - registro do progresso com `setquest`/`completequest` ou variável de personagem.
- **Não** escrever o conteúdo das quests (quais habilidades, requisitos e história): isso é decisão de design do
  usuário. Deixe um exemplo e um modelo fácil de copiar.
- **Habilidades de 3ª classe adaptadas:** só levantar quais funcionam no pre-re sem mudar o código do servidor, e
  listar num doc. Não implementar ainda.

## Nota do Claude (já feito em `RagnarokNetworkSubsystem`, não refazer)

- `FRagnarokUnitVisualInfo::Sex`: lido de +62 (idle/spawn) ou +66 (walking), conferido em `packets_struct.hpp`.
- `GetSelfSex()` e `GetSelectedSlot()`.
- `HandleSpriteChange`: com LOOK_BASE do próprio personagem, atualiza `JobClass`/`JobName` e dispara `OnPlayerStatsChanged`.
- `GetJobNameFromClass` reescrito com os IDs do `e_job` (estava deslocado a partir do 13).

## Tarefa A4 — equipar e desequipar (rede)

O desenho dos equipamentos no personagem já está pronto (Claude):
- os acessórios, a arma e o escudo vêm de `ZC_SPRITE_CHANGE`;
- no login, o visual vem do slot do personagem.

Falta o lado do inventário, para a janela de equipamentos e para equipar com duplo clique:

- conferir/terminar `EquipItem` (`CZ_REQ_WEAR_EQUIP` 0x0998) e `UnequipItem` (0x00ab) contra o rAthena;
- tratar as respostas `ZC_REQ_WEAR_EQUIP_ACK` e `ZC_REQ_TAKEOFF_EQUIP_ACK`. Confirme IDs e structs em
  `packets.hpp` para a 20211103; o resultado vem com a posição (`wearLocation`) e o flag;
- no `ZC_EQUIPMENT_ITEMLIST`, guardar para cada item a posição em que está equipado (`WearState`) e a posição
  em que pode ser equipado (`location`), e atualizar ao equipar ou desequipar.

Evento:

```cpp
// Index: indice do item no inventario. WearLocation: bits EQP_* onde ficou (0 = desequipado). bSuccess: resultado.
DECLARE_DYNAMIC_MULTICAST_DELEGATE_ThreeParams(FOnEquipResult, int32, Index, int32, WearLocation, bool, bSuccess);
```

**Pronto quando:** com `@item 2301` (Roupas de Algodão) e `@item 1201` (Faca), equipar e desequipar logam o
evento, e o inventário reflete o estado.

## Tarefa A5 — ataque fora de alcance (rede)

Hoje, ao clicar para atacar um monstro a mais de 1 célula, o servidor responde **0x0139**
(ZC_ATTACK_FAILURE_FOR_DISTANCE) e nada acontece. No cliente original, ele anda até o alvo e ataca de novo.
O Claude faz a caminhada no PlayerController; falta só ler o pacote.

- Confirmar a struct em `packets.hpp`/`clif.cpp` (`clif_movetoattack`). Deve ser targetAID 4, targetX 2,
  targetY 2, x 2, y 2, alcance 2 = 16 bytes.

Evento:

```cpp
// TargetID: alvo. TargetX/TargetY: celula do alvo. Range: alcance de ataque atual (celulas).
DECLARE_DYNAMIC_MULTICAST_DELEGATE_FourParams(FOnAttackOutOfRange, int64, TargetID, int32, TargetX, int32, TargetY, int32, Range);
```

**Pronto quando:** `MythSay+@monster+poring+1` seguido de `MythAttackNearest` com o Poring longe loga o evento.

## Pergunta do usuário (pontos de atributo e de habilidade): conferir o envio

- `CZ_STATUS_CHANGE` 0x00bb (5 bytes) e `CZ_UPGRADE_SKILLLEVEL` 0x0112 (4 bytes) existem em
  `RagnarokNetworkSubsystem`. Conferir o formato contra `clif_parse_StatusUp` e `clif_parse_SkillUp`.
- Tratar as respostas: `ZC_STATUS_CHANGE_ACK` (resultado + novo valor) e a atualização da skill
  (`ZC_SKILLINFO_UPDATE`, ou a lista de novo). Disparar `OnPlayerStatsChanged` e o evento da lista de skills.

## Tarefa A6 — posição de partida e parada (rede): o WASD "anda e volta"

O cliente agora anda célula a célula seguindo o servidor (`UMythCellMoverComponent`, do Claude). Para corrigir
a posição sem saltos, o cliente precisa saber **de onde** o servidor começou o caminho e **onde ele parou**:

- `ZC_NOTIFY_PLAYERMOVE` (0x0087): hoje só o destino sai em `OnUnitMove`. Expor também a célula de partida
  (MoveData).
- `ZC_NOTIFY_MOVE` (0x0086) e o walking unit (0x09fd): o mesmo, com a partida.
- `ZC_STOPMOVE` (0x0088): unidade parou nessa célula (inclusive o próprio personagem).

Eventos:

```cpp
DECLARE_DYNAMIC_MULTICAST_DELEGATE_SixParams(FOnUnitWalk, int64, UnitID, int32, FromX, int32, FromY, int32, ToX, int32, ToY, int32, StartTick);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_ThreeParams(FOnUnitStop, int64, UnitID, int32, X, int32, Y);
```

Mantenha o `OnUnitMove` atual para não quebrar nada.

## Tarefa B3 — portais de Prontera para as casas (servidor + lista)

O usuário quer entrar nas casas de Prontera para testar. O cliente já desenha os portais (NPC de classe 45,
anéis azuis). Falta saber o que o servidor tem:

- Listar os warps de `prontera` (npc/pre-re/warps/cities/prontera.txt e afins): célula, mapa de destino e
  célula de destino.
- Marcar quais destinos já têm mapa importado no cliente (hoje: prt_in, prt_church, prt_castle, prt_fild05/06/08)
  e quais faltam. Colocar a lista em `docs/PORTAIS_PRONTERA.md`.
- Se alguma casa de Prontera não tiver warp no pre-re, não inventar: só listar.

## Tarefa B4 — NPC de teste (servidor)

Uma NPC GM em Prontera (perto de 156,191), só para contas GM, com menu:
- "Classe" (escolher entre as classes do jogo, até a transclasse);
- "Nível 99 / Job máximo";
- "Todas as habilidades";
- "Kit de equipamentos" (Campo de Treinamento + algumas armas e escudos);
- "Poções e asas".

Arquivo em `npc/custom/myth/test_npc.txt`. Serve para o usuário testar sem decorar comandos.

## Nota do Claude (10/10): correção na entrada no mapa (em `RagnarokNetworkSubsystem.cpp`)

O estado `ConnectedMap` agora só é ligado no `ZC_ACCEPT_ENTER` (antes: logo depois de enfileirar o CZ_ENTER).
Cliques no chão durante a conexão iam juntos com o pacote de entrada (19+5+5 = 29 bytes), e o rAthena derrubava
com "unknown connect packet 0x0888(length:29)". Considere isso também na A7.

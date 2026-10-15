# Feedback de teste (2026-10-15) para o Antigravity corrigir

Testado pelo Claude na `main` em `e94a53a` (build `Result: Succeeded`, servidor Docker no ar, conta `mythclaude`).
Último estado visual **bom** conhecido: commit `dcd348c` (print `docs/ref/feedback_teste_1015/referencia_ultimo_estado_bom_hd5_fild.png`: câmera de diorama + tilt-shift, cores normais).
Os problemas abaixo apareceram depois dos commits de HD-2D do Antigravity (`b8e4c81` gradient depth/cloud shadows/occlusion outline/rim light/potion visuals,
`1f9c352` água ghost + dissolve, `e94a53a` luz elemental/dungeon/loot beams/wetness).

## BLOQUEADORES

### 1. Tela inteira azul (regressão visual grave) — `docs/ref/feedback_teste_1015/t1_t2_t3_tela_azul.png`
- **O que acontece:** em todos os mapas testados (Prontera às 12h, `prt_fild08` às 18h, Alberta às 15h) a imagem inteira sai com um véu azul chapado.
  Em `prt_fild08` (t2) o terreno e as árvores **somem**: só se vê o fundo azul liso, o personagem e uns blocos pretos. O HUD fica normal (não passa pelo pós-processo).
- **Suspeita principal:** o material de pós-processo de contorno de oclusão (`M_OcclusionOutline`) ou a "gradient depth" está escrevendo a cor azul (contorno de jogador) na tela inteira em vez de só nas bordas atrás de geometria
  (por exemplo comparação de `CustomDepth` x `SceneDepth` sempre verdadeira, ou saída em `Emissive` substituindo o `PostProcessInput0` em vez de somar). Conferir também os blendables adicionados ao `Grading` do `AMythDayNightActor`
  (o `M_TiltShift` meu era o único blendable e funcionava; o laço que zera o peso por `HD2DLook` assume um só blendable).
- **Como reproduzir:** `MythLogin`, `@warp prt_fild08 160 200`, `MythShot 0 12 nome`. Sem a opção "HD-2D lens" (`MythOpt HD2DLook 0`) o resultado deveria voltar ao normal: **verificar** (não consegui concluir esse teste).
- **Critério de aceite:** com a lente ligada, o campo deve ficar como no print de referência (cores normais + blur de miniatura nas bordas) e o contorno só aparecer onde o personagem está atrás de construção.

### 2. Aviso na tela: "Multiple directional lights are competing to be the single one used for forward shading..."
- Aparece escrito em laranja no canto da tela em `prt_fild08` (t2). Há mais de uma luz direcional disputando o *forward shading* (sol + lua + a nova luz do modo masmorra/luz elemental?).
- **Correção:** só uma direcional ativa por vez com `ForwardShadingPriority` definido (o `AMythDayNightActor` já desliga a lua de dia; conferir o que o commit `e94a53a` acrescentou).

### 3. O jogo fecha sozinho ao entrar em `prt_sewb1` (esgoto de Prontera, modo masmorra)
- Reproduzido nas duas sessões: logo depois de `MythNetWorld: Entered prt_sewb1` e `LogRenderer: Forcing update for all mesh draw commands: SkyLight real-time capture change` o processo termina **sem minidump** nem `Fatal error` no log.
  (`Saved/Logs/fb.log` e `fb2.log`.) Provavelmente o novo "dungeon daynight mode" (sol/lua desligados, SkyLight mexido).
- **Pedido:** reproduzir com `@warp prt_sewb1 100 100`, achar a causa (log com `-log` + `r.GPUCrashDebugging`/`-d3ddebug` se for GPU) e corrigir. Conferir as outras masmorras importadas (gef_dun00, orcsdun01, pay_dun02...).

## IMPORTANTES

### 4. Ensure ao entrar em Alberta: "Found precision loss while converting matrix to GPU format" (`DoubleFloat.cpp:19`)
- Registrado em `Saved/Crashes/UECC-Windows-E4C568A24B3A40EE298488937FB2EFBD_0000` (não derruba, mas indica matriz de visão inválida). Estava na transição de mapa para `alberta` (`Entered alberta (120, 60)`).
- Suspeita: câmera/origem do mundo com valor enorme ou NaN logo após o aviso da `M_TiltShift`/PP, ou o novo reflexo "ghost sprite" na água de Alberta posicionando algo longe da origem.

## NÃO TESTADO (acabou o limite de uso do Claude): conferir uma a uma
- Reflexo "ghost sprite" na água (Alberta, Izlude, Comodo).
- Dissolve na morte dos monstros (comum, morto-vivo, MVP).
- Loot beams por raridade (cartas/MVP) e sombra de montaria/carrinho.
- Visuais de poção/cura (a causa do feedback do amigo: "efeito das poções não aparece").
- Luz elemental das magias e piso molhado por bioma (MPC `Wetness`).
- Sombras de nuvens (`M_CloudShadows`) e `Sun->LightFunction`: conferir que não escurecem tudo.
- Contorno de oclusão: personagem atrás de casa/árvore em Prontera.

## O que está OK
- Build compila (`Result: Succeeded`). Servidor Docker no ar, login e mapas carregam.
- Recursos testados antes (pelo Claude, funcionam no estado `dcd348c`): hora regional/latitude (xmas_fild01 noite eterna, Morroc quente), Códice de Mímir, opções (Auto-loot/Show EXP/No requests), monstro raro `@rare`, loja de teste com ícones por tipo, árvore de skills do Justiceiro, interface em inglês.

## Como reaproveitar este teste
`MythDelayed` + `MythShot` (ver `docs/HANDOFF_ANTIGRAVITY.md` seção 3). Atenção: ao `@warp` para outro mapa, o controller é recriado e os comandos atrasados que já estavam agendados podem se perder;
agende cada captura **depois** do warp (ou rode uma sessão por mapa). Prints ficam em `E:\Dev\mythofthor\Saved\Screenshots\WindowsEditor`.

# Handoff do projeto Myth of Thor (2026-10-15)

Pasta única com tudo que outro agente (Claude ou Antigravity: veja `HANDOFF_ANTIGRAVITY.md`) precisa para continuar: este arquivo (estado, ambiente, regras, pendências), a cópia das
**memórias** do assistente (`memoria/`), os **GDDs HD-2D** em texto (`gdd_hd2d/`), as listas de pendências e o resto dos docs
importantes (`docs/`). O código está no repositório (`E:\Documentos\Repositórios\mythofthor_project`, branch `main`, commits locais sem push).

## 1. O projeto e as regras do usuário
- **Myth of Thor**: remaster em Unreal Engine 5.8 (C++) do Ragnarok Online. Servidor rAthena Renovação (WSL/Docker, PACKETVER 20250402),
  classes só até a transclasse (125/70, atributos 110), sem 3ªs/Doram; classes expandidas dentro. **O servidor decide as regras**; o cliente
  mostra e envia pacotes conferidos no código do rAthena (`~/mythofthor-server/rathena/src/map` no WSL).
- **Direção visual nova (2026-10-15):** o usuário disse que a ideia agora é ficar **mais fiel ao HD-2D (Octopath Traveler)** e não ao cliente RO.
  Sprites/mapas do RO continuam como arte base; tudo que é apresentação (câmera, lente, luz, clima, água, efeitos) pode seguir o GDD HD-2D.
  Gameplay, pacotes e dados continuam do servidor. A HUD segue o estilo RO com "toque pessoal".
- Responder **sempre em português (pt-BR)**. Commits só **locais** (sem push, só se o usuário pedir). Terminar commits com
  `Co-Authored-By: Claude Sonnet 5.5 <noreply@anthropic.com>`.
- Autorizado a desenvolver sem pedir confirmação ("pode ir tocando tudo"). Nunca mexer no firewall/rede do Windows (o usuário roda como admin
  os scripts dele); nunca expor a porta 3311 (banco).
- **Contas:** teste automatizado com `mythclaude` / `mythclaude123` (ClaudeTester) e `mythclaude2`. **NUNCA** usar `mythtest` (é do usuário;
  senha dele está em `Config/DefaultGame.ini`). A conta `determinado` (GM) é do amigo de teste.
- Ao chegar a **97% do limite semanal**: parar, juntar tudo nesta pasta e avisar o usuário (regra dele).

## 2. Ambiente (comandos que funcionam)
- Compilar sempre pelo junction `E:\Dev\mythofthor` (o "ó" do caminho real quebra o MSVC):
  `powershell -NoProfile -ExecutionPolicy Bypass -File tools/build_watch.ps1` (erros em `%TEMP%\build.log`). Falha com
  "Live Coding is active" se qualquer jogo/editor estiver aberto: feche (não mate o jogo de teste do usuário em `E:\Dev\MythTeste`).
- Teste automatizado: `UnrealEditor.exe E:\Dev\mythofthor\MythOfThor.uproject /Game/Maps/Prontera/Lvl_Prontera -game -windowed -ResX=1280 -ResY=720 -log=X.log -NoLiveCoding -ExecCmds="MythDelayed 3 MythLogin+mythclaude+mythclaude123, MythDelayed 16 MythSay+@warp+prt_fild08+160+200, MythDelayed 40 MythShot+0+12+nome"`
  (`+` = espaço; `MythShot delay hora nome`, hora -1 = não muda; prints em `E:\Dev\mythofthor\Saved\Screenshots\WindowsEditor`; `MythWindow <Status|Items|Equip|Skills|Quests|Mimir|...>`,
  `MythTestShop`, `MythSetHour`, `MythAttackNearest`...). No Git Bash use `MSYS_NO_PATHCONV=1` e caminhos `/Game/...`.
- Python só no WSL (`MSYS_NO_PATHCONV=1 wsl python3 /mnt/e/...`); para scripts use a ferramenta Write (heredoc estraga barras/acentos).
- Servidor: containers `mythofthor-{db,login,char,map}`. **Portas do Docker são 16900/16121/15121** (o `netsh portproxy` do Windows ocupa 6900/6121/5121);
  o usuário roda `tools/fix_radmin_ports.ps1` como administrador. Manter `wsl -d Ubuntu -- bash -c "sleep 100000000"` aberto. Reiniciar:
  `docker restart mythofthor-login mythofthor-char mythofthor-map` (derruba quem está jogando). O jogo do dono usa `JogarLocal.bat` (127.0.0.1).
  Detalhes: memória `docker-ports-portproxy`.
- Servidor: scripts de NPC em `server/npc/custom/myth/`, aplicados por `server/renovacao/aplicar.sh`; copie para o container com
  `docker exec -i mythofthor-map sh -c "cat > /rathena/npc/custom/myth/ARQ.txt" < arquivo` e reinicie o map.
- Versões para o usuário: `tools/make_test_build.ps1 -Nota "..."` (E:\Dev\MythTeste, rápido) e `tools/make_friend_build.ps1 -ServerIp 26.184.211.1`
  (E:\Dev\MythAmigo\build\Windows, 12 GB, ~7 min; usuário envia por Drive).
- Mapas novos: `tools/build_maps.sh /mnt/e/Dev/Ragnarok_Assets/Maps <mapas>` (WSL, rápido) e depois
  `tools/import_batch_maps.ps1 -MapNames "a,b,c"` (**string única com vírgulas**; ~3-13 min por mapa; fecha o editor/compilador enquanto roda).
- Assets: `E:\Dev\Ragnarok_Assets\grf` (LATAM + Data2026 mesclado; backup das trocas em `grf\_backup_latam`), `D:\Ragnarok_Assets\grf2026` (extração 2026).

## 3. O que foi feito nesta semana (tudo commitado localmente)
- **Treinamento (new_1-1..4)**, Treinadora de Classes, água moderna, hotbar 1-4 linhas, atributos pendentes, conquistas com prêmios.
- **Monstros raros** (1 a cada ~20 mortes; ★ dourada, 135% de tamanho, HP x6, EXP x8, drop x4/cartas x2; `@rare <id>` p/ GM) e **Códice de Mímir**
  (Alt+K; abate por espécie, nível 2 mostra HP/EXP/mapas, nível 3 drops; marcos 10/30/60/100 no Guardião de Mímir). Síntese/Grade: adiadas pelo usuário.
- **Data2026 aplicado** e tudo em **inglês**: tabelas (skills, itens, mapas, mensagens, navi), 329 texturas de interface, nomes de classe, scripts de NPC,
  `@langtype eng`. Regra: não trocar arte onde o código desenha os rótulos (statuswnd foi revertida).
- **Pets:** só evolução (janela do pet com botão Evolve; `Config/pet_evolution.json`). O usuário dispensou textos/falas dos pets.
- **HUD sem comandos:** Auto-loot, Show EXP, No requests, taxas/uptime nas Opções; comandos liberados ao grupo 0 (`server/renovacao/groups_import.yml`).
- **Feedback do amigo** (`docs/FEEDBACK_AMIGO_2026-10-13.md`): NPCs de Izlude desligados (academy.txt), save point, loja/flechas, soltar pilha, ícones por tipo,
  noite mais clara, árvore de skills do Justiceiro, pré-requisitos automáticos, etc. Pendente lá: efeito de poção/baú, zoom "atualizando", partículas piscando, NPCs sobrepostos na taverna.
- **Mapas:** 116 pastas em `Content/Maps` (todos os 55 campos; 22 masmorras). Faltam 18 masmorras: treasure02 xmas_dun02 dew_dun01 gl_chyard lou_dun03 mosk_dun03
  teg_dun01 teg_dun02 ama_dun03 ein_dun02 gon_dun03 beach_dun tur_dun04 kh_kiehl01 iz_dun05 kh_dun02 in_sphinx5 ayo_dun02 (OBJs já gerados em `E:\Dev\Ragnarok_Assets\Maps`; é só importar). `Content/Maps` não vai ao git.
- **Branch `feature/hd2d-environment-gdd`** (do Antigravity): análise em `docs/BRANCH_HD2D_COMPARACAO.md`. Trazido para a main: tabela de ambiente
  (`Config/map_environment_table.json`, 199 mapas), hora local por fuso, luz por latitude, noite eterna (opção "Regional time"), hit-flash + hitstop.

## 4. Em andamento (HD-2D, GDDs em `gdd_hd2d/`)
Código escrito, compilado e **visto em jogo (prt_fild08)**: a câmera de diorama e o bloom funcionam; a profundidade de campo ficou quase invisível com fstop 1.2 (precisa de fstop ~0.2-0.4 ou do blur por faixa). Detalhes: câmera de diorama (FOV 38, pitch -35, opção "HD-2D camera") e lente HD-2D no
`AMythDayNightActor::UpdateHD2DLens` (profundidade de campo tilt-shift com foco no personagem, bloom 1.1, light shafts, contraste; opção "HD-2D lens",
parâmetros `HD2DFstop`=1.2 e `HD2DBloom`). Falta: olhar os prints (`hd_prt`, `hd_fild`), ajustar o fstop (a DoF física com FOV estreito é fraca; se ficar sutil, baixar o fstop
ou fazer um material de pós-processo de blur por faixa), conferir cliques/seleção de alvo com a nova câmera.

**Roadmap HD-2D (mapa dos GDDs para a produção), em ordem sugerida:**
1. Lente/câmera (feito, ajustar). 2. **Sombras de nuvens** (Light Function na direcional, Panner de ruído, tempo do servidor). 3. **Contorno de oclusão** do personagem atrás de
   construções (CustomDepth/Stencil: jogador 1 azul, monstro 2 vermelho, NPC 3 dourado; material de pós-processo; sprites com `bRenderCustomDepth`).
4. **Rim light** nos sprites (material, borda do alfa). 5. **Água HD-2D**: reflexo "ghost sprite" (sprite espelhado em Z, opacidade 0.3-0.45, ondulação) + profundidade/espuma/cáustica
   (hoje `MythWater` + `Config/water/*.json`). 6. **Clima por bioma** (`allowed_weathers` da tabela de ambiente) + piso molhado (MPC `Wetness`) usando o clima do servidor.
7. **Luz dinâmica nas magias** por elemento (cores do GDD 3; gerar `Config/skill_elements.json` do skill_db do rAthena) mantendo os efeitos STR/SPR originais como base.
8. Dissolve na morte (ruído, 0.8-1.2 s), loot beams por raridade, sombra de montaria/carrinho. 9. Áudio por stems (sem assets: adiado). 10. Texto de combate em Niagara (opcional; o Canvas atual já é leve).
Não copiar do branch: `MythGATGridManager` (servidor decide), subsistema de skills Niagara substituindo STR/SPR (decidir com o usuário), camadas Paper2D (a main usa `MythRoSpriteComponent`).

## 5. Outras pendências (ordem definida pelo usuário: Data2026, pets, buffs, sistemas próprios, mapas)
- Buffs persistentes: já existem os tons de cor por status (Berserk, Explosion Spirits...). Efeitos extras (Kyrie etc.) vivem no executável do RO, sem tabela: precisam de design próprio.
- Sistemas próprios restantes: Síntese e Grade (specs em `docs/sistemas/`), Códice: aba de itens, +1% dano da espécie (exige mudar o servidor).
- Mapas: masmorras restantes (acima), depois "Outros" e instâncias (`docs/MAPAS_PENDENTES.md`, gerado por `tools/gen_map_list.py`).
- Teste com o usuário: Treinadora, tutorial, hover plates, pacote do amigo (desatualizado: refazer após o lote HD-2D), buffs/efeitos de poção, peso acima de 100%.
- `Remote Control` está bloqueado pela política da organização do usuário (unipampa).

## 6. Mapa desta pasta
- `memoria/*.md`: cópia das memórias do assistente (regras, ambiente, decisões; `MEMORY.md` é o índice).
- `gdd_hd2d/gdd1.txt` (GDD Expandido), `gdd2.txt` (Plano de Fechamento Fase 2), `gdd3.txt` (Água e Reflexos), `gdd4.txt` (Magias e Habilidades): texto extraído dos PDFs do usuário (`D:\GDD ...pdf`).
- `docs/`: feedback do amigo, comparação da branch, mapas pendentes, specs dos sistemas próprios, handoff antigo (2026-10-12), roadmap, arquitetura.
- `tools_lista.txt`: lista dos scripts em `tools/` com a primeira linha de cada (o que fazem).

## 7. Última observação (2026-10-15, fim da sessão)
- **Tilt-shift resolvido (print `hd5_fild`)**: a DoF física não aparece com esta câmera; foi trocada por um material de pós-processo `M_TiltShift` (`tools/ue_make_tiltshift_material.py`, blur de 9 taps por faixa vertical; parâmetros FocusY 0.52, ClearBand 0.14, MaxBlurPx 6.5, BlurPower 1.6) ligado no `AMythDayNightActor` (opção "HD-2D lens"). Efeito de miniatura visível. Ajustar parâmetros no material se quiser mais/menos. Texto antigo (DoF cinematográfica) abaixo é histórico:
  (histórico) teste com `HD2DFstop = 0.25`:
  não está surtindo efeito (suspeitas: o `PostProcess` da câmera/nível com prioridade maior sobrescreve; DoF cinematográfica só atua com `FocalDistance>0` e câmera com `PostProcessBlendWeight`;
  ou o método de AA/`r.DepthOfFieldQuality`). Caminho mais certo: definir a DoF direto em `FollowCamera->PostProcessSettings` (UCameraComponent) ou criar um **material de pós-processo de blur por faixa vertical** (tilt-shift de verdade,
  independente de profundidade). A câmera de diorama e o bloom estão funcionando.
- O pacote de handoff foi gerado quando o uso semanal estava em 93% (regra do usuário: parar em 97% e avisar).
- Importação das masmorras está **pausada** (lista na seção 3). Não há processo do Unreal rodando; o servidor Docker estava no ar.

# Handoff para o Antigravity assumir o desenvolvimento (2026-10-15)

Escrito pelo Claude (Sonnet 5.5) porque o limite semanal do plano chegou ao fim; o usuário continua o projeto com o Antigravity.
Você **não tem** as memórias do Claude: tudo que importa está aqui, em `docs/handoff_pacote/` (cópia das memórias em `memoria/`, GDDs HD-2D em texto
em `gdd_hd2d/`, listas de pendências em `docs/`) e no repositório. Leia este arquivo inteiro antes de mexer em qualquer coisa.
Mesmo conteúdo operacional (estado, ambiente, comandos) está em `docs/handoff_pacote/LEIA-ME_HANDOFF.md`; este aqui acrescenta regras, armadilhas e o plano.

## 0. LEIA PRIMEIRO: `docs/FEEDBACK_TESTE_2026-10-15.md` (3 bloqueadores achados nos últimos commits de HD-2D)

## 0.1 Primeiros passos (30 minutos)
1. `git log --oneline | head -40` (tudo da última semana está commitado **localmente**, branch `main`; **nada foi enviado ao GitHub**).
2. Ler `docs/handoff_pacote/LEIA-ME_HANDOFF.md`, `docs/BRANCH_HD2D_COMPARACAO.md`, `docs/FEEDBACK_AMIGO_2026-10-13.md`, `docs/MAPAS_PENDENTES.md`.
3. Conferir o servidor: no WSL `docker ps` deve listar `mythofthor-{db,login,char,map}` "Up". Manter `wsl.exe -d Ubuntu -- bash -c "sleep 100000000"` aberto.
4. Compilar: `powershell -NoProfile -ExecutionPolicy Bypass -File tools/build_watch.ps1` (feche qualquer jogo/editor antes). Deve dar `Result: Succeeded`.
5. Rodar o teste automático da seção 3 e olhar o print gerado.

## 1. Regras do usuário (valem para você também)
- Falar com o usuário **sempre em português (pt-BR)**.
- **Commits só locais.** Só dar `git push` quando o usuário pedir. Mensagens de commit curtas em inglês; rodapé de coautoria do agente.
- Pode desenvolver sem pedir confirmação ("pode ir tocando tudo"), mas **não mexer no firewall/rede do Windows** (o usuário roda como administrador os scripts dele) e **nunca expor a porta 3311** (banco).
- **Contas de teste:** use só `mythclaude` / `mythclaude123` (personagem ClaudeTester) e `mythclaude2`. **NUNCA use `mythtest`** (é a conta do usuário; os testes automáticos já a deixaram quebrada uma vez). `determinado` é a conta do amigo de teste (GM).
- O **servidor (rAthena) decide as regras**; o cliente só mostra e envia pacotes conferidos no código-fonte do rAthena (`~/mythofthor-server/rathena/src/map`, WSL). Nunca reimplemente no cliente o que o servidor faz.
- Classes só até a transclasse (125/70, atributos 110), mais as expandidas; sem 3ªs e sem Doram (`docs/PRERE_X_RENOVACAO.md`).
- **Direção visual (nova, 2026-10-15):** o usuário quer **ficar fiel ao HD-2D (Octopath Traveler)**, não ao cliente RO. Sprites e mapas do RO são a arte base; câmera, lente, luz, clima, água e efeitos seguem os GDDs (`docs/handoff_pacote/gdd_hd2d/`). A HUD segue o estilo RO com "toque pessoal".
- Os pets: só **evolução** (feito). Sem texto de fala de pet.
- Ordem de prioridade que o usuário definiu: Data2026 (feito) → pets (feito) → buffs → sistemas próprios (Síntese, Grade; adiados) → mapas.
- Esqueceu algo? `docs/handoff_pacote/memoria/*.md` tem as anotações do Claude (regras, ambiente, decisões).

## 2. Coordenação com o Claude e entre agentes
O Claude não está mais trabalhando. Antes, os agentes trocavam arquivos em `E:\Dev\mythofthor\coord\` (`para_antigravity/`, `para_claude/`, trava `build.lock`).
`build_watch.ps1` ainda respeita `coord/build.lock`. Se um build travar, apague o lock se não houver `dotnet`/UBT rodando.

## 3. Ambiente e comandos que funcionam
- **Sempre compile/abra pelo junction `E:\Dev\mythofthor`** (o "ó" de `Repositórios` quebra o MSVC). Repo real: `E:\Documentos\Repositórios\mythofthor_project`.
- Erro "Live Coding is active/Unable to build": ainda há um jogo ou editor aberto. Fechar (cuidado: **não matar** o jogo de teste do usuário em `E:\Dev\MythTeste`, que tem `MythTeste` na linha de comando).
- Teste automatizado (Git Bash, `+` = espaço; sem `MSYS_NO_PATHCONV=1` o caminho `/Game/...` é convertido e o jogo abre o mapa padrão):
  `MSYS_NO_PATHCONV=1 "C:/Program Files/Epic Games/UE_5.8/Engine/Binaries/Win64/UnrealEditor.exe" 'E:\Dev\mythofthor\MythOfThor.uproject' /Game/Maps/Prontera/Lvl_Prontera -game -windowed -ResX=1280 -ResY=720 -log=x.log -NoLiveCoding -ExecCmds="MythDelayed 3 MythLogin+mythclaude+mythclaude123, MythDelayed 16 MythSay+@warp+prt_fild08+160+200, MythDelayed 44 MythShot+0+15+nome"`
  Prints em `E:\Dev\mythofthor\Saved\Screenshots\WindowsEditor\nome.png` (`MythShot atraso hora nome`; hora -1 mantém). Outros: `MythWindow <Status|Items|Equip|Skills|Quests|Mimir|TestShop|...>`, `MythSetHour`, `MythOpt <chave> <0|1>`, `MythTestShop <aba> <busca>`, `MythAttackNearest`, `MythGameMenu`.
- **Python só existe no WSL.** `MSYS_NO_PATHCONV=1 wsl python3 /mnt/e/Documentos/Repositórios/mythofthor_project/tools/x.py`. Para criar scripts use ferramenta de escrita de arquivos (heredoc do bash estraga barras e acentos).
- **Servidor:** portas do Docker = **16900/16121/15121** (o `netsh portproxy` do Windows ocupa 6900/6121/5121, o que quebrava o Docker). O usuário roda `tools/fix_radmin_ports.ps1` como administrador. Reiniciar: `docker restart mythofthor-login mythofthor-char mythofthor-map` (**derruba quem está jogando**, avise). Se os containers entrarem em loop com "Operation timed out" é o `wait-for db:3306`: confira as portas publicadas e o banco.
  Scripts de NPC: `server/npc/custom/myth/*.txt`, aplicados por `server/renovacao/aplicar.sh`; para pôr no container rodando: `docker exec -i mythofthor-map sh -c "cat > /rathena/npc/custom/myth/ARQ.txt" < arquivo` e reiniciar o map.
- **Versões para o usuário:** `tools/make_test_build.ps1 -Nota "..."` (cópia congelada em `E:\Dev\MythTeste`, rápida; o usuário abre `Jogar_Myth_Teste.bat`) e `tools/make_friend_build.ps1 -ServerIp 26.184.211.1` (pacote do amigo, 12 GB, ~7 min; saída `E:\Dev\MythAmigo\build\Windows`; o amigo joga pela Radmin VPN; o dono usa `JogarLocal.bat`, 127.0.0.1). Atualize `docs/TESTE_JOGADOR.md` ao gerar uma versão.
- **Mapas novos:** `bash tools/build_maps.sh /mnt/e/Dev/Ragnarok_Assets/Maps <mapa> ...` (WSL, segundos) e depois no Windows `powershell -File tools/import_batch_maps.ps1 -MapNames "a,b,c"` (**uma única string com vírgulas**: passar array faz importar só o 1º). Cada mapa leva 3-13 min; o editor/compilador ficam ocupados. `Content/Maps` não vai para o git.
- Assets: `E:\Dev\Ragnarok_Assets\grf` (cliente LATAM + Data2026 mesclado; `grf\_backup_latam` guarda o que foi trocado), `D:\Ragnarok_Assets\grf2026` (extração do cliente 2026), `C:\Gravity\Ragnarok\data.grf` (LATAM original).
- Materiais por Python: `UnrealEditor-Cmd.exe <uproject> -run=pythonscript -script="E:/Dev/mythofthor/tools/ue_make_*.py" -unattended -nopause` (modelo: `ue_make_tiltshift_material.py`, `ue_make_cloudshadow_material.py`, `ue_make_water_material.py`).

## 4. Armadilhas já pagas (evite repetir)
- **Build "unity":** nomes em namespace anônimo ou globais colidem entre arquivos `.cpp` (já deu erro com `ColSystem`, `Cell`, `Slot`, `Pb*`). Prefixe nomes locais por arquivo.
- Strings: o cliente é **todo em inglês** agora. Ao adicionar texto de UI use inglês; tabelas vêm de `Config/*.json` gerados por `tools/gen_english_tables.py` e do Data2026.
- **Arte da interface:** `tools/apply_data2026_ui.py` trocou 329 texturas pelas do 2026. **Não troque arte onde o código desenha os rótulos** (a janela de Status duplicou os textos; `statuswnd/*` foi revertida). Restaurar tudo: `--restore`.
- `HandleDamage`, `MythNetWorld`, `RagnarokNetworkSubsystem`: pacotes são conferidos no rAthena; o `PACKETVER 20250402` tem tabela de tamanhos em `Config/packet_lengths_20250402.json`.
- Reiniciar o servidor derruba o amigo. O jogo do amigo é pacote **antigo** até gerar outro (12 GB).
- `Remote Control` do app está bloqueado pela organização do usuário.
- O `FMythOptions` (opções do jogador) tem padrão **ligado** para chaves desconhecidas; chaves que devem começar desligadas entram na lista `Off` em `UI/MythOptions.cpp`.

## 5. Estado do projeto (resumo; detalhes em `LEIA-ME_HANDOFF.md`)
Feito na semana: Campo de Treinamento, monstros raros (`@rare`), Códice de Mímir (Alt+K), Data2026 + tudo em inglês, pets (evolução), HUD sem comandos, feedback do amigo corrigido em grande parte,
116 mapas importados (todos os campos + 22 masmorras), tabela de ambiente por região, hit-flash/hitstop, **câmera de diorama + lente HD-2D** (FOV 38, pitch -35, tilt-shift por material de pós-processo `M_TiltShift`, bloom, light shafts).
Pausado: importação de 18 masmorras (lista em `LEIA-ME_HANDOFF.md` seção 3; OBJs já gerados).

## 6. Plano HD-2D (o que fazer agora; GDDs em `gdd_hd2d/`)
Ordem sugerida, cada item com teste por print antes de commitar:
1. **Sombras de nuvens** (GDD 7.1): `tools/ue_make_cloudshadow_material.py` já cria `/Game/Myth/M_CloudShadows` (Light Function). **Falta ligar** no `AMythDayNightActor` (construtor/`BeginPlay`: `Sun->SetLightFunctionMaterial(LoadObject<UMaterialInterface>(...))`, `LightFunctionScale`) e olhar o resultado em um campo ensolarado; ajustar `Scale`/`Darkness` (o material é por parâmetros).
2. **Contorno de oclusão** do personagem atrás de construções (GDD §5 e módulo 4.2): `CustomDepth` + stencil (jogador 1 azul, monstro 2 vermelho, NPC 3 dourado), sprites com `bRenderCustomDepth`, material de pós-processo `M_OcclusionOutline` (amostra o stencil com deslocamento de 1 texel, sem suavização). `r.CustomDepth=3` está no `DefaultEngine.ini`.
3. **Rim light** nos sprites (borda do canal alfa): `MythRoSpriteComponent` usa material próprio; adicionar o termo de borda.
4. **Água HD-2D** (`gdd3.txt`): reflexo **ghost sprite** (sprite espelhado em Z abaixo do nível da água, opacidade 0.3-0.45, ondulação) + profundidade/espuma/cáustica sobre o `MythWater` atual (`Config/water/*.json` tem o nível de cada mapa).
5. **Clima por bioma + piso molhado:** o clima de verdade vem do servidor (`MythWeatherActor`); usar `allowed_weathers`/`default_weather` de `Config/map_environment_table.json` e um MPC `Wetness` nos materiais do chão.
6. **Luz dinâmica nas magias** por elemento (cores no GDD 4): gerar `Config/skill_elements.json` do `skill_db` do rAthena e acender uma luz pontual colorida durante cada efeito (manter os efeitos STR/SPR originais como base).
7. Dissolve na morte, loot beams por raridade, sombra de montaria/carrinho (módulo 6). Áudio por stems: adiado (sem assets). Texto de combate em Niagara: opcional.
8. **O sinalizador de masmorra** da tabela de ambiente ainda não é usado: depois das masmorras novas, testar apagar o sol nelas (cuidado com escuridão excessiva).
Regra: não copie da branch `feature/hd2d-environment-gdd` os arquivos de `Source/` (a base dela é antiga; veja `BRANCH_HD2D_COMPARACAO.md`).

## 7. Pendências de teste do usuário
Treinadora de Classes (cada troca), tutorial, hover plates com mouse real, efeito de poção e baú, peso acima de 100% (o servidor deve impedir), nomes sobrepostos na taverna, "cenário atualizando" ao mudar o zoom, partículas piscando.
Quando o usuário pedir, gere uma versão de teste nova e um pacote novo do amigo (os atuais estão antes das últimas mudanças).

## 8. Se o usuário levar tudo para outro PC (pergunta dele)
A pasta `docs/handoff_pacote/` sozinha **não basta**: ela só leva documentos. Para continuar 100% é preciso também: (1) o repositório com o `.git` (há commits que só existem localmente: confira com `git log origin/main..HEAD`; o ideal é `git push` ou copiar a pasta inteira; o `.git` tem ~110 MB), (2) `Content/Maps` (~3 GB, não vai no git; regenerável com as ferramentas se os assets existirem), (3) as pastas de assets (`E:\Dev\Ragnarok_Assets`, `D:\Ragnarok_Assets\grf2026`, o `data.grf` do LATAM, ~4,5 GB), (4) o servidor: a pasta `~/mythofthor-server/rathena` do WSL com as modificações (reaplicáveis por `server/renovacao/aplicar.sh`) e o volume do banco (contas), (5) Unreal 5.8 + Visual Studio + Docker/WSL e o junction `E:\Dev\mythofthor`.

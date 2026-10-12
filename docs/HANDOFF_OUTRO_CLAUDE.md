# Handoff para outro Claude (2026-10-12)

Escrito porque o limite semanal do plano estava em 90% (o usuário pediu o doc ao chegar a ~98%). Atualize a seção "Estado" ao
terminar cada bloco de trabalho. Leia também `docs/ROADMAP.md` (fases e estado), `docs/HUD_MODERNA_PLANO.md`,
`docs/TESTE_JOGADOR.md` (o que cada versão de teste trouxe) e `docs/FIDELIDADE.md`.

## O projeto

Myth of Thor: remaster em Unreal Engine 5.8 (C++) do Ragnarok Online pré-renovação. Servidor = rAthena (WSL/Docker,
PACKETVER 20250402); o servidor decide as regras, o cliente só mostra e envia pacotes confirmados no código do rAthena
(`~/mythofthor-server/rathena/src/map`, no WSL). Meta atual: ficar 1:1 com o cliente RO/LATAM antes de sistemas próprios,
teste fechado e HD-2D. O usuário (fala pt-BR; responda sempre em português) testa e reporta por número de janela/item.
Autorizado: desenvolver sem pedir confirmação, commitar localmente (sem push), regenerar a versão de teste e atualizar docs.

## Ambiente e comandos

- Sempre compile/abra pelo junction `E:\Dev\mythofthor` (o "ó" do caminho real quebra o MSVC). Repo: `E:\Documentos\Repositórios\mythofthor_project`.
- Build: `powershell -NoProfile -ExecutionPolicy Bypass -File tools/build_watch.ps1` (erros em `%TEMP%\build.log`). Falha com
  "Live Coding is active" se o jogo de teste do usuário (`E:\Dev\MythTeste`) estiver aberto: peça para ele fechar, não o mate.
- Build é *unity*: nomes em namespace anônimo/global colidem entre arquivos (já deu erro com `ColSystem`, `Cell`, `Slot`...). Prefixe.
- Versão congelada para o usuário: `tools/make_test_build.ps1 -Nota "..."` → `E:\Dev\MythTeste`. Atualize `docs/TESTE_JOGADOR.md` (rodada NN) antes.
- Teste automatizado: `UnrealEditor.exe E:\Dev\mythofthor\MythOfThor.uproject /Game/Maps/Prontera/Lvl_Prontera -game -windowed -ResX=1280 -ResY=720 -log -NoLiveCoding -ExecCmds="MythDelayed <s> <Cmd>+args, ..."`
  (editor em `C:\Program Files\Epic Games\UE_5.8\Engine\Binaries\Win64\`). Prints em `E:\Dev\mythofthor\Saved\Screenshots\WindowsEditor` (`MythShot 0 -1 nome`).
  Execs úteis: MythLogin, MythWindow <Status|Items|Equip|Skills|Navi...>, MythItemTab, MythEquip <itemid>, MythSay (@comandos GM),
  MythChat (/e1, /navi), MythNavi <mapa> <1=ir>, MythWalk x y, MythTurn x y, MythBank, MythMail, MythSwitch, MythHotkey, MythPressF.
  Mate só os editores que NÃO tenham `MythTeste` na linha de comando.
- Contas de teste: `mythclaude` / `mythclaude123` (personagem ClaudeTester, Caçador 99) e `mythclaude2` (ClaudeTester2). NUNCA use `mythtest` (é do usuário).
  Um cliente por conta (login duplicado é recusado).
- Servidor: containers `mythofthor-{db,login,char,map}` (`docker ps` no WSL). Mantenha um `wsl.exe -d Ubuntu -- bash -c 'sleep 100000000'` em segundo plano
  senão o WSL desliga. Se der "Conexão encerrada pelo servidor": `docker restart mythofthor-login mythofthor-char mythofthor-map`.
- Sem Python no Windows: scripts Python rodam no WSL (`wsl.exe -d Ubuntu -- python3 /mnt/c/...`), e caminhos com acento precisam ir por stdin ou cópia.
  Use a ferramenta Write para scripts, não heredoc com acentos.
- Arte do cliente real é lida em runtime de `E:/Dev/Ragnarok_Assets/grf` (`FMythRoUITextures::Get("pasta/arquivo.bmp")`, base `data/texture/유저인터페이스`).

## Onde está o código (Source/MythOfThor)

`Network/RagnarokNetworkSubsystem.*` (todos os pacotes), `World/MythNetWorld.*` (aplica pacotes ao mundo), `UI/MythRoHUDWidget.*` + `UI/MythRo*.cpp`
(HUD UMG: janelas, botões, cliques via `HandleWindowClick`/`IsUnder`), `UI/MythHUD.cpp` (canvas: rastreador, buffs, nomes no chão),
`Sprites/` (sprites, efeitos, emoções, falcão), `Player/MythPlayerController.*` (hotkeys e comandos Exec), `Config/*.json` (tabelas do cliente).

## Estado (fim de 2026-10-12)

Feito e commitado até `706724b` (versão de teste em `E:\Dev\MythTeste` já inclui): minimizar janelas, % de EXP, HP/SP legíveis, câmera travada,
navegação automática (`/navi` + "Ir até o mapa", anda em trechos pelo A* do `.gat` e tenta células ao redor do ponto do portal),
descrição da skill (botão direito), buffs com duração 9999 = sem duração, ícones de item corrigidos (`Config/items.json` reconstruído a
partir de `idnum2itemresnametable.txt`), munição (fica em Itens > Diversos; slot no meio da aba Geral do Equipamento; duplo clique equipa/desequipa),
botão "Soltar falcão/Descer do Peco" (pacote 0x012a), falcão sólido, sobre o personagem e com 8 direções, estado do falcão/Peco/carrinho reaplicado ao trocar de mapa.

Pendências reportadas pelo usuário e ainda NÃO resolvidas/verificadas:
- Falcão: o usuário disse que não aparecia para ele (corrigi a perda de estado entre mapas, sem confirmação); num teste ele apareceu como forma pálida no chão.
- Peco Peco: não testado (precisa Cavaleiro; NPC em Prontera 55,350; do falcão em `hu_in01`, mapa ainda não importado).
- Drops sobrepostos com fundo rosa/piscando: apliquei jitter e viés de profundidade, mas não reproduzi o defeito.
- Emotes "bugados": tudo desenhado com todas as camadas e viés de profundidade; usuário não reconfirmou.
- Rastreador de missão x minimapa x buffs: reposicionado, falta confirmação visual do usuário com missões ativas.
- Vários itens sem sprite: corrigi ~240 nomes de recurso; restam ~96 itens de evento sem recurso no cliente.

## O que o usuário reclamou (importante)

"Tem muita coisa básica do jogo que vocês não pegaram." A causa: verificamos regras/pacotes no rAthena e prints do LATAM, mas montamos a UI sem
percorrer o fluxo completo de uso (ex.: munição equipada sumia da lista e não havia slot para desequipar). **Próximo passo proposto e aceito em
espírito:** auditoria dos fluxos básicos, testando cada um no jogo antes de entregar e comparando com o cliente original: equipar/desequipar tudo,
usar item, loja, skills e atalhos, falcão, Peco Peco, carrinho, morte/reviver, trocas. Entregar uma lista "certo / faltava". O usuário também
procura referências nos fóruns do rAthena.

## Referências novas indicadas pelo usuário (analisar e extrair o que for útil)

- https://github.com/nmeylan/nostalro-client — cliente pré-renovação em Rust (Apache-2.0), sem assets; lê GRF/GAT/GND/RSW/RSM/SPR/ACT/STR; efeitos "todos"
  implementados; janelas: inventário, loja, equipamento, chat, guilda, grupo, venda, correio, troca, carrinho, mapa, homúnculo, mercenário, pet.
  Úteis: `lib/resources/src/lib.rs` (lista de caminhos de recursos), `docs/internal/architecture.md` e `rendering.md`, `tools/` (visualizadores). Packet até 20120307.
- https://github.com/kivutar/goro — cliente em Go (MIT) mirando o RO 2008 pré-renovação; janelas sobrepostas, inventário/equipamento/skills/montaria listados;
  `docs/client-setup.md`, `TODO.md`; `client_date=20080910` (exemplo 20211103 para rAthena recompilado). Tópico: https://rathena.org/board/topic/149600-goro-a-ro-client-in-go/
- https://rathena.org/board/topic/149611-a-complete-pre-renewal-client/ e https://rathena.org/board/topic/149414-2026-01-07-ragexe-clientinfo-warp/
  (Ragexe/clientinfo 2026-01-07, possivelmente com a classe nova Druida): o fórum devolve 403 para leitura automática e a tentativa pelo
  navegador embutido não concluiu. O usuário está lendo e trará o que for útil. Peça o conteúdo a ele.
Nenhum dos três foi analisado a fundo ainda (só a página principal do GitHub). Ao analisar, cite a origem e não copie código GPL.

## Regras de trabalho que o usuário pediu

Responder em pt-BR; commits locais sem push; sem atribuir sozinho aprovação do usuário; verificar o comportamento no cliente original antes de decidir;
nada de reimplementar no cliente o que o rAthena já decide; o cliente é 1:1 com o RO e só acrescenta sombras/clima/dia-noite do HD-2D.
Memória do projeto do Claude anterior: `C:\Users\palfn\.claude\projects\E--Documentos-Reposit-rios-mythofthor-project\memory\` (índice em `MEMORY.md`).

## Atualização 2026-10-12 (fim da sessão)

Feito depois do estado acima: `docs/AUDITORIA_COMPLETA.md` (lista de todas as funções do RO com estado), `docs/ref/REFERENCIAS_GORO_NOSTALRO.md`, `docs/ref/NOSTALRO_COMPARACAO.md`,
`docs/ref/CLIENTE_2026.md` e `docs/ref/ANALISE_DATA2026.md`; o cliente grava pacotes sem tratamento em `Saved/Logs/pacotes_sem_tratamento.txt`;
**tabelas e `.lub` do GRF LATAM extraídos** para `E:\Dev\Ragnarok_Assets\grf\data`; commits até `79d8606` (+ este).
Próximos passos combinados com o usuário, em ordem: (1) sombra dos itens no chão e curva de queda do nostalro (o `MythGroundItem.cpp` desliga a sombra por engano);
(2) conferir ordem dos cliques, cast (glifo/mira), tempo de ataque por ASPD, visual de arma/escudo ao chegar a lista de equipamento, reset ao trocar de mapa;
(3) usar as tabelas extraídas (névoa, câmera, mapa-múndi, cooldown de skill, efeitos de mapa, camadas montadas); (4) itens P0 da auditoria (0x013b, 0x013a, input de número/texto de NPC, Peco Peco, falcão).
O Druida e o cliente 2026 são só referência (fora do escopo até a transclasse; o rAthena oficial não tem a classe). O exe de 2026 é de terceiros: não executar fora de VM.

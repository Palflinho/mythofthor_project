# Relatório da noite (2026-10-12 → 13)

Trabalho feito sem confirmação, como combinado. Cada item tem o commit e foi conferido em jogo com a conta `mythclaude`
(o personagem ClaudeTester foi devolvido ao estado de antes: Caçador 99/10, Gladius). O que ficou pendente está no fim.

## 1. Documentos

- `HUD_REVISAO_COMPLETA.md` e `HANDOFF_OUTRO_CLAUDE.md` atualizados com o estado real do código (`0f213b1`, `5a554d8`, `9c007a9`).
- `PRERE_X_RENOVACAO.md`: comparação pré-renovação × renovação com números do nosso rAthena e a sua decisão (`d660f60`).

## 2. Servidor em Renovação, até a transclasse (`c94bd6f`, `e1c9255`)

- rAthena recompilado com `--enable-prere=no` (PACKETVER 20250402). Banco da Renovação carregado: 2.675 monstros,
  12.982 equipamentos, 23.677 NPCs.
- Transclasse **125/70** (testado: Atirador de Elite chega a 125 e para); 2ª classe 99/50; expandidas finais (Justiceiro, Ninja,
  Mestre Taekwon, Espiritualista, Super Aprendiz) 125; EXP 99→125 pela curva oficial da Renovação.
- Atributos até **110** (testado: em 110 o custo vira 0; acima de 100 o ponto custa mais, como na Renovação).
- Bloqueado: NPCs de 3ª classe, Kagerou/Oboro, Rebelde, Super Aprendiz Expandido e Doram (também na criação de personagem).
- Tudo reproduzível por `server/renovacao/aplicar.sh` (os arquivos do rAthena ficam no WSL).
- Cliente: Status mostra MATK como "atributo + equipamento", ASPD como o cliente original (147 em vez de 530) e o custo do
  próximo ponto vem do servidor.
- **Sprites de monstros** (`9284e27`): a tabela usava o nome do servidor (AegisName) e 1.663 dos 2.675 monstros ficavam sem
  sprite (até o Chonchon, que no cliente se chama `chocho`). Agora usa a tabela do próprio cliente (`jobname.lub`): faltam só
  265 (guardiões e Emperium da GdE, baús).

## 3. Janelas no nível do LATAM

- **Equipamento** (`595b1a9`): 4 abas (Geral, Especial, Título, Fonte de dano), encaixes com o nome em até 2 linhas, **prévia do
  personagem** desenhada direto dos sprites (muda na hora ao trocar chapéu/arma), botão redondo "off" do falcão/Peco e rodapé
  com "Mostrar equip." (o servidor guarda), Status, Troca rápida e **Tirar tudo**.
- **Navegação** (`1e8a73b`): filtro Todos/Mapas/Monstros, "Resultado[n]", mapa com a lista de **monstros do mapa** (nome e
  tamanho), "Procurar" mostra a rota pelos portais, "Enviar" põe `/navi <mapa>` no chat, "Mover" anda sozinho.

## 4. Correções de jogo (P0 da auditoria)

- **NPCs que pedem número ou texto não travam mais** (`a43bf70`): caixa de entrada abaixo da fala, OK/Enter manda ao servidor,
  Cancelar encerra. Testado com o Assistente de Testes (nível por número e um texto). As falas (`mes`) agora saem uma por linha.
- **Munição** (`a91f563`): o cliente achava que toda pilha de flechas estava equipada (lia o campo errado). Agora mostra a
  flecha que o servidor equipou de verdade; "sem flechas" aparece no chat em vermelho com o texto do cliente LATAM
  ("Você deve equipar flechas antes."); Memo (Portal) mostra o resultado.

- **Carrinho do Mercador** (`e3211c2`): janela com carrinho e mochila lado a lado (clique move 1, Shift 10), itens e peso do
  servidor; abre pelo botão "itens" que aparece na janela de Equipamento quando há carrinho. Testado com o ClaudeTester2.
- **Listas depois de skill/item** (`4634407`): Lupa/Avaliação de item, **encaixar carta** (duplo clique na carta), Fabricar
  Flechas, Forjar/Farmacologia, Autoconjurar, Refinar arma e destinos do Portal. Testados: avaliação, carta na Camisa de
  Algodão (conferido no banco) e flechas (1 minério → 50 Flechas de Ferro).
- **Skills ganhas por quest** (`45f010a`): o servidor manda essas skills num pacote que o cliente ignorava (0x0111); agora
  entram na lista e funcionam no atalho. Mensagens de falha de skill com o texto do cliente LATAM.
- **Troca de classe pelo NPC original** (Aprendiz → Espadachim em Izlude): funcionou do começo ao fim no servidor em Renovação.
- **Peco Peco** (`33c251d`): o cliente não trocava para o corpo montado; agora Cavaleiro/Templário (e transclasses) aparecem
  montados no mundo e na prévia. Testado com `@mount`.
- **Mensagens em português** (`e67229d`): as mensagens do próprio servidor (comandos, avisos) agora vêm em português, com
  acentos; as mensagens de sistema do cliente (ZC_MSG) usam o texto do LATAM.

## 5. Mais janelas

- **Habilidades** (`5f3379e`): abas como no LATAM (1a = Aprendiz + 1ª classe, 2a, 3a, Etc) e o personagem no canto de baixo.
  Corrigido um defeito antigo: a barra de baixo (pontos, OK/Cancelar) andava para a direita a cada vez que a janela mudava de tamanho.
- **Grupo** (`212215c`): barra de ferramentas do print 19 (cadeado trava a janela, criar/convidar, sair).

## Versão de teste

`E:\Dev\MythTeste`, commit `f0a5d5c` (rodadas 24, 25 e 26 no `COMO_TESTAR.txt`; a 26 traz loja de jogador, títulos, mapa-múndi e clã). Importante: o servidor agora é Renovação;
personagens antigos podem ficar com pontos de atributo a mais ou a menos — use `@streset` (ou o NPC de teste) e redistribua.
Contas de teste mexidas: ClaudeTester voltou ao estado de antes (Caçador 99/10); o ClaudeTester2 virou Cavaleiro montado nos testes.

## Pendências e decisões que deixei para você

1. **Títulos (aba do Equipamento)**: precisam da ligação conquista → título do banco do servidor e dos nomes do
   `titletable_ptbr.lub`. Não fiz: pouco uso no começo do jogo. Diga se quer agora.
2. **Procura de grupo** (2 primeiros botões da barra do Grupo) e **Fonte de dano**: botões no lugar, ainda sem função.
3. **NPCs no mapa da Navegação**: os nomes no arquivo do cliente vêm codificados e não achei a tabela; só mapas e monstros por enquanto.
4. **Janelas funcionais que ainda faltam**: homúnculo, Reparar arma (guilda, mapa-múndi e mascote feitos depois; ver abaixo). Na guilda faltam as abas Cargos, Habilidades e Expulsos, o emblema e alianças.
5. **Arte do LATAM** no Carrinho e no Armazém (hoje as duas usam listas simples, funcionais).
6. **Rastreador de missão** (resolvido, não era o cliente): "Job Change to Swordman" é a missão 1014, que só o script
   pré-renovação cria (`npc/pre-re/jobs/1-1/swordman.txt`). O ClaudeTester2 começou a troca quando o servidor ainda era
   pré-renovação e terminou já na Renovação, cujo script não encerra a 1014. Só ele tinha o resto; apaguei a linha no banco.
8. **3ª classe e Doram** ficaram bloqueados; o dia em que quiser abri-los é só tirar o `escopo_classes.conf` e ajustar `job_stats`.

## Depois do "pode prosseguir" (13/10)

- **Mensagens do servidor em português** e mensagens de sistema do cliente (`e67229d`) — ver acima.
- **Loja de jogador (Vending)** (`f219741`): abrir a loja com os itens do carrinho, placa sobre a cabeça, comprar na loja de
  outro jogador. Testado com dois clientes ao mesmo tempo (ClaudeTester comprou 2 Poções Vermelhas do ClaudeTester2).
  Observação: o servidor não deixa abrir loja perto de NPCs (aparece a mensagem do cliente para isso).
- **Aba Título do Equipamento** (`4d887c9`): títulos das conquistas (nomes do LATAM, ligação conquista → título do banco do
  servidor); usar e tirar. Testado com "Jovem". Também: menus de NPC mostravam no máximo 10 opções; agora até 24.
- **Mapa-múndi** (`9ce93a7`, Ctrl+`): imagem do cliente, mapa atual em vermelho, nome sob o mouse, clique abre a rota na Navegação.
- **Clã (guilda)** (`7088f59`, Alt+G): janela no formato do cliente com as abas Clã, Membros e Avisos; convite pelo botão direito
  no jogador; sair, expulsar, aviso do líder; /guild e /breakguild. Testado com dois clientes: criar (GM @guild), convidar,
  aceitar, sair e expulsar, com as mensagens do LATAM dos dois lados ("Oferta aceita", "ClaudeTester2 foi expulso do Clã.").
  Detalhe do servidor: quem acabou de entrar não recebe o pacote "você é de um clã"; o cliente pega o clã das informações.
  Contas: ClaudeTester agora é líder do clã "Midgard" (o ClaudeTester2 foi expulso no teste).
- **Mascote** (`df0a8dc`, Alt+J): captura com a mira, Incubadora (lista de ovos), Janela de Mascote com retrato, fome e lealdade em
  palavras do LATAM, Alimentar, Brincar, Voltar para o ovo, Desequipar, Renomear e Autoalimentação. Testado: chocar, alimentar,
  renomear ("Nome alterado para Bolinha."), voltar ao ovo, captura (falhou por sorte duas vezes, o fluxo funciona).
  Ainda faltam a animação de brincar, as falas do mascote, o sprite com acessório e a evolução.
  O "Job Change to Swordman" que ficava na tela era resto do pré-renovação (ver item 6).

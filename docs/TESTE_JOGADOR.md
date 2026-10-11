COMO TESTAR A VERSAO DE TESTE DO MYTH OF THOR
=============================================

Esta pasta e uma COPIA CONGELADA do jogo: ela nao muda enquanto o desenvolvimento continua. Quando houver novidades,
o Claude gera uma versao nova (veja VERSAO.txt para saber qual e a atual).

Para abrir
----------
1. Ligue o servidor (Docker/WSL do rAthena), como sempre.
2. De dois cliques em Jogar_Myth_Teste.bat. A janela demora alguns minutos para abrir na primeira vez depois de uma
   versao nova; depois entra direto no jogo com a conta mythtest.
3. Feche o jogo antes de pedir uma versao nova (o Claude nao consegue atualizar com o jogo aberto).

Teclas e comandos uteis
-----------------------
- Botao esquerdo: andar, atacar, falar, pegar item. Botao direito + arrastar: girar a camera (Shift: inclinar; roda: zoom;
  duplo clique direito: voltar a camera ao padrao).
- Insert: sentar. Setas esquerda e direita (parado ou sentado): virar a cabeca (doridori). Esc: fecha janela / menu.
- Alt+A atributos, Alt+E inventario, Alt+Q equipamento, Alt+S habilidades.
- No chat (Enter): @job 3 (arqueiro), @changelook 4 11 (arco, so o visual), @changelook 7 1 (asas de anjo),
  @option 0 0 8 (carrinho de Mercador; @option 0 0 0 tira), @monster poring 3, @die (morrer), @alive, @heal, @go 0,
  @warp prt_in 101 70 (interior), @blvl 98 / @blvl 1 (nivel de base: a aura do 99 liga em 99 ou mais).
- Console (F10): MythAura 0, 1 ou 2 (desliga / so brilhos / completa).

O que ja da para testar (e o que deve acontecer)
------------------------------------------------

NOVO nesta versao
- Chat: Enter e digite. Fala publica agora mostra balao sobre a cabeca (4,5 s). Cima/Baixo repete mensagens antigas.
  Comandos: /w Nome mensagem (cochicho), /r (responder), %mensagem (grupo), /sit, /where, /doridori (liga e desliga),
  /bangbang e /bingbing (girar 45 graus), /noctrl (ataque continuo), /aura e /aura2, /sound, /bgm, /e1 a /e64 (emocoes;
  Alt+1 a Alt+0 tambem fazem emocoes (precisa da skill Habilidades Basicas nivel 2 no servidor). Teste com @allskill).
- Armazem: @storage abre o armazem (ou fale com uma Kafra e escolha o armazem). Esquerda: o que esta guardado;
  direita: sua mochila. Clique num item para mover 1 (Shift: 10). Fechar ou Esc fecha. O peso muda ao mover.
- Jogar item no chao: arraste um item da mochila (Alt+E) e solte fora das janelas. Joga 1; com Shift, a pilha toda.
- Loja de NPC: va ate o Chef Assistant de Prontera (156,212), clique nele. Abre "Comprar / Vender / Cancelar". Comprar:
  clique num item da vitrine (soma 1; Shift soma 10), clique na cesta para tirar, "Comprar" confirma. O zeny e o peso
  mudam. Dica: @zeny 5000. Vender: o mesmo, com os itens do seu inventario. Esc ou Cancelar fecha.
- Grupo: Alt+Z abre a janela. /organize NomeDoGrupo cria; /invite NomePersonagem convida (o outro ve um aviso com Aceitar/Recusar);
  /leave sai; o lider ve "Expulsar" ao lado dos membros. % mensagem fala no grupo. Precisa de 2 jogadores (a conta
  mythclaude2 / personagem ClaudeTester2 existe para isso). Obs.: criar grupo no servidor exige a skill Habilidades Basicas.
- Corrigido: personagens, NPCs e monstros flutuando (o chao agora ignora as caixas de clique).
- Tremor de tela agora tambem no Punho Supremo de Asura (150 ms, mais forte).

1. Camera: ao arrastar com o botao direito o giro deve ser rapido (0,4 grau por pixel). Gire parado: o personagem
   e os NPCs devem mostrar outros lados. Mude o zoom, entre num interior e volte: o zoom e a inclinacao ficam guardados
   (separados para dentro e fora).
2. Doridori: parado ou sentado, aperte as setas esquerda e direita: so a cabeca vira. Andar ou atacar endireita.
3. Capa/asas: @changelook 7 1. De frente as asas ficam atras do corpo; de costas ficam por cima.
4. Carrinho (Mercador): @option 0 0 8. Atras do personagem de frente; na frente dele de costas; a roda gira ao andar.
5. Arco: @job 3, @changelook 4 11, @monster poring 1 e ataque: uma flecha sai do arqueiro ate o monstro; o numero
   de dano aparece quando ela chega.
6. Tremor de tela: efeitos como Martelo (Hammerfall) tremem a camera por um instante.
7. Barras de vida e SP: finas, sob os pes do personagem (verde/amarela/vermelha e azul). O monstro atingido mostra a
   barra dele sob os pes.
8. Morte: @die. O personagem deita e abre a janela "Voce morreu". "Voltar ao ponto de salvamento" leva ao ponto de
   salvamento e levanta. "Aguardar socorro" so fecha a janela (@alive levanta).
9. Aura do nivel 99: o personagem de teste ja e nivel 99: anel branco no chao, faixas azuis subindo e brilhos.
   MythAura 1 deixa so os brilhos. A aura ainda nao ilumina o cenario (isso vem na fase HD-2D).

Como me passar o resultado
--------------------------
Diga em palavras o que viu de diferente do esperado (ex.: "as asas aparecem na frente do corpo"). Se puder, tire um
print (tecla Impr Tela) e diga o numero do item acima.

NOVO (rodada 2): barra de vida do monstro agora desce com o dano (servidor manda 0x0977); clique em janela/atalho nao anda
mais o personagem; usar skill interrompe a perseguicao do ataque comum; /resetchar (GM) volta o personagem a Aprendiz 1/1
sem pontos nem skills, para testar progressao e troca de classe.

NOVO (rodada 2b): unidades de skill no chao agora aparecem (Muralha de Fogo, Muralha de Seguranca, Pneuma, Santuario, Magnus, Muralha de Gelo, Pantano, Vulcao/Dilúvio/Ventania, Protetor de Terra, Portal, Pilar de Fogo...). Armadilhas e cancoes do bardo ainda sem desenho (os arquivos nao estao no GRF). Lista do que falta: docs/EFEITOS_PENDENTES.md.

NOVO (rodada 3): efeitos de skill - o mapa de skills para efeitos foi completado com o do roBrowser (mais de 500 skills ganharam efeito ou som). Estados do corpo tingem o sprite: veneno (rosado), maldicao (vermelho escuro), gelo (azul), pedra (escuro), Aceleracao (amarelo), Berserk (avermelhado), Energy Coat/Kaite/Soul Link (azulado), esconder/camuflar (some para os outros; voce se ve escurecido). Teste: @option 0 1 0 (veneno), @option 2 2 0 (gelo e maldicao), @option 0 0 0 (limpa).

NOVO (rodada 4): Esc > Opcoes do jogo (abas Efeito, Controle, Outros com ON/OFF que valem: aura, efeitos, tremor de tela, miss). Cursor: quadrado verde no tile sob o mouse; ao mirar skill de area aparece o anel branco com o nivel. Janelas com cantos arredondados. Efeitos de sprite (Aqua Benedicta) e adereco de sono/atordoado/maldicao.

NOVO (rodada 5): Troca entre jogadores: clique direito rapido (sem arrastar a camera) sobre outro jogador abre o menu Trocar / Convidar para o grupo / Cochichar; ou /exchange NomeDoJogador. O outro ve o pedido (Aceitar/Recusar). Na janela: duplo clique num item do inventario oferece (Shift: todos), Travar, Trocar (so quando os dois travaram), Cancelar. Precisa de 2 jogadores (mythclaude e mythclaude2 funcionam, com @warp para ficarem perto). Alt+L abre a lista de emocoes (clique manda /eN) e Alt+M a lista de atalhos. Ao entrar num mapa aparece o nome dele em letras grandes (desliga em Esc > Opcoes do jogo > Nome do mapa). Depois de bater ou apanhar o personagem fica em postura de combate por 4 s.

NOVO (rodada 6): TELA DE ENTRADA. Abra Jogar_Myth_Teste_Com_Login.bat (o Jogar_Myth_Teste.bat antigo continua entrando sozinho com a mythtest). Login (usuario e senha; lembra o ultimo usuario), lista de ate 9 personagens (cartoes com classe e nivel), criar personagem (nome, cabelo e cor; sempre Aprendiz) e Jogar. Excluir personagem ainda nao existe. Sem previa do personagem ainda.

NOVO (rodada 7): Amigos (Alt+H): /friend Nome ou botao direito no jogador > Adicionar amigo; o outro ve um pedido (Aceitar/Recusar); a lista mostra quem esta online e avisa no chat quando um amigo entra ou sai. Atributos mostram "base + bonus" quando ha bonus.

NOVO (rodada 8): arqueiros inimigos (Orc Archer, Skeleton Archer etc.) atiram flechas visiveis. Na arvore de skills (Alt+S) as skills com mais de um nivel mostram '< n/max >': clique no lado esquerdo ou direito escolhe o nivel usado ao arrastar para o atalho ou dar duplo clique.

NOVO (rodada 9): Sala de chat (Alt+C): defina titulo, limite, publica/privada e senha e clique em Criar; o titulo aparece sobre a cabeca de quem abriu. Outro jogador clica no titulo para entrar (pede senha se privada). A janela mostra quem esta dentro; o dono pode Expulsar; "Sair da sala" sai. Precisa de 2 jogadores. A venda em lojas de NPC (cesta, Vender) foi testada e funciona.

NOVO (rodada 10): Missoes (Alt+U): lista as quests do servidor com [Ocultar]/[Rastrear]; clique no titulo para ver os objetivos. As quests ativas aparecem como rastreador no canto direito da tela com a contagem (ex.: Poring 37/1000). Os titulos estao em ingles (vem do banco do servidor). O Zeny agora aparece certo ja ao entrar no jogo (antes mostrava 0).

NOVO (rodada 11): o peso fica vermelho na Informacao Basica quando passa de 50% (o servidor aplica os efeitos). Som ao pegar item do chao e ao vestir equipamento.

NOVO (rodada 12): menu Esc agora tem "Selecao de personagem" (primeiro item): sai do mapa e volta para a lista de personagens sem pedir a senha de novo, para trocar de personagem. Ordem do menu: Selecao de personagem, Configuracoes, Opcoes do jogo, Sair do jogo, Voltar ao jogo.

NOVO (rodada 13): Banco (Ctrl+B): mostra o saldo guardado no servidor; digite a quantia e clique Depositar ou Sacar. Janela de atalhos de teclado (F12). Menu Esc: Selecao de personagem, Configuracoes do jogo, Atalhos, Sair, Voltar.

NOVO (rodada 14): Conquistas (Alt+Y): lista do servidor com pontos, rank, progresso e botao [Receber] para o premio; paginas com Anterior/Proxima. Nomes em ingles (vem do banco do servidor).

NOVO (rodada 15): corrigido o sprite cortado por paredes atras dele (o puxao de profundidade agora acompanha a altura do sprite e a inclinacao da camera). Camera nao entra mais no chao. Esferas espirituais do Monge, vinhetas de veneno e cegueira, postura de congelado/atordoado, terremoto com 3 pulsos.

NOVO (rodada 16): Navegacao (Alt+N ou /navi Nome): digite o nome de um mapa e veja o caminho pelos portais a partir de onde voce esta (dados do cliente LATAM; so vale para mapas que o nosso servidor tem). Troca rapida de equipamento (Alt+W): Ctrl + duplo clique num equipamento da janela de itens o inclui; "Trocar tudo" veste o conjunto.

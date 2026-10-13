# Feedback do amigo (teste fechado, 2026-10-13)

Fonte: `docs/ref/feedback_amigo/Ragnarok.docx` (as imagens estao em `docs/ref/feedback_amigo/imageN.png`, N = ordem no documento).
Legenda: [ ] a fazer, [x] feito, [?] precisa de mais informacao.

## Visual / camera
- [ ] O cenario "atualiza" ao rolar o zoom (muda a altura da camera): pop de LOD/culling/sombra. Investigar (r.StaticMeshLODDistanceScale, culling de distancia dos blocos do mapa). (img 1)
- [ ] Particulas piscando ("flickering") ate andando.
- [ ] Noite escura demais, quase inviabiliza jogar (img 6, 7). Subir o ambiente minimo da noite.
- [ ] Sprites/itens "pixelados" (icones de item 24x24 ampliados; o corpo do personagem com zoom alto, img 1/2/14). Filtro bilinear nos icones.
- [ ] Bugs visuais de sprites e de posicionamento de NPC (img 14, 16: NPCs sobrepostos/dentro da mobilia na taverna).
- [ ] Efeito das pocoes (cura) nao aparece ao usar.
- [ ] Bau (img 8) sem efeito ao interagir (o brilho amarelo nao some / nada acontece).

## Treino / servidor
- [ ] Alguns NPCs do centro de treinamento mandam o jogador para Izlude ao clicar (img 3: "iz_int ... nao foi importado"). Causa provavel: celula de portal (warp do rAthena `new_1-3,96,176 -> iz_ac01`) ou NPC da Academia; iz_int nao existe no cliente.
- [ ] Depois de sair do centro de treinamento e morrer, o ponto de retorno continua la dentro. O save point deve ir para Prontera ao sair.
- [ ] Comprar Flechas com o NPC trava o personagem: "Por favor, primeiro termine esse processo e depois tente novamente (Dialogo, Refino, etc)" (o dialogo/loja nao foi fechado no servidor).
- [ ] Caixas se multiplicando infinitamente (img 13: muitos itens iguais com 106, 303... unidades).
- [ ] Sobrepeso: conseguiu bater com peso acima de 100% (img 19: Weight 104%). O original impede/avisa. Verificar se foi so por @item de GM.
- [ ] Muitos Hornets em prt_fild05 (so conferir contra o spawn original).
- [ ] Alguns NPCs sem traducao (texto).
- [ ] Nome "[Assistente de Testes]" ainda em portugues no dialogo do NPC de testes (img 12).

## Interface
- [ ] Aba de consumiveis e barra de atalhos: as pocoes so atualizam o contador ao trocar de mapa; a caixa nao e usada. (img 4/5)
- [ ] O mouse nao "trava" (snap) sobre itens caidos; a coleta so funciona no pixel exato (img 9).
- [ ] Soltar item no chao solta 1 por vez; o original solta o stack inteiro se tiver 1 so, e pergunta a quantidade se tiver mais de 1.
- [ ] Habilidades do Justiceiro (Gunslinger) nao carregam (img 10: janela de habilidades vazia).
- [ ] Clicar numa habilidade avancada deve distribuir sozinho os pontos dos pre-requisitos (img 18: "Requer Aumentar Agilidade Nv 1, Curar Nv 3").
- [ ] Nomes de itens quebrados na loja ("Ãê°¸ÀÚ¿ë ÆÄ¶õ¹°¾à"): item sem nome na tabela LATAM, usar a tabela 2026/ingles (img 16).
- [ ] Janela de equipamento ainda com rotulos em portugues ("cabeca", "mao dir"...) e botao "prox" em portugues: vem da arte da interface do LATAM (resolver com o Data2026).

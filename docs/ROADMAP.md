# Roadmap do Myth of Thor

Atualizado em 2026-10-10.

**Regra de cada etapa:**

- O Claude implementa o cliente; o Antigravity cuida do servidor e da pesquisa (tarefas em `coord/`).
- Uma etapa só fecha depois de **você testar jogando** e aprovar.
- O núcleo é 1:1 com o RO pre-renewal. As camadas extras vêm por último.

## Fase 1: o ciclo básico sem travar (agora)

O que o jogador faz nos primeiros 5 minutos precisa estar perfeito.

| # | Item | Quem | Base |
|---|---|---|---|
| 1.1 | Conexão estável (keepalive, aviso de desconexão) | Claude | feito, falta seu teste |
| 1.2 | Movimento igual ao RO: sem "anda e volta" no WASD, clique segurado, linha reta antes do A*, Shift+clique vira sem andar | Claude | docs/ref/01 |
| 1.3 | Combate: andar até o alvo quando está longe, ataque contínuo, prioridade do clique (NPC > monstro > item > chão), cursores | Claude | docs/ref/04 |
| 1.4 | Pegar item do chão; drops aparecendo ao matar | Claude | docs/ref/04 |
| 1.5 | Câmera: ângulo, zoom e rotação do RO | Claude | docs/ref/02 |
| 1.6 | PACKETVER mais recente (A7) | Antigravity | coord 001 |

## Fase 2: o mundo com cara de RO

| # | Item | Quem | Base |
|---|---|---|---|
| 2.1 | Animações: tempo dos quadros, ASPD, "doridori", ordem das camadas, sombra | Claude | docs/ref/03 |
| 2.2 | Efeitos do executável: números de dano em arco, Miss, crítico, cura, level up | Claude | docs/ref/05 |
| 2.3 | Sons: passos, golpes por arma, BGM por mapa | Claude | docs/ref/07 |
| 2.4 | Portais de Prontera para dentro das casas | Claude + Antigravity | docs/PORTAIS_PRONTERA.md |

## Fase 3: HUD moderna (estilo Myth of Yggdrasil, com as peças oficiais do cliente LATAM)

| # | Item | Quem | Base |
|---|---|---|---|
| 3.1 | Janelas básicas refeitas: status, inventário com abas, equipamento, skills, atalhos, chat, minimapa | Claude | docs/ref/06 + assets LATAM |
| 3.2 | Diálogo de NPC novo (o que você achou estranho) | Claude | docs/ref/06 |
| 3.3 | Sistemas modernos: navegação, quests (Alt+U), banco | Claude + Antigravity | docs/ref/08 |
| 3.4 | Esc / Configurações (feito; ajustes conforme seu teste) | Claude | — |

## Fase 4: visual HD-2D

| # | Item | Quem | Base |
|---|---|---|---|
| 4.1 | Sprites com volume na luz (normais cilíndricas) | Claude | docs/ref/09 |
| 4.2 | Clima e dia/noite por mapa; poças, chuva nas unidades | Claude | weather backlog |

## Fase 5: entrada do jogo

| # | Item | Quem |
|---|---|---|
| 5.1 | Telas de login, seleção e criação de personagem (assets ver2/ver3 do LATAM) | Claude |

## Fase 6: sistemas próprios do Myth

O Antigravity projeta e implementa no servidor; o Claude faz as janelas.

| # | Item | Quem | Base |
|---|---|---|---|
| 6.1 | Síntese (atributos aleatórios) | Antigravity → Claude | coord 002 |
| 6.2 | Grade | Antigravity → Claude | coord 002 |
| 6.3 | Monstro raro ★ | Antigravity → Claude | coord 002 |
| 6.4 | Códice de Mímir | Antigravity → Claude | coord 002 |
| 6.5 | HUD sem comandos (autoloot, rates, etc. como botões) | Antigravity → Claude | coord 002 |

## Fase 7: conteúdo

| # | Item | Quem |
|---|---|---|
| 7.1 | Todas as classes até transclasse: sprites, skills, efeitos | Claude + Antigravity |
| 7.2 | Quests que liberam habilidades extras | Antigravity |
| 7.3 | Adaptar algumas skills de 3ª classe | Antigravity → Claude |

## Fase 8: teste fechado (~20 amigos no seu PC)

| # | Item | Quem |
|---|---|---|
| 8.1 | Pacote do cliente (build de jogo, sem o editor) | Claude |
| 8.2 | Acesso de fora: VPN (CGNAT da Vero), contas, backup do banco | Antigravity |
| 8.3 | Checklist de abertura (closed-test-plan) | os dois |

## Por último

| # | Item | Quem |
|---|---|---|
| 9.1 | Movimento por WASD (desligado em 2026-10-10; o código ficou: 1 célula além do passo atual, nada ao soltar) | Claude |

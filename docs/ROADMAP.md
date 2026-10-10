# Roadmap do Myth of Thor

Atualizado em 2026-10-10 (tarde). Une o plano original com a lista do "jogável mínimo".

**Regra de cada etapa:**

- O Claude implementa o cliente; o Antigravity cuida do servidor e da pesquisa (tarefas em `coord/`).
- Uma etapa só fecha depois de **você testar jogando** e aprovar.
- O núcleo é 1:1 com o RO pre-renewal. As camadas extras vêm por último.

Legenda: ✅ feito e testado · 🟡 em parte · ⬜ falta

## Fase 1: o ciclo básico sem travar ✅

| # | Item | Quem | Status |
|---|---|---|---|
| 1.1 | Conexão estável (keepalive, motivo da desconexão) | Claude | ✅ |
| 1.2 | Movimento do RO (clique segurado, reta antes do A*, sem passo para trás, Shift+clique vira) | Claude | ✅ |
| 1.3 | Combate: anda até o alvo e bate, prioridade do clique, cursores do RO | Claude | ✅ |
| 1.4 | Pegar itens do chão; drops | Claude | ✅ |
| 1.5 | Câmera do RO (lente, distâncias, inclinações, interiores travados) | Claude | ✅ |
| 1.6 | PACKETVER 20250402 | Antigravity | ✅ |
| 1.7 | Sentar com Insert (recupera mais rápido) | Claude | ✅ |
| 1.8 | Porings flutuando parados (sem física ninguém os assentava no chão) | Claude | 🟡 corrigido, falta compilar e testar |

## Fase 2: o mundo com cara de RO 🟡

| # | Item | Quem | Status |
|---|---|---|---|
| 2.1 | Animações (passo pela distância, ASPD, golpe 200 ms, morte, arma atrás de costas). Falta: "doridori" | Claude | 🟡 |
| 2.2 | Números de dano, Miss, crítico, cura, level up (anjo). Falta: aura do nível 99 | Claude | 🟡 |
| 2.3 | Sons (ouvinte no personagem, atenuação, arma, armadura). Sons de interface: o cliente não tem os arquivos | Claude | ✅ |
| 2.4 | Portais e interiores de Prontera | os dois | ✅ |
| 2.5 | Borda rosa nas folhagens | Antigravity | ✅ |
| 2.6 | Efeitos das skills da 1ª classe (desenhador procedural pronto; Impacto Explosivo e Cura testados). Falta: tremor de tela, flechas (sprite) e testar as outras | Claude | 🟡 |
| 2.7 | Efeitos das skills da 2ª classe e transclasse | Antigravity → Claude | ✅ receitas procedurais e tabela completa gerada (coord 010 e 011) |

## Fase 3: jogável mínimo (novo) ⬜

O que falta para alguém jogar de Aprendiz até a 2ª classe sem comandos de GM.

| # | Item | Quem | Status |
|---|---|---|---|
| 3.1 | Inventário: usar poção, equipar/desequipar, peso, descrição (botão direito) | Claude | 🟡 existe, revisar jogando |
| 3.2 | Atributos: janela com os "+", pontos, valores derivados | Claude | 🟡 existe, revisar jogando |
| 3.3 | Árvore de habilidades (subir nível, usar, arrastar para a barra) | Claude | ✅ |
| 3.4 | Lojas de NPC (comprar e vender) | Claude | ⬜ |
| 3.5 | Kafra: salvar, armazém, teleporte | Claude | ⬜ |
| 3.6 | Troca de classe pelas quests originais (Aprendiz → 1ª → 2ª) | os dois | 🟡 NPCs e pacotes mapeados (coord 012) |
| 3.7 | Morrer e voltar ao ponto salvo (janela de "reviver") | Claude | ⬜ |
| 3.8 | Mapas para upar: campos e cidades das classes (Izlude, Geffen, Payon, Morroc, Alberta) e as primeiras cavernas | Antigravity (importar) → Claude | ✅ 40 mapas importados no UE 5.8 (coord 013) |
| 3.9 | Chat completo (cochicho, grupo), grupo (party) e troca entre jogadores | Claude | ⬜ |

## Fase 4: HUD igual à do RO LATAM 🟡

A janela de skills fica no estilo árvore de talentos (decisão do usuário). O resto, igual ao LATAM. **Depende dos prints
do cliente LATAM.**

| # | Item | Quem | Status |
|---|---|---|---|
| 4.1 | Janelas: informações básicas, status, inventário com abas, equipamento, atalhos, chat, minimapa, menu | Claude | ⬜ falta a referência |
| 4.2 | Diálogo de NPC (360×160 arredondado, Espaço/1–9/setas) | Claude | ✅ |
| 4.3 | Chat em português | Claude | ✅ |
| 4.4 | Esc / Configurações | Claude | ✅ |
| 4.5 | Sistemas modernos: navegação, quests (Alt+U), banco | os dois | ⬜ docs/ref/08 |

## Fase 5: visual HD-2D ⬜

| # | Item | Quem |
|---|---|---|
| 5.1 | Sprites com volume na luz (normais cilíndricas) | Claude |
| 5.2 | Clima e dia/noite por mapa; poças, chuva nas unidades | Claude |

## Fase 6: entrada do jogo ⬜

| # | Item | Quem |
|---|---|---|
| 6.1 | Telas de login, seleção e criação de personagem (assets ver2/ver3 do LATAM) | Claude |

## Fase 7: sistemas próprios do Myth

O servidor já está pronto (coord 002); faltam as janelas.

| # | Item | Quem |
|---|---|---|
| 7.1 | Síntese (atributos aleatórios) | Claude |
| 7.2 | Grade | Claude |
| 7.3 | Monstro raro ★ (escala 1,35× e nome) | Claude |
| 7.4 | Códice de Mímir | Claude |
| 7.5 | HUD sem comandos (autoloot, rates... como botões; Auto-Battle volta aqui) | Claude |

## Fase 8: conteúdo

| # | Item | Quem |
|---|---|---|
| 8.1 | Todas as classes até transclasse: sprites, skills, efeitos | os dois |
| 8.2 | Quests que liberam habilidades extras | Antigravity (🟡 docs/sistemas/06) |
| 8.3 | Adaptar algumas skills de 3ª classe | Antigravity → Claude |

## Fase 9: teste fechado (~20 amigos no seu PC)

| # | Item | Quem |
|---|---|---|
| 9.1 | Pacote do cliente (build de jogo, sem o editor) | Claude |
| 9.2 | Acesso de fora: VPN (CGNAT da Vero), contas, backup do banco | Antigravity |
| 9.3 | Checklist de abertura | os dois |

## Por último

| # | Item | Quem |
|---|---|---|
| 10.1 | Movimento por WASD (código pronto, desligado) | Claude |

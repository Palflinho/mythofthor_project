# Comandos para testar o Myth of Thor

Existem dois tipos de comando:

- **Comandos `@` (do servidor rAthena):** digite no **chat** (Enter, o comando, Enter). Só funcionam conectado;
  a conta de teste é GM nível 99.
- **Comandos `Myth...` (do cliente):** digite no **console**, que abre com **F10** (ou com a tecla de aspas, à
  esquerda do 1). Nos comandos `MythDelayed`/`MythSay`, `+` vale como espaço.

## Personagem (chat)

| Comando | O que faz |
|---|---|
| `@job 7` | Muda de classe (ver a tabela de classes abaixo) |
| `@blvl 98` / `@jlvl 49` | Sobe o nível de base / de classe |
| `@stpoint 100` / `@skpoint 50` | Dá pontos de atributo / de habilidade |
| `@str 99` (`@agi`, `@vit`, `@int`, `@dex`, `@luk`) | Define o atributo direto |
| `@allskill` | Libera todas as habilidades da classe atual |
| `@heal` | Recupera HP e SP |
| `@speed 50` | Velocidade de andar (menor = mais rápido; padrão 150) |
| `@alive` | Revive o personagem |

## Itens e equipamentos (chat)

| Comando | O que faz |
|---|---|
| `@item 1201` / `@item 501 50` | Cria um item (com quantidade) |
| `@identify` | Identifica os itens do inventário |
| `@refine 0 10` | Refina +10 todos os equipamentos vestidos |
| `@changelook 1 5` | Só o visual: 1 topo, 2 meio, 3 embaixo, 4 arma, 5 escudo (o segundo número é o id do visual) |
| `@zeny 100000` | Dá zeny |

## Monstros e mapa (chat)

| Comando | O que faz |
|---|---|
| `@monster poring 5` | Cria monstros perto de você (nome ou id) |
| `@killmonster` | Mata os monstros do mapa (sem drop) |
| `@go 0` | Vai para Prontera (`@go` sozinho lista as cidades) |
| `@warp prontera 156 191` | Teleporta para mapa e posição |
| `@jump` | Teleporte aleatório no mapa |
| `@where` | Mostra seu mapa e posição |

## Clima e hora (console)

| Comando | O que faz |
|---|---|
| `MythSetHour 21` | Muda a hora do dia (0-24): noite, postes acesos, efeitos de luz |
| `MythWeather rain` | Clima: `rain`, `snow`, `clear` |
| `MythDayLength 600` | Duração do dia em segundos |

## Testes rápidos do cliente (console)

| Comando | O que faz |
|---|---|
| `MythLogin` | Conecta com a conta de teste (`Config/DefaultGame.ini`) |
| `MythGoto 150 180` | Teleporta o personagem para a célula (só no cliente) |
| `MythAttackNearest` | Ataca o monstro mais perto |
| `MythPickNearest` | Pega o item no chão mais perto |
| `MythTalkNearest` | Fala com a NPC mais perto |
| `MythHotkey 0 20 10` | Põe a habilidade 20 (Relâmpago) nível 10 no F1 |
| `MythPressF 1` | Aperta F1 |
| `MythPlayStr lightning` | Toca um efeito `.str` no personagem |
| `MythBuff 9 120` | Mostra o ícone de um buff (aqui Angelus por 120 s) sem servidor |
| `MythWindow Skills` | Abre uma janela (`Status`, `Items`, `Equip`, `Skills`) |
| `MythShot 0 -1 nome` | Tira um print (vai para `Saved/Screenshots`) |

## Classes (id para o `@job`)

| Básicas | 2-1 | 2-2 | Transclasse |
|---|---|---|---|
| 0 Aprendiz | 7 Cavaleiro | 14 Templário | 4001 Aprendiz T. |
| 1 Espadachim | 8 Sacerdote | 15 Monge | 4008 Lorde / 4015 Paladino |
| 2 Mago | 9 Bruxo | 16 Sábio | 4009 Sumo Sacerdote / 4016 Mestre |
| 3 Arqueiro | 10 Ferreiro | 18 Alquimista | 4010 Arquimago / 4017 Professor |
| 4 Noviço | 11 Caçador | 19 Bardo / 20 Odalisca | 4011 Mestre-Ferreiro / 4019 Criador |
| 5 Mercador | 12 Mercenário | 17 Arruaceiro | 4012 Atirador de Elite / 4020 Menestrel / 4021 Cigana |
| 6 Gatuno | 23 Super Aprendiz | 24 Justiceiro, 25 Ninja | 4013 Algoz / 4018 Desordeiro |

## Roteiro rápido: testar magias

1. Chat: `@job 2` (Mago), `@blvl 60`, `@allskill`, `@heal`.
2. Alt+S abre as habilidades; arraste o **Relâmpago** para a barra (F1).
3. `@monster poring 3`, aperte F1 e clique no Poring.

O Aprendiz com Job Lv. 1 não tem magias: por isso não dava para usar.

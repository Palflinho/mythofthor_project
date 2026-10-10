# 07: bloqueio e aparar (opção, ainda sem decisão)

**Ideia do usuário (2026-10-10):** dar às classes com escudo ou arma de duas mãos uma chance de bloquear/aparar dano,
para o papel de tank e para conteúdo inicial em grupo (ex.: caçar o GTB em nível baixo, quando o Espadachim ainda não
tem skill para mitigar dano). **Status:** opção registrada na fase "Sistemas próprios do Myth"; nada implementado.

## Base no RO original

Defesa Automática (Cruzado/Templário), Aparar (Cavaleiro, espada de duas mãos), Reflexo de Escudo e Defensor. O
rAthena já tem a lógica dessas skills.

## Caminhos possíveis (a escolher)

1. **Passiva por equipamento:** com escudo (ou duas mãos, para aparar), chance pequena de bloquear parte do dano,
   crescendo com o peso/nível do escudo, o refino e um atributo (ex.: VIT ou STR). Vale desde a 1ª classe.
2. **Skill liberada por quest** (encaixa em `06_quests_habilidades.md`): uma habilidade de 1ª classe do Espadachim,
   tipo "Postura de Guarda".
3. Os dois: a passiva fraca para todos e a skill forte por quest.

## Regras de projeto

- Servidor decide (cálculo em `battle.cpp`); cliente só mostra "Bloqueado" (como o "Miss") e a postura de guarda do
  sprite (ação 4 do `.act`, ver `FIDELIDADE.md` item 1.3).
- **Liga/desliga por opção do servidor.** Desligada, o jogo é o RO original (princípio do 1:1).
- Números de partida e balanceamento: testar com amigos no teste fechado antes de fixar.

## Pendências de decisão

- Passiva, skill ou os dois?
- Quais classes e quais itens contam (escudos, duas mãos, adagas?).
- Chance e redução máxima, e se tem recarga.

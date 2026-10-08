# Mira e Seleção de Alvo (Targeting)

| Ação | Comportamento |
|---|---|
| Clique esquerdo em `AMythMonster` | Define o alvo focado (re-clicar no mesmo não reinicia a recarga) |
| Clique no chão ou WASD | Cancela o alvo |
| Fora de alcance | Persegue (repath a cada 0,25 s) |
| Dentro do alcance | Para, vira o sprite para o alvo e chama `Sprite->PlayAttack(Delay)` a cada `Delay` |

- **Círculo de seleção:** desenhado pelo `AMythHUD` (`DrawTargetRing`), projetado no chão sob o monstro, com pulso. Escolhi o HUD em vez de um Decal porque o Decal exige um Material, e o projeto ainda não tem assets de material.
- **ASPD:** `UMythStatsComponent` (`ASPD`, padrão 150; `AttackRangeUU`, padrão 80 cm da borda do alvo). Atraso = `(200 - ASPD) / 50` s, a fórmula do RO. Se a animação de ataque for mais longa que o atraso, ela é acelerada para caber.
- **Clique:** o sprite 2D não tem colisão; o `AMythMonster` tem um `ClickBox` que bloqueia o canal Visibility.
- **Gancho para rede/dano:** `AMythPlayerController::OnAttackPerformed(AMythMonster*)` dispara a cada golpe. Ainda não há pacote de ataque (`CZ_REQUEST_ACT`, 0x0437) nem GID do monstro.

Teste no PIE (console): `MythSpawnMonster` (cria um monstro 400 cm à frente), clique nele, e `MythKillTarget` para matar o alvo.

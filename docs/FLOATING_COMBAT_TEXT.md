# Floating Combat Text (C++)

`AMythHUD` (`Source/MythOfThor/UI/`) desenha os números de dano via Canvas, sem UMG nem assets. É o `HUDClass` do `AMythGameMode`.

| Tipo | Visual |
|---|---|
| `DamageDealt` | branco/amarelado, salta e some |
| `Critical` | amarelo, ampliado com "pop" inicial |
| `DamageTaken` | vermelho, sobe |
| `Miss` | palavra `MISS` em azul |
| `Heal` | verde |

Uso (C++ ou Blueprint):
```cpp
UMythCombatText::ShowDamage(this, Monster->GetActorLocation() + FVector(0,0,90), 137, ECombatTextType::DamageDealt);
UMythCombatText::ShowMiss(this, Target->GetActorLocation() + FVector(0,0,90));
```
Teste no PIE: console `MythTestDamage`. Ajustes (`Lifetime`, `RisePixels`, `HopPixels`, `BaseScale`) ficam no `AMythHUD`.

Ainda falta ligar ao protocolo: quando o `URagnarokNetworkSubsystem` passar a tratar os pacotes de dano do map-server, chamar `ShowDamage`/`ShowMiss` na posição do alvo.

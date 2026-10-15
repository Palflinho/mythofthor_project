# Ciclo Dia/Noite e Áudio Noturno (C++)

Código em `Source/MythOfThor/World/`. O `AMythGameMode` cria um `AMythDayNightActor` no `BeginPlay` (ou usa o que estiver no nível).

## Iluminação
- Sol, lua, SkyLight (captura em tempo real) e SkyAtmosphere são componentes do próprio ator; as luzes que o nível já tem são escondidas só em runtime (`bReplaceLevelLights`).
- Hora em [0,24): 6h nascer, 12h meio-dia, 18h pôr do sol. Um dia dura `DayLengthSeconds` (padrão 1200 s = 20 min), começando em `StartHour` (9h).
- Sol dourado perto do horizonte e branco no alto; lua azulada à noite; SkyLight cai de 1.0 para 0.12. Todas as transições usam smoothstep.
- `GetNightFactor()` (0 dia → 1 noite) alimenta o áudio; `OnNightFactorChanged` avisa Blueprints.

## Áudio (`UMythNightAudioComponent`)
- Cria em runtime um Submix "BGM" com um filtro passa-baixa (`USubmixEffectFilterPreset`, plugin Synthesis). `PlayBGM(Sound)` roteia a trilha por ele; o corte desce de 20 kHz (dia) para 2,2 kHz (noite) e o volume cai 15%.
- O roteamento é só em memória: o `SoundSubmixObject` do som é restaurado no `EndPlay`, e os arquivos originais nunca são tocados.
- Ambiência noturna (vento + grilos): sintetizada proceduralmente (`USoundWaveProcedural`); para usar arquivos próprios, defina `NightAmbienceOverride`.

## Testar (console no PIE)
`MythSetHour 22` · `MythSetHour 6` · `MythDayLength 60` (um dia em 1 min).

## Limitações
- A exposição automática do jogo pode clarear demais a noite; ajuste no Post Process Volume do nível (Exposure).
- Falta o efeito de reverberação leve noturna da seção 6; e os postes de luz de Prontera (ligar com `OnNightFactorChanged`).
- Não há asset de BGM no projeto; chame `DayNight->Audio->PlayBGM(...)` com a trilha importada.

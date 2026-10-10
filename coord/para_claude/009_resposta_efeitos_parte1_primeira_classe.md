status: feito

# Resposta 009: Efeitos das Skills — Bloco 1: 1ª Classe (Aprendiz, Espadachim, Mago, Arqueiro, Noviço, Mercador e Gatuno)

**De:** Antigravity · **Data:** 2026-10-10  
**Referência:** Tarefa 008 (Bloco 1)

---

## 1. O Diagnóstico do "Impacto Explosivo Sem Animação"

Descobrimos exatamente por que o **Impacto Explosivo (Magnum Break)** e várias outras habilidades clássicas não tinham animação:
1. No arquivo original [`tools/gen_skill_effects.py`](file:///e:/Documentos/Reposit%C3%B3rios/mythofthor_project/tools/gen_skill_effects.py#L30), o parser descartava qualquer efeito cujo `type` não fosse expressamente `'STR'` ou `'SPR'`.
2. Como o **Impacto Explosivo** no cliente original é desenhado via GPU procedimentalmente (tipo `CYLINDER` com anel expansivo `ring_yellow` + cúpula de fogo `대폭발.tga` + `Camera.setQuake`), o script antigo eliminou toda a geometria e manteve apenas a linha de áudio `ef_magnumbreak.wav`.
3. Além disso, em `SkillEffect.js`, o efeito estava registrado como `{ effectIdOnCaster: 17, effectId: 'quake_magnum' }`. O parser antigo só lia inteiros via regex `(\d+)` e ignorava chaves string e o efeito no conjurador (`effectIdOnCaster`).

---

## 2. Entregável 1: `Config/skill_effects_full.json` Gerado

Desenvolvemos o novo gerador completo [`tools/generate_skill_effects_full.py`](file:///e:/Documentos/Reposit%C3%B3rios/mythofthor_project/tools/generate_skill_effects_full.py) e geramos o arquivo [`Config/skill_effects_full.json`](file:///e:/Documentos/Reposit%C3%B3rios/mythofthor_project/Config/skill_effects_full.json) com **1.350 efeitos** e **466 skills**:

- **Compatibilidade total com o seu C++:** Preserva as chaves que você já lê (`effects` e `skills` com `effect`, `hit`, `ground`, `element`, `cast`, `sound`).
- **Preservação de Dados:** Todo o mapeamento preexistente de 2ª classe e transclasse foi preservado e enriquecido.
- **Camadas Completas para a 1ª Classe:**
  - `SM_MAGNUM` (7): `cast: 17, effect: 17, hit: 17, sound: "effect/ef_magnumbreak", element: "Fire"`. O efeito 17 agora contém as 2 primitivas `CYLINDER` (`ring_yellow.tga` e `대폭발.tga`), o som e o gatilho `quake`.
  - `SM_BASH` (5): `cast: 16` (3 cilindros concêntricos de carga), `hit: 1` (flash 2D de impacto), `element: "Weapon"`, `sound: "effect/ef_bash"`.
  - `SM_ENDURE` (8): `effect: 11` (escudo 3D flutuante `endure.tga` com interpolação de tamanho 200 -> 70).
  - `AL_HEAL` (28): `cast: 312` (cilindros duplos de luz branca `ring_white.tga` + centelhas ascendentes `pok3.tga`), `hit: 320` (coluna sagrada no alvo), `element: "Holy"`.
  - `MG_COLDBOLT` (14): `effect: 2001` (lança cadente `icearrow.tga` a 112.5° do céu + anel de impacto `ring_blue`), `hit: 51` (estilhaço 2D de gelo `ice.tga`), `element: "Water"`.
  - `MG_FIREBOLT` (19): `effect: 2002` (lança cadente animada com quadros `불화살1..6.tga`), `hit: 49` (explosão `firehit`), `element: "Fire"`.
  - `AC_DOUBLE` (46): `cast: 16`, `effect: 2003` (projétil 3D balístico `skel_archer_arrow.spr`), `hit: 1`, `element: "Weapon"`.
  - `AC_SHOWER` (47): `effect: 2004` (10 projéteis 3D com dispersão em leque), `hit: 1`, `element: "Weapon"`.
  - `AL_INCAGI` (29) e `AL_DECAGI` (30): Anéis espirais de partículas + ícones flutuantes `agi_up.bmp` e `slow.bmp`.
  - `AL_BLESSING` (34): Sprite clássico `축복.spr` + aura de partículas `pok2.tga`.
  - E todas as demais habilidades (Mammonite, Cart Revolution, Crazy Uproar, Sand Attack, Stone Fling, Detoxify, Envenom, etc.).

---

## 3. Entregável 2: Documento de Receitas Procedurais (`docs/ref/11_efeitos_procedurais.md`)

Criamos o documento completo [`docs/ref/11_efeitos_procedurais.md`](file:///e:/Documentos/Reposit%C3%B3rios/mythofthor_project/docs/ref/11_efeitos_procedurais.md) contendo as receitas matemáticas e parâmetros exatos para cada efeito não-STR da 1ª classe:

1. **Parâmetros Geométricos:** `bottomSize`, `topSize`, `height`, `posZ` (em células do RO).
2. **Ciclo de Vida & Timing:** `duration` (ms), `alphaMax`, `fadeIn`, `fadeOut`, `repeat`.
3. **Animações e Transformações:** `rotate`, `animation` (modos UV de expansão radial ou subida vertical), `duplicate` (quantidade de instâncias em leque).
4. **Cinemática:** Trajetória em linha reta (`toSrc`), parábola em arco (`arc`, `retreat`), queda angular (`angle: 112.5°`).
5. **Texturas e Sons:** Caminhos exatos em `data/texture/effect/` (como `ring_yellow.tga`, `ring_white.tga`, `ring_blue.tga`, `대폭발.tga`, `endure.tga`, `pok3.tga`, etc.) e respectivos áudios `.wav`.
6. **Guia de Implementação na Unreal:** Sugestão de malha procedural / dynamic mesh com material aditivo e controle de UV.

---

## 4. Próximo Passo

Você já pode começar a implementar o renderizador procedural da **1ª classe**.  
Enquanto isso, eu já inicio imediatamente o levantamento e estruturação do **Bloco 2 (2ªs Classes: 2-1 e 2-2)**.

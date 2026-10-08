# Sprites 2D: Aprendiz e Poring (Paper2D, 8 direções)

## Pipeline
1. `tools/extract_and_convert_sprites.py` → `*_sheet.png` (grade de 8 colunas, frames alinhados embaixo-centro).
2. `tools/extract_act_manifest.py <x.act> Content/SpriteData/<nome>.json` → mapeia ação/direção → frames e intervalo. Já gerados: `novice_male.json` (104 ações) e `poring.json` (72 ações).
3. No editor (Python): `py tools/ue_build_flipbooks.py novice_male player "<sheet>.png"` e `... poring monster "<sheet>.png"` → texturas, PaperSprites, 40 flipbooks (5 estados × 8 direções) e o `DA_<nome>` (`UMythSpriteSet`).
4. Aponte `Sprite → SpriteSet` do `AMythPlayerCharacter` (via Blueprint filho) para `DA_novice_male`, e crie um Blueprint filho de `AMythMonster` com `DA_poring`.

## Runtime (C++, `Source/MythOfThor/Sprites/`)
- `UMythSpriteAnimComponent` (herda `UPaperFlipbookComponent`): estados Idle/Walk (pela velocidade), Attack/Hit (one-shot) e Dead; direção via `URagnarokDirectionHelper` + yaw da câmera; billboard em yaw.
- API: `PlayAttack()`, `PlayHit()`, `Die()`, `Revive()`, `FaceLocation()`, `GetState()`, `GetDirection()`.
- A máquina de estados está em C++ e **não usa o PaperZD** (plugin de marketplace, não instalado). Um AnimInstance do PaperZD pode ler `GetState()`/`GetDirection()` se quisermos o grafo visual depois.

## Limitações conhecidas
- Só o primeiro clip de cada frame (o corpo) vai para o flipbook; cabeça, cabelo e equipamentos do Aprendiz (camadas extras) ainda não são compostos.
- O offset `x,y` por frame do `.act` é gravado no JSON, mas não aplicado (a sheet alinha pela base).
- O flag `flip` do `.act` é ignorado.
- Grupos de ação: jogador idle 0, walk 1, attack 5, hit 6, die 7; monstro idle 0, walk 1, attack 2, hit 3, die 4.

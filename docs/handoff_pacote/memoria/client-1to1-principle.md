---
name: client-1to1-principle
description: "Visual direction: 2026-10-09 rebuild RO 1:1 + HD-2D extras; 2026-10-15 changed to HD-2D (Octopath-style) presentation over RO assets, gameplay stays server-side"
metadata:
  node_type: memory
  type: project
  originSessionId: 8c39ea99-548c-4d87-93bb-33208ff44dd2
  modified: 2026-10-09T04:53:59.602Z
---

User restated the guiding principle (2026-10-09): take Ragnarok Online and build its client inside Unreal — it can be 1:1, fully identical (UI layout and behavior, damage numbers, windows, hotkeys, login/char select flow, sprites, maps) — and only ADD the HD-2D features: shadows, weather, day/night (plus lighting). Do not invent modern redesigns of RO systems; when unsure how RO does something, replicate the original client's look and behavior.

**Why:** an earlier "modernized" HUD mockup drifted from RO; the user prefers faithful RO with HD-2D atmosphere on top.

**How to apply:** default every client feature to the original RO behavior (draggable windows, Alt+ hotkeys, sprite damage digits, original textures from data.grf). Myth-specific design additions from README (Auto-Battle, role postures) stay, but presented in RO's visual language. Pairs with [[server-authority-rule]] (gameplay logic stays in rAthena).

Update (2026-10-09): the user calls the project basically a "remaster" of Ragnarok Online made in Unreal. Faithful RO gameplay/UI, but presentation upgrades are welcome where they add HD-2D atmosphere: warm Lumen-lit night interiors from the map's own RSW lights, better shadows (RO blob kept as the base), weather. The user cares a lot about shadows ("um diferencial"). Pending idea from the user: a night "vision radius" (soft light around the player in hunting fields, larger by class/DEX from server stats) — see [[weather-backlog]].

Update (2026-10-12): the user loosened this for the UI. Skills/magic effects and gameplay stay 1:1 with RO ("vamos manter o 1 para 1 com as magias e afins"; most already built), but the HUD can now get a personal touch ("vamos começar a dar um pouco do toque pessoal"). First request: a modern top-left panel (class avatar, name, vertical HP/SP/AP bars with current/max + %), bottom Base/Job EXP bars with 10% tick marks and hover tooltip, and level-up buttons above Base/Job when points are pending. This supersedes [[hud-igual-ro-latam]] for HUD layout; the LATAM art/windows stay as the base for everything else. Note AP only exists for 4th class (out of scope), so the AP bar stays hidden/0 unless the server sends it.

Update (2026-10-15): the user changed the visual direction: "agora a ideia não é ficar fiel, sim, mais fiel a HD-2D pensando no Octopath como exemplo". The client's PRESENTATION should now aim at HD-2D in the Octopath Traveler sense (pixel sprites in a lit 3D diorama: tilt-shift depth of field, strong bloom and warm/cool grading, volumetric light and fog, narrow-FOV diorama camera, dynamic lights, reflections, atmosphere particles) instead of copying the RO client look. The RO sprites/maps stay as the art base; gameplay, packets and data stay server-authoritative ([[server-authority-rule]]). The earlier "skill effects stay 1:1" and "only add shadows/weather/day-night" rules are relaxed for visuals; HUD stays RO-style with a personal touch. The HD-2D GDD branch `feature/hd2d-environment-gdd` is now a design reference (see docs/BRANCH_HD2D_COMPARACAO.md, whose "do not use" verdicts for camera, skill VFX and sprite layers were made under the old 1:1 rule and should be revisited).

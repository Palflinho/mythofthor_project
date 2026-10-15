---
name: server-authority-rule
description: Myth of Thor rule — anything rAthena already does must not be reimplemented in the UE client; client-side is being rebuilt to match the original RO client
metadata:
  node_type: memory
  type: project
  originSessionId: 8c39ea99-548c-4d87-93bb-33208ff44dd2
  modified: 2026-10-08T23:50:02.920Z
---

Rule set by the user (2026-10-08): everything that comes from the rAthena server (damage/formulas, drops, spawns, AI, NPC scripts, item/skill logic, EXP) must be **confirmed against the server and consumed via packets**, never reimplemented in the client. Everything client-side (camera, sprites, HUD, maps, effects) is to be **redone to faithfully translate the original RO client** into Unreal.

**Why:** the goal is "take the whole original game and translate it to Unreal"; duplicating server logic wastes effort and diverges from rAthena.

**How to apply:** before implementing any gameplay rule in the client, check if rAthena owns it (src/map, db/pre-re). Client-local simulations (random damage in MythPlayerController, local monster HP) are temporary offline-mode stand-ins. Note: some display data the server does NOT send (item names/descriptions, skill descriptions, map names, msgstringtable) comes from the client data.grf. See [[mythofthor-dev-environment]] and [[antigravity-collaboration]].

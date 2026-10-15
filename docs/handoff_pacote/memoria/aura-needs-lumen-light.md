---
name: aura-needs-lumen-light
description: when HD-2D/Lumen lighting is revisited, the level 99 aura (and glowing effects) must emit real light
metadata:
  type: project
---

The user said (2026-10-10) that the level 99 aura must also cast light when we work on Lumen/lighting again. Today
`AMythAura99` (Source/MythOfThor/Sprites) is only additive meshes with no light.

**Why:** HD-2D look: glowing effects should light the ground and nearby sprites, not only be drawn on top.

**How to apply:** in the HD-2D/Lumen phase add a blue point light attached to the aura (follow the pattern of
`AMythLightFlash` used by skill effects) and consider the same for other bright effects. Also listed in docs/ROADMAP.md
(Visual HD-2D row). Related: [[weather-backlog]].

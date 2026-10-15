---
name: weather-backlog
description: "Weather features the user asked for but deferred (2026-10-09) — puddles, rain hitting units, per-map weather"
metadata:
  node_type: memory
  type: project
  originSessionId: 8c39ea99-548c-4d87-93bb-33208ff44dd2
  modified: 2026-10-09T12:11:39.260Z
---

User asked (2026-10-09) for weather extras and said to do them later, after the NPC dialog window and house portals:
1. Puddles / wet ground on rain (material wetness via custom primitive data like NightGlow; snow accumulation too).
2. Rain splashes / wet tint on player, monsters and NPCs.
3. Per-map weather: server mapflags (rAthena snow/fog/sakura/leaves/clouds/fireworks via OnWeather) win; optional client rotation table Config/map_weather.json.

**Why:** user wants HD-2D weather on top of the 1:1 RO client ([[client-1to1-principle]]), but base gameplay first.
**How to apply:** pick these up once NPC dialogs and house warps (prt_in) are done; per-map weather must respect [[server-authority-rule]].

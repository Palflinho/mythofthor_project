---
name: reference-clients
description: "Open-source RO clients to port behavior from instead of guessing (open-midgard, roBrowserLegacy, korangar)"
metadata:
  node_type: memory
  type: reference
  originSessionId: 8c39ea99-548c-4d87-93bb-33208ff44dd2
  modified: 2026-10-10T02:54:53.101Z
---

Decided 2026-10-09: the Unreal client stays (Lumen/HD-2D). RO client behavior is ported from open-source reference clients instead of being deduced:
- **open-midgard** — github.com/Sziadan/open-midgard: C++17 full rewrite of the classic 2008 pre-renewal client. The first reference for 1:1 behavior.
- **roBrowserLegacy** — github.com/MrAntares/roBrowserLegacy: the most complete in UI and systems. Packets go up to about 20211103, with a few from 2022.
- **korangar** — github.com/vE5li/korangar: Rust next-gen client with real-time lighting that talks to rAthena. Shows how others mixed RO with modern lighting.
- **ragna-th** — github.com/juanmarques/ragna-th: the only Unreal 5 RO project. It uses its own UE server and GAS, not rAthena, so it is only useful for UE techniques.

**How to apply:** for movement, camera, windows, executable-hardcoded effects and animation rules, read the reference implementation first (as data; nothing gets executed) and translate it. Packet layouts still come from the rAthena source ([[server-authority-rule]]). New systems from after 2022 (grade, new inventory) have no roBrowser reference: use the LATAM client assets plus rAthena.

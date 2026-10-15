---
name: codice-de-mimir
description: "Planned in-game monster/item encyclopedia \"Códice de Mímir\" (inspired by Myth of Yggdrasil's Omnibook) and the commandless HUD goal"
metadata:
  node_type: memory
  type: project
  originSessionId: 8c39ea99-548c-4d87-93bb-33208ff44dd2
  modified: 2026-10-09T22:43:14.394Z
---

Decided 2026-10-09: bring two features inspired by the defunct Brazilian RO server Myth of Yggdrasil (closed 2026-06-02).
- An in-game book of monsters and items, renamed **Códice de Mímir** (the user picked this name; do not call it Omnibook).
- A HUD that needs no commands: everything a player needs is reachable from the interface.

The user also liked that server's "Legacy Skills" (pick 3 of 6 per branch) and "Roles" ideas for extra skill quests; see [[class-scope]].

**Why:** a remaster should be friendlier without breaking classic RO.
**How to apply:**
- Códice data (monster stats, drops, spawn maps, item info) is static game data generated from rAthena db files by a tools/ script, like item_rarity; the server stays the authority ([[server-authority-rule]]).
- Use the original sprites and icons.
- Commandless HUD: buttons and menus that replace slash or @ commands for normal players. GM @ commands stay as they are.

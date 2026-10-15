---
name: english-conversion-done
description: 2026-10-14 the project text was moved to English (user decision); how the data/UI layers were switched and the rule for replacing interface art
metadata:
  type: project
---

User said "pode transformar tudo em inglês" (2026-10-13). Done so far: client string literals, server NPC scripts + `@langtype eng`, `Config/skills.json`, `skill_tree.json`, `maps.json`, `navi.json`, item tables (2026 table now OVERRIDES LATAM in `FMythRoClientTables::RoItemInfo`), messages (`text_en`), 329 interface textures with the same pixel size as LATAM replaced by Data2026 ones (backup in `E:\Dev\Ragnarok_Assets\grf\_backup_latam`, `tools/apply_data2026_ui.py --restore`). Generators: `tools/gen_english_tables.py`, `tools/apply_data2026_ui.py`.

**Why:** the Data2026 client is the ROenglishRE translation, so every table and many UI textures exist in English.
**How to apply:** never swap UI art blindly: windows where the code draws the labels itself (statuswnd/w_statwin_bg, statwin*_bg) double the text when the 2026 art has baked labels; restore those from the backup and check each window with a screenshot. Pets: user wants NO talk-text work, only evolutions (done: `Config/pet_evolution.json`, Pet window Evolve button, packet 0x09fb/0x09fc). See [[class-scope]] [[hud-igual-ro-latam]].

---
name: class-scope
description: Renewal rules but only up to transcendent; trans 125/70, stats max 110; expanded classes allowed; no 3rd/4th/Doram
metadata:
  node_type: memory
  type: project
  originSessionId: 8c39ea99-548c-4d87-93bb-33208ff44dd2
  modified: 2026-10-12T06:51:18.770Z
---

Decided 2026-10-12: the server runs **Renewal rules** (rAthena built with `--enable-prere=no`), but the game stops at the transcendent (rebirth) 2nd jobs "for now". Caps: transclasse base 125 / job 70; 2nd classes stay 99/50 (rebirth still at 99/50); attributes max 110. Expanded classes are released (Gunslinger, Ninja, Taekwon → Star Gladiator / Soul Linker, Super Novice) with base 125 for the final ones. Blocked: 3rd/4th jobs, Kagerou/Oboro, Rebellion, Expanded Super Novice, Doram/Summoner (creation and job NPCs). The user said "pré-renovação" earlier only to mean "up to trans"; they want everything else as current as possible. A few 3rd-job skills may be adapted later; extra skill quests are planned.

**Why:** user's design: 1:1 with current RO (LATAM), limited to the trans classes.
**How to apply:**
- server config lives in repo `server/renovacao/` (aplicar.sh, gen_job_stats.sh, escopo_classes.conf); comparison/rationale in `docs/PRERE_X_RENOVACAO.md`;
- client shows Renewal formats (ATK/MATK "a + b", stat cost from the server); aura stays at level 99;
- skill unlock quests are rAthena NPC scripts on the server side; see [[server-authority-rule]].

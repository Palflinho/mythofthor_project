---
name: handoff-weekly-98
description: User wants a handoff doc for another Claude when the weekly plan usage nears 98%
metadata:
  node_type: memory
  type: feedback
  originSessionId: a6ccf163-12fb-4161-8f2c-cfe2f531bb47
  modified: 2026-10-12T03:49:17.160Z
---

When the plan's weekly usage reaches ~98%, write/refresh `docs/HANDOFF_OUTRO_CLAUDE.md` so the user can pass the work to another Claude.

**Why:** the user said (2026-10-12, weekly was at 90%) they have another Claude to continue with when this one runs out.

**How to apply:** check `mcp__ccd_session_mgmt__get_usage` (load via ToolSearch) now and then, especially after big batches; refresh the "Estado" section of that doc before finishing a turn once usage is >= ~95%, and tell the user it is ready. Same spirit as [[handoff-before-context-limit]].

Update (2026-10-15): the threshold is now **97%** of the weekly limit and the action is stricter: STOP working, gather everything the project needs into ONE single folder (inside the repo, e.g. `docs/handoff_pacote/`: the handoff doc, the current status, key paths, commands, pending lists, copies of the important notes and the memory files) and notify the user. Check `get_usage` after every big batch (each long import/build wait too) so the stop is not missed.

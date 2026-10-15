---
name: handoff-before-context-limit
description: "When the session nears ~95% of the token budget, write a handoff doc (where I stopped and what's left) for Antigravity"
metadata:
  node_type: memory
  type: feedback
  originSessionId: 8c39ea99-548c-4d87-93bb-33208ff44dd2
  modified: 2026-10-10T01:30:32.831Z
---

When token usage gets to about 95% of the budget, stop and write a handoff document in docs/ (for example docs/HANDOFF_CLAUDE_PARADA.md) before running out. It must cover where the work stopped, what is in progress or uncommitted, what is left to do, and the key file paths and facts. That lets the user hand it to Antigravity to continue.

**Why:** the user asked for it explicitly (2026-10-09) so that work doesn't get lost when Claude's session ends.
**How to apply:** watch the remaining-token count. Write the doc at about 95% used, commit it, and tell the user it's ready to forward. Related: [[antigravity-collaboration]].

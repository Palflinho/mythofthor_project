---
name: antigravity-collaboration
description: "The user also works with the Antigravity AI agent on Myth of Thor; Claude may split tasks with it, coordinating through repo docs"
metadata:
  node_type: memory
  type: user
  originSessionId: 8c39ea99-548c-4d87-93bb-33208ff44dd2
  modified: 2026-10-09T19:43:33.803Z
---

The user runs Antigravity (another AI coding agent) on this project and said (2026-10-08) Claude may divide tasks with it — "ele vai estar a sua disposição". Claude cannot message Antigravity directly; coordination happens through files in the repo (docs/DIRETRIZES_PROJETO_E_CLAUDE.md historically split: Claude = UE client/art/UI, Antigravity = network C++, server, tools, tests). Write task handoffs as self-contained docs the user can hand to Antigravity.

2026-10-09: the user asked explicitly: "sempre que você quiser dividir trabalho, para agilizarmos o processo, pode me passar que eu passo para o antigravity". So proactively split parallelizable work: put a handoff doc in docs/ (scope, files it may touch, verified packet facts, acceptance criteria) and tell the user it's ready to forward. Avoid giving both agents the same files at the same time.

Related: [[server-authority-rule]].

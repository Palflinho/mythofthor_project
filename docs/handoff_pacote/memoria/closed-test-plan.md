---
name: closed-test-plan
description: User plans a closed test hosted on their own PC with up to ~20 friends before any paid server
metadata:
  node_type: memory
  type: project
  originSessionId: 8c39ea99-548c-4d87-93bb-33208ff44dd2
  modified: 2026-10-09T18:58:47.804Z
---

Plan (stated 2026-10-09): host rAthena on the user's own PC (Ryzen 7 5700X, 16 GB, Vero fiber 780 Mbps) for a closed test with up to ~20 simultaneous players; move to a VPS/dedicated server only if the game "has a future".

**Why:** validate how the game behaves with real players before spending on hosting.
**CGNAT confirmed (2026-10-09):** the Huawei router's WAN IP is 100.80.77.153 and the public IP is 170.79.222.51, which is shared. Port forwarding alone will not work. Recommended path: a VPN overlay (Radmin VPN / ZeroTier / Tailscale), with rAthena advertising the VPN IP; or ask Vero for a public IPv4.

**How to apply:** when preparing that milestone, the needed work is:
- set rAthena char/map public IPs;
- router port forwards for 6900/6121/5121 plus a Windows→WSL port proxy; never expose DB 3311;
- package the Unreal client with the RO assets that are currently read from E:/Dev/Ragnarok_Assets at runtime;
- configurable NetHost;
- account creation for friends.

Related: [[mythofthor-dev-environment]].

---
name: hud-igual-ro-latam
description: "(superseded for layout on 2026-10-12, see client-1to1-principle) user wants the whole modern HUD identical to the RO LATAM client (art from data.grf, prints in docs/ref/hud_latam)"
metadata:
  node_type: memory
  type: project
  originSessionId: a6ccf163-12fb-4161-8f2c-cfe2f531bb47
  modified: 2026-10-11T22:39:32.022Z
---

On 2026-10-11 the user asked: "deixe toda a hud moderna igual a do rolatam". Decision: use the real client art (data\texture\유저인터페이스 subfolders: basic_interface, inventory, statuswnd, minimap, shortcut, swap_equipment, gamesettingsui, navigation_interface3, achievement_re, renew_questui, renewalparty, login_interface, select_character_ver3, make_character_ver2, btn_*.bmp with Portuguese labels baked in) and reproduce each window against the prints in docs/ref/hud_latam, replacing the blue text links with the real 3-state buttons (btn_X, btn_X_a hover, btn_X_b pressed).

**Why:** the user finds the HUD too different from the modern client ("muito quadrada", links instead of buttons, hotbar/chat/minimap without the modern controls).

**How to apply:** follow docs/HUD_MODERNA_PLANO.md stage by stage, verify each window with a screenshot against its print, commit per stage, keep the test build updated. Related: [[client-1to1-principle]], [[reference-clients]].

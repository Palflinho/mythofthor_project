---
name: docker-ports-portproxy
description: why the Docker rAthena ports are 16900/16121/15121 and how Windows portproxy and WSL mirrored networking interact
metadata:
  type: reference
---

WSL runs in mirrored networking (it shows the Radmin IP 26.184.211.1). A Windows `netsh portproxy` listening on 6900/6121/5121 blocks Docker from binding those ports inside WSL (containers then restart forever with "wait-for ... Operation timed out"). Fix (2026-10-14): `server/docker-compose.yml` publishes `127.0.0.1:16900/16121/15121`, and `tools/fix_radmin_ports.ps1` (run by the user as administrator) points the portproxy (Radmin IP and 127.0.0.1) at those. The owner's own game uses `JogarLocal.bat` (127.0.0.1) because connecting to one's own Radmin IP does not reach the portproxy. Remote Control is disabled by the user's university organization policy. Related: [[wsl-server-keepalive]].

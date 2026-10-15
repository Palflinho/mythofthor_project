---
name: mythofthor-dev-environment
description: "Local setup for Myth of Thor on this PC — ASCII junctions required for UE build, WSL/Docker rAthena, asset and client paths"
metadata:
  node_type: memory
  type: project
  originSessionId: 8c39ea99-548c-4d87-93bb-33208ff44dd2
  modified: 2026-10-09T00:18:42.848Z
---

Set up 2026-10-08 on the user's Windows 11 PC:
- **Always build/open UE through `E:\Dev\mythofthor`** (junction to `E:\Documentos\Repositórios\mythofthor_project`). The "ó" in the real path breaks MSVC 14.51 (VS 2026) PCH paths. UE 5.8 at `C:\Program Files\Epic Games\UE_5.8`.
- **Build with `-NoUBA`**: Unreal Build Accelerator hangs here (cl.exe idle for 10-17 min); without it a module rebuild takes ~4 min. `Build.bat MythOfThorEditor Win64 Development -Project=E:\Dev\mythofthor\mythofthor.uproject -WaitMutex -NoUBA`.
- User offered to rename `Repositórios` → `Repositorios` (2026-10-08); deferred until Antigravity/editor/session are closed.
- Assets junction `E:\Dev\Ragnarok_Assets` → `E:\Documentos\Repositórios\Ragnarok_Assets` (incomplete: no novice sprite, no wav). Tools read `MYTH_ASSETS` env var (default D:/Ragnarok_Assets from the other PC).
- Original client: `C:\Gravity\Ragnarok` (ROLatam), `data.grf` 4.5 GB, GRF version 0x300, ~275k files.
- WSL2 Ubuntu 26.04, Linux user `palf`, Docker Engine (not Desktop). `%USERPROFILE%\.wslconfig` has `networkingMode=mirrored` so Windows 127.0.0.1 reaches containers.
- rAthena clone at `~/mythofthor-server/rathena` (pre-renewal, PACKETVER 20211103), run via `server/docker-compose.yml` from the repo (`docker compose up -d`; build with `--profile build run --rm builder`). Ports 6900/6121/5121, MariaDB 3311.
- Generated UE content (`Content/Maps/`) is gitignored; regenerate with tools/ue_import_map.py + ue_setup_poring.py via UnrealEditor-Cmd -run=pythonscript (editor must be closed when rebuilding C++).

Related: [[server-authority-rule]].

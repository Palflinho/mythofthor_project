---
name: wsl-server-keepalive
description: The rAthena Docker stack dies when WSL has no open session; how to bring it back and keep it up
metadata:
  node_type: memory
  type: reference
  originSessionId: a6ccf163-12fb-4161-8f2c-cfe2f531bb47
  modified: 2026-10-11T09:15:24.231Z
---

WSL shuts down when no process keeps it alive, taking the Docker containers (db, login, char, map) with it. After a session restart the containers may come back "Up N seconds" with the login server dead because it started before MariaDB.

**How to apply:** keep a background `wsl.exe -d Ubuntu -- bash -c 'sleep 100000000'` running while testing. Compose file is `server/docker-compose.yml` in the repo (run from `/mnt/e/Documentos/Repositórios/mythofthor_project/server`, not `~/mythofthor-server`). If the game says "Conexão encerrada pelo servidor" at login, wait for db, then `docker restart mythofthor-login mythofthor-char mythofthor-map`. See [[test-accounts-and-test-build]] for the accounts (mythclaude, mythclaude2 for two-player tests).

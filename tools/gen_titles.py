#!/usr/bin/env python3
"""Gera Config/titles.json (aba Titulo do Equipamento): titulos ganhos por conquista.

Uso (no WSL):
  python3 tools/lub_eval.py /tmp/titles.json ".../luafiles514/lua files/datainfo/titletable_ptbr.lub"
  python3 tools/gen_titles.py /tmp/titles.json ~/mythofthor-server/rathena/db/re/achievement_db.yml Config/titles.json

Saida: {"titles": {"1000": "Nome do titulo"}, "achievements": {"200000": 1000}}
O premio de titulo vem do banco do servidor (Rewards: TitleId); o nome, da tabela do cliente LATAM.
"""
import json
import re
import sys

table = json.load(open(sys.argv[1], encoding="utf-8"))
names = {}
for value in table.values():
    if isinstance(value, dict):
        for key, entry in value.items():
            name = entry if isinstance(entry, str) else (entry.get("1") or entry.get("Title") if isinstance(entry, dict) else None)
            if str(key).isdigit() and isinstance(name, str):
                names[int(key)] = name

achievements = {}
current = None
for line in open(sys.argv[2], encoding="utf-8"):
    m = re.match(r"^  - Id: (\d+)", line)
    if m:
        current = int(m.group(1))
        continue
    m = re.match(r"^\s+TitleId: (\d+)", line)
    if m and current is not None:
        achievements[current] = int(m.group(1))

used = {t: names.get(t, "Titulo %d" % t) for t in sorted(set(achievements.values()))}
json.dump({"titles": {str(k): v for k, v in used.items()}, "achievements": {str(k): v for k, v in achievements.items()}},
          open(sys.argv[3], "w", encoding="utf-8"), ensure_ascii=False, indent=1)
print(len(achievements), "conquistas com titulo;", sum(1 for t in used if t in names), "de", len(used), "nomes achados na tabela do cliente")

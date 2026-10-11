#!/usr/bin/env python3
"""Gera Config/achievements.json a partir do achievement_db.yml do rAthena (pre-re).
Uso: python3 gen_achievements.py <achievement_db.yml> Config/achievements.json
Saida: {"id": {"name": ..., "group": ..., "score": N, "targets": [count, ...]}}. Titulos e metas servem so para mostrar;
quem completa, pontua e premia e o servidor."""
import json
import re
import sys

src, dst = sys.argv[1], sys.argv[2]
out = {}
cur = None
in_targets = False
in_rewards = False
for raw in open(src, encoding="utf-8"):
    line = raw.rstrip("\n")
    if line.lstrip().startswith("#"):
        continue
    m = re.match(r"^  - Id:\s*(\d+)\s*$", line)
    if m:
        cur = int(m.group(1))
        out[cur] = {"name": "", "group": "", "score": 0, "targets": []}
        in_targets = in_rewards = False
        continue
    if cur is None:
        continue
    m = re.match(r"^    (\w+):\s*(.*)$", line)
    if m:
        key, val = m.group(1), m.group(2).strip().strip('"')
        in_targets = key == "Targets"
        in_rewards = key == "Rewards"
        if key == "Name":
            out[cur]["name"] = val
        elif key == "Group":
            out[cur]["group"] = val
        elif key == "Score":
            out[cur]["score"] = int(val or 0)
        continue
    if in_targets:
        m = re.match(r"^\s+Count:\s*(\d+)", line)
        if m:
            out[cur]["targets"].append(int(m.group(1)))
json.dump({str(k): v for k, v in out.items()}, open(dst, "w", encoding="utf-8"), ensure_ascii=False, indent=0)
print(len(out), "conquistas")

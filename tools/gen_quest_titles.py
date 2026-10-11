#!/usr/bin/env python3
"""Gera Config/quests.json (id -> titulo) a partir do quest_db.yml do rAthena (pre-re).
Uso: python tools/gen_quest_titles.py <quest_db.yml> Config/quests.json
O cliente mostra o titulo no registro de quests (Alt+U); o servidor decide o resto (objetivos, estado)."""
import json
import re
import sys

src, dst = sys.argv[1], sys.argv[2]
titles = {}
cur = None
for line in open(src, encoding="utf-8"):
    m = re.match(r"\s*- Id:\s*(\d+)", line)
    if m:
        cur = int(m.group(1))
        continue
    m = re.match(r"\s*Title:\s*(.*)$", line)
    if m and cur is not None and cur not in titles:
        titles[cur] = m.group(1).strip().strip('"').strip("'")
json.dump({str(k): v for k, v in titles.items()}, open(dst, "w", encoding="utf-8"), ensure_ascii=False, indent=0)
print(len(titles), "titulos")

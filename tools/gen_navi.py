#!/usr/bin/env python3
"""Gera Config/navi.json (rotas do /navi) a partir dos .lub de navegacao do cliente LATAM, ja convertidos com tools/lub_eval.py.
Uso: python3 gen_navi.py navi_map.json navi_link.json Config/navi.json
Saida: {"maps": {"prontera": "Prontera"}, "links": [[de, x, y, para, x, y], ...]}"""
import json
import sys

maps_json = json.load(open(sys.argv[1], encoding="utf-8"))["Navi_Map"]
links_json = json.load(open(sys.argv[2], encoding="utf-8"))["Navi_Link"]
maps = {}
for _, row in maps_json.items():
    name = str(row.get("2", "")).strip()
    maps[row["1"]] = name
links = []
for _, row in links_json.items():
    links.append([row["1"], row["7"], row["8"], row["9"], row["10"], row["11"]])
json.dump({"maps": maps, "links": links}, open(sys.argv[3], "w", encoding="utf-8"), ensure_ascii=False, separators=(",", ":"))
print(len(maps), "mapas,", len(links), "links")

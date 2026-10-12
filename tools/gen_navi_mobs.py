#!/usr/bin/env python3
"""Gera Config/navi_mobs.json (monstros por mapa para a janela de Navegacao) a partir do navi_mob do cliente LATAM e do
banco de monstros do rAthena (nome, nivel e tamanho que o servidor usa, os mesmos que aparecem no jogo).

Uso (no WSL):
  python3 tools/lub_eval.py /tmp/navi_mob.json ".../luafiles514/lua files/navigation/navi_mob_br.lub"
  python3 tools/gen_navi_mobs.py /tmp/navi_mob.json ~/mythofthor-server/rathena/db/re/mob_db.yml Config/navi_mobs.json

Saida: {"mapas": {"prt_fild08": [["Poring", 1, 0], ...]}}  (nome, nivel, tamanho 0 pequeno / 1 medio / 2 grande)
O navi_mob traz o nome codificado (\\x1c...\\x1c); a chave usada e o nome de sprite (AegisName do rAthena).
"""
import json
import re
import sys

navi = json.load(open(sys.argv[1], encoding="utf-8"))["Navi_Mob"]

mobs = {}
cur = {}
for line in open(sys.argv[2], encoding="utf-8"):
    m = re.match(r"^  - Id: (\d+)", line)
    if m:
        if cur.get("aegis"):
            mobs[cur["aegis"]] = cur
        cur = {}
        continue
    m = re.match(r"^    (AegisName|Name|Level|Size): (.+)$", line)
    if m:
        key, val = m.group(1), m.group(2).strip().strip('"')
        cur[{"AegisName": "aegis", "Name": "name", "Level": "level", "Size": "size"}[key]] = val
if cur.get("aegis"):
    mobs[cur["aegis"]] = cur

SIZES = {"Small": 0, "Medium": 1, "Large": 2}
out = {}
missing = set()
for _, row in navi.items():
    mapa, aegis = row["1"], str(row["6"])
    mob = mobs.get(aegis)
    if not mob:
        missing.add(aegis)
        continue
    entry = [mob.get("name", aegis), int(mob.get("level", row.get("7", 1))), SIZES.get(mob.get("size", "Small"), 0)]
    lista = out.setdefault(mapa, [])
    if entry not in lista:
        lista.append(entry)
# Ordem do proprio navi_mob (a do print do LATAM), sem reordenar
json.dump({"mapas": out}, open(sys.argv[3], "w", encoding="utf-8"), ensure_ascii=False, separators=(",", ":"))
print(len(out), "mapas,", sum(len(v) for v in out.values()), "entradas; sem cadastro no rAthena:", len(missing))

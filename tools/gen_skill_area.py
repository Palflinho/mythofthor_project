#!/usr/bin/env python3
"""
Gera Config/skill_area.json: raio (em celulas) da area de cada skill de chao, para o anel do cursor de mira.
Fonte: skill_db.yml do rAthena pre-renovacao. Raio = Unit.Layout quando a skill cria uma unidade (layout n = quadrado de
lado 2n+1) ou SplashArea (raio). Skills sem nenhum dos dois ficam fora (anel de 1 celula).
Uso (WSL): python3 tools/gen_skill_area.py
"""
import json
import os
import re

HOME = os.path.expanduser('~')
DB = os.path.join(HOME, 'mythofthor-server', 'rathena', 'db', 'pre-re', 'skill_db.yml')
BASE = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
if not os.path.isdir(os.path.join(BASE, 'Config')):
    BASE = '/mnt/e/Documentos/Repositórios/mythofthor_project'

text = open(DB, encoding='utf-8').read()
out = {}
for block in re.split(r'\n  - Id: ', text)[1:]:
    sid = int(re.match(r'(\d+)', block).group(1))
    if re.search(r'TargetType: Ground', block) is None:
        continue
    radius = None
    m = re.search(r'\n    Unit:\n(?:      .*\n)*?      Layout: (\d+)', block)
    if m:
        radius = int(m.group(1))
    else:
        s = re.search(r'\n    SplashArea: (\d+)', block)
        if s:
            radius = int(s.group(1))
        else:
            levels = [int(x) for x in re.findall(r'SplashArea:\n(?:      - Level: \d+\n        Area: (\d+)\n)+', block)]
            areas = re.findall(r'Area: (\d+)', block)
            if areas:
                radius = max(int(a) for a in areas)
    if radius is not None:
        out[str(sid)] = radius
json.dump(out, open(os.path.join(BASE, 'Config', 'skill_area.json'), 'w'), indent=1, sort_keys=True)
print(len(out), 'skills de chao com area')

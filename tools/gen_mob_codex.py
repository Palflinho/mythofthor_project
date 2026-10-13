"""
Generates Config/mob_codex.json for the Mimir Codex window (Source/MythOfThor/UI/MythRoMimir.cpp) from the rAthena Renewal
databases: monster name, level, HP, element, race, size, base EXP and the drop table (item id + rate).

Usage (WSL): python3 tools/gen_mob_codex.py [rathena=~/mythofthor-server/rathena]
"""
import json
import os
import sys

import yaml

try:
    Loader = yaml.CSafeLoader
except AttributeError:
    Loader = yaml.SafeLoader


def load(path):
    with open(path, encoding='utf-8') as f:
        return yaml.load(f, Loader=Loader)


def main():
    rath = sys.argv[1] if len(sys.argv) > 1 else os.path.expanduser('~/mythofthor-server/rathena')
    db = os.path.join(rath, 'db', 're')
    aegis = {}
    for name in ('item_db_etc.yml', 'item_db_usable.yml', 'item_db_equip.yml'):
        for it in load(os.path.join(db, name)).get('Body', []):
            aegis[it['AegisName']] = it['Id']
    mobs = {}
    for src in (os.path.join(db, 'mob_db.yml'), os.path.join(rath, 'db', 'import', 'mob_db.yml')):
        if not os.path.exists(src):
            continue
        for m in (load(src) or {}).get('Body', []) or []:
            mid = m.get('Id')
            if not mid:
                continue
            drops = []
            for d in m.get('Drops', []) or []:
                iid = aegis.get(d.get('Item'))
                if iid:
                    drops.append([iid, int(d.get('Rate', 0))])
            mobs[str(mid)] = {
                'n': m.get('Name', m.get('AegisName', str(mid))),
                'l': int(m.get('Level', 1)),
                'h': int(m.get('Hp', 1)),
                'x': int(m.get('BaseExp', 0)),
                'e': '%s %d' % (m.get('Element', 'Neutral'), int(m.get('ElementLevel', 1))),
                'r': m.get('Race', 'Formless'),
                's': m.get('Size', 'Small'),
                'm': 1 if (m.get('MvpExp') or 'Boss' in (m.get('Class') or '')) else 0,
                'd': drops,
            }
    dest = os.path.join(os.path.dirname(os.path.abspath(__file__)), '..', 'Config', 'mob_codex.json')
    with open(dest, 'w', encoding='utf-8', newline='\n') as f:
        json.dump(mobs, f, ensure_ascii=False, separators=(',', ':'))
    print(len(mobs), 'monsters ->', os.path.normpath(dest))


if __name__ == '__main__':
    main()

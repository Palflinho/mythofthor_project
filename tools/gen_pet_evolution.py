"""
Generates Config/pet_evolution.json for the Pet window (Source/MythOfThor/UI/MythRoPet.cpp) from the rAthena pet_db.yml:
for every pet (mob id) the evolutions it can do: the target pet, the egg item id of the target (the client sends it in
CZ_PET_EVOLUTION 0x09fb) and the items consumed. The server checks and does everything (loyalty, items, egg swap).

Usage (WSL): python3 tools/gen_pet_evolution.py [rathena=~/mythofthor-server/rathena]
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
        return yaml.load(f, Loader=Loader) or {}


def main():
    rath = sys.argv[1] if len(sys.argv) > 1 else os.path.expanduser('~/mythofthor-server/rathena')
    db = os.path.join(rath, 'db', 're')
    mob_id, mob_name = {}, {}
    for m in load(os.path.join(db, 'mob_db.yml')).get('Body', []):
        mob_id[m['AegisName']] = m['Id']
        mob_name[m['Id']] = m.get('Name', m['AegisName'])
    item_id = {}
    for name in ('item_db_etc.yml', 'item_db_usable.yml', 'item_db_equip.yml'):
        for it in load(os.path.join(db, name)).get('Body', []):
            item_id[it['AegisName']] = it['Id']
    pets = load(os.path.join(db, 'pet_db.yml')).get('Body', [])
    egg_of = {p['Mob']: item_id.get(p.get('EggItem')) for p in pets}
    out = {}
    for p in pets:
        evos = []
        for e in p.get('Evolution', []) or []:
            target = e.get('Target')
            items = []
            for r in e.get('ItemRequirements', []) or []:
                iid = item_id.get(r.get('Item'))
                if iid:
                    items.append([iid, int(r.get('Amount', 1))])
            tid = mob_id.get(target)
            egg = egg_of.get(target)
            if tid and egg:
                evos.append({'target': tid, 'name': mob_name.get(tid, target), 'egg': egg, 'items': items})
        if evos and p['Mob'] in mob_id:
            out[str(mob_id[p['Mob']])] = evos
    dest = os.path.join(os.path.dirname(os.path.abspath(__file__)), '..', 'Config', 'pet_evolution.json')
    with open(dest, 'w', encoding='utf-8', newline='\n') as f:
        json.dump(out, f, ensure_ascii=False, separators=(',', ':'))
    print(len(out), 'pets with evolutions ->', os.path.normpath(dest))


if __name__ == '__main__':
    main()

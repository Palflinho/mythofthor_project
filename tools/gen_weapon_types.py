"""
Gera Config/weapon_types.json: ID do item de arma -> tipo de arma do cliente (WeaponType: 1 adaga ... 11 arco ... 23 cajado
de duas maos).

Para o proprio personagem o servidor manda o ID do item da arma (ex.: 1714 Arco Gakkung) e nao o tipo, mas o cliente
escolhe pelo tipo o sprite da arma na mao, a animacao de ataque (Config/weapon_action.json) e a flecha do arco. O cliente
original tira o tipo do ClassNum da itemInfo; aqui sai do SubType do item_db do rAthena (a mesma ordem do enum
weapon_type), ou do View quando o item tem um.

Uso (WSL): python3 tools/gen_weapon_types.py ~/mythofthor-server/rathena [Config/weapon_types.json]
"""
import json
import os
import re
import sys

# enum weapon_type do rAthena (W_DAGGER = 1 ... W_2HSTAFF = 23) = WeaponType do cliente
SUBTYPES = ['Dagger', '1hSword', '2hSword', '1hSpear', '2hSpear', '1hAxe', '2hAxe', 'Mace', '2hMace', 'Staff', 'Bow',
            'Knuckle', 'Musical', 'Whip', 'Book', 'Katar', 'Revolver', 'Rifle', 'Gatling', 'Shotgun', 'Grenade', 'Huuma',
            '2hStaff']


def main():
    root = sys.argv[1]
    dest = sys.argv[2] if len(sys.argv) > 2 else os.path.join(os.path.dirname(__file__), '..', 'Config', 'weapon_types.json')
    out = {}
    for name in ('item_db_equip.yml',):
        cur = {}

        def flush():
            if cur.get('type') == 'Weapon' and 'id' in cur:
                view = cur.get('view')
                sub = cur.get('sub')
                if view is not None and 0 < view <= 30:
                    out[str(cur['id'])] = view
                elif sub in SUBTYPES:
                    out[str(cur['id'])] = SUBTYPES.index(sub) + 1

        for line in open(os.path.join(root, 'db', 're', name), encoding='utf-8', errors='replace'):
            m = re.match(r'^\s*-\s*Id:\s*(\d+)', line)
            if m:
                flush()
                cur = {'id': int(m.group(1))}
                continue
            m = re.match(r'^\s{4}(Type|SubType|View):\s*(\S+)', line)
            if m:
                key = {'Type': 'type', 'SubType': 'sub', 'View': 'view'}[m.group(1)]
                cur[key] = int(m.group(2)) if key == 'view' and m.group(2).isdigit() else m.group(2)
        flush()
    with open(dest, 'w', encoding='utf-8') as f:
        json.dump(out, f, separators=(',', ':'), sort_keys=True)
    print('weapon_types.json: %d armas -> %s' % (len(out), os.path.normpath(dest)))


if __name__ == '__main__':
    main()

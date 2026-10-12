"""
Gera Config/weapon_types.json: ID do item -> visual que o cliente desenha.
  weapons: ID da arma -> tipo de arma do cliente (WeaponType: 1 adaga ... 11 arco ... 16 katar ... 23 cajado de 2 maos)
  shields: ID do escudo -> view do escudo (1 guarda, 2 broquel, 3 escudo, 4 espelho...)

Para o proprio personagem o servidor manda o ID do item (ex.: 1714 Arco Gakkung, 2101 Guarda) e nao o tipo/view, mas o
cliente escolhe pelo tipo o sprite da arma na mao, a animacao de ataque (Config/weapon_action.json), a flecha do arco e o
sprite do escudo. O cliente original tira isso do ClassNum da itemInfo; aqui sai do item_db do rAthena: SubType da arma
(a mesma ordem do enum weapon_type) ou View, e View do escudo (Locations: Left_Hand).

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
    weapons, shields = {}, {}
    cur = {}

    def flush():
        if 'id' not in cur:
            return
        view = cur.get('view')
        if cur.get('type') == 'Weapon':
            if view is not None and 0 < view <= 30:
                weapons[str(cur['id'])] = view
            elif cur.get('sub') in SUBTYPES:
                weapons[str(cur['id'])] = SUBTYPES.index(cur['sub']) + 1
        elif cur.get('type') == 'Armor' and cur.get('left') and view:
            shields[str(cur['id'])] = view

    in_locations = False
    for line in open(os.path.join(root, 'db', 're', 'item_db_equip.yml'), encoding='utf-8', errors='replace'):
        m = re.match(r'^\s*-\s*Id:\s*(\d+)', line)
        if m:
            flush()
            cur = {'id': int(m.group(1))}
            in_locations = False
            continue
        m = re.match(r'^\s{4}(\w+):\s*(\S*)', line)
        if m:
            key, value = m.group(1), m.group(2)
            in_locations = key == 'Locations'
            if key == 'Type':
                cur['type'] = value
            elif key == 'SubType':
                cur['sub'] = value
            elif key == 'View' and value.isdigit():
                cur['view'] = int(value)
            continue
        if in_locations and re.match(r'^\s{6}Left_Hand:\s*true', line):
            cur['left'] = True
    flush()
    with open(dest, 'w', encoding='utf-8') as f:
        json.dump({'weapons': weapons, 'shields': shields}, f, separators=(',', ':'), sort_keys=True)
    print('weapon_types.json: %d armas, %d escudos -> %s' % (len(weapons), len(shields), os.path.normpath(dest)))


if __name__ == '__main__':
    main()

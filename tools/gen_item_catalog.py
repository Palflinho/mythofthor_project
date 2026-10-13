"""
Gera Config/item_catalog.json: catalogo de itens para a Loja de Testes (janela de GM).
Cada item: {"i": id, "n": nome em ingles (item_db), "p": nome em portugues (Config/items.json, quando legivel),
"t": "equip" | "card" | "use" | "etc", "s": subtipo (arma/armadura/posicao)}.
Fonte: item_db_equip.yml, item_db_usable.yml, item_db_etc.yml do rAthena (modo Renovacao) e Config/items.json.

Uso (WSL): python3 tools/gen_item_catalog.py [~/mythofthor-server/rathena]
"""
import json
import os
import re
import sys

OK_NAME = re.compile(r"^[\w \-'\[\]\.,:()+!/&%]+$", re.UNICODE)


def parse(path):
    items = []
    cur = None
    in_loc = False
    for line in open(path, encoding='utf-8', errors='replace'):
        m = re.match(r'^\s*-\s*Id:\s*(\d+)', line)
        if m:
            cur = {'id': int(m.group(1)), 'name': '', 'type': 'Etc', 'sub': ''}
            items.append(cur)
            in_loc = False
            continue
        if cur is None:
            continue
        if re.match(r'^    Locations:', line):
            in_loc = True
            continue
        if in_loc:
            m = re.match(r'^      (\w+):\s*true', line)
            if m:
                cur['sub'] = cur['sub'] or m.group(1)
                continue
            in_loc = False
        m = re.match(r'^    Name:\s*(.+)', line)
        if m and not cur['name']:
            cur['name'] = m.group(1).strip().strip('"')
        m = re.match(r'^    Type:\s*(\w+)', line)
        if m:
            cur['type'] = m.group(1)
        m = re.match(r'^    SubType:\s*(\w+)', line)
        if m and not cur['sub']:
            cur['sub'] = m.group(1)
    return items


def main():
    rat = os.path.expanduser(sys.argv[1] if len(sys.argv) > 1 else '~/mythofthor-server/rathena')
    root = os.path.join(os.path.dirname(os.path.abspath(__file__)), '..')
    pt = json.load(open(os.path.join(root, 'Config', 'items.json'), encoding='utf-8'))
    out = []
    for fn in ('item_db_equip.yml', 'item_db_usable.yml', 'item_db_etc.yml'):
        for it in parse(os.path.join(rat, 'db', 're', fn)):
            t = it['type']
            kind = 'equip' if t in ('Weapon', 'Armor') else 'card' if t == 'Card' else 'use' if t in ('Healing', 'Usable', 'Delayconsume', 'Cash') else 'etc'
            p = pt.get(str(it['id']), {}).get('name', '')
            if not OK_NAME.match(p or ''):
                p = ''
            out.append({'i': it['id'], 'n': it['name'], 'p': p, 't': kind, 's': it['sub']})
    out.sort(key=lambda e: e['i'])
    dest = os.path.join(root, 'Config', 'item_catalog.json')
    with open(dest, 'w', encoding='utf-8') as f:
        json.dump(out, f, ensure_ascii=False, separators=(',', ':'))
    cnt = {}
    for e in out:
        cnt[e['t']] = cnt.get(e['t'], 0) + 1
    print('itens', len(out), cnt, '->', os.path.normpath(dest))


if __name__ == '__main__':
    main()

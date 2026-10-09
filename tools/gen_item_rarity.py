"""
Gera Config/item_rarity.json: raridade visual de cada item (nome colorido e aura no chao, estilo Diablo 4),
calculada pelas taxas de drop do proprio rAthena (db/pre-re/mob_db.yml + item_db*.yml). So aparencia:
nenhuma regra de jogo muda.

Faixas (melhor taxa entre todos os monstros; Rate do mob_db em 1/10000):
  0 comum     >= 10%      (ou item que nao cai de monstro: loja, missao...)
  1 incomum   1% .. 10%
  2 raro      0,1% .. 1%
  3 epico     0,01% .. 0,1%
  4 lendario  < 0,01% e cartas (premios de MVP nao contam: incluem itens comuns)

Uso (WSL):
  python3 tools/gen_item_rarity.py ~/mythofthor-server/rathena
"""
import json
import os
import re
import sys


def parse_items(paths):
    """AegisName -> (Id, Type) de todos os item_db*.yml."""
    out = {}
    for path in paths:
        cur = {}
        for line in open(path, encoding='utf-8', errors='replace'):
            m = re.match(r'^\s*-\s*Id:\s*(\d+)', line)
            if m:
                cur = {'id': int(m.group(1))}
                continue
            m = re.match(r'^\s*AegisName:\s*(\S+)', line)
            if m and 'id' in cur and 'aegis' not in cur:
                cur['aegis'] = m.group(1)
                out[cur['aegis']] = [cur['id'], 'Etc']
                continue
            m = re.match(r'^\s*Type:\s*(\S+)', line)
            if m and cur.get('aegis') in out:
                out[cur['aegis']][1] = m.group(1)
    return out


def parse_drops(path):
    """AegisName do item -> (melhor taxa, se e premio de MVP)."""
    best = {}
    section = None
    item = None
    for line in open(path, encoding='utf-8', errors='replace'):
        if re.match(r'^\s*-\s*Id:\s*\d+', line):
            section = None
        m = re.match(r'^\s*(MvpDrops|Drops):\s*$', line)
        if m:
            section = m.group(1)
            continue
        m = re.match(r'^\s*-\s*Item:\s*(\S+)', line)
        if m and section:
            item = m.group(1)
            continue
        m = re.match(r'^\s*Rate:\s*(\d+)', line)
        if m and section and item:
            rate = int(m.group(1))
            prev = best.get(item, (0, False))
            best[item] = (max(prev[0], rate), prev[1] or section == 'MvpDrops')
            item = None
    return best


def tier(rate, mvp, item_type):
    if item_type == 'Card':
        return 4
    if rate == 0 or rate >= 1000:
        return 0
    if rate >= 100:
        return 1
    if rate >= 10:
        return 2
    if rate >= 1:
        return 3
    return 4


def main():
    if len(sys.argv) < 2:
        print(__doc__)
        sys.exit(1)
    db = os.path.join(os.path.expanduser(sys.argv[1]), 'db', 'pre-re')
    items = parse_items([os.path.join(db, f) for f in sorted(os.listdir(db)) if f.startswith('item_db') and f.endswith('.yml')])
    drops = parse_drops(os.path.join(db, 'mob_db.yml'))
    out = {}
    counts = [0] * 5
    for aegis, (iid, itype) in items.items():
        rate, mvp = drops.get(aegis, (0, False))
        t = tier(rate, mvp, itype)
        if itype == 'Card':
            t = 4
        counts[t] += 1
        if t > 0:
            out[str(iid)] = t  # comum e o padrao: fica fora do arquivo
    dest = os.path.join(os.path.dirname(os.path.abspath(__file__)), '..', 'Config', 'item_rarity.json')
    with open(dest, 'w', encoding='utf-8') as f:
        json.dump(out, f, indent=0, sort_keys=True)
    print('itens: %d | comum %d, incomum %d, raro %d, epico %d, lendario %d -> %s'
          % (len(items), *counts, os.path.normpath(dest)))
    for name in ('Jellopy', 'Red_Potion', 'Knife', 'Poring_Card', 'Clip', 'Apple'):
        if name in items:
            print('  %-12s -> %d' % (name, tier(*drops.get(name, (0, False)), items[name][1])))


if __name__ == '__main__':
    main()

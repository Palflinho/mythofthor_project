"""
Gera docs/MAPAS_PENDENTES.md: lista dos mapas com monstros do rAthena (Renovacao) agrupados por tipo (campos, dungeons,
cidades/outros), com o nivel dos monstros, os MVPs e se o mapa ja esta importado no Unreal (Content/Maps).
Instancias: lidas de npc/instances (nome e arquivo).

Uso (WSL): python3 tools/gen_map_list.py [~/mythofthor-server/rathena]
"""
import os
import re
import sys

MAX_LEVEL = 130  # transclasse ate 125: monstros acima disso ficam em "fora do escopo"


def load_mobs(rat):
    mobs = {}
    cur = None
    for line in open(os.path.join(rat, 'db', 're', 'mob_db.yml'), encoding='utf-8', errors='replace'):
        m = re.match(r'^  - Id:\s*(\d+)', line)
        if m:
            cur = {'id': int(m.group(1)), 'aegis': '', 'name': '', 'lv': 0, 'mvp': False}
            mobs[cur['id']] = cur
            continue
        if cur is None:
            continue
        for key, f in (('AegisName', 'aegis'), ('Name', 'name')):
            mm = re.match(r'^    %s:\s*(.+)' % key, line)
            if mm and not cur[f]:
                cur[f] = mm.group(1).strip().strip('"')
        mm = re.match(r'^    Level:\s*(\d+)', line)
        if mm:
            cur['lv'] = int(mm.group(1))
        if re.match(r'^    MvpExp:\s*[1-9]', line):
            cur['mvp'] = True
    by_aegis = {m['aegis'].lower(): m for m in mobs.values()}
    return mobs, by_aegis


def main():
    rat = os.path.expanduser(sys.argv[1] if len(sys.argv) > 1 else '~/mythofthor-server/rathena')
    root = os.path.join(os.path.dirname(os.path.abspath(__file__)), '..')
    mobs, by_aegis = load_mobs(rat)
    imported = {d.lower().replace('_', '') for d in os.listdir(os.path.join(root, 'Content', 'Maps'))}
    spawns = {}  # mapa -> {mob_id: qtd}
    kind = {}
    base = os.path.join(rat, 'npc', 're', 'mobs')
    for dp, _, files in os.walk(base):
        sub = os.path.relpath(dp, base).split(os.sep)[0]
        for fn in files:
            if not fn.endswith('.txt'):
                continue
            for line in open(os.path.join(dp, fn), encoding='utf-8', errors='replace'):
                m = re.match(r'^([\w@\-\.]+),(?:\d+,\d+,\d+,\d+)?\s*\t(?:monster|boss_monster)\t[^\t]*\t(\d+),(\d+)', line)
                if not m:
                    continue
                mp, mid, qty = m.group(1), int(m.group(2)), int(m.group(3))
                if mid not in mobs:
                    continue
                spawns.setdefault(mp, {})[mid] = spawns.get(mp, {}).get(mid, 0) + qty
                kind.setdefault(mp, sub)
    rows = []
    for mp, mm in spawns.items():
        lvs = [mobs[i]['lv'] for i in mm]
        # nivel tipico: mediana ponderada pela quantidade (as ervas de nivel 1 de varias dungeons nao contam)
        pairs = sorted((mobs[i]['lv'], q) for i, q in mm.items() if mobs[i]['lv'] > 1 and q > 0) or sorted((mobs[i]['lv'], q) for i, q in mm.items())
        half = sum(q for _, q in pairs) / 2.0
        acc = 0
        med = pairs[-1][0]
        for lv, q in pairs:
            acc += q
            if acc >= half:
                med = lv
                break
        mvps = sorted({mobs[i]['name'] for i in mm if mobs[i]['mvp']})
        top = sorted(mm, key=lambda i: -mm[i])[:4]
        rows.append({'map': mp, 'kind': kind[mp], 'lo': min(lvs), 'hi': max(lvs), 'med': med, 'mvps': mvps,
                     'top': [mobs[i]['name'] for i in top], 'done': mp.lower().replace('_', '') in imported})
    inst = []
    ib = os.path.join(rat, 'npc', 'instances')
    if os.path.isdir(ib):
        inst = sorted(f[:-4] for f in os.listdir(ib) if f.endswith('.txt'))
    groups = {}
    for r in rows:
        grp = 'Fora do escopo (nível típico acima de %d)' % MAX_LEVEL if r['med'] > MAX_LEVEL else {'fields': 'Campos', 'dungeons': 'Dungeons', '.': 'Outros (castelos, tesouros, eventos)'}.get(r['kind'], r['kind'])
        groups.setdefault(grp, []).append(r)
    out = ['# Mapas, dungeons e instâncias: o que falta implementar', '',
           'Gerado por `tools/gen_map_list.py` a partir dos spawns do rAthena (Renovação, `npc/re/mobs`) e do que já está em `Content/Maps`.',
           'Nível = faixa dos monstros do mapa. ✅ = já importado no Unreal. Prioridade sugerida: o que está no caminho de 1 a 99 e da transclasse.', '']
    total = len(rows)
    done = sum(1 for r in rows if r['done'])
    out.append('**Resumo:** %d mapas com monstros, %d importados (%d%%).' % (total, done, 100 * done // max(total, 1)))
    out.append('')
    order = sorted(groups, key=lambda g: (g.startswith('Fora'), g))
    for g in order:
        rs = sorted(groups[g], key=lambda r: (r['med'], r['map']))
        out.append('## %s (%d mapas, %d importados)' % (g, len(rs), sum(1 for r in rs if r['done'])))
        out.append('')
        out.append('| Mapa | Nível típico (faixa) | Monstros principais | MVP | Unreal |')
        out.append('|---|---|---|---|---|')
        for r in rs:
            out.append('| `%s` | %d (%d-%d) | %s | %s | %s |' % (r['map'], r['med'], r['lo'], r['hi'], ', '.join(r['top']), ', '.join(r['mvps']) or '-', '✅' if r['done'] else '⬜'))
        out.append('')
    cities = set()
    for sub in ('warps', os.path.join('re', 'warps')):
        cd = os.path.join(rat, 'npc', sub, 'cities')
        if os.path.isdir(cd):
            cities.update(f[:-4] for f in os.listdir(cd) if f.endswith('.txt'))
    out.append('## Cidades (npc/warps/cities)')
    out.append('')
    out.append('Cada cidade tem o mapa principal e interiores (lojas, guildas, casas); só o mapa principal é conferido aqui.')
    out.append('')
    out.append('| Cidade | Unreal |')
    out.append('|---|---|')
    for c in sorted(cities):
        out.append('| `%s` | %s |' % (c, '✅' if c.replace('_', '') in imported else '⬜'))
    out.append('')
    out.append('## Instâncias (npc/instances)')
    out.append('')
    out.append('Instâncias criam cópias de mapas por grupo; cada uma precisa dos mapas dela importados. Arquivos do rAthena:')
    out.append('')
    desc = {'EndlessTower': 'Torre sem Fim (andares sucessivos, a partir do nível 50)', 'OrcsMemory': 'Memória dos Orcs (grupo, nível ~50)',
            'SealedShrine': 'Santuário Selado (grupo, nível alto)', 'NydhoggsNest': 'Ninho de Nidhogg (fim de jogo, fora do escopo)'}
    for i in inst:
        out.append('- `%s`: %s' % (i, desc.get(i, '')))
    out.append('')
    dest = os.path.join(root, 'docs', 'MAPAS_PENDENTES.md')
    open(dest, 'w', encoding='utf-8', newline='\n').write('\n'.join(out))
    print('mapas %d (importados %d), instancias %d -> %s' % (total, done, len(inst), dest))


if __name__ == '__main__':
    main()

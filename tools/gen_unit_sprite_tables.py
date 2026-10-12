"""
Gera Config/unit_sprites.json: ID visual do servidor -> nome do sprite do cliente (data/sprite/...).

O servidor manda so o "job"/class de cada unidade (pacotes 0x09fd/0x09fe/0x09ff). O cliente original resolve o
sprite pelas tabelas compiladas jobname.lub / npcidentity.lub; como elas sao bytecode, usamos as mesmas fontes de
nomes que o rAthena:
  - NPCs:     enum JT_* de src/map/npc.hpp      -> data/sprite/npc/<nome minusculo>.spr
  - Monstros: AegisName de db/re/mob_db.yml (Renovacao, desde 2026-10-12; --prere usa db/pre-re) -> data/sprite/몬스터/<aegis minusculo>.spr

Uso (WSL):
  python3 tools/gen_unit_sprite_tables.py ~/mythofthor-server/rathena [--assets /mnt/e/Dev/Ragnarok_Assets/grf]
"""
import json
import os
import re
import sys


def parse_npc_enum(path):
    """Le o enum de JT_ (valores implicitos e explicitos) e devolve {id: nome}."""
    text = open(path, encoding='utf-8', errors='replace').read()
    start = text.find('enum e_job_types')
    body = text[start:text.find('};', start)]
    out = {}
    value = -1
    for raw in body.splitlines():
        line = raw.split('//')[0].strip().rstrip(',')
        m = re.match(r'^([A-Za-z_]\w*)\s*(?:=\s*(\w+))?$', line)  # inclui NPC_RANGE*_START, que fixam a contagem
        if not m:
            continue
        name, explicit = m.group(1), m.group(2)
        if explicit is not None:
            value = int(explicit, 0) if explicit[0].isdigit() else out_names.get(explicit, value + 1)
        else:
            value += 1
        out_names[name] = value
        if name.startswith('JT_') and not name.endswith(('_START', '_END')):
            out.setdefault(value, name[3:].lower())
    return out


out_names = {}


def parse_mob_db(path):
    """Le Id/AegisName de cada entrada do mob_db.yml (sem depender de PyYAML)."""
    out = {}
    cur = None
    for line in open(path, encoding='utf-8', errors='replace'):
        m = re.match(r'^\s*-\s*Id:\s*(\d+)', line)
        if m:
            cur = int(m.group(1))
            continue
        m = re.match(r'^\s*AegisName:\s*(\S+)', line)
        if m and cur is not None:
            out[cur] = m.group(1).lower()
            cur = None
    return out


def main():
    if len(sys.argv) < 2:
        print(__doc__)
        sys.exit(1)
    rathena = os.path.expanduser(sys.argv[1])
    assets = sys.argv[sys.argv.index('--assets') + 1] if '--assets' in sys.argv else None
    npcs = parse_npc_enum(os.path.join(rathena, 'src/map/npc.hpp'))
    mobs = parse_mob_db(os.path.join(rathena, 'db/pre-re/mob_db.yml' if '--prere' in sys.argv else 'db/re/mob_db.yml'))

    # Tabela do proprio cliente (npcidentity.lub + jobname.lub avaliados por tools/lub_eval.py): id -> nome do sprite.
    # E o que o cliente original usa; o AegisName do rAthena nem sempre e o nome do arquivo (CHONCHON = chocho.spr).
    if '--jobname' in sys.argv:
        table = json.load(open(sys.argv[sys.argv.index('--jobname') + 1], encoding='utf-8')).get('JobNameTable', {})
        def exists(folder, name):
            return not assets or os.path.exists(os.path.join(assets, 'data', 'sprite', folder, name + '.spr'))
        fixed = 0
        for key, raw in table.items():
            if not str(key).isdigit() or not isinstance(raw, str):
                continue
            uid, name = int(key), raw.lower()
            if uid in mobs and exists('몬스터', name):
                fixed += mobs[uid] != name
                mobs[uid] = name
            elif uid in npcs and exists('npc', name):
                fixed += npcs[uid] != name
                npcs[uid] = name
            elif uid not in mobs and uid not in npcs and 1000 < uid < 4000 and exists('몬스터', name):
                mobs[uid] = name
        print('nomes corrigidos pela tabela do cliente:', fixed)

    if assets:
        def have(folder, name):
            return os.path.exists(os.path.join(assets, 'data', 'sprite', folder, name + '.spr'))
        miss_npc = [n for n in npcs.values() if not have('npc', n)]
        miss_mob = [n for n in mobs.values() if not have('몬스터', n)]
        print('NPCs sem sprite: %d de %d (ex.: %s)' % (len(miss_npc), len(npcs), miss_npc[:5]))
        print('Monstros sem sprite: %d de %d (ex.: %s)' % (len(miss_mob), len(mobs), miss_mob[:5]))

    dest = os.path.join(os.path.dirname(os.path.abspath(__file__)), '..', 'Config', 'unit_sprites.json')
    with open(dest, 'w', encoding='utf-8') as f:
        json.dump({'npc': {str(k): v for k, v in sorted(npcs.items())},
                   'mob': {str(k): v for k, v in sorted(mobs.items())}}, f, ensure_ascii=False, indent=0)
    print('gravado', os.path.normpath(dest), '- npcs', len(npcs), 'monstros', len(mobs))


if __name__ == '__main__':
    main()

"""
Gera Config/skill_vfx_classes.json: para cada skill do rAthena (db/re/skill_db.yml), o elemento e a "classe de efeito" que o
UMythSkillManager usa quando o cliente original nao tem animacao para ela (mesma classe + cor do elemento = visual generico).

effect_class: none (passiva), melee_hit, ranged_hit, magic_hit, ground_area, trap, buff_self, buff_target, heal, debuff.
Saida: {"<id>": {"class": "...", "element": "Fire", "area": 5, "name": "AEGIS_NAME"}}

Uso (WSL): python3 tools/gen_skill_vfx_classes.py [~/mythofthor-server/rathena]
"""
import json
import os
import re
import sys

HEAL = ('HEAL', 'CURE', 'RESURRECTION', 'RESSURECTION', 'RECOVERY', 'SANCTUARY', 'PNEUMA', 'MEDITATIO', 'APPLEIDUN', 'SAFETYWALL')
DEBUFF = ('CURSE', 'SILENCE', 'STUN', 'BLIND', 'SLEEP', 'POISON', 'STONE', 'CONFUSE', 'FREEZE', 'SLOW', 'DECAGI', 'DECREASE', 'SIGNUM',
          'CRAZY', 'ANKLE', 'PROVOKE', 'WEAKNESS', 'DISARM', 'STRIP', 'QUAGMIRE', 'ENTANGLE', 'LULLABY', 'DEEPSLEEP')


def main():
    rat = os.path.expanduser(sys.argv[1] if len(sys.argv) > 1 else '~/mythofthor-server/rathena')
    root = os.path.join(os.path.dirname(os.path.abspath(__file__)), '..')
    out = {}
    cur = None
    for line in open(os.path.join(rat, 'db', 're', 'skill_db.yml'), encoding='utf-8', errors='replace'):
        m = re.match(r'^\s*-\s*Id:\s*(\d+)', line)
        if m and not line.startswith('      '):
            cur = {'id': int(m.group(1)), 'name': '', 'type': 'None', 'target': 'Passive', 'range': 0, 'element': 'Neutral', 'area': 0}
            out[cur['id']] = cur
            continue
        if cur is None:
            continue
        for key, f in (('Name', 'name'), ('Type', 'type'), ('TargetType', 'target')):
            mm = re.match(r'^    %s:\s*(\S+)' % key, line)
            if mm:
                cur[f] = mm.group(1)
        mm = re.match(r'^    Range:\s*(-?\d+)', line)
        if mm:
            cur['range'] = int(mm.group(1))
        mm = re.match(r'^    Element:\s*(\w+)', line)
        if mm and cur['element'] == 'Neutral':
            cur['element'] = mm.group(1)
        mm = re.match(r'^    (?:SplashArea|Area):\s*(\d+)', line)
        if mm:
            cur['area'] = int(mm.group(1))
        mm = re.match(r'^\s+(?:Size|Area):\s*(\d+)', line)
        if mm and cur['target'] == 'Ground' and not cur['area']:
            cur['area'] = int(mm.group(1))
    res = {}
    counts = {}
    for sid, s in out.items():
        name = s['name'].upper()
        t = s['target']
        if t == 'Passive':
            cls = 'none'
        elif t == 'Ground':
            cls = 'ground_area'
        elif t == 'Trap':
            cls = 'trap'
        elif t == 'Attack':
            cls = 'magic_hit' if s['type'] == 'Magic' else ('melee_hit' if s['range'] <= 3 else 'ranged_hit')
        elif t == 'Self':
            cls = 'heal' if any(k in name for k in HEAL) else 'buff_self'
        else:  # Support
            if any(k in name for k in HEAL):
                cls = 'heal'
            elif any(k in name for k in DEBUFF):
                cls = 'debuff'
            else:
                cls = 'buff_target'
        counts[cls] = counts.get(cls, 0) + 1
        res[str(sid)] = {'class': cls, 'element': s['element'], 'area': s['area'], 'name': s['name']}
    dest = os.path.join(root, 'Config', 'skill_vfx_classes.json')
    with open(dest, 'w', encoding='utf-8') as f:
        json.dump(res, f, ensure_ascii=False, separators=(',', ':'), sort_keys=True)
    print('skills', len(res), counts, '->', os.path.normpath(dest))


if __name__ == '__main__':
    main()

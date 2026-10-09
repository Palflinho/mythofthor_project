"""
Gera Config/skill_effects.json: que efeito visual cada skill toca e o elemento dela (cor da luz do Lumen).

Fontes:
  - efeitos (id -> .str/.spr/.wav) e skill -> efeito: tabelas do roBrowser (src/DB/Effects/EffectTable.js e
    src/DB/Skills/SkillEffect.js + SkillConst.js), lidas como DADOS (nada e executado). No cliente original essa
    tabela fica embutida no executavel.
  - elemento de cada skill: db/pre-re/skill_db.yml do rAthena (Element: Fire, Water, ...).

Uso (WSL):
  python3 tools/gen_skill_effects.py ~/mythofthor-server/rathena <pasta_com_os_js_do_roBrowser>
"""
import json
import os
import re
import sys


def parse_effects(path):
    txt = open(path, encoding='latin-1').read().replace(chr(13), '')
    out = {}
    for m in re.finditer(r'\n\t\t(\d+):\s*\[(.*?\})\]', txt, re.S):
        items = []
        for obj in re.finditer(r'\{(.*?)\}', m.group(2), re.S):
            body = '\n'.join(l for l in obj.group(1).splitlines() if not l.strip().startswith('//'))
            t = re.search(r"type:\s*'(\w+)'", body)
            f = re.search(r"file:\s*'([^']+)'", body)
            w = re.search(r"wav:\s*'([^']+)'", body)
            a = re.search(r'attachedEntity:\s*(true|false)', body)
            if t and t.group(1) in ('STR', 'SPR') and f:
                items.append({'type': t.group(1), 'file': f.group(1), 'wav': w.group(1) if w else '',
                              'attached': bool(a and a.group(1) == 'true')})
            elif w:
                items.append({'type': 'WAV', 'file': '', 'wav': w.group(1), 'attached': True})
        if items:
            out[m.group(1)] = items
    return out


def parse_skill_effects(js_dir):
    consts = dict((n, int(v)) for n, v in re.findall(r'(\w+)\s*:\s*(\d+)', open(os.path.join(js_dir, 'SkillConst.js'), encoding='latin-1').read()))
    txt = open(os.path.join(js_dir, 'SkillEffect.js'), encoding='latin-1').read()
    out = {}
    for m in re.finditer(r'SkillEffect\[SK\.(\w+)\]\s*=\s*\{(.*?)\}', txt, re.S):
        if m.group(1) not in consts:
            continue
        entry = {}
        for k, v in re.findall(r'(\w*[Ee]ffectId)\s*:\s*(\d+)', m.group(2)):
            entry[{'effectId': 'effect', 'hitEffectId': 'hit', 'groundEffectId': 'ground', 'successEffectId': 'success'}.get(k, k)] = int(v)
        out[consts[m.group(1)]] = entry
    return out


def parse_elements(path):
    out = {}
    cur = None
    for line in open(path, encoding='utf-8', errors='replace'):
        m = re.match(r'^\s*-\s*Id:\s*(\d+)', line)
        if m:
            cur = int(m.group(1))
            continue
        m = re.match(r'^\s*Element:\s*(\w+)', line)
        if m and cur is not None and cur not in out:
            out[cur] = m.group(1)
    return out


def main():
    if len(sys.argv) < 3:
        print(__doc__)
        sys.exit(1)
    rathena, js = os.path.expanduser(sys.argv[1]), sys.argv[2]
    effects = parse_effects(os.path.join(js, 'EffectTable.js'))
    skills = parse_skill_effects(js)
    elements = parse_elements(os.path.join(rathena, 'db', 'pre-re', 'skill_db.yml'))
    out_skills = {}
    for sid in set(skills) | set(elements):
        e = dict(skills.get(sid, {}))
        if elements.get(sid) and elements[sid] != 'Neutral':
            e['element'] = elements[sid]
        if e:
            out_skills[str(sid)] = e
    dest = os.path.join(os.path.dirname(os.path.abspath(__file__)), '..', 'Config', 'skill_effects.json')
    with open(dest, 'w', encoding='utf-8') as f:
        json.dump({'effects': effects, 'skills': out_skills}, f, ensure_ascii=False, indent=0, sort_keys=True)
    print('efeitos %d, skills com efeito %d, skills com elemento %d -> %s'
          % (len(effects), len(skills), sum(1 for v in out_skills.values() if 'element' in v), os.path.normpath(dest)))


if __name__ == '__main__':
    main()

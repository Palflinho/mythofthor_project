"""
Moves the client tables from the LATAM Portuguese text to the English text of the Data2026 client (ROenglishRE translation):
  - Config/skills.json      name + description lines of every skill the 2026 client knows (SKID ids, skilldescript.lub, skillinfolist.lub)
  - Config/skill_tree.json  the skill names used by the skill window
  - Config/maps.json        map display names (mapnametable.txt)
Only text is replaced; ids, resource names and every other field stay. Entries the 2026 client does not know keep the LATAM text.
The .lub files are read as text (nothing is executed).

Usage (WSL): python3 tools/gen_english_tables.py [grf2026 data dir = /mnt/d/Ragnarok_Assets/grf2026/data]
"""
import json
import os
import re
import sys

ROOT = os.path.join(os.path.dirname(os.path.abspath(__file__)), '..')
CFG = os.path.join(ROOT, 'Config')


def read(path):
    with open(path, 'rb') as f:
        raw = f.read()
    for enc in ('utf-8', 'cp1252', 'cp949'):
        try:
            return raw.decode(enc)
        except UnicodeDecodeError:
            continue
    return raw.decode('latin-1')


def unescape(s):
    return s.replace('\\"', '"').replace('\\\\', '\\')


def main():
    data = sys.argv[1] if len(sys.argv) > 1 else '/mnt/d/Ragnarok_Assets/grf2026/data'
    z = os.path.join(data, 'luafiles514', 'lua files', 'skillinfoz')

    # SKID name -> id
    ids = {}
    for m in re.finditer(r'^\s*([A-Z0-9_]+)\s*=\s*(\d+)\s*,?', read(os.path.join(z, 'skillid.lub')), re.M):
        ids[m.group(1)] = int(m.group(2))

    # SKID.NAME -> SkillName
    names = {}
    info = read(os.path.join(z, 'skillinfolist.lub'))
    for m in re.finditer(r'\[SKID\.([A-Z0-9_]+)\]\s*=\s*\{(.*?)SkillName\s*=\s*"((?:[^"\\]|\\.)*)"', info, re.S):
        names[m.group(1)] = unescape(m.group(3))

    # SKID.NAME -> description lines
    descs = {}
    text = read(os.path.join(z, 'skilldescript.lub'))
    for m in re.finditer(r'\[SKID\.([A-Z0-9_]+)\]\s*=\s*\{(.*?)\n\t\}', text, re.S):
        # (the source has stray digits before a colour code on a few lines, e.g. "14^CC3399Requirement")
        lines = [re.sub(r'^\d+(\^[0-9A-Fa-f]{6})', r'\1', unescape(x)) for x in re.findall(r'"((?:[^"\\]|\\.)*)"', m.group(2))]
        if lines:
            descs[m.group(1)] = lines

    # skills.json
    path = os.path.join(CFG, 'skills.json')
    skills = json.load(open(path, encoding='utf-8'))
    changed = 0
    seen = set()
    for key, e in skills.items():
        if not isinstance(e, dict) or id(e) in seen:
            continue
        seen.add(id(e))
        aegis = e.get('aegis_name')
        if not aegis:
            continue
        n = names.get(aegis)
        d = descs.get(aegis)
        if n:
            e['name'] = n
        if d:
            e['description'] = d
        if n or d:
            changed += 1
    json.dump(skills, open(path, 'w', encoding='utf-8', newline='\n'), ensure_ascii=False, separators=(',', ':'))
    print('skills.json: %d skills in English (2026 knows %d names, %d descriptions)' % (changed, len(names), len(descs)))

    # skill_tree.json: names by aegis
    path = os.path.join(CFG, 'skill_tree.json')
    tree = json.load(open(path, encoding='utf-8'))
    tn = 0
    for sid, e in tree.get('skills', {}).items():
        n = names.get(e.get('aegis'))
        if n:
            e['name'] = n
            tn += 1
    json.dump(tree, open(path, 'w', encoding='utf-8', newline='\n'), ensure_ascii=False, indent=1)
    print('skill_tree.json: %d names' % tn)

    # maps.json
    path = os.path.join(CFG, 'maps.json')
    maps = json.load(open(path, encoding='utf-8'))
    en = {}
    for line in read(os.path.join(data, 'mapnametable.txt')).splitlines():
        m = re.match(r'^([\w@-]+)\.rsw#([^#]*)#', line)
        if m:
            en[m.group(1).lower()] = m.group(2).strip()
    mc = 0
    for k, e in maps.items():
        n = en.get(k.lower())
        if n and isinstance(e, dict):
            e['name'] = n
            mc += 1
    json.dump(maps, open(path, 'w', encoding='utf-8', newline='\n'), ensure_ascii=False, indent=2)
    print('maps.json: %d of %d maps in English' % (mc, len(maps)))


if __name__ == '__main__':
    main()


def navi_names(data):
    """Config/navi.json: map names of the /navi window and the world map caption in English (mapnametable.txt)."""
    path = os.path.join(CFG, 'navi.json')
    navi = json.load(open(path, encoding='utf-8'))
    en = {}
    for line in read(os.path.join(data, 'mapnametable.txt')).splitlines():
        m = re.match(r'^([\w@-]+)\.rsw#([^#]*)#', line)
        if m:
            en[m.group(1).lower()] = m.group(2).strip()
    n = 0
    for k in list(navi.get('maps', {}).keys()):
        e = en.get(k.lower())
        if e:
            navi['maps'][k] = e
            n += 1
    json.dump(navi, open(path, 'w', encoding='utf-8', newline='\n'), ensure_ascii=False, separators=(',', ':'))
    print('navi.json: %d map names in English' % n)


if __name__ == '__main__':
    navi_names(sys.argv[1] if len(sys.argv) > 1 else '/mnt/d/Ragnarok_Assets/grf2026/data')

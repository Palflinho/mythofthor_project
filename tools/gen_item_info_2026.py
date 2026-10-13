"""
Gera Config/item_info_2026.json a partir do System/LuaFiles514/itemInfo.lua do cliente 2026 (iRO, em ingles): o LATAM que usamos nao
conhece ~14 mil itens que o rAthena atual tem (episodios novos). O cliente usa esta tabela so para os IDs que o item_info.json (LATAM,
em portugues) nao tem: nome, descricao (cores ^RRGGBB), slots, ClassNum e o nome do recurso (icone/sprite em coreano).

O .lua e lido como TEXTO (nada e executado). Strings entre aspas "..." (cp949 nos nomes de recurso).

Uso (WSL): python3 tools/gen_item_info_2026.py "/mnt/d/Ragnarok_Assets/Data2026_/Data2026/System/LuaFiles514/itemInfo.lua"
"""
import json
import os
import re
import sys

ITEM_RE = re.compile(r'\n\t\[(\d+)\]\s*=\s*\{(.*?)\n\t\},?', re.S)
STR_RE = r'\b%s\s*=\s*"((?:[^"\\]|\\.)*)"'
LIST_RE = r'\b%s\s*=\s*\{(.*?)\n\t\t\}'


def dec(raw, korean=False):
    data = raw.encode('latin-1')
    for enc in (('cp949', 'utf-8') if korean else ('utf-8', 'cp1252', 'cp949')):
        try:
            return data.decode(enc)
        except UnicodeDecodeError:
            continue
    return data.decode('latin-1')


def field(body, name, korean=False):
    m = re.search(STR_RE % name, body)
    return dec(m.group(1).replace('\\"', '"'), korean) if m else ''


def lines(body, name):
    m = re.search(LIST_RE % name, body, re.S)
    if not m:
        return []
    out = [dec(s.replace('\\"', '"')) for s in re.findall(r'"((?:[^"\\]|\\.)*)"', m.group(1))]
    return [s for s in out if s != ''] if out != [''] else []


def number(body, name):
    m = re.search(r'\b%s\s*=\s*(-?\d+)' % name, body)
    return int(m.group(1)) if m else 0


def main():
    src = sys.argv[1]
    text = open(src, 'rb').read().decode('latin-1')
    out = {}
    for m in ITEM_RE.finditer(text):
        body = m.group(2)
        name = field(body, 'identifiedDisplayName')
        res = field(body, 'identifiedResourceName', korean=True)
        if not name and not res:
            continue
        out[m.group(1)] = {
            'name': name,
            'desc': lines(body, 'identifiedDescriptionName'),
            'resource': res,
            'unidentified_name': field(body, 'unidentifiedDisplayName'),
            'unidentified_desc': lines(body, 'unidentifiedDescriptionName'),
            'slots': number(body, 'slotCount'),
            'view': number(body, 'ClassNum'),
        }
    dest = os.path.join(os.path.dirname(os.path.abspath(__file__)), '..', 'Config', 'item_info_2026.json')
    with open(dest, 'w', encoding='utf-8') as f:
        json.dump(out, f, ensure_ascii=False, separators=(',', ':'), sort_keys=True)
    print('itens: %d -> %s (%.1f MB)' % (len(out), os.path.normpath(dest), os.path.getsize(dest) / 1e6))


if __name__ == '__main__':
    main()

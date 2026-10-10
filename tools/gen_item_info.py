"""
Gera Config/item_info.json a partir do System/itemInfo.lua do cliente original (bRO, em portugues):
nome, descricao (linhas com as cores ^RRGGBB do cliente), slots, ClassNum (visual) e o nome do recurso
(sprite/ilustracao em coreano) de cada item, identificado e nao identificado.

O .lua e lido como TEXTO (nada e executado). Os textos em portugues estao em cp1252 e os nomes de recurso
em cp949 (coreano), como no cliente.

Uso: python3 tools/gen_item_info.py <caminho do itemInfo.lua>
"""
import json
import os
import re
import sys

ITEM_RE = re.compile(r'\[(\d+)\]\s*=\s*\{(.*?)\n\t\t\},', re.S)
# \b: "identifiedDisplayName" nao pode casar dentro de "unidentifiedDisplayName"
STR_RE = r'\b%s\s*=\s*\[=\[(.*?)\]=\]'
LIST_RE = r'\b%s\s*=\s*\{(.*?)\}'


def decode(raw, korean=False):
    data = raw.encode('latin-1')
    for enc in (('cp949', 'cp1252') if korean else ('cp1252', 'cp949')):
        try:
            return data.decode(enc)
        except UnicodeDecodeError:
            continue
    return data.decode('latin-1')


def field(body, name, korean=False):
    m = re.search(STR_RE % name, body, re.S)
    return decode(m.group(1), korean) if m else ''


def lines(body, name):
    m = re.search(LIST_RE % name, body, re.S)
    if not m:
        return []
    return [decode(s) for s in re.findall(r'\[=\[(.*?)\]=\]', m.group(1), re.S)]


def number(body, name):
    m = re.search(r'%s\s*=\s*(-?\d+)' % name, body)
    return int(m.group(1)) if m else 0


def main():
    src = sys.argv[1]
    text = open(src, 'rb').read().decode('latin-1')  # bytes intactos; cada campo e decodificado depois
    out = {}
    for m in ITEM_RE.finditer(text):
        body = m.group(2)
        out[m.group(1)] = {
            'name': field(body, 'identifiedDisplayName'),
            'desc': lines(body, 'identifiedDescriptionName'),
            'resource': field(body, 'identifiedResourceName', korean=True),
            'unidentified_name': field(body, 'unidentifiedDisplayName'),
            'unidentified_desc': lines(body, 'unidentifiedDescriptionName'),
            'slots': number(body, 'slotCount'),
            'view': number(body, 'ClassNum'),
        }
    dest = os.path.join(os.path.dirname(os.path.abspath(__file__)), '..', 'Config', 'item_info.json')
    with open(dest, 'w', encoding='utf-8') as f:
        json.dump(out, f, ensure_ascii=False, indent=0, sort_keys=True)
    print('itens: %d -> %s' % (len(out), os.path.normpath(dest)))


if __name__ == '__main__':
    main()

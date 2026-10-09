"""
Gera Config/status_icons.json: id do estado (EFST_*, o "type" dos pacotes de status) -> icone e nome do buff.

Fonte: tabela do roBrowser (src/DB/Status/StatusInfo.js + StatusConst.js), lida como DADOS (nada e executado);
no cliente original isso fica em stateiconimginfo.lub / efstids. Os icones sao data/texture/effect/<icone>.tga.

Uso: python3 tools/gen_status_icons.py <pasta_com_os_js> <raiz_dos_assets>
"""
import json
import os
import re
import sys


def unescape(s):
    raw = re.sub(r'\\x([0-9a-fA-F]{2})', lambda m: chr(int(m.group(1), 16)), s)
    try:
        return raw.encode('latin-1').decode('cp949')
    except UnicodeError:
        return raw


def main():
    js, root = sys.argv[1], sys.argv[2]
    consts = dict((n, int(v)) for n, v in re.findall(r'(\w+)\s*:\s*(-?\d+)', open(os.path.join(js, 'StatusConst.js'), encoding='latin-1').read()))
    txt = open(os.path.join(js, 'StatusInfo.js'), encoding='latin-1').read()
    effect_dir = os.path.join(root, 'data', 'texture', 'effect')
    on_disk = {n.lower(): n for n in os.listdir(effect_dir)}
    out, missing = {}, 0
    for m in re.finditer(r'StatusInfo\[SC\.(\w+)\]\s*=\s*\{(.*?)\n\t\};', txt, re.S):
        key, body = m.group(1), m.group(2)
        icon = re.search(r'icon:\s*"([^"]*)"', body)
        if key not in consts or not icon:
            continue
        name = unescape(icon.group(1))
        real = on_disk.get(name.lower())
        if not real:
            missing += 1
            continue
        title = re.search(r'descript:\s*\[\s*\["([^"]*)",\s*(COLOR_\w+)', body)
        out[str(consts[key])] = {
            'icon': real,
            'name': title.group(1) if title else key.title().replace('_', ' '),
            'debuff': bool(title and title.group(2) == 'COLOR_TITLE_DEBUFF'),
        }
    dest = os.path.join(os.path.dirname(os.path.abspath(__file__)), '..', 'Config', 'status_icons.json')
    with open(dest, 'w', encoding='utf-8') as f:
        json.dump(out, f, ensure_ascii=False, indent=0, sort_keys=True)
    print('estados com icone: %d (sem arquivo: %d) -> %s' % (len(out), missing, os.path.normpath(dest)))


if __name__ == '__main__':
    main()

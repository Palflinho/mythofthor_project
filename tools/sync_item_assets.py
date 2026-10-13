"""
Copia para a pasta de assets que o jogo usa (E:\\Dev\\Ragnarok_Assets\\grf) os arquivos do cliente 2026 que o LATAM nao tem e que os
itens do rAthena precisam: icone (texture/유저인터페이스/item/<recurso>.bmp), sprite no chao (sprite/아이템/<recurso>.spr/.act),
ilustracao (collection/<recurso>.bmp, cardbmp/<recurso>.bmp). Nunca sobrescreve um arquivo que ja existe.

Uso (WSL): python3 tools/sync_item_assets.py [origem 2026=/mnt/d/Ragnarok_Assets/grf2026/data] [destino=/mnt/e/Dev/Ragnarok_Assets/grf/data]
"""
import json
import os
import shutil
import sys

ROOT = os.path.join(os.path.dirname(os.path.abspath(__file__)), '..', 'Config')
UI = 'texture/유저인터페이스/'


def main():
    src = sys.argv[1] if len(sys.argv) > 1 else '/mnt/d/Ragnarok_Assets/grf2026/data'
    dst = sys.argv[2] if len(sys.argv) > 2 else '/mnt/e/Dev/Ragnarok_Assets/grf/data'
    info = json.load(open(os.path.join(ROOT, 'item_info.json'), encoding='utf-8'))
    n26 = json.load(open(os.path.join(ROOT, 'item_info_2026.json'), encoding='utf-8'))
    cat = json.load(open(os.path.join(ROOT, 'item_catalog.json'), encoding='utf-8'))
    resources = set()
    for e in cat:
        it = info.get(str(e['i'])) or n26.get(str(e['i']))
        if it and it.get('resource'):
            resources.add(it['resource'])
    dirs = [UI + 'item', 'sprite/아이템', UI + 'collection', UI + 'cardbmp']
    have_src = {d: {f.lower(): f for f in os.listdir(os.path.join(src, d))} if os.path.isdir(os.path.join(src, d)) else {} for d in dirs}
    have_dst = {d: {f.lower() for f in os.listdir(os.path.join(dst, d))} if os.path.isdir(os.path.join(dst, d)) else set() for d in dirs}
    wanted = []
    for r in resources:  # o cliente usa nomes sem diferenciar maiusculas (os arquivos extraidos estao em minusculas)
        wanted += [(dirs[0], r + '.bmp'), (dirs[1], r + '.spr'), (dirs[1], r + '.act'), (dirs[2], r + '.bmp'), (dirs[3], r + '.bmp')]
    copied = missing = 0
    for d, name in wanted:
        key = name.lower()
        if key in have_dst[d]:
            continue
        real = have_src[d].get(key)
        if not real:
            missing += 1
            continue
        os.makedirs(os.path.join(dst, d), exist_ok=True)
        shutil.copyfile(os.path.join(src, d, real), os.path.join(dst, d, key))
        have_dst[d].add(key)
        copied += 1
    print('recursos', len(resources), '| copiados', copied, '| ainda faltando em ambos', missing)


if __name__ == '__main__':
    main()

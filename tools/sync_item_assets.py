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
    wanted = []
    for r in resources:
        wanted += [UI + 'item/' + r + '.bmp', 'sprite/아이템/' + r + '.spr', 'sprite/아이템/' + r + '.act',
                   UI + 'collection/' + r + '.bmp', UI + 'cardbmp/' + r + '.bmp']
    copied = missing = 0
    for rel in wanted:
        s, d = os.path.join(src, rel), os.path.join(dst, rel)
        if os.path.exists(d) or not os.path.exists(s):
            if not os.path.exists(d):
                missing += 1
            continue
        os.makedirs(os.path.dirname(d), exist_ok=True)
        shutil.copyfile(s, d)
        copied += 1
    print('recursos', len(resources), '| copiados', copied, '| ainda faltando em ambos', missing)


if __name__ == '__main__':
    main()

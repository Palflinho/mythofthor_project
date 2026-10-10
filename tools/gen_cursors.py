#!/usr/bin/env python3
"""
Gera os cursores do cliente (data/sprite/cursors.spr/.act) como PNG para o cursor de hardware da Unreal.

Cada acao do .act e um cursor (0 seta, 1 falar, 2 clique, 3 trava, 4 girar camera, 5 atacar, 7 portal,
8 proibido, 9 pegar item, 10 mira de skill: mesma tabela do roBrowser Cursor.js). Usa o 1o quadro de cada
acao, o de maior area; o ponto de origem do quadro e o ponto quente (hotspot) do cursor.

Saida: Content/Myth/Cursors/cursor_<acao>.png e Config/cursors.json ({acao: {png, hotspot: [x, y], size}}).

Uso: python3 tools/gen_cursors.py <pasta com data/sprite/cursors.spr e .act> [--scale 1]
"""
import json
import os
import sys

from PIL import Image

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
from extract_act_manifest import parse_act  # noqa: E402
from extract_and_convert_sprites import decode_spr  # noqa: E402

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))


def main():
    src = sys.argv[1]
    scale = int(sys.argv[sys.argv.index('--scale') + 1]) if '--scale' in sys.argv else 1
    spr_path = os.path.join(src, 'data', 'sprite', 'cursors.spr')
    act_path = os.path.join(src, 'data', 'sprite', 'cursors.act')
    frames, meta = decode_spr(spr_path)
    n_indexed = meta['indexed_count']
    _, actions, _ = parse_act(act_path)

    out_dir = os.path.join(ROOT, 'Content', 'Myth', 'Cursors')
    os.makedirs(out_dir, exist_ok=True)
    table = {}
    for a, motions in enumerate(actions):
        if not motions:
            continue
        # Cursores animados (falar, portal, mira) comecam pequenos: usa o quadro de maior area
        def area(m):
            total = 0
            for c in m:
                if c['sprite'] >= 0:
                    img = frames[c['sprite'] + (n_indexed if c['rgba'] else 0)]
                    total += img.width * img.height
            return total
        best = max(motions, key=area)
        clips = [c for c in best if c['sprite'] >= 0]
        if not clips:
            continue
        # Cada camada e centrada em (x, y) relativo a origem do quadro
        boxes = []
        for c in clips:
            img = frames[c['sprite'] + (n_indexed if c['rgba'] else 0)]
            if c['flip']:
                img = img.transpose(Image.FLIP_LEFT_RIGHT)
            left = c['x'] - img.width // 2
            top = c['y'] - img.height // 2
            boxes.append((img, left, top))
        min_x = min(0, min(l for _, l, _ in boxes))
        min_y = min(0, min(t for _, _, t in boxes))
        max_x = max(l + i.width for i, l, _ in boxes)
        max_y = max(t + i.height for i, _, t in boxes)
        canvas = Image.new('RGBA', (max_x - min_x, max_y - min_y), (0, 0, 0, 0))
        for img, l, t in boxes:
            canvas.alpha_composite(img, (l - min_x, t - min_y))
        hot = (-min_x, -min_y)
        if scale != 1:
            canvas = canvas.resize((canvas.width * scale, canvas.height * scale), Image.NEAREST)
            hot = (hot[0] * scale, hot[1] * scale)
        name = 'cursor_%d' % a
        canvas.save(os.path.join(out_dir, name + '.png'))
        table[str(a)] = {'png': 'Myth/Cursors/' + name, 'hotspot': list(hot), 'size': [canvas.width, canvas.height],
                         'frames': len(motions)}
        print('acao %2d: %dx%d hotspot %s (%d quadros)' % (a, canvas.width, canvas.height, hot, len(motions)))

    with open(os.path.join(ROOT, 'Config', 'cursors.json'), 'w', encoding='utf-8') as f:
        json.dump(table, f, indent=1)
    print('%d cursores em %s' % (len(table), out_dir))


if __name__ == '__main__':
    main()

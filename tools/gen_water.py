"""
Gera Config/water/<mapa>.json com a agua de cada mapa (RSW + GND), no mesmo criterio do roBrowser (Loaders/Ground.js):
o plano de agua fica na altura do RSW e so aparece nos blocos do chao (tiles do GND) onde algum canto esta abaixo da
agua (altura do chao > nivel - altura da onda).

Campos: level, type, wh (altura da onda), ws, wp, anim (quadros por troca), w/h (tiles do GND), scale e "rows":
{y: [x0, comprimento, x0, comprimento, ...]} (trechos consecutivos de tiles com agua por linha).
Altura da agua na Unreal = -level * 10 (mesma escala do .obj: ver tools/ue_import_map.py, escala 10).

Uso (WSL): python3 tools/gen_water.py [pasta com os mapas extraidos, padrao /mnt/e/Dev/Ragnarok_Assets]
"""
import json
import os
import struct
import sys

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
from compile_map_to_obj import read_gnd  # noqa: E402


def read_water(rsw_path):
    with open(rsw_path, 'rb') as f:
        if f.read(4) != b'GRSW':
            return None
        major, minor = struct.unpack('2B', f.read(2))
        ver = major + minor / 10.0
        f.read(40)
        f.read(40)
        if ver >= 1.4:
            f.read(40)
        f.read(40)
        water = {'level': 0.0, 'type': 0, 'wh': 1.0, 'ws': 2.0, 'wp': 50.0, 'anim': 3}
        if ver >= 1.3:
            water['level'] = struct.unpack('f', f.read(4))[0]
        if ver >= 1.8:
            water['type'] = struct.unpack('i', f.read(4))[0]
            water['wh'], water['ws'], water['wp'] = struct.unpack('3f', f.read(12))
        if ver >= 1.9:
            water['anim'] = struct.unpack('i', f.read(4))[0]
        return water


def main():
    root = sys.argv[1] if len(sys.argv) > 1 else '/mnt/e/Dev/Ragnarok_Assets'
    dest = os.path.join(os.path.dirname(os.path.abspath(__file__)), '..', 'Config', 'water')
    os.makedirs(dest, exist_ok=True)
    seen = set()
    made = 0
    for dp, _, files in os.walk(root):
        for fn in files:
            if not fn.lower().endswith('.rsw'):
                continue
            name = fn[:-4].lower()
            if name in seen:
                continue
            gnd = os.path.join(dp, name + '.gnd')
            if not os.path.exists(gnd):
                continue
            seen.add(name)
            try:
                water = read_water(os.path.join(dp, fn))
                g = read_gnd(gnd)
            except Exception as ex:  # noqa: BLE001
                print('pulou %s: %s' % (name, ex))
                continue
            if not water:
                continue
            w, h = g['width'], g['height']
            limit = water['level'] - water['wh']
            rows = {}
            for y in range(h):
                runs = []
                x = 0
                while x < w:
                    hs = g['tiles'][y * w + x]['heights']
                    if max(hs) > limit:
                        x0 = x
                        while x < w and max(g['tiles'][y * w + x]['heights']) > limit:
                            x += 1
                        runs += [x0, x - x0]
                    else:
                        x += 1
                if runs:
                    rows[str(y)] = runs
            if not rows:
                continue
            out = dict(water)
            out.update({'w': w, 'h': h, 'scale': g['scale'], 'rows': rows})
            with open(os.path.join(dest, name + '.json'), 'w', encoding='utf-8') as f:
                json.dump(out, f, separators=(',', ':'))
            tiles = sum(runs[i + 1] for runs in rows.values() for i in range(0, len(runs), 2))
            print('%s: nivel %.1f tipo %d, %d tiles com agua' % (name, water['level'], water['type'], tiles))
            made += 1
    print('mapas com agua: %d -> %s' % (made, os.path.normpath(dest)))


if __name__ == '__main__':
    main()

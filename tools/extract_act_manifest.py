#!/usr/bin/env python3
"""
Myth of Thor - ACT -> JSON manifest
-----------------------------------
Le o .act (animacoes) do Ragnarok e gera um JSON que diz, para cada acao,
quais frames da spritesheet (gerada por extract_and_convert_sprites.py) tocar e por quanto tempo.

Convencao do RO: action_index = grupo * 8 + direcao (0=Sul .. 7=Sudeste, igual ao ERagnarokDirection).
  Jogador : 0 idle, 1 walk, 2 sit, 3 pickup, 4 standby, 5 attack1, 6 hit, 7 die, ...
  Monstro : 0 idle, 1 walk, 2 attack, 3 hit, 4 die

Uso: python extract_act_manifest.py <arquivo.act> <saida.json>   (le o .spr ao lado para a contagem de frames)
"""
import json
import os
import struct
import sys


def parse_act(path):
    with open(path, 'rb') as f:
        d = f.read()
    if d[:2] != b'AC':
        raise ValueError('Arquivo ACT invalido')
    minor, major = d[2], d[3]
    ver = major * 100 + minor  # 205 = v2.5
    n_actions = struct.unpack_from('<H', d, 4)[0]
    p = 16  # 2 magic + 2 versao + 2 contagem + 10 reservados

    actions = []
    for _ in range(n_actions):
        n_motions = struct.unpack_from('<I', d, p)[0]; p += 4
        motions = []
        for _ in range(n_motions):
            p += 32  # range1/range2 (nao usados)
            n_clips = struct.unpack_from('<I', d, p)[0]; p += 4
            clips = []
            for _ in range(n_clips):
                x, y, spr, flip = struct.unpack_from('<iiii', d, p); p += 16
                sprite_type = 0
                if ver >= 200:
                    p += 4  # cor RGBA
                    p += 4  # xscale
                    if ver >= 204:
                        p += 4  # yscale
                    p += 4  # rotacao
                    sprite_type = struct.unpack_from('<i', d, p)[0]; p += 4
                    if ver >= 205:
                        p += 8  # largura/altura
                clips.append({'x': x, 'y': y, 'sprite': spr, 'flip': bool(flip), 'rgba': sprite_type == 1})
            if ver >= 200:
                p += 4  # event id
            if ver >= 203:
                n_attach = struct.unpack_from('<i', d, p)[0]; p += 4
                p += 16 * n_attach
            motions.append(clips)
        actions.append(motions)

    intervals = [1.0] * n_actions
    if ver >= 201:
        n_events = struct.unpack_from('<I', d, p)[0]; p += 4
        p += 40 * n_events
    if ver >= 202:
        intervals = list(struct.unpack_from('<%df' % n_actions, d, p))
    return ver, actions, intervals


def main():
    if len(sys.argv) < 3:
        print(__doc__)
        sys.exit(1)
    act_path, out_path = sys.argv[1], sys.argv[2]
    # Le a contagem de frames do .spr irmao (mesmo nome): indexados + RGBA, na ordem da spritesheet.
    indexed, rgba = 0, 0
    spr_path = act_path[:-4] + '.spr'
    if os.path.exists(spr_path):
        with open(spr_path, 'rb') as f:
            hdr = f.read(8)
        indexed, rgba = struct.unpack_from('<HH', hdr, 4)

    ver, actions, intervals = parse_act(act_path)
    out = {'source': act_path.replace(chr(92), '/').split('/')[-1], 'version': '%d.%d' % (ver // 100, ver % 100), 'sprite_count': indexed + rgba,
           'grid_columns': min(8, indexed + rgba), 'actions': []}
    for i, motions in enumerate(actions):
        frames = []
        for clips in motions:
            # Frame "principal": primeiro clip valido (corpo). Clips extras (sombra/cabeca) ficam em 'layers'.
            valid = [c for c in clips if c['sprite'] >= 0]
            if not valid:
                frames.append({'sprite': -1, 'layers': []})
                continue
            main_clip = valid[0]
            idx = main_clip['sprite'] + (indexed if main_clip['rgba'] else 0)
            frames.append({'sprite': idx, 'x': main_clip['x'], 'y': main_clip['y'], 'flip': main_clip['flip'], 'layers': len(valid) - 1})
        out['actions'].append({
            'index': i, 'group': i // 8, 'direction': i % 8,
            'interval_ms': round(intervals[i] * 25.0, 2),
            'frames': frames,
        })
    with open(out_path, 'w', encoding='utf-8') as f:
        json.dump(out, f, ensure_ascii=False, indent=1)
    print('OK: %d acoes -> %s' % (len(actions), out_path))


if __name__ == '__main__':
    main()

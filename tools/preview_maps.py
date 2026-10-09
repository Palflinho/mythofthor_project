#!/usr/bin/env python3
"""
Myth of Thor - Map OBJ & GAT Preview Inspector
----------------------------------------------
Inspeciona a geometria dos mapas compilados (.OBJ, .MTL, GAT e luzes)
calculando bounds 3D, contagem de vértices, faces, malhas de colisão,
e gerando visualização top-down da área caminhável.
"""

import os
import sys
import json
import struct

PHASE1_MAPS = [
    'new_1-1', 'new_1-2', 'new_1-3', 'new_1-4',
    'prt_fild00', 'prt_fild01', 'prt_fild02', 'prt_fild03',
    'prt_fild04', 'prt_fild05', 'prt_fild06', 'prt_fild07',
    'prt_fild08', 'prt_fild09', 'prt_fild10', 'prt_fild11',
    'prt_sewb1', 'prt_sewb2', 'prt_sewb3', 'prt_sewb4',
    'izlude'
]

def inspect_obj(obj_path: str):
    v_count = 0
    vt_count = 0
    f_count = 0
    min_x, max_x = float('inf'), float('-inf')
    min_y, max_y = float('inf'), float('-inf')
    min_z, max_z = float('inf'), float('-inf')

    with open(obj_path, 'r', encoding='utf-8', errors='ignore') as f:
        for line in f:
            if line.startswith('v '):
                parts = line.strip().split()
                if len(parts) >= 4:
                    x, y, z = float(parts[1]), float(parts[2]), float(parts[3])
                    v_count += 1
                    if x < min_x: min_x = x
                    if x > max_x: max_x = x
                    if y < min_y: min_y = y
                    if y > max_y: max_y = y
                    if z < min_z: min_z = z
                    if z > max_z: max_z = z
            elif line.startswith('vt '):
                vt_count += 1
            elif line.startswith('f '):
                f_count += 1

    return {
        'v_count': v_count,
        'vt_count': vt_count,
        'f_count': f_count,
        'bounds': {
            'min': (round(min_x, 1), round(min_y, 1), round(min_z, 1)),
            'max': (round(max_x, 1), round(max_y, 1), round(max_z, 1)),
            'span': (round(max_x - min_x, 1), round(max_y - min_y, 1), round(max_z - min_z, 1))
        }
    }

def count_faces(obj_path: str) -> int:
    if not os.path.exists(obj_path):
        return 0
    count = 0
    with open(obj_path, 'r', encoding='utf-8', errors='ignore') as f:
        for line in f:
            if line.startswith('f '):
                count += 1
    return count

def generate_gat_ascii_preview(gat_path: str, cols: int = 50, rows: int = 25) -> str:
    if not os.path.exists(gat_path):
        return "[GAT ausente]"
    with open(gat_path, 'rb') as f:
        data = f.read()
    if len(data) < 14:
        return "[GAT inválido]"
    w, h = struct.unpack_from('<II', data, 6)
    cells = [struct.unpack_from('<4fI', data, 14 + i * 20)[4] for i in range(w * h)]

    # Constrói grid em ASCII
    out_lines = []
    for r in range(rows - 1, -1, -1):
        line = []
        gy = int(r * h / rows)
        for c in range(cols):
            gx = int(c * w / cols)
            idx = gy * w + gx
            ctype = cells[idx] if idx < len(cells) else 1
            # 0 = caminhável, 1 = parede, 5 = vão
            if ctype in (1, 5):
                line.append('#')
            else:
                line.append('.')
        out_lines.append(''.join(line))
    return '\n'.join(out_lines)

def main():
    base_dir = sys.argv[1] if len(sys.argv) > 1 else r'E:\Dev\Ragnarok_Assets\grf\data'
    print("=" * 78)
    print(f"MYTH OF THOR — RELATÓRIO DE PRÉVIA DOS 21 MAPAS DA FASE 1")
    print(f"Diretório: {base_dir}")
    print("=" * 78)

    summary = []
    for m in PHASE1_MAPS:
        obj_file = os.path.join(base_dir, f"{m}_full.obj")
        gat_floor = os.path.join(base_dir, f"{m}_full_gat_floor.obj")
        gat_walls = os.path.join(base_dir, f"{m}_full_gat_walls.obj")
        lights_file = os.path.join(base_dir, f"{m}_full_lights.json")
        gat_raw = os.path.join(base_dir, f"{m}.gat")

        if not os.path.exists(obj_file):
            print(f"[-] {m}: NÃO ENCONTRADO!")
            continue

        sz_mb = os.path.getsize(obj_file) / (1024 * 1024)
        info = inspect_obj(obj_file)
        f_floor = count_faces(gat_floor)
        f_walls = count_faces(gat_walls)

        num_lights = 0
        if os.path.exists(lights_file):
            with open(lights_file, 'r', encoding='utf-8') as lf:
                lj = json.load(lf)
                num_lights = len(lj.get('lights', []))

        print(f"\n[MAPA: {m}]")
        print(f"  Tamanho OBJ : {sz_mb:.2f} MB")
        print(f"  Vértices    : {info['v_count']:,} | Coords UV: {info['vt_count']:,} | Faces: {info['f_count']:,}")
        print(f"  Extensão 3D : X: {info['bounds']['span'][0]} | Y: {info['bounds']['span'][1]} | Z: {info['bounds']['span'][2]}")
        print(f"  Colisão GAT : {f_floor:,} faces piso | {f_walls:,} faces parede")
        print(f"  Luzes Noite : {num_lights} postes registrados")

        ascii_art = generate_gat_ascii_preview(gat_raw, cols=40, rows=12)
        print("  Prévia Top-Down (GAT . = livre, # = parede):")
        for line in ascii_art.splitlines():
            print(f"    {line}")

        summary.append({
            'map': m,
            'size_mb': round(sz_mb, 2),
            'vertices': info['v_count'],
            'faces': info['f_count'],
            'gat_floor': f_floor,
            'gat_walls': f_walls,
            'lights': num_lights
        })

    print("\n" + "=" * 78)
    print("TABELA RESUMO GERAL:")
    print(f"{'Mapa':<12} | {'Tamanho':<8} | {'Vértices':<10} | {'Faces':<10} | {'GAT Piso':<10} | {'GAT Parede':<10} | {'Luzes':<6}")
    print("-" * 78)
    for s in summary:
        print(f"{s['map']:<12} | {s['size_mb']:>5.2f} MB | {s['vertices']:>10,} | {s['faces']:>10,} | {s['gat_floor']:>10,} | {s['gat_walls']:>10,} | {s['lights']:>6}")
    print("=" * 78)

if __name__ == '__main__':
    main()

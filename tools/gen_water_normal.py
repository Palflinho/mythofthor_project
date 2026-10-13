"""
Gera tools/data/water_normal.png: normal map de agua (256x256, repete sem emenda) feito de ondas senoidais cruzadas; usado
pela agua moderna (M_WaterModern, tools/ue_make_water_material.py). Sem dependencias externas.

Uso: python3 tools/gen_water_normal.py
"""
import math
import os
import random
import struct
import zlib

N = 256


def main():
    random.seed(7)
    waves = []
    for _ in range(14):
        fx = random.choice([1, 2, 3, 4, 5, 6, 8])
        fy = random.choice([1, 2, 3, 4, 5, 6, 8])
        waves.append((fx, fy, random.uniform(0, 2 * math.pi), 1.0 / (fx * fx + fy * fy) ** 0.5))
    height = [[0.0] * N for _ in range(N)]
    for y in range(N):
        for x in range(N):
            v = 0.0
            for fx, fy, ph, amp in waves:
                v += amp * math.sin(2 * math.pi * (fx * x + fy * y) / N + ph)
            height[y][x] = v
    strength = 4.0
    rows = []
    for y in range(N):
        row = bytearray([0])  # filtro PNG "none"
        for x in range(N):
            dx = (height[y][(x + 1) % N] - height[y][(x - 1) % N]) * strength
            dy = (height[(y + 1) % N][x] - height[(y - 1) % N][x]) * strength
            nx, ny, nz = -dx, -dy, 1.0
            ln = math.sqrt(nx * nx + ny * ny + nz * nz)
            row += bytes([int((nx / ln * 0.5 + 0.5) * 255), int((ny / ln * 0.5 + 0.5) * 255), int((nz / ln * 0.5 + 0.5) * 255)])
        rows.append(bytes(row))
    raw = b''.join(rows)

    def chunk(tag, data):
        c = struct.pack('>I', len(data)) + tag + data
        return c + struct.pack('>I', zlib.crc32(tag + data) & 0xffffffff)

    png = b'\x89PNG\r\n\x1a\n' + chunk(b'IHDR', struct.pack('>IIBBBBB', N, N, 8, 2, 0, 0, 0)) + chunk(b'IDAT', zlib.compress(raw, 9)) + chunk(b'IEND', b'')
    dest = os.path.join(os.path.dirname(os.path.abspath(__file__)), 'data')
    os.makedirs(dest, exist_ok=True)
    path = os.path.join(dest, 'water_normal.png')
    with open(path, 'wb') as f:
        f.write(png)
    print('gerado', os.path.normpath(path), len(png), 'bytes')


if __name__ == '__main__':
    main()

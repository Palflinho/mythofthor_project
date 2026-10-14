"""
Replaces the LATAM Portuguese interface art (texture/유저인터페이스) with the English versions of the Data2026 client, only for the
files that exist in both with the SAME pixel size (those are the translated button/label textures; a different size would break the
layouts built around the LATAM art). Every replaced file keeps its old copy in <dest>/../_backup_latam/ (python3 ... --restore undoes).

Usage (WSL): python3 tools/apply_data2026_ui.py [--restore] [--list-other-sizes]
"""
import os
import shutil
import struct
import sys

SRC = '/mnt/d/Ragnarok_Assets/grf2026/data/texture/유저인터페이스'
DST = '/mnt/e/Dev/Ragnarok_Assets/grf/data/texture/유저인터페이스'
BK = '/mnt/e/Dev/Ragnarok_Assets/grf/_backup_latam/texture/유저인터페이스'


def dims(p):
    try:
        with open(p, 'rb') as f:
            h = f.read(32)
        if h[:2] == b'BM':
            w, hh = struct.unpack('<ii', h[18:26])
            return (w, abs(hh))
        if h[:8] == b'\x89PNG\r\n\x1a\n':
            return struct.unpack('>II', h[16:24])
    except Exception:
        pass
    return None


def main():
    if '--restore' in sys.argv:
        n = 0
        for dp, _, fs in os.walk(BK):
            for f in fs:
                rel = os.path.relpath(os.path.join(dp, f), BK)
                shutil.copyfile(os.path.join(dp, f), os.path.join(DST, rel))
                n += 1
        print('restored', n)
        return
    low = {}
    for dp, _, fs in os.walk(DST):
        for f in fs:
            rel = os.path.relpath(os.path.join(dp, f), DST)
            low[rel.replace('\\', '/').lower()] = rel
    n = 0
    other = []
    for dp, _, fs in os.walk(SRC):
        for f in fs:
            rel = os.path.relpath(os.path.join(dp, f), SRC)
            existing = low.get(rel.replace('\\', '/').lower())
            if not existing:
                continue
            s, d = os.path.join(SRC, rel), os.path.join(DST, existing)
            if os.path.getsize(s) == os.path.getsize(d):
                continue
            ds, dd = dims(s), dims(d)
            if ds is None or dd is None or ds != dd:
                other.append(rel)
                continue
            b = os.path.join(BK, existing)
            if not os.path.exists(b):
                os.makedirs(os.path.dirname(b), exist_ok=True)
                shutil.copyfile(d, b)
            shutil.copyfile(s, d)
            n += 1
    print('replaced %d interface textures (same size); %d others left alone' % (n, len(other)))
    if '--list-other-sizes' in sys.argv:
        for r in other:
            print(' ', r)


if __name__ == '__main__':
    main()

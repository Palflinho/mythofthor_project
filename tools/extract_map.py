"""
Extrai um mapa do data.grf com tudo o que ele usa, numa pasta autocontida para o compile_map_to_obj.py:
  <saida>/<mapa>.rsw/.gnd/.gat
  <saida>/model/...    (os .rsm citados no .rsw)
  <saida>/texture/...  (texturas do .gnd e de cada .rsm)

Uso (WSL):
  python3 tools/extract_map.py prt_in /mnt/e/Dev/Ragnarok_Assets/Prontera/prt_in [--grf /mnt/c/Gravity/Ragnarok/data.grf]
Depois:
  python3 tools/compile_map_to_obj.py /mnt/e/Dev/Ragnarok_Assets/Prontera/prt_in
"""
import os
import sys

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
from grf_extract import GrfArchive  # noqa: E402
import compile_map_to_obj as cm  # noqa: E402


def save(grf, internal, out_dir, rel):
    """Extrai data\\<internal> para <out_dir>/<rel>. Devolve o caminho local (ou None se nao existir no GRF)."""
    entry = grf.get_entry('data\\' + internal)
    if not entry:
        return None
    dest = os.path.join(out_dir, *rel.replace('\\', '/').split('/'))
    if not os.path.exists(dest):
        os.makedirs(os.path.dirname(dest), exist_ok=True)
        with open(dest, 'wb') as f:
            f.write(grf.extract_entry(entry))
    return dest


def main():
    if len(sys.argv) < 3:
        print(__doc__)
        sys.exit(1)
    name, out_dir = sys.argv[1], sys.argv[2]
    grf_path = sys.argv[sys.argv.index('--grf') + 1] if '--grf' in sys.argv else '/mnt/c/Gravity/Ragnarok/data.grf'
    grf = GrfArchive(grf_path)
    os.makedirs(out_dir, exist_ok=True)

    paths = {}
    for ext in ('rsw', 'gnd', 'gat'):
        paths[ext] = save(grf, '%s.%s' % (name, ext), out_dir, '%s.%s' % (name, ext))
        if not paths[ext]:
            raise SystemExit('%s.%s nao existe no GRF' % (name, ext))

    textures = set(cm.read_gnd(paths['gnd'])['textures'])
    models = sorted(set(m['filename'] for m in cm.read_rsw(paths['rsw'])['models']))
    missing = []
    for model in models:
        local = save(grf, 'model\\' + model, out_dir, 'model/' + model)
        if not local:
            missing.append('model/' + model)
            continue
        try:
            textures.update(cm.read_rsm(local)['textures'])
        except Exception as e:  # noqa: BLE001
            print('  aviso: %s (%s)' % (model, e))
    for tex in sorted(textures):
        if tex and not save(grf, 'texture\\' + tex, out_dir, 'texture/' + tex):
            missing.append('texture/' + tex)
    print('%s: %d modelos, %d texturas, %d faltando no GRF' % (name, len(models), len(textures), len(missing)))
    for m in missing[:15]:
        print('  faltando:', m)


if __name__ == '__main__':
    main()

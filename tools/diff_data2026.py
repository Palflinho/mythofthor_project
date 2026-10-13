"""
Compara o indice do LATAM (C:\\Gravity\\Ragnarok\\data.grf) com o do cliente 2026 (data.grf + 2026.grf, esta por cima) e gera
docs/ref/DATA2026_ATUALIZADOS.md: os arquivos que existem nos dois mas com TAMANHO diferente (o 2026 mudou), agrupados por pasta e tipo,
para decidir o que vale trazer. Nada e extraido; so leitura dos indices.

Uso (WSL): python3 tools/diff_data2026.py
"""
import os
import sys

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
from grf_extract import GrfArchive  # noqa: E402

LATAM = '/mnt/c/Gravity/Ragnarok/data.grf'
D26 = '/mnt/d/Ragnarok_Assets/Data2026_/Data2026/data.grf'
D26B = '/mnt/d/Ragnarok_Assets/Data2026_/Data2026/2026.grf'

# Pastas que o jogo usa (importam); o resto so entra na contagem
RELEVANT = {
    'sprite': 'sprites (corpos, monstros, NPCs, cabelos, acessorios, armas)',
    'texture': 'texturas (UI, efeitos, mapas)',
    'wav': 'sons',
    'model': 'modelos 3D de mapa',
}


def main():
    a = GrfArchive(LATAM)
    b = GrfArchive(D26)
    c = GrfArchive(D26B)
    new = dict(b.entries)
    new.update(c.entries)
    changed = []
    for k, e in a.entries.items():
        n = new.get(k)
        if n is not None and n.rsize != e.rsize and e.rsize > 0:
            changed.append((k, e.rsize, n.rsize))
    changed.sort()
    groups = {}
    for k, old, nw in changed:
        parts = k.replace('\\', '/').split('/')
        top = parts[1] if len(parts) > 1 and parts[0] == 'data' else parts[0]
        sub = parts[2] if len(parts) > 3 else ''
        ext = os.path.splitext(k)[1].lower()
        groups.setdefault((top, sub), []).append((k, old, nw, ext))
    out = ['# Data2026: arquivos atualizados (existem no LATAM e no 2026 com tamanho diferente)', '',
           'Gerado por `tools/diff_data2026.py` so pelos indices (nada extraido). Total: **%d** arquivos.' % len(changed), '',
           'O LATAM tem 275.761 arquivos e o 2026 241.448 (+2026.grf por cima). A UI do LATAM e em portugues: nao trocar UI, mensagens e idioma.', '']
    out.append('| Pasta | Subpasta | Arquivos | Tipos |')
    out.append('|---|---|---|---|')
    for (top, sub), items in sorted(groups.items(), key=lambda kv: -len(kv[1])):
        exts = {}
        for _, _, _, e in items:
            exts[e] = exts.get(e, 0) + 1
        out.append('| `%s` | `%s` | %d | %s |' % (top, sub, len(items), ', '.join('%s %d' % (x or '-', n) for x, n in sorted(exts.items(), key=lambda t: -t[1])[:4])))
    out += ['', '## Amostra por grupo (ate 12 arquivos, tamanho antigo -> novo)', '']
    for (top, sub), items in sorted(groups.items(), key=lambda kv: -len(kv[1]))[:25]:
        out.append('### %s / %s (%d)' % (top, sub, len(items)))
        for k, old, nw, _ in items[:12]:
            out.append('- `%s` %d -> %d' % (k, old, nw))
        out.append('')
    dest = os.path.join(os.path.dirname(os.path.abspath(__file__)), '..', 'docs', 'ref', 'DATA2026_ATUALIZADOS.md')
    open(dest, 'w', encoding='utf-8', newline='\n').write('\n'.join(out))
    print('alterados', len(changed), '->', os.path.normpath(dest))


if __name__ == '__main__':
    main()

"""
Aplica as ATUALIZACOES do cliente 2026 (arquivos que o LATAM tem com tamanho diferente) sobre o conjunto de assets do jogo, so nas
categorias de arte do mundo: capas/asas (sprite/로브), acessorios de cabeca, sprites de item, monstros, corpos de classe, NPCs,
texturas e modelos de mapa, efeitos, sons e arquivos de mapa (.rsw/.gnd). NAO toca na interface (texture/유저인터페이스), mensagens
(.lub, .txt de livros) nem no idioma: o LATAM e em portugues.
Cada arquivo trocado tem a copia antiga guardada em <destino>/../_backup_latam/ (volta com --restore).

Uso (WSL): python3 tools/apply_data2026_updates.py [--dry] [--restore] [origem=/mnt/d/Ragnarok_Assets/grf2026/data] [destino=/mnt/e/Dev/Ragnarok_Assets/grf/data]
"""
import os
import shutil
import sys

CATEGORIES = [
    ('sprite/로브/', 'capas e asas'),
    ('sprite/악세사리/', 'acessorios de cabeca'),
    ('sprite/아이템/', 'sprites de item'),
    ('sprite/몬스터/', 'monstros'),
    ('sprite/인간족/', 'corpos de classe'),
    ('sprite/npc/', 'NPCs'),
    ('texture/effect/', 'efeitos'),
    ('wav/', 'sons'),
    ('model/', 'modelos de mapa'),
    ('texture/prontera_re/', 'texturas de Prontera'),
    ('texture/splen/', 'texturas de Splendide'),
    ('texture/모로코/', 'texturas de Morroc'),
    ('texture/프론테라/', 'texturas de Prontera (antigas)'),
    ('texture/기타마을/', 'texturas de vilas'),
]
MAP_EXT = ('.rsw', '.gnd', '.gat')


def wanted(rel):
    low = rel.replace('\\', '/').lower()
    if '/' not in low and low.endswith(MAP_EXT):
        return 'arquivo de mapa'
    for prefix, name in CATEGORIES:
        if low.startswith(prefix.lower()):
            return name
    return None


def main():
    args = [a for a in sys.argv[1:] if not a.startswith('--')]
    dry = '--dry' in sys.argv
    restore = '--restore' in sys.argv
    src = args[0] if len(args) > 0 else '/mnt/d/Ragnarok_Assets/grf2026/data'
    dst = args[1] if len(args) > 1 else '/mnt/e/Dev/Ragnarok_Assets/grf/data'
    backup = os.path.join(os.path.dirname(dst.rstrip('/')), '_backup_latam')
    if restore:
        n = 0
        for dp, _, files in os.walk(backup):
            for f in files:
                rel = os.path.relpath(os.path.join(dp, f), backup)
                shutil.copyfile(os.path.join(dp, f), os.path.join(dst, rel))
                n += 1
        print('restaurados', n)
        return
    names = {}
    for dp, _, files in os.walk(dst):
        for f in files:
            rel = os.path.relpath(os.path.join(dp, f), dst)
            names[rel.replace('\\', '/').lower()] = rel
    counts = {}
    for dp, _, files in os.walk(src):
        for f in files:
            rel = os.path.relpath(os.path.join(dp, f), src)
            cat = wanted(rel)
            existing = names.get(rel.replace('\\', '/').lower())
            if not cat or not existing:
                continue
            s, d = os.path.join(src, rel), os.path.join(dst, existing)
            if os.path.getsize(s) == os.path.getsize(d):
                continue
            counts[cat] = counts.get(cat, 0) + 1
            if dry:
                continue
            b = os.path.join(backup, existing)
            if not os.path.exists(b):
                os.makedirs(os.path.dirname(b), exist_ok=True)
                shutil.copyfile(d, b)
            shutil.copyfile(s, d)
    print('trocados%s: %s | total %d' % (' [simulacao]' if dry else '', counts, sum(counts.values())))


if __name__ == '__main__':
    main()

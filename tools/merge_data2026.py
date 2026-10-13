"""
Junta o Data2026 (cliente kRO 2026 extraido em D:\\Ragnarok_Assets\\grf2026) ao conjunto de assets que o jogo usa (E:\\Dev\\Ragnarok_Assets\\grf),
SEM sobrescrever nada: so copia arquivos que o LATAM nao tem (episodios novos: monstros, NPCs, itens, efeitos, sons, mapas novos).
A comparacao ignora maiusculas/minusculas. Arquivos de texto ofuscados do 2026 (letras invertidas, a<->z) sao desfeitos na copia.

Uso (WSL): python3 tools/merge_data2026.py [origem=/mnt/d/Ragnarok_Assets/grf2026/data] [destino=/mnt/e/Dev/Ragnarok_Assets/grf/data] [--dry]
"""
import os
import shutil
import sys

OBFUSCATED_EXT = ('.xml',)  # tabelas em XML do cliente vem com as letras invertidas (a<->z, A<->Z)


def unscramble(raw):
    out = bytearray(raw)
    for i, b in enumerate(out):
        if 0x61 <= b <= 0x7A:
            out[i] = 0xDB - b
        elif 0x41 <= b <= 0x5A:
            out[i] = 0x9B - b
    return bytes(out)


def is_scrambled_xml(raw):
    return raw[:5] == b'<?cno'  # "<?xml" com as letras invertidas


def main():
    args = [a for a in sys.argv[1:] if not a.startswith('--')]
    dry = '--dry' in sys.argv
    src = args[0] if len(args) > 0 else '/mnt/d/Ragnarok_Assets/grf2026/data'
    dst = args[1] if len(args) > 1 else '/mnt/e/Dev/Ragnarok_Assets/grf/data'
    have = set()
    for dp, _, files in os.walk(dst):
        rel = os.path.relpath(dp, dst)
        for f in files:
            have.add(os.path.normpath(os.path.join(rel, f)).lower())
    copied = skipped = decoded = 0
    size = 0
    for dp, _, files in os.walk(src):
        rel = os.path.relpath(dp, src)
        for f in files:
            key = os.path.normpath(os.path.join(rel, f)).lower()
            if key in have:
                skipped += 1
                continue
            s = os.path.join(dp, f)
            d = os.path.join(dst, rel, f.lower() if f.lower().endswith(('.bmp', '.tga', '.spr', '.act', '.str', '.wav')) else f)
            if not dry:
                os.makedirs(os.path.dirname(d), exist_ok=True)
                if f.lower().endswith(OBFUSCATED_EXT):
                    raw = open(s, 'rb').read()
                    if is_scrambled_xml(raw):
                        open(d, 'wb').write(unscramble(raw))
                        decoded += 1
                    else:
                        shutil.copyfile(s, d)
                else:
                    shutil.copyfile(s, d)
            copied += 1
            size += os.path.getsize(s)
    print('copiados %d (%.2f GB), ja existiam %d, xml desembaralhados %d%s' % (copied, size / 1e9, skipped, decoded, ' [simulacao]' if dry else ''))


if __name__ == '__main__':
    main()

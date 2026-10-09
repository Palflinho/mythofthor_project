"""
Gera Config/job_sprites.json: id da classe -> nome do sprite do corpo (data/sprite/인간족/몸통/<sexo>/<nome>_<sexo>.spr).

Fonte: tabela do roBrowser (src/DB/Jobs/JobNameTable.js + JobConst.js), lida como DADOS (nada e executado);
no cliente original essa tabela fica no executavel/lua. So entram as classes do jogo: ate a transclasse
(classes basicas, 2-1/2-2, transclasses, bebes e as expandidas), sem 3a/4a classe.

Uso: python3 tools/gen_job_sprites.py <pasta_com_os_js> <raiz_dos_assets>
"""
import json
import os
import re
import sys

# 3a/4a classe e montarias delas: fora do jogo (ver memoria class-scope)
EXCLUDED_PREFIXES = ('RUNE_KNIGHT', 'WARLOCK', 'RANGER', 'ARCHBISHOP', 'MECHANIC', 'GUILLOTINE_CROSS', 'ROYAL_GUARD',
                     'SORCERER', 'MINSTREL', 'WANDERER', 'SURA', 'GENETIC', 'SHADOW_CHASER', 'SUPERNOVICE2', 'KAGEROU',
                     'OBORO', 'REBELLION', 'SUMMONER', 'STAR_EMPEROR', 'SOUL_REAPER', 'DRAGON_KNIGHT', 'MEISTER',
                     'SHADOW_CROSS', 'ARCH_MAGE', 'CARDINAL', 'WINDHAWK', 'IMPERIAL_GUARD', 'BIOLO', 'ABYSS_CHASER',
                     'ELEMENTAL_MASTER', 'INQUISITOR', 'TROUBADOUR', 'TROUVERE', 'SKY_EMPEROR', 'SOUL_ASCETIC',
                     'SHINKIRO', 'SHIRANUI', 'NIGHT_WATCH', 'HYPER_NOVICE', 'SPIRIT_HANDLER')


def unescape(s):
    raw = re.sub(r'\\x([0-9a-fA-F]{2})', lambda m: chr(int(m.group(1), 16)), s)
    return raw.encode('latin-1').decode('cp949')


def main():
    js, root = sys.argv[1], sys.argv[2]
    consts = dict((n, int(v)) for n, v in re.findall(r'(\w+)\s*:\s*(\d+)', open(os.path.join(js, 'JobConst.js'), encoding='latin-1').read()))
    txt = open(os.path.join(js, 'JobNameTable.js'), encoding='latin-1').read()
    names = {}
    for key, val in re.findall(r'JobNameTable\[JobId\.(\w+)\]\s*=\s*"([^"]*)"', txt):
        names[key] = unescape(val)
    for args in re.findall(r'duplicateEntry\(([^)]*)\)', txt):
        keys = [a.strip().replace('JobId.', '') for a in args.split(',')]
        if keys[0] in names and not keys[0].startswith('origin'):
            for k in keys[1:]:
                names[k] = names[keys[0]]
    body = os.path.join(root, 'data', 'sprite', '인간족', '몸통')
    out, missing = {}, []
    for key, name in sorted(names.items(), key=lambda kv: consts.get(kv[0], 99999)):
        if key not in consts or key.startswith(EXCLUDED_PREFIXES):
            continue
        # Bardo so tem masculino e Odalisca so feminino: basta existir em um dos sexos
        ok = any(os.path.exists(os.path.join(body, sex, '%s_%s.spr' % (name, sex))) for sex in ('남', '여'))
        if ok:
            out[str(consts[key])] = name
        else:
            missing.append('%s(%d)=%s' % (key, consts[key], name))
    dest = os.path.join(os.path.dirname(os.path.abspath(__file__)), '..', 'Config', 'job_sprites.json')
    with open(dest, 'w', encoding='utf-8') as f:
        json.dump(out, f, ensure_ascii=False, indent=0, sort_keys=True)
    print('classes com sprite: %d -> %s' % (len(out), os.path.normpath(dest)))
    if missing:
        print('sem sprite:', ', '.join(missing))


if __name__ == '__main__':
    main()

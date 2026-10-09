"""
Gera Config/equip_sprites.json: como achar os sprites de equipamento no personagem.

  hats:        view id do acessorio (LOOK_HEAD_TOP/MID/BOTTOM) -> sufixo (data/sprite/악세사리/<sexo>/<sexo><sufixo>.spr)
  weapons:     view id da arma (LOOK_WEAPON) -> sufixo (data/sprite/인간족/<classe>/<classe>_<sexo><sufixo>.spr);
               view id sem nome usa "_<id>", como o cliente
  shields:     view id do escudo (LOOK_SHIELD) -> nome (data/sprite/방패/<classe>/<classe>_<sexo>_<nome>_방패.spr)
  weapon_jobs: classe -> pasta da classe para armas/escudos (transclasses usam a da classe base)

Fonte: tabelas do roBrowser (DB/Items/HatTable.js, WeaponTable.js, WeaponType.js, ShieldTable.js,
DB/Jobs/JobNameTable.js, JobConst.js), lidas como DADOS (nada e executado). No cliente original ficam em
accessoryid/accname.lub, weapontable.lub e no executavel. Classes de 3a/4a ficam de fora (ver gen_job_sprites.py).

Uso: python3 tools/gen_equip_sprites.py <pasta_equip_js> <pasta_jobs_js> <raiz_dos_assets>
"""
import json
import os
import re
import sys

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
from gen_job_sprites import EXCLUDED_PREFIXES, unescape  # noqa: E402

# Herancas do WeaponJobTable.js do roBrowser, sem as 3as classes: transclasse e bebe usam a pasta da classe base
WEAPON_JOB_DUPS = [
    ('NOVICE', 'NOVICE_H', 'NOVICE_B'), ('SWORDMAN', 'SWORDMAN_H', 'SWORDMAN_B'), ('MAGICIAN', 'MAGICIAN_H', 'MAGICIAN_B'),
    ('ARCHER', 'ARCHER_H', 'ARCHER_B'), ('ACOLYTE', 'ACOLYTE_H', 'ACOLYTE_B'), ('MERCHANT', 'MERCHANT_H', 'MERCHANT_B'),
    ('THIEF', 'THIEF_H', 'THIEF_B'), ('KNIGHT', 'KNIGHT_H', 'KNIGHT_B'), ('KNIGHT2', 'KNIGHT2_H', 'KNIGHT2_B'),
    ('PRIEST', 'PRIEST_H', 'PRIEST_B'), ('WIZARD', 'WIZARD_H', 'WIZARD_B'), ('BLACKSMITH', 'BLACKSMITH_H', 'BLACKSMITH_B'),
    ('HUNTER', 'HUNTER_H', 'HUNTER_B'), ('ASSASSIN', 'ASSASSIN_H', 'ASSASSIN_B'), ('CRUSADER', 'CRUSADER_H', 'CRUSADER_B'),
    ('CRUSADER2', 'CRUSADER2_H', 'CRUSADER2_B'), ('MONK', 'MONK_H', 'MONK_B'), ('SAGE', 'SAGE_H', 'SAGE_B'),
    ('ROGUE', 'ROGUE_H', 'ROGUE_B'), ('ALCHEMIST', 'ALCHEMIST_H', 'ALCHEMIST_B'), ('BARD', 'BARD_H', 'BARD_B'),
    ('DANCER', 'DANCER_H', 'DANCER_B'),
]


def js_pairs(path, pattern):
    return re.findall(pattern, open(path, encoding='latin-1').read())


def main():
    equip_js, jobs_js, root = sys.argv[1], sys.argv[2], sys.argv[3]
    sprite = os.path.join(root, 'data', 'sprite')

    hats = {}
    for vid, suffix in js_pairs(os.path.join(equip_js, 'HatTable.js'), r'"(\d+)":\s*"([^"]*)"'):
        name = unescape(suffix)
        if any(os.path.exists(os.path.join(sprite, '악세사리', sex, sex + name + '.spr')) for sex in ('남', '여')):
            hats[vid] = name

    wtypes = dict(js_pairs(os.path.join(equip_js, 'WeaponType.js'), r'(\w+):\s*(\d+)'))
    weapons = {}
    for key, suffix in js_pairs(os.path.join(equip_js, 'WeaponTable.js'), r'WeaponName\[WeaponType\.(\w+)\]\s*=\s*"([^"]*)"'):
        if key in wtypes:
            weapons[wtypes[key]] = unescape(suffix)

    shields = dict((k, unescape(v)) for k, v in js_pairs(os.path.join(equip_js, 'ShieldTable.js'), r'(\d+):\s*"([^"]*)"'))

    consts = dict((n, int(v)) for n, v in js_pairs(os.path.join(jobs_js, 'JobConst.js'), r'(\w+)\s*:\s*(\d+)'))
    names = dict((k, unescape(v)) for k, v in js_pairs(os.path.join(jobs_js, 'JobNameTable.js'), r'JobNameTable\[JobId\.(\w+)\]\s*=\s*"([^"]*)"'))
    for group in WEAPON_JOB_DUPS:
        for k in group[1:]:
            names[k] = names[group[0]]
    weapon_jobs = {}
    for key, name in names.items():
        if key in consts and not key.startswith(EXCLUDED_PREFIXES) and os.path.isdir(os.path.join(sprite, '인간족', name)):
            weapon_jobs[str(consts[key])] = name

    out = {'hats': hats, 'weapons': weapons, 'shields': shields, 'weapon_jobs': weapon_jobs}
    dest = os.path.join(os.path.dirname(os.path.abspath(__file__)), '..', 'Config', 'equip_sprites.json')
    with open(dest, 'w', encoding='utf-8') as f:
        json.dump(out, f, ensure_ascii=False, indent=0, sort_keys=True)
    print('acessorios %d, tipos de arma %d, escudos %d, classes com pasta de arma %d -> %s'
          % (len(hats), len(weapons), len(shields), len(weapon_jobs), os.path.normpath(dest)))


if __name__ == '__main__':
    main()

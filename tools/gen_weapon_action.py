"""
Gera Config/weapon_action.json: qual das tres animacoes de ataque o jogador usa, por classe e tipo de arma.

O corpo do jogador tem tres grupos de ataque (acoes 5, 10 e 11 do .act: ATTACK1, ATTACK2, ATTACK3) e o cliente escolhe
um pela classe e pela arma (ex.: Cacador com arco usa o ATTACK3, que e o de puxar a corda; sem arma, o ATTACK1). A tabela
vem do roBrowserLegacy (src/DB/Jobs/WeaponAction.js, WeaponType.js e JobConst.js), lida aqui como dado.

Saida: {"jobs": {"<classe>": {"<tipo de arma>": 0|1|2}}, "jobs_sex": {"<classe>": {"f": {...}, "m": {...}}}}

Uso (WSL): python3 tools/gen_weapon_action.py <pasta com os tres .js> [Config/weapon_action.json]
"""
import json
import os
import re
import sys


def parse_consts(path):
    """Le 'NOME: numero,' de um export default { ... }."""
    out = {}
    for m in re.finditer(r'^\s*([A-Za-z_]\w*)\s*:\s*(-?\d+)\s*,?', open(path, encoding='utf-8').read(), re.M):
        out[m.group(1)] = int(m.group(2))
    return out


def parse_block(body, wtypes):
    table = {}
    for m in re.finditer(r'this\[WeaponType\.(\w+)\]\s*=\s*(\d+)', body):
        if m.group(1) in wtypes:
            table[str(wtypes[m.group(1)])] = int(m.group(2))
    return table


def main():
    src = sys.argv[1]
    dest = sys.argv[2] if len(sys.argv) > 2 else os.path.join(os.path.dirname(__file__), '..', 'Config', 'weapon_action.json')
    wtypes = parse_consts(os.path.join(src, 'WeaponType.js'))
    jobs = parse_consts(os.path.join(src, 'JobConst.js'))
    text = open(os.path.join(src, 'WeaponAction.js'), encoding='utf-8').read()

    single, by_sex = {}, {}
    # WeaponAction[JobId.X] = new (function () { ... })();   ou   = [ new (function () { /*f*/ })(), new (...)() ];
    for m in re.finditer(r'WeaponAction\[JobId\.(\w+)\]\s*=\s*(\[|new)', text):
        job = jobs.get(m.group(1))
        if job is None:
            continue
        start = m.end()
        if m.group(2) == '[':
            end = text.index('];', start)
            blocks = re.findall(r'function\s*\(\)\s*\{(.*?)\}\)\(\)', text[start:end], re.S)
            if len(blocks) == 2:
                by_sex[str(job)] = {'f': parse_block(blocks[0], wtypes), 'm': parse_block(blocks[1], wtypes)}
        else:
            end = text.index('})();', start)
            single[str(job)] = parse_block(text[start:end], wtypes)

    # duplicateEntry(origem, copia1, copia2, ...)
    for m in re.finditer(r'duplicateEntry\((.*?)\);', text, re.S):
        names = re.findall(r'JobId\.(\w+)', m.group(1))
        if not names or names[0] not in jobs:
            continue
        origin = str(jobs[names[0]])
        for name in names[1:]:
            if name not in jobs:
                continue
            if origin in single:
                single[str(jobs[name])] = single[origin]
            elif origin in by_sex:
                by_sex[str(jobs[name])] = by_sex[origin]

    with open(dest, 'w', encoding='utf-8') as f:
        json.dump({'jobs': single, 'jobs_sex': by_sex}, f, ensure_ascii=False, indent=1, sort_keys=True)
    print('weapon_action.json: %d classes, %d por sexo -> %s' % (len(single), len(by_sex), os.path.normpath(dest)))


if __name__ == '__main__':
    main()

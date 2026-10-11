#!/usr/bin/env python3
"""
Completa o mapa skill -> efeito de Config/skill_effects_full.json a partir do SkillEffect.js do roBrowserLegacy
(referencia de comportamento; so numeros e nomes de efeito, nenhum codigo copiado).

- So PREENCHE estagios vazios: as entradas ja auditadas a mao (1a classe) nao sao alteradas.
- Ids de texto do roBrowser ('98_beforecast', 'ef_arrow_projectile') viram ids numericos novos (a partir de 3000),
  copiando as partes do efeito de mesmo nome da tabela 'effects' (que ja tem as chaves de texto).
- Listas ([254, 'quake']) viram um efeito novo que junta as partes dos itens.

Estagios do jogo: effect, cast, ground, hit.
  effectId, successEffectId                -> effect
  effectIdOnCaster, beginCastEffectId      -> cast
  groundEffectId                           -> ground
  hitEffectId, beforeHitEffectId, releaseEffectId -> hit (beforeHit so se nao houver hit)

Uso: python3 tools/gen_skill_map_from_rb.py   (no WSL, com ~/references/roBrowserLegacy)
"""
import json
import os
import re

HOME = os.path.expanduser('~')
RB = os.path.join(HOME, 'references', 'roBrowserLegacy', 'src', 'DB')
BASE = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
if not os.path.isdir(RB):
    BASE = '/mnt/e/Documentos/Repositórios/mythofthor_project'
OUT = os.path.join(BASE, 'Config', 'skill_effects_full.json')

STAGE_OF = {
    'effectId': 'effect', 'successEffectId': 'effect',
    'effectIdOnCaster': 'cast', 'beginCastEffectId': 'cast',
    'groundEffectId': 'ground',
    'hitEffectId': 'hit', 'releaseEffectId': 'hit', 'beforeHitEffectId': 'hit',
}


def consts():
    text = open(os.path.join(RB, 'Skills', 'SkillConst.js'), encoding='latin-1').read()
    return {m.group(1): int(m.group(2)) for m in re.finditer(r'(\w+)\s*:\s*(\d+)', text)}


def match_brace(text, start):
    depth = 0
    for i in range(start, len(text)):
        if text[i] == '{':
            depth += 1
        elif text[i] == '}':
            depth -= 1
            if depth == 0:
                return i
    return -1


def parse_skill_effect():
    text = open(os.path.join(RB, 'Skills', 'SkillEffect.js'), encoding='latin-1').read()
    table = {}
    for m in re.finditer(r'SkillEffect\[SK\.(\w+)\]\s*=\s*', text):
        name = m.group(1)
        pos = m.end()
        if text.startswith('{', pos):
            end = match_brace(text, pos)
            body = text[pos + 1:end]
            fields = {}
            for f in re.finditer(r'(\w+)\s*:\s*(\[[^\]]*\]|\'[^\']*\'|\d+)', body):
                key, val = f.group(1), f.group(2)
                if key not in STAGE_OF:
                    continue
                if val.startswith('['):
                    items = []
                    for it in re.findall(r'\'[^\']*\'|\d+', val):
                        items.append(it.strip("'") if it.startswith("'") else int(it))
                    fields[key] = items
                elif val.startswith("'"):
                    fields[key] = val.strip("'")
                else:
                    fields[key] = int(val)
            table[name] = fields
        else:
            a = re.match(r'SkillEffect\[SK\.(\w+)\]', text[pos:])
            if a and a.group(1) in table:
                table[name] = dict(table[a.group(1)])
    return table


def main():
    data = json.load(open(OUT, encoding='utf-8'))
    effects = data['effects']
    skills = data['skills']
    sk = consts()
    table = parse_skill_effect()
    next_id = [3000]
    made = {}

    def numeric(ref):
        """ref (int, 'texto' ou lista) -> id numerico de um efeito existente/criado, ou 0."""
        if isinstance(ref, int):
            return ref if str(ref) in effects else 0
        if isinstance(ref, str):
            if ref.isdigit():
                return numeric(int(ref))
            key = 'str:' + ref
            if key in made:
                return made[key]
            if ref not in effects:
                return 0
            nid = next_id[0]
            next_id[0] += 1
            effects[str(nid)] = effects[ref]
            made[key] = nid
            return nid
        if isinstance(ref, list):
            parts = []
            for it in ref:
                n = numeric(it)
                if n:
                    parts += effects[str(n)]
            if not parts:
                return 0
            key = 'list:' + json.dumps(ref)
            if key in made:
                return made[key]
            nid = next_id[0]
            next_id[0] += 1
            effects[str(nid)] = parts
            made[key] = nid
            return nid
        return 0

    added = filled = 0
    for name, fields in table.items():
        sid = sk.get(name)
        if sid is None:
            continue
        entry = skills.setdefault(str(sid), {})
        before = {k: entry.get(k) for k in ('effect', 'cast', 'ground', 'hit')}
        # beforeHit so entra em 'hit' se nao houver hitEffectId
        order = [k for k in fields if k != 'beforeHitEffectId'] + (['beforeHitEffectId'] if 'beforeHitEffectId' in fields else [])
        for key in order:
            stage = STAGE_OF[key]
            if entry.get(stage):
                continue
            if stage == 'hit' and key == 'beforeHitEffectId' and 'hitEffectId' in fields:
                continue
            n = numeric(fields[key])
            if n:
                entry[stage] = n
        after = {k: entry.get(k) for k in ('effect', 'cast', 'ground', 'hit')}
        if before != after:
            filled += 1
            if not any(before.values()):
                added += 1
    json.dump(data, open(OUT, 'w', encoding='utf-8'), indent=2, ensure_ascii=False, sort_keys=True)
    print('skills no roBrowser:', len(table), '| entradas alteradas:', filled, '| antes sem nenhum efeito:', added, '| efeitos novos:', next_id[0] - 3000)


if __name__ == '__main__':
    main()

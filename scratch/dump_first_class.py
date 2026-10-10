#!/usr/bin/env python3
"""
Analisa e extrai todas as skills da 1ª classe e todos os seus efeitos (STR e procedurais) do roBrowserLegacy.
"""
import os
import re
import json

JS_DIR = r"\\wsl.localhost\Ubuntu\home\palf\references\roBrowserLegacy\src\DB"
RATHENA_SKILL_DB = r"\\wsl.localhost\Ubuntu\home\palf\mythofthor-server\rathena\db\pre-re\skill_db.yml"

def load_skill_consts():
    path = os.path.join(JS_DIR, "Skills", "SkillConst.js")
    with open(path, "r", encoding="latin-1") as f:
        text = f.read()
    consts = {}
    for m in re.finditer(r'(\w+)\s*:\s*(\d+)', text):
        consts[m.group(1)] = int(m.group(2))
    return consts

def load_elements():
    elements = {}
    cur_id = None
    with open(RATHENA_SKILL_DB, "r", encoding="utf-8", errors="replace") as f:
        for line in f:
            m = re.match(r'^\s*-\s*Id:\s*(\d+)', line)
            if m:
                cur_id = int(m.group(1))
                continue
            m = re.match(r'^\s*Element:\s*(\w+)', line)
            if m and cur_id is not None:
                elements[cur_id] = m.group(1)
    return elements

def load_skill_effects():
    path = os.path.join(JS_DIR, "Skills", "SkillEffect.js")
    with open(path, "r", encoding="latin-1") as f:
        text = f.read()
    
    # Mapeia SkillEffect[SK.<NAME>] = { ... }
    # ou SkillEffect[SK.<NAME>] = SkillEffect[SK.<OTHER>]
    entries = {}
    for m in re.finditer(r'SkillEffect\[SK\.(\w+)\]\s*=\s*(.*?);', text, re.S):
        skill_name = m.group(1)
        val = m.group(2).strip()
        entries[skill_name] = val
    return entries

def load_effect_table():
    path = os.path.join(JS_DIR, "Effects", "EffectTable.js")
    with open(path, "r", encoding="latin-1") as f:
        text = f.read()
    
    # Extrai cada bloco id: [ ... ]
    # Pode ser numérico ou string 'quake_magnum': [ ... ]
    effects = {}
    # Divide por chaves de nível superior
    pattern = re.compile(r'\n\t(?:(\d+)|\'([^\']+)\'):\s*\[', re.S)
    matches = list(pattern.finditer(text))
    for i, m in enumerate(matches):
        key = m.group(1) if m.group(1) is not None else m.group(2)
        start = m.end()
        end = matches[i+1].start() if i+1 < len(matches) else len(text)
        block = text[start:end]
        # Pega até o fechamento do colchete correspondente
        depth = 1
        bracket_end = -1
        for idx, ch in enumerate(block):
            if ch == '[':
                depth += 1
            elif ch == ']':
                depth -= 1
                if depth == 0:
                    bracket_end = idx
                    break
        if bracket_end != -1:
            effects[key] = block[:bracket_end].strip()
        else:
            effects[key] = block.strip()
    return effects

if __name__ == "__main__":
    consts = load_skill_consts()
    elements = load_elements()
    sk_effects = load_skill_effects()
    eff_table = load_effect_table()
    
    print(f"Total consts: {len(consts)}, SkillEffects: {len(sk_effects)}, EffectTable keys: {len(eff_table)}")
    
    # Classes da 1ª classe
    first_class_prefixes = ["NV_", "SM_", "MG_", "AC_", "AL_", "MC_", "TF_"]
    first_class_skills = {}
    for name, raw in sk_effects.items():
        if any(name.startswith(p) for p in first_class_prefixes):
            sid = consts.get(name)
            first_class_skills[name] = {"id": sid, "raw": raw}
            print(f"{name} (ID {sid}): {raw}")

#!/usr/bin/env python3
"""
Extrai e audita todas as skills e efeitos de 2ª Classe (2-1 e 2-2) do roBrowser.
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
    return {m.group(1): int(m.group(2)) for m in re.finditer(r'(\w+)\s*:\s*(\d+)', text)}

def load_skill_effects():
    path = os.path.join(JS_DIR, "Skills", "SkillEffect.js")
    with open(path, "r", encoding="latin-1") as f:
        text = f.read()
    entries = {}
    for m in re.finditer(r'SkillEffect\[SK\.(\w+)\]\s*=\s*([^;]+);', text):
        entries[m.group(1)] = m.group(2).strip()
    return entries

def load_elements():
    elements = {}
    cur_id = None
    if os.path.exists(RATHENA_SKILL_DB):
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

def load_effect_table():
    path = os.path.join(JS_DIR, "Effects", "EffectTable.js")
    with open(path, "r", encoding="latin-1") as f:
        text = f.read()
    
    effects = {}
    pattern = re.compile(r'\n\t(?:(\d+)|\'([^\']+)\'|(\w+)):\s*\[', re.S)
    matches = list(pattern.finditer(text))
    for i, m in enumerate(matches):
        key = m.group(1) or m.group(2) or m.group(3)
        start = m.end()
        end = matches[i+1].start() if i+1 < len(matches) else len(text)
        block = text[start:end]
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

def main():
    consts = load_skill_consts()
    sk_effects = load_skill_effects()
    elements = load_elements()
    eff_table = load_effect_table()
    
    # 2-1 and 2-2 classes + quest skills
    # Knight, Priest, Wizard, Blacksmith, Hunter, Assassin
    # Crusader, Monk, Sage, Rogue, Alchemist, Bard, Dancer
    second_prefixes = ('KN_', 'PR_', 'WZ_', 'BS_', 'HT_', 'AS_', 'CR_', 'MO_', 'SA_', 'RG_', 'AM_', 'BA_', 'DC_', 'BD_')
    
    # Transclass prefixes to filter out if any match CR_ or similar
    trans_skills = {
        'CR_ALCHEMY', 'CR_SYNTHESISPOTION', 'CR_SLIMPITCHER', 'CR_FULLPROTECTION', 'CR_ACIDDEMONSTRATION', 'CR_CULTIVATION'
    }
    
    skills_data = {}
    all_referenced_effects = set()
    
    for name, sid in sorted(consts.items(), key=lambda x: x[1]):
        if any(name.startswith(p) for p in second_prefixes) and name not in trans_skills:
            # Check if this is within pre-re 2nd class / quest
            if sid > 1100 and sid not in (3036, 3037):
                continue
            raw_eff = sk_effects.get(name, '{}')
            elem = elements.get(sid, "Neutral")
            
            # Parse fields from raw_eff
            parsed_fields = {}
            for field in ['effectId', 'effectIdOnCaster', 'hitEffectId', 'groundEffectId', 'beforeHitEffectId', 'beginCastEffectId', 'successEffectId', 'releaseEffectId']:
                m = re.search(rf'\b{field}:\s*(\[[^\]]+\]|\'[^\']+\'|\d+|\w+)', raw_eff)
                if m:
                    val = m.group(1)
                    parsed_fields[field] = val
                    # find all effect tokens (numbers or quoted strings)
                    tokens = re.findall(r"(\d+|'[^']+'|[a-zA-Z_]\w*)", val)
                    for t in tokens:
                        t_clean = t.strip("'\"")
                        if t_clean not in ('true', 'false', 'srcAID', 'const', 'src', 'EntityManager'):
                            all_referenced_effects.add(t_clean)
            
            skills_data[sid] = {
                "name": name,
                "element": elem,
                "raw": raw_eff,
                "fields": parsed_fields
            }
            
    print(f"Total skills 2ª classe identificadas: {len(skills_data)}")
    print(f"Total efeitos referenciados: {len(all_referenced_effects)}")
    
    # Check coverage in EffectTable
    found = {}
    missing = []
    for eff in sorted(all_referenced_effects, key=lambda x: (int(x) if x.isdigit() else 99999, str(x))):
        if str(eff) in eff_table:
            found[str(eff)] = eff_table[str(eff)]
        else:
            missing.append(eff)
            
    print(f"Efeitos encontrados no EffectTable: {len(found)}")
    print(f"Efeitos ausentes ou especiais: {missing}")
    
    # Save audit info
    with open("scratch/second_class_audit.json", "w", encoding="utf-8") as f:
        json.dump({
            "skills": skills_data,
            "effects_found": list(found.keys()),
            "effects_missing": missing
        }, f, indent=2, ensure_ascii=False)

if __name__ == "__main__":
    main()

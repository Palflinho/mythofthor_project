#!/usr/bin/env python3
"""
Extrai todas as skills da 2ª classe (2-1 e 2-2) e seus efeitos do roBrowserLegacy.
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

def load_skill_effects():
    path = os.path.join(JS_DIR, "Skills", "SkillEffect.js")
    with open(path, "r", encoding="latin-1") as f:
        text = f.read()
    
    entries = {}
    for m in re.finditer(r'SkillEffect\[SK\.(\w+)\]\s*=\s*(.*?);', text, re.S):
        skill_name = m.group(1)
        val = m.group(2).strip()
        entries[skill_name] = val
    return entries

if __name__ == "__main__":
    consts = load_skill_consts()
    sk_effects = load_skill_effects()
    
    second_class_prefixes = [
        "KN_", "WZ_", "HT_", "PR_", "BS_", "AS_",
        "CR_", "SA_", "BA_", "BD_", "DC_", "MO_", "AM_", "RG_"
    ]
    
    found = {}
    for name, raw in sk_effects.items():
        if any(name.startswith(p) for p in second_class_prefixes):
            sid = consts.get(name)
            found[name] = {"id": sid, "raw": raw}
            print(f"{name} (ID {sid}): {raw}")
    print(f"\nTotal skills 2a classe encontradas: {len(found)}")

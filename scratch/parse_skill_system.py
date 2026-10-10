#!/usr/bin/env python3
"""
Extrator completo de SkillEffect e EffectTable do roBrowserLegacy + skill_db do rAthena.
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

if __name__ == "__main__":
    consts = load_skill_consts()
    print("SkillConst carregadas:", len(consts))
    elements = load_elements()
    print("Elements carregados:", len(elements))
    
    # Teste de 1st class skills
    first_class_skills = [
        "NV_BASIC", "NV_FIRSTAID", "NV_PLAYDEAD",
        "SM_SWORD", "SM_TWOHAND", "SM_RECOVERY", "SM_BASH", "SM_PROVOKE", "SM_MAGNUM", "SM_ENDURE",
        "SM_AUTOBERSERK", "SM_MOVINGRECOVERY", "SM_FATALBLOW",
        "MG_COLDBOLT", "MG_FROSTDIVER", "MG_STONECURSE", "MG_FIREBALL", "MG_FIREWALL", "MG_FIREBOLT",
        "MG_LIGHTNINGBOLT", "MG_THUNDERSTORM", "MG_SIGHT", "MG_NAPALMBEAT", "MG_SAFETYWALL", "MG_SOULSTRIKE",
        "MG_SRECOVERY", "MG_ENERGYCOAT",
        "AC_DOUBLE", "AC_SHOWER", "AC_CONCENTRATION", "AC_OWL", "AC_VULTURE", "AC_MAKINGARROW", "AC_CHARGEARROW",
        "AL_RUWACH", "AL_PNEUMA", "AL_TELEPORT", "AL_WARP", "AL_HEAL", "AL_INCAGI", "AL_DECAGI",
        "AL_HOLYWATER", "AL_CRUCIS", "AL_ANGELUS", "AL_BLESSING", "AL_CURE", "AL_DEMONBANE", "AL_DP", "AL_HOLYLIGHT",
        "MC_INCCARRY", "MC_DISCOUNT", "MC_OVERCHARGE", "MC_PUSHCART", "MC_IDENTIFY", "MC_VENDING", "MC_MAMMONITE",
        "MC_CARTREVOLUTION", "MC_CHANGECART", "MC_LOUD",
        "TF_DOUBLE", "TF_MISS", "TF_STEAL", "TF_HIDING", "TF_POISON", "TF_DETOXIFY",
        "TF_SANDATTACK", "TF_BACKSLIDE", "TF_FINDSTONE", "TF_THROWSTONE"
    ]
    for s in first_class_skills:
        print(f"{s}: ID={consts.get(s)}, Element={elements.get(consts.get(s), 'None')}")

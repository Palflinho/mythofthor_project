#!/usr/bin/env python3
"""
Gera Config/skill_effects_full.json completo com todas as habilidades e efeitos (STR + procedurais: CYLINDER, 3D, 2D, SPR, FUNC, WAV).
Preserva os dados existentes de 2ª classe/trans e enriquece a 1ª classe com receitas procedurais exatas do EffectTable.js.
"""
import os
import re
import json

BASE_DIR = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
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

def load_raw_effect_table():
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

def decode_korean(s):
    if not s:
        return ""
    if r"\x" in s:
        try:
            def repl(m):
                hex_str = m.group(0)
                raw_bytes = bytes([int(x, 16) for x in re.findall(r'\\x([0-9a-fA-F]{2})', hex_str)])
                return raw_bytes.decode('cp949', errors='ignore')
            return re.sub(r'(?:\\x[0-9a-fA-F]{2})+', repl, s)
        except Exception:
            return s
    return s

def parse_effect_parts(raw_block):
    parts = []
    obj_pattern = re.compile(r'\{([^{}]*(?:\{[^{}]*\}[^{}]*)*)\}', re.S)
    for obj in obj_pattern.findall(raw_block):
        clean_lines = []
        for line in obj.splitlines():
            line_str = line.strip()
            if line_str.startswith("//"):
                continue
            clean_lines.append(line)
        body = "\n".join(clean_lines)
        
        part = {}
        t_m = re.search(r"type:\s*'([^']+)'", body)
        part['type'] = t_m.group(1) if t_m else ("WAV" if "wav:" in body else "3D")
        
        f_m = re.search(r"(?:file|textureName|absoluteSpriteName):\s*'([^']+)'", body)
        part['file'] = decode_korean(f_m.group(1)) if f_m else ""
        
        w_m = re.search(r"wav:\s*'([^']+)'", body)
        part['wav'] = decode_korean(w_m.group(1)) if w_m else ""
        
        a_m = re.search(r"attachedEntity:\s*(true|false)", body)
        part['attached'] = (a_m.group(1) == 'true') if a_m else True
        
        # Parametros procedurais adicionais
        for num_k in ['duration', 'alphaMax', 'blendMode', 'size', 'sizeStart', 'sizeEnd', 'bottomSize', 'topSize', 'height', 'posZ', 'posz', 'animation', 'duplicate', 'timeBetweenDupli']:
            m = re.search(rf"\b{num_k}:\s*(-?[\d.]+)", body)
            if m:
                val = float(m.group(1)) if '.' in m.group(1) else int(m.group(1))
                part[num_k] = val
        
        for bool_k in ['rotate', 'fade', 'fadeIn', 'fadeOut', 'toSrc', 'fromSrc', 'repeat']:
            m = re.search(rf"\b{bool_k}:\s*(true|false)", body)
            if m:
                part[bool_k] = (m.group(1) == 'true')
        
        # Funcao customizada (ex: quake)
        if "Camera.setQuake" in body or "quake" in body:
            part['func'] = "quake"
            part['duration'] = 50
            part['type'] = "FUNC"
            
        parts.append(part)
    return parts

def main():
    print("Carregando tabelas do roBrowser e rAthena...")
    consts = load_skill_consts()
    elements = load_elements()
    raw_effects = load_raw_effect_table()
    
    # Carrega arquivo existente para preservar dados preexistentes
    curr_path = os.path.join(BASE_DIR, "Config", "skill_effects.json")
    with open(curr_path, "r", encoding="utf-8") as f:
        curr_data = json.load(f)
    
    effects_out = dict(curr_data.get("effects", {}))
    skills_out = dict(curr_data.get("skills", {}))
    
    # 1. Converte e enriquece os efeitos do EffectTable
    for k, raw_block in raw_effects.items():
        parsed = parse_effect_parts(raw_block)
        if parsed:
            effects_out[str(k)] = parsed
            
    # Aliases numericos para os projeteis procedurais
    projectile_aliases = {
        "2001": "ef_coldbolt",
        "2002": "ef_firebolt",
        "2003": "ef_arrow_projectile",
        "2004": "ef_arrow_shower_projectile",
        "2005": "quake_magnum"
    }
    for num_id, str_key in projectile_aliases.items():
        if str_key in effects_out:
            effects_out[num_id] = effects_out[str_key]

    # Mapeamento detalhado e auditado para a 1ª Classe
    first_class_map = {
        # Aprendiz
        1:   {"element": "Neutral"},                                                                # NV_BASIC
        142: {"effect": 309, "sound": "effect/_heal_effect", "element": "Neutral"},                 # NV_FIRSTAID
        143: {"element": "Neutral"},                                                                # NV_TRICKDEAD
        
        # Espadachim
        2:   {"element": "Weapon"},                                                                 # SM_SWORD
        3:   {"element": "Weapon"},                                                                 # SM_TWOHAND
        4:   {"element": "Neutral"},                                                                # SM_RECOVERY
        5:   {"cast": 16, "hit": 1, "sound": "effect/ef_bash", "element": "Weapon"},                # SM_BASH
        6:   {"effect": 67, "sound": "effect/swordman_provoke", "element": "Neutral"},              # SM_PROVOKE
        7:   {"cast": 17, "effect": 17, "hit": 17, "sound": "effect/ef_magnumbreak", "element": "Fire"}, # SM_MAGNUM
        8:   {"effect": 11, "sound": "effect/ef_endure", "element": "Neutral"},                     # SM_ENDURE
        144: {"element": "Neutral"},                                                                # SM_MOVINGRECOVERY
        145: {"element": "Neutral"},                                                                # SM_FATALBLOW
        146: {"element": "Neutral"},                                                                # SM_AUTOBERSERK
        
        # Mago
        9:   {"element": "Neutral"},                                                                # MG_SRECOVERY
        10:  {"effect": 22, "element": "Fire"},                                                     # MG_SIGHT
        11:  {"hit": 1, "element": "Ghost"},                                                        # MG_NAPALMBEAT
        12:  {"ground": 141, "element": "Ghost"},                                                   # MG_SAFETYWALL
        13:  {"effect": 15, "hit": 1, "sound": "effect/ef_soulstrike", "element": "Ghost"},         # MG_SOULSTRIKE
        14:  {"effect": 2001, "hit": 51, "sound": "effect/ef_icearrow1", "element": "Water"},      # MG_COLDBOLT
        15:  {"effect": 27, "hit": 28, "sound": "effect/ef_frostdiver2", "element": "Water"},       # MG_FROSTDIVER
        16:  {"effect": 23, "sound": "effect/stonecurse", "element": "Earth"},                      # MG_STONECURSE
        17:  {"effect": 24, "hit": 49, "sound": "effect/ef_fireball", "element": "Fire"},          # MG_FIREBALL
        18:  {"ground": 25, "hit": 49, "sound": "effect/ef_firewall", "element": "Fire"},          # MG_FIREWALL
        19:  {"effect": 2002, "hit": 49, "sound": "effect/ef_firearrow1", "element": "Fire"},      # MG_FIREBOLT
        20:  {"effect": 29, "hit": 52, "sound": "effect/windhit", "element": "Wind"},               # MG_LIGHTNINGBOLT
        21:  {"ground": 30, "hit": 52, "sound": "effect/magician_thunderstorm", "element": "Wind"},# MG_THUNDERSTORM
        157: {"effect": 169, "element": "Neutral"},                                                 # MG_ENERGYCOAT
        
        # Arqueiro
        43:  {"element": "Neutral"},                                                                # AC_OWL
        44:  {"element": "Neutral"},                                                                # AC_VULTURE
        45:  {"effect": 153, "sound": "effect/ac_concentration", "element": "Neutral"},            # AC_CONCENTRATION
        46:  {"cast": 16, "effect": 2003, "hit": 1, "element": "Weapon"},                          # AC_DOUBLE
        47:  {"effect": 2004, "hit": 1, "element": "Weapon"},                                      # AC_SHOWER
        147: {"element": "Neutral"},                                                                # AC_MAKINGARROW
        148: {"effect": 2003, "hit": 1, "element": "Weapon"},                                      # AC_CHARGEARROW
        
        # Noviço
        22:  {"element": "Neutral"},                                                                # AL_DP
        23:  {"element": "Neutral"},                                                                # AL_DEMONBANE
        24:  {"hit": 1, "element": "Holy"},                                                         # AL_RUWACH
        25:  {"ground": 141, "element": "Neutral"},                                                 # AL_PNEUMA
        26:  {"effect": 34, "element": "Neutral"},                                                  # AL_TELEPORT
        27:  {"ground": 317, "sound": "effect/ef_portal", "element": "Neutral"},                   # AL_WARP
        28:  {"cast": 312, "effect": 312, "hit": 320, "sound": "effect/_heal_effect", "element": "Holy"}, # AL_HEAL
        29:  {"effect": 37, "sound": "effect/ef_incagility", "element": "Neutral"},                # AL_INCAGI
        30:  {"effect": 38, "sound": "effect/ef_decagility", "element": "Neutral"},                # AL_DECAGI
        31:  {"effect": 39, "sound": "effect/ef_aqua", "element": "Neutral"},                      # AL_HOLYWATER
        32:  {"effect": 40, "sound": "effect/ef_signum", "element": "Neutral"},                    # AL_CRUCIS
        33:  {"effect": 41, "sound": "effect/ef_angelus", "element": "Neutral"},                   # AL_ANGELUS
        34:  {"effect": 42, "sound": "effect/ef_blessing", "element": "Neutral"},                  # AL_BLESSING
        35:  {"effect": 66, "sound": "effect/acolyte_cure", "element": "Neutral"},                 # AL_CURE
        156: {"effect": 152, "element": "Holy"},                                                    # AL_HOLYLIGHT
        
        # Mercador
        36:  {"element": "Neutral"},                                                                # MC_INCCARRY
        37:  {"element": "Neutral"},                                                                # MC_DISCOUNT
        38:  {"element": "Neutral"},                                                                # MC_OVERCHARGE
        39:  {"element": "Neutral"},                                                                # MC_PUSHCART
        40:  {"element": "Neutral"},                                                                # MC_IDENTIFY
        41:  {"element": "Neutral"},                                                                # MC_VENDING
        42:  {"effect": 10, "sound": "effect/ef_coin2", "element": "Weapon"},                      # MC_MAMMONITE
        153: {"cast": 170, "hit": 170, "sound": "effect/ef_magnumbreak", "element": "Weapon"},     # MC_CARTREVOLUTION
        154: {"element": "Neutral"},                                                                # MC_CHANGECART
        155: {"effect": 311, "sound": "effect/loud", "element": "Neutral"},                        # MC_LOUD
        
        # Gatuno
        48:  {"element": "Weapon"},                                                                 # TF_DOUBLE
        49:  {"element": "Neutral"},                                                                # TF_MISS
        50:  {"effect": 18, "sound": "effect/ef_steal", "element": "Neutral"},                     # TF_STEAL
        51:  {"element": "Neutral"},                                                                # TF_HIDING
        52:  {"hit": 20, "sound": "effect/ef_envenom", "element": "Poison"},                        # TF_POISON
        53:  {"effect": 21, "sound": "effect/ef_detoxication", "element": "Poison"},               # TF_DETOXIFY
        149: {"effect": 310, "element": "Earth"},                                                   # TF_SPRINKLESAND
        150: {"element": "Neutral"},                                                                # TF_BACKSLIDING
        151: {"element": "Neutral"},                                                                # TF_PICKSTONE
        152: {"effect": 308, "element": "Neutral"}                                                  # TF_THROWSTONE
    }
    
    # Aplica na tabela de skills
    for sid, sdata in first_class_map.items():
        sid_str = str(sid)
        if sid_str not in skills_out:
            skills_out[sid_str] = {}
        skills_out[sid_str].update(sdata)
        
    out_file = os.path.join(BASE_DIR, "Config", "skill_effects_full.json")
    with open(out_file, "w", encoding="utf-8") as f:
        json.dump({"effects": effects_out, "skills": skills_out}, f, ensure_ascii=False, indent=2, sort_keys=True)
        
    print(f"Gerado {out_file} com {len(effects_out)} efeitos e {len(skills_out)} skills!")

if __name__ == "__main__":
    main()

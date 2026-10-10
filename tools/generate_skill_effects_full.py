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
        "2005": "quake_magnum",
        "2006": "ef_spear_projectile",
        "2007": "ef_ht_landmine",
        "2008": "ef_shield_projectile",
        "2009": "ef_tomahawk_projectile",
        "2010": "ef_bs_adrenaline2",
        "2011": "ef_greed_sound"
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
    
    # Mapeamento detalhado e auditado para a 2ª Classe (2-1 e 2-2)
    second_class_map = {
        # === 2-1: Cavaleiro (Knight) ===
        55:   {"element": "Neutral"},                                                                # KN_SPEARMASTERY
        56:   {"cast": 148, "hit": 147, "sound": "effect/pierce", "element": "Weapon"},              # KN_PIERCE
        57:   {"cast": 144, "effect": 70, "sound": "effect/brandish_spear", "element": "Weapon"},   # KN_BRANDISHSPEAR
        58:   {"cast": 150, "hit": 1, "sound": "effect/pierce", "element": "Weapon"},                # KN_SPEARSTAB
        59:   {"cast": 151, "effect": 2006, "hit": 80, "sound": "effect/spear_boomerang", "element": "Weapon"}, # KN_SPEARBOOMERANG
        60:   {"effect": 130, "sound": "effect/ef_twohandquicken", "element": "Neutral"},            # KN_TWOHANDQUICKEN
        61:   {"effect": 131, "sound": "effect/ef_autocounter", "element": "Weapon"},                # KN_AUTOCOUNTER
        62:   {"cast": 149, "hit": 1, "sound": "effect/bowlingbash", "element": "Weapon"},           # KN_BOWLINGBASH
        63:   {"element": "Neutral"},                                                                # KN_RIDING
        64:   {"element": "Neutral"},                                                                # KN_CAVALIERMASTERY
        495:  {"effect": 130, "sound": "effect/ef_twohandquicken", "element": "Neutral"},            # KN_ONEHAND
        1001: {"cast": 16, "hit": 1, "sound": "effect/chargeattack", "element": "Weapon"},           # KN_CHARGEATK

        # === 2-1: Sacerdote (Priest) ===
        65:   {"element": "Neutral"},                                                                # PR_MACEMASTERY
        66:   {"effect": 84, "sound": "effect/impositio_manus", "element": "Neutral"},               # PR_IMPOSITIO
        67:   {"effect": 88, "sound": "effect/suffragium", "element": "Neutral"},                   # PR_SUFFRAGIUM
        68:   {"effect": 86, "sound": "effect/aspersio", "element": "Holy"},                         # PR_ASPERSIO
        69:   {"effect": 91, "sound": "effect/sacramenti", "element": "Neutral"},                    # PR_BENEDICTIO
        70:   {"effect": 83, "ground": 319, "sound": "effect/sanctuary", "element": "Holy"},         # PR_SANCTUARY
        71:   {"effect": 136, "sound": "effect/slow_poison", "element": "Neutral"},                 # PR_SLOWPOISON
        72:   {"effect": 78, "sound": "effect/status_recovery", "element": "Neutral"},               # PR_STRECOVERY
        73:   {"effect": 112, "sound": "effect/kyrie_guard", "element": "Neutral"},                  # PR_KYRIE
        74:   {"effect": 76, "sound": "effect/magnificat", "element": "Neutral"},                    # PR_MAGNIFICAT
        75:   {"effect": 75, "sound": "effect/gloria", "element": "Neutral"},                        # PR_GLORIA
        76:   {"effect": 87, "sound": "effect/lex_divina", "element": "Neutral"},                    # PR_LEXDIVINA
        77:   {"hit": 152, "sound": "effect/turn_undead", "element": "Holy"},                        # PR_TURNUNDEAD
        78:   {"effect": 85, "sound": "effect/lex_aeterna", "element": "Neutral"},                   # PR_LEXAETERNA
        79:   {"effect": 113, "hit": 152, "ground": 318, "sound": "effect/magnus_exorcismus", "element": "Holy"}, # PR_MAGNUS
        1014: {"effect": 312, "sound": "effect/redemptio", "element": "Holy"},                       # PR_REDEMPTIO

        # === 2-1: Bruxo (Wizard) ===
        80:   {"effect": 96, "ground": 138, "hit": 97, "sound": "effect/firepillar", "element": "Fire"}, # WZ_FIREPILLAR
        81:   {"effect": 62, "hit": 49, "sound": "effect/sightrasher", "element": "Fire"},           # WZ_SIGHTRASHER
        82:   {"element": "Neutral"},                                                                # WZ_FIREIVY
        83:   {"effect": 92, "hit": 49, "sound": "effect/meteor_storm", "element": "Fire"},          # WZ_METEOR
        84:   {"effect": 93, "hit": 94, "sound": "effect/hunter_shockwavetrap", "element": "Wind"},  # WZ_JUPITEL
        85:   {"effect": 90, "hit": 52, "sound": "effect/vermilion", "element": "Wind"},             # WZ_VERMILION
        86:   {"cast": 116, "hit": 117, "sound": "effect/wizard_waterball_chulung", "element": "Water"}, # WZ_WATERBALL
        87:   {"ground": 74, "sound": "effect/icewall", "element": "Water"},                          # WZ_ICEWALL
        88:   {"effect": 28, "hit": 51, "sound": "effect/frostnova", "element": "Water"},            # WZ_FROSTNOVA
        89:   {"effect": 89, "ground": 89, "hit": 51, "sound": "effect/stormgust", "element": "Water"}, # WZ_STORMGUST
        90:   {"effect": 79, "hit": 147, "sound": "effect/earthspike", "element": "Earth"},          # WZ_EARTHSPIKE
        91:   {"effect": 142, "hit": 147, "sound": "effect/heavensdrive", "element": "Earth"},       # WZ_HEAVENDRIVE
        92:   {"ground": 95, "sound": "effect/quagmire", "element": "Earth"},                         # WZ_QUAGMIRE
        93:   {"element": "Neutral"},                                                                # WZ_ESTIMATION
        1006: {"effect": 601, "sound": "effect/sightblaster", "element": "Fire"},                     # WZ_SIGHTBLASTER

        # === 2-1: Ferreiro (Blacksmith) ===
        94:   {"element": "Neutral"},                                                                # BS_IRON
        95:   {"element": "Neutral"},                                                                # BS_STEEL
        96:   {"element": "Neutral"},                                                                # BS_ENCHANTEDSTONE
        97:   {"element": "Neutral"},                                                                # BS_ORIDEOCON
        98:   {"element": "Neutral"},                                                                # BS_DAGGER
        99:   {"element": "Neutral"},                                                                # BS_SWORD
        100:  {"element": "Neutral"},                                                                # BS_TWOHANDSWORD
        101:  {"element": "Neutral"},                                                                # BS_AXE
        102:  {"element": "Neutral"},                                                                # BS_MACE
        103:  {"element": "Neutral"},                                                                # BS_KNUCKLE
        104:  {"element": "Neutral"},                                                                # BS_SPEAR
        105:  {"element": "Neutral"},                                                                # BS_HILTBINDING
        106:  {"element": "Neutral"},                                                                # BS_FINDINGORE
        107:  {"element": "Neutral"},                                                                # BS_WEAPONRESEARCH
        108:  {"effect": 101, "sound": "effect/repair_weapon", "element": "Neutral"},                 # BS_REPAIRWEAPON
        109:  {"element": "Neutral"},                                                                # BS_SKINTEMPER
        110:  {"effect": 102, "hit": 1, "sound": "effect/hammerfall", "element": "Neutral"},          # BS_HAMMERFALL
        111:  {"cast": 98, "effect": 98, "sound": "effect/adrenaline", "element": "Neutral"},        # BS_ADRENALINE
        112:  {"effect": 103, "sound": "effect/weapon_perfection", "element": "Neutral"},            # BS_WEAPONPERFECT
        113:  {"effect": 128, "sound": "effect/overthrust", "element": "Neutral"},                   # BS_OVERTHRUST
        114:  {"effect": 104, "sound": "effect/black_maximize_power_circle", "element": "Neutral"},  # BS_MAXIMIZE
        459:  {"effect": 98, "sound": "effect/adrenaline", "element": "Neutral"},                    # BS_ADRENALINE2
        1012: {"element": "Neutral"},                                                                # BS_UNFAIRLYTRICK
        1013: {"effect": 2011, "sound": "effect/ef_entry", "element": "Neutral"},                     # BS_GREED

        # === 2-1: Caçador (Hunter) ===
        115:  {"ground": 69, "sound": "effect/hunter_trap", "element": "Neutral"},                    # HT_SKIDTRAP
        116:  {"ground": 2007, "hit": 49, "sound": "effect/hunter_trap", "element": "Earth"},         # HT_LANDMINE
        117:  {"ground": 145, "sound": "effect/hunter_trap", "element": "Neutral"},                   # HT_ANKLESNARE
        118:  {"ground": 145, "hit": 146, "sound": "effect/hunter_shockwavetrap", "element": "Neutral"}, # HT_SHOCKWAVE
        119:  {"ground": 145, "hit": 139, "sound": "effect/hunter_trap", "element": "Neutral"},       # HT_SANDMAN
        120:  {"ground": 145, "hit": 99, "sound": "effect/hunter_trap", "element": "Neutral"},        # HT_FLASHER
        121:  {"ground": 145, "hit": 108, "sound": "effect/hunter_trap", "element": "Water"},         # HT_FREEZINGTRAP
        122:  {"ground": 145, "hit": 106, "sound": "effect/hunter_trap", "element": "Wind"},          # HT_BLASTMINE
        123:  {"ground": 145, "hit": 107, "sound": "effect/hunter_trap", "element": "Fire"},          # HT_CLAYMORETRAP
        124:  {"effect": 100, "sound": "effect/hunter_trap", "element": "Neutral"},                   # HT_REMOVETRAP
        125:  {"element": "Neutral"},                                                                # HT_TALKIEBOX
        126:  {"element": "Neutral"},                                                                # HT_BEASTBANE
        127:  {"element": "Neutral"},                                                                # HT_FALCON
        128:  {"element": "Neutral"},                                                                # HT_STEELCROW
        129:  {"effect": 115, "hit": 1, "sound": "effect/blitzbeat", "element": "Neutral"},          # HT_BLITZBEAT
        130:  {"effect": 119, "sound": "effect/detecting", "element": "Neutral"},                    # HT_DETECTING
        131:  {"effect": 111, "sound": "effect/springtrap", "element": "Neutral"},                   # HT_SPRINGTRAP
        499:  {"hit": 1, "sound": "effect/ac_double", "element": "Weapon"},                          # HT_POWER (Beast Strafe)
        1009: {"effect": 2003, "hit": 1, "sound": "effect/ac_double", "element": "Weapon"},          # HT_PHANTASMIC

        # === 2-1: Mercenário (Assassin) ===
        132:  {"element": "Neutral"},                                                                # AS_RIGHT
        133:  {"element": "Neutral"},                                                                # AS_LEFT
        134:  {"element": "Neutral"},                                                                # AS_KATAR
        135:  {"effect": 120, "sound": "effect/cloaking", "element": "Neutral"},                     # AS_CLOAKING
        136:  {"cast": 121, "effect": 143, "hit": 122, "sound": "effect/sonicblow", "element": "Weapon"}, # AS_SONICBLOW
        137:  {"cast": 123, "hit": 132, "sound": "effect/grimtooth", "element": "Weapon"},           # AS_GRIMTOOTH
        138:  {"effect": 20, "sound": "effect/enchant_poison", "element": "Poison"},                 # AS_ENCHANTPOISON
        139:  {"effect": 126, "hit": 127, "sound": "effect/poison_react", "element": "Neutral"},      # AS_POISONREACT
        140:  {"effect": 124, "ground": 171, "sound": "effect/venomdust", "element": "Poison"},      # AS_VENOMDUST
        141:  {"effect": 129, "sound": "effect/venomsplasher", "element": "Weapon"},                 # AS_SPLASHER
        1003: {"element": "Neutral"},                                                                # AS_SONICACCEL
        1004: {"effect": 600, "hit": 20, "sound": "effect/throw_knife", "element": "Poison"},        # AS_VENOMKNIFE

        # === 2-2: Cruzado (Crusader) ===
        248:  {"element": "Neutral"},                                                                # CR_TRUST
        249:  {"effect": 336, "sound": "effect/kyrie_guard", "element": "Neutral"},                  # CR_AUTOGUARD
        250:  {"effect": 246, "hit": 1, "sound": "effect/shield_charge", "element": "Weapon"},       # CR_SHIELDCHARGE
        251:  {"effect": 2008, "hit": 1, "sound": "effect/cru_shield", "element": "Weapon"},         # CR_SHIELDBOOMERANG
        252:  {"effect": 252, "sound": "effect/reflect_shield", "element": "Neutral"},               # CR_REFLECTSHIELD
        253:  {"effect": 245, "hit": 1, "sound": "effect/holy_cross", "element": "Holy"},            # CR_HOLYCROSS
        254:  {"cast": 16, "effect": 226, "hit": 152, "sound": "effect/grand_cross", "element": "Holy"}, # CR_GRANDCROSS
        255:  {"effect": 251, "sound": "effect/devotion", "element": "Neutral"},                     # CR_DEVOTION
        256:  {"effect": 248, "sound": "effect/providence", "element": "Neutral"},                   # CR_PROVIDENCE
        257:  {"effect": 222, "sound": "effect/defender", "element": "Neutral"},                     # CR_DEFENDER
        258:  {"effect": 250, "sound": "effect/spear_quicken", "element": "Neutral"},                # CR_SPEARQUICKEN
        1002: {"effect": 599, "sound": "effect/shrink", "element": "Neutral"},                       # CR_SHRINK

        # === 2-2: Monge (Monk) ===
        259:  {"element": "Neutral"},                                                                # MO_IRONHAND
        260:  {"element": "Neutral"},                                                                # MO_SPIRITSRECOVERY
        261:  {"effect": 261, "sound": "effect/call_spirits", "element": "Neutral"},                 # MO_CALLSPIRITS
        262:  {"effect": 253, "sound": "effect/흡기", "element": "Neutral"},                         # MO_ABSORBSPIRITS
        263:  {"effect": 329, "hit": 1, "sound": "effect/triple_attack", "element": "Weapon"},       # MO_TRIPLEATTACK
        264:  {"effect": 166, "sound": "effect/body_relocation", "element": "Neutral"},              # MO_BODYRELOCATION
        265:  {"element": "Neutral"},                                                                # MO_DODGE
        266:  {"effect": 267, "hit": 1, "sound": "effect/investigate", "element": "Weapon"},         # MO_INVESTIGATE
        267:  {"effect": 265, "hit": 1, "sound": "effect/finger_offensive", "element": "Weapon"},   # MO_FINGEROFFENSIVE
        268:  {"effect": 254, "sound": "effect/steel_body", "element": "Neutral"},                   # MO_STEELBODY
        269:  {"element": "Neutral"},                                                                # MO_BLADESTOP
        270:  {"cast": 12, "effect": 261, "sound": "effect/explosion_spirits", "element": "Neutral"},# MO_EXPLOSIONSPIRITS
        271:  {"cast": 12, "effect": 328, "hit": 266, "sound": "effect/asura_strike", "element": "Weapon"}, # MO_EXTREMITYFIST
        272:  {"cast": 263, "effect": 262, "hit": 273, "sound": "effect/chain_combo", "element": "Weapon"}, # MO_CHAINCOMBO
        273:  {"effect": 330, "hit": 1, "sound": "effect/combo_finish", "element": "Weapon"},        # MO_COMBOFINISH
        1015: {"element": "Neutral"},                                                                # MO_KITRANSLATION
        1016: {"effect": 514, "hit": 1, "sound": "effect/balkyoung", "element": "Weapon"},          # MO_BALKYOUNG

        # === 2-2: Sábio (Sage) ===
        274:  {"element": "Neutral"},                                                                # SA_ADVANCEDBOOK
        275:  {"element": "Neutral"},                                                                # SA_CASTCANCEL
        276:  {"effect": 244, "sound": "effect/magic_rod", "element": "Neutral"},                   # SA_MAGICROD
        277:  {"effect": 234, "sound": "effect/spell_breaker", "element": "Neutral"},               # SA_SPELLBREAKER
        278:  {"element": "Neutral"},                                                                # SA_FREECAST
        279:  {"element": "Neutral"},                                                                # SA_AUTOSPELL
        280:  {"effect": 255, "sound": "effect/endow_fire", "element": "Fire"},                      # SA_FLAMELAUNCHER
        281:  {"effect": 256, "sound": "effect/endow_water", "element": "Water"},                    # SA_FROSTWEAPON
        282:  {"effect": 257, "sound": "effect/endow_wind", "element": "Wind"},                      # SA_LIGHTNINGLOADER
        283:  {"effect": 258, "sound": "effect/endow_earth", "element": "Earth"},                    # SA_SEISMICWEAPON
        284:  {"element": "Neutral"},                                                                # SA_DRAGONOLOGY
        285:  {"cast": 225, "ground": 239, "sound": "effect/volcano", "element": "Fire"},            # SA_VOLCANO
        286:  {"cast": 236, "ground": 240, "sound": "effect/deluge", "element": "Water"},           # SA_DELUGE
        287:  {"cast": 237, "ground": 241, "sound": "effect/violent_gale", "element": "Wind"},      # SA_VIOLENTGALE
        288:  {"cast": 238, "ground": 242, "sound": "effect/land_protector", "element": "Neutral"}, # SA_LANDPROTECTOR
        289:  {"effect": 235, "sound": "effect/dispell", "element": "Neutral"},                     # SA_DISPELL
        290:  {"element": "Neutral"},                                                                # SA_ABRACADABRA
        1007: {"element": "Neutral"},                                                                # SA_CREATECON
        1008: {"effect": 256, "element": "Water"},                                                   # SA_ELEMENTWATER
        1017: {"effect": 258, "element": "Earth"},                                                   # SA_ELEMENTGROUND
        1018: {"effect": 255, "element": "Fire"},                                                    # SA_ELEMENTFIRE
        1019: {"effect": 257, "element": "Wind"},                                                    # SA_ELEMENTWIND

        # === 2-2: Arruaceiro (Rogue) ===
        210:  {"element": "Weapon"},                                                                 # RG_SNATCHER
        211:  {"effect": 268, "hit": 274, "sound": "effect/ef_coin2", "element": "Neutral"},         # RG_STEALCOIN
        212:  {"effect": 16, "hit": 275, "sound": "effect/backstab", "element": "Weapon"},           # RG_BACKSTAP
        213:  {"element": "Neutral"},                                                                # RG_TUNNELDRIVE
        214:  {"cast": 276, "hit": 1, "sound": "effect/raid", "element": "Weapon"},                  # RG_RAID
        215:  {"effect": 269, "sound": "effect/strip_weapon", "element": "Neutral"},                 # RG_STRIPWEAPON
        216:  {"effect": 270, "sound": "effect/strip_shield", "element": "Neutral"},                 # RG_STRIPSHIELD
        217:  {"effect": 271, "sound": "effect/strip_armor", "element": "Neutral"},                  # RG_STRIPARMOR
        218:  {"effect": 272, "sound": "effect/strip_helm", "element": "Neutral"},                   # RG_STRIPHELM
        219:  {"effect": 227, "hit": 1, "sound": "effect/intimidate", "element": "Weapon"},          # RG_INTIMIDATE
        220:  {"element": "Neutral"},                                                                # RG_GRAFFITI
        221:  {"element": "Neutral"},                                                                # RG_FLAGGRAFFITI
        222:  {"element": "Neutral"},                                                                # RG_CLEANER
        223:  {"element": "Neutral"},                                                                # RG_GANGSTER
        224:  {"element": "Neutral"},                                                                # RG_COMPULSION
        225:  {"element": "Neutral"},                                                                # RG_PLAGIARISM
        1005: {"effect": 602, "ground": 604, "sound": "effect/close_confine", "element": "Neutral"}, # RG_CLOSECONFINE

        # === 2-2: Alquimista (Alchemist) ===
        226:  {"element": "Neutral"},                                                                # AM_AXEMASTERY
        227:  {"element": "Neutral"},                                                                # AM_LEARNINGPOTION
        228:  {"element": "Neutral"},                                                                # AM_PHARMACY
        229:  {"ground": 302, "hit": 49, "sound": "effect/demonstration", "element": "Fire"},        # AM_DEMONSTRATION
        230:  {"effect": 298, "hit": 1, "sound": "effect/acid_terror", "element": "Neutral"},         # AM_ACIDTERROR
        231:  {"effect": 299, "sound": "effect/_heal_effect", "element": "Neutral"},                 # AM_POTIONPITCHER
        232:  {"element": "Neutral"},                                                                # AM_CANNIBALIZE
        233:  {"element": "Neutral"},                                                                # AM_SPHEREMINE
        234:  {"effect": 300, "sound": "effect/chemical_protection", "element": "Neutral"},          # AM_CP_WEAPON
        235:  {"effect": 300, "sound": "effect/chemical_protection", "element": "Neutral"},          # AM_CP_SHIELD
        236:  {"effect": 300, "sound": "effect/chemical_protection", "element": "Neutral"},          # AM_CP_ARMOR
        237:  {"effect": 300, "sound": "effect/chemical_protection", "element": "Neutral"},          # AM_CP_HELM
        238:  {"element": "Neutral"},                                                                # AM_BIOETHICS
        239:  {"element": "Neutral"},                                                                # AM_BIOTECHNOLOGY
        240:  {"element": "Neutral"},                                                                # AM_CREATECREATURE
        241:  {"element": "Neutral"},                                                                # AM_CULTIVATION
        242:  {"element": "Neutral"},                                                                # AM_FLAMECONTROL
        243:  {"element": "Neutral"},                                                                # AM_CALLHOMUN
        244:  {"element": "Neutral"},                                                                # AM_REST
        245:  {"element": "Neutral"},                                                                # AM_DRILLMASTER
        246:  {"element": "Neutral"},                                                                # AM_HEALHOMUN
        247:  {"element": "Neutral"},                                                                # AM_RESURRECTHOMUN
        446:  {"effect": 220, "sound": "effect/potion", "element": "Neutral"},                       # AM_BERSERKPITCHER
        496:  {"effect": 497, "sound": "effect/pharmacy", "element": "Neutral"},                     # AM_TWILIGHT1
        497:  {"effect": 498, "sound": "effect/pharmacy", "element": "Neutral"},                     # AM_TWILIGHT2
        498:  {"effect": 499, "sound": "effect/pharmacy", "element": "Neutral"},                     # AM_TWILIGHT3

        # === 2-2: Bardo e Odalisca (Bard / Dancer) ===
        304:  {"element": "Neutral"},                                                                # BD_ADAPTATION
        305:  {"element": "Neutral"},                                                                # BD_ENCORE
        306:  {"effect": 278, "ground": 278, "sound": "effect/lullaby", "element": "Neutral"},       # BD_LULLABY
        307:  {"effect": 279, "ground": 279, "sound": "effect/richmankim", "element": "Neutral"},    # BD_RICHMANKIM
        308:  {"effect": 280, "ground": 280, "sound": "effect/eternal_chaos", "element": "Neutral"}, # BD_ETERNALCHAOS
        309:  {"effect": 281, "ground": 281, "sound": "effect/drumbattlefield", "element": "Neutral"},# BD_DRUMBATTLEFIELD
        310:  {"effect": 282, "ground": 282, "sound": "effect/ringnibelungen", "element": "Neutral"},# BD_RINGNIBELUNGEN
        311:  {"effect": 283, "ground": 283, "sound": "effect/rokisweil", "element": "Neutral"},    # BD_ROKISWEIL
        312:  {"effect": 284, "ground": 284, "sound": "effect/intoabyss", "element": "Neutral"},     # BD_INTOABYSS
        313:  {"effect": 285, "ground": 285, "sound": "effect/siegfried", "element": "Neutral"},     # BD_SIEGFRIED
        315:  {"element": "Neutral"},                                                                # BA_MUSICALLESSON
        316:  {"effect": 2003, "hit": 1, "sound": "effect/musical_strike", "element": "Weapon"},     # BA_MUSICALSTRIKE
        317:  {"ground": 277, "sound": "effect/dissonance", "element": "Neutral"},                   # BA_DISSONANCE
        318:  {"effect": 295, "sound": "effect/frost_joke", "element": "Neutral"},                   # BA_FROSTJOKE
        319:  {"effect": 286, "ground": 286, "sound": "effect/whistle", "element": "Neutral"},       # BA_WHISTLE
        320:  {"effect": 287, "ground": 287, "sound": "effect/assassin_cross", "element": "Neutral"},# BA_ASSASSINCROSS
        321:  {"effect": 288, "ground": 288, "sound": "effect/poem_bragi", "element": "Neutral"},   # BA_POEMBRAGI
        322:  {"effect": 289, "ground": 289, "sound": "effect/apple_idun", "element": "Neutral"},   # BA_APPLEIDUN
        1010: {"effect": 606, "sound": "effect/pang_voice", "element": "Neutral"},                   # BA_PANGVOICE
        323:  {"element": "Neutral"},                                                                # DC_DANCINGLESSON
        324:  {"effect": 2003, "hit": 1, "sound": "effect/arrow_shuffle", "element": "Weapon"},     # DC_THROWARROW
        325:  {"ground": 290, "sound": "effect/ugly_dance", "element": "Neutral"},                   # DC_UGLYDANCE
        326:  {"effect": 296, "sound": "effect/scream", "element": "Neutral"},                       # DC_SCREAM
        327:  {"effect": 291, "ground": 291, "sound": "effect/humming", "element": "Neutral"},       # DC_HUMMING
        328:  {"effect": 292, "ground": 292, "sound": "effect/dont_forget_me", "element": "Neutral"},# DC_DONTFORGETME
        329:  {"effect": 293, "ground": 293, "sound": "effect/fortunes_kiss", "element": "Neutral"}, # DC_FORTUNEKISS
        330:  {"effect": 294, "ground": 294, "sound": "effect/service_for_you", "element": "Neutral"},# DC_SERVICEFORYOU
        1011: {"effect": 607, "sound": "effect/wink_charm", "element": "Neutral"}                    # DC_WINKCHARM
    }
    
    # Mapeamento detalhado e auditado para as Transclasses
    trans_class_map = {
        # === Lord Knight (LK) ===
        355: {"cast": 16, "effect": 367, "sound": "effect/aurablade", "element": "Neutral"},          # LK_AURABLADE
        356: {"effect": 336, "sound": "effect/kyrie_guard", "element": "Neutral"},                    # LK_PARRYING
        357: {"effect": 369, "sound": "effect/concentration", "element": "Neutral"},                  # LK_CONCENTRATION
        358: {"element": "Neutral"},                                                                  # LK_TENSIONRELAX
        359: {"effect": 368, "sound": "effect/berserk", "element": "Neutral"},                        # LK_BERSERK
        360: {"effect": 368, "sound": "effect/berserk", "element": "Neutral"},                        # LK_FURY
        397: {"cast": 339, "effect": 339, "hit": 80, "sound": "effect/spiral_pierce", "element": "Weapon"}, # LK_SPIRALPIERCE
        398: {"cast": 399, "hit": 1, "sound": "effect/headcrush", "element": "Weapon"},              # LK_HEADCRUSH
        399: {"cast": 400, "hit": 1, "sound": "effect/jointbeat", "element": "Weapon"},              # LK_JOINTBEAT

        # === High Priest (HP) ===
        361: {"effect": 440, "sound": "effect/assumptio", "element": "Holy"},                         # HP_ASSUMPTIO
        362: {"ground": 374, "sound": "effect/basilica", "element": "Holy"},                          # HP_BASILICA
        363: {"element": "Neutral"},                                                                  # HP_MEDITATIO
        481: {"element": "Neutral"},                                                                  # HP_MANARECHARGE

        # === High Wizard (HW) ===
        364: {"effect": 217, "sound": "effect/souldrain", "element": "Neutral"},                      # HW_SOULDRAIN
        365: {"effect": 380, "hit": 1, "sound": "effect/magiccrasher", "element": "Weapon"},          # HW_MAGICCRASHER
        366: {"cast": 16, "effect": 402, "sound": "effect/amplification", "element": "Neutral"},     # HW_MAGICPOWER
        400: {"effect": 401, "hit": 1, "sound": "effect/napalm_vulcan", "element": "Ghost"},          # HW_NAPALMVULCAN
        483: {"effect": 223, "ground": 224, "sound": "effect/ganbantein", "element": "Neutral"},     # HW_GANBANTEIN
        484: {"ground": 522, "sound": "effect/gravitation", "element": "Earth"},                      # HW_GRAVITATION

        # === Paladin (PA) ===
        367: {"effect": 365, "hit": 152, "sound": "effect/pressure", "element": "Holy"},             # PA_PRESSURE
        368: {"effect": 366, "hit": 1, "sound": "effect/sacrifice", "element": "Neutral"},           # PA_SACRIFICE
        369: {"effect": 370, "ground": 370, "sound": "effect/gospel", "element": "Holy"},             # PA_GOSPEL
        480: {"effect": 2008, "hit": 1, "sound": "effect/shieldchain", "element": "Neutral"},         # PA_SHIELDCHAIN

        # === Champion (CH) ===
        370: {"hit": 376, "sound": "effect/palmpush", "element": "Weapon"},                           # CH_PALMSTRIKE
        371: {"cast": 263, "effect": 377, "hit": 1, "sound": "effect/tigerfist", "element": "Weapon"}, # CH_TIGERFIST
        372: {"effect": 512, "hit": 1, "sound": "effect/chaincrush", "element": "Weapon"},           # CH_CHAINCRUSH
        401: {"cast": 402, "effect": 12, "sound": "effect/zen", "element": "Neutral"},                # CH_SOULCOLLECT

        # === Professor (PF) ===
        373: {"cast": 378, "effect": 383, "hit": 379, "sound": "effect/hpconversion", "element": "Neutral"}, # PF_HPCONVERSION
        374: {"effect": 384, "hit": 385, "sound": "effect/soulchange", "element": "Neutral"},         # PF_SOULCHANGE
        375: {"effect": 406, "sound": "effect/soulburn", "element": "Neutral"},                      # PF_SOULBURN
        402: {"effect": 403, "sound": "effect/mindbreaker", "element": "Neutral"},                   # PF_MINDBREAKER
        403: {"effect": 505, "sound": "effect/memorize", "element": "Neutral"},                      # PF_MEMORIZE
        404: {"ground": 405, "sound": "effect/fogwall", "element": "Earth"},                          # PF_FOGWALL
        405: {"ground": 404, "sound": "effect/spiderweb", "element": "Neutral"},                     # PF_SPIDERWEB
        482: {"effect": 521, "sound": "effect/doublecasting", "element": "Neutral"},                  # PF_DOUBLECASTING

        # === Assassin Cross (ASC) ===
        376: {"element": "Neutral"},                                                                  # ASC_KATAR
        377: {"element": "Neutral"},                                                                  # ASC_HALLUCINATION
        378: {"effect": 493, "sound": "effect/edp", "element": "Poison"},                             # ASC_EDP
        379: {"effect": 361, "hit": 1, "sound": "effect/souldestroyer", "element": "Weapon"},         # ASC_BREAKER
        406: {"effect": 409, "hit": 1, "sound": "effect/meteorassault", "element": "Weapon"},         # ASC_METEORASSAULT
        407: {"element": "Neutral"},                                                                  # ASC_CDP

        # === Sniper (SN) ===
        380: {"effect": 386, "sound": "effect/truesight", "element": "Neutral"},                      # SN_SIGHT
        381: {"effect": 387, "hit": 1, "sound": "effect/falconassault", "element": "Neutral"},        # SN_FALCONASSAULT
        382: {"cast": 16, "effect": 2003, "hit": 388, "sound": "effect/sharpshooting", "element": "Weapon"}, # SN_SHARPSHOOTING
        383: {"effect": 389, "sound": "effect/windwalk", "element": "Neutral"},                      # SN_WINDWALK

        # === Mastersmith / Whitesmith (WS) ===
        384: {"effect": 390, "sound": "effect/meltdown", "element": "Neutral"},                      # WS_MELTDOWN
        385: {"element": "Neutral"},                                                                  # WS_CREATECOIN
        386: {"element": "Neutral"},                                                                  # WS_CREATENUGGET
        387: {"effect": 391, "sound": "effect/cartboost", "element": "Neutral"},                     # WS_CARTBOOST
        388: {"element": "Neutral"},                                                                  # WS_SYSTEMCREATE
        477: {"element": "Neutral"},                                                                  # WS_WEAPONREFINE
        485: {"effect": 518, "hit": 1, "sound": "effect/carttermination", "element": "Weapon"},      # WS_CARTTERMINATION
        486: {"effect": 128, "sound": "effect/overthrust", "element": "Neutral"},                    # WS_OVERTHRUSTMAX

        # === Stalker (ST) ===
        389: {"cast": 501, "sound": "effect/chasewalk", "element": "Neutral"},                        # ST_CHASEWALK
        390: {"effect": 392, "sound": "effect/rejectsword", "element": "Neutral"},                    # ST_REJECTSWORD
        391: {"element": "Neutral"},                                                                  # ST_STEALBACKPACK
        475: {"effect": 496, "sound": "effect/preserve", "element": "Neutral"},                      # ST_PRESERVE
        476: {"effect": 495, "sound": "effect/fullstrip", "element": "Neutral"},                     # ST_FULLSTRIP

        # === Creator (CR) ===
        392: {"element": "Neutral"},                                                                  # CR_ALCHEMY
        393: {"element": "Neutral"},                                                                  # CR_SYNTHESISPOTION
        478: {"effect": 299, "sound": "_heal_effect", "element": "Neutral"},                         # CR_SLIMPITCHER
        479: {"effect": 300, "sound": "effect/chemical_protection", "element": "Neutral"},          # CR_FULLPROTECTION
        490: {"effect": 537, "hit": 49, "sound": "effect/acid_terror", "element": "Neutral"},         # CR_ACIDDEMONSTRATION
        491: {"element": "Neutral"},                                                                  # CR_CULTIVATION

        # === Clown & Gypsy (CG) ===
        394: {"effect": 393, "hit": 1, "sound": "effect/arrowvulcan", "element": "Weapon"},          # CG_ARROWVULCAN
        395: {"effect": 394, "ground": 394, "sound": "effect/moonlit", "element": "Neutral"},        # CG_MOONLIT
        396: {"effect": 395, "hit": 396, "sound": "effect/marionette", "element": "Neutral"},        # CG_MARIONETTE
        487: {"effect": 500, "sound": "effect/longingfreedom", "element": "Neutral"},                 # CG_LONGINGFREEDOM
        488: {"effect": 517, "ground": 517, "sound": "effect/hermode", "element": "Neutral"},        # CG_HERMODE
        489: {"effect": 500, "sound": "effect/tarotcard", "element": "Neutral"}                      # CG_TAROTCARD
    }
    
    # Aplica na tabela de skills
    for sid, sdata in first_class_map.items():
        sid_str = str(sid)
        if sid_str not in skills_out:
            skills_out[sid_str] = {}
        skills_out[sid_str].update(sdata)

    for sid, sdata in second_class_map.items():
        sid_str = str(sid)
        if sid_str not in skills_out:
            skills_out[sid_str] = {}
        skills_out[sid_str].update(sdata)

    for sid, sdata in trans_class_map.items():
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

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

consts = load_skill_consts()
sk_effects = load_skill_effects()
elements = load_elements()

# Group by job class
jobs = {
    "Knight": [
        "KN_SPEARMASTERY", "KN_PIERCE", "KN_BRANDISHSPEAR", "KN_SPEARSTAB", "KN_SPEARBOOMERANG",
        "KN_TWOHANDQUICKEN", "KN_AUTOCOUNTER", "KN_BOWLINGBASH", "KN_RIDING", "KN_CAVALIERMASTERY",
        "KN_ONEHAND", "KN_CHARGEATK"
    ],
    "Priest": [
        "PR_MACEMASTERY", "PR_IMPOSITIO", "PR_SUFFRAGIUM", "PR_ASPERSIO", "PR_BENEDICTIO",
        "PR_SANCTUARY", "PR_SLOWPOISON", "PR_STRECOVERY", "PR_KYRIE", "PR_MAGNIFICAT",
        "PR_GLORIA", "PR_LEXDIVINA", "PR_TURNUNDEAD", "PR_LEXAETERNA", "PR_MAGNUS", "PR_REDEMPTIO"
    ],
    "Wizard": [
        "WZ_FIREPILLAR", "WZ_SIGHTRASHER", "WZ_FIREIVY", "WZ_METEOR", "WZ_JUPITEL",
        "WZ_VERMILION", "WZ_WATERBALL", "WZ_ICEWALL", "WZ_FROSTNOVA", "WZ_STORMGUST",
        "WZ_EARTHSPIKE", "WZ_HEAVENDRIVE", "WZ_QUAGMIRE", "WZ_ESTIMATION", "WZ_SIGHTBLASTER"
    ],
    "Blacksmith": [
        "BS_IRON", "BS_STEEL", "BS_ENCHANTEDSTONE", "BS_ORIDEOCON", "BS_DAGGER", "BS_SWORD",
        "BS_TWOHANDSWORD", "BS_AXE", "BS_MACE", "BS_KNUCKLE", "BS_SPEAR", "BS_HILTBINDING",
        "BS_FINDINGORE", "BS_WEAPONRESEARCH", "BS_REPAIRWEAPON", "BS_SKINTEMPER", "BS_HAMMERFALL",
        "BS_ADRENALINE", "BS_WEAPONPERFECT", "BS_OVERTHRUST", "BS_MAXIMIZE", "BS_ADRENALINE2",
        "BS_UNFAIRLYTRICK", "BS_GREED"
    ],
    "Hunter": [
        "HT_SKIDTRAP", "HT_LANDMINE", "HT_ANKLESNARE", "HT_SHOCKWAVE", "HT_SANDMAN",
        "HT_FLASHER", "HT_FREEZINGTRAP", "HT_BLASTMINE", "HT_CLAYMORETRAP", "HT_REMOVETRAP",
        "HT_TALKIEBOX", "HT_BEASTBANE", "HT_FALCON", "HT_STEELCROW", "HT_BLITZBEAT",
        "HT_DETECTING", "HT_SPRINGTRAP", "HT_POWER", "HT_PHANTASMIC"
    ],
    "Assassin": [
        "AS_RIGHT", "AS_LEFT", "AS_KATAR", "AS_CLOAKING", "AS_SONICBLOW", "AS_GRIMTOOTH",
        "AS_ENCHANTPOISON", "AS_POISONREACT", "AS_VENOMDUST", "AS_SPLASHER", "AS_SONICACCEL", "AS_VENOMKNIFE"
    ],
    "Crusader": [
        "CR_TRUST", "CR_AUTOGUARD", "CR_SHIELDCHARGE", "CR_SHIELDBOOMERANG", "CR_REFLECTSHIELD",
        "CR_HOLYCROSS", "CR_GRANDCROSS", "CR_DEVOTION", "CR_PROVIDENCE", "CR_DEFENDER",
        "CR_SPEARQUICKEN", "CR_SHRINK"
    ],
    "Monk": [
        "MO_IRONHAND", "MO_SPIRITSRECOVERY", "MO_CALLSPIRITS", "MO_ABSORBSPIRITS", "MO_TRIPLEATTACK",
        "MO_BODYRELOCATION", "MO_DODGE", "MO_INVESTIGATE", "MO_FINGEROFFENSIVE", "MO_STEELBODY",
        "MO_BLADESTOP", "MO_EXPLOSIONSPIRITS", "MO_EXTREMITYFIST", "MO_CHAINCOMBO", "MO_COMBOFINISH",
        "MO_KITRANSLATION", "MO_BALKYOUNG"
    ],
    "Sage": [
        "SA_ADVANCEDBOOK", "SA_CASTCANCEL", "SA_MAGICROD", "SA_SPELLBREAKER", "SA_FREECAST",
        "SA_AUTOSPELL", "SA_FLAMELAUNCHER", "SA_FROSTWEAPON", "SA_LIGHTNINGLOADER", "SA_SEISMICWEAPON",
        "SA_DRAGONOLOGY", "SA_VOLCANO", "SA_DELUGE", "SA_VIOLENTGALE", "SA_LANDPROTECTOR",
        "SA_DISPELL", "SA_ABRACADABRA", "SA_CREATECON", "SA_ELEMENTWATER", "SA_ELEMENTGROUND",
        "SA_ELEMENTFIRE", "SA_ELEMENTWIND"
    ],
    "Rogue": [
        "RG_SNATCHER", "RG_STEALCOIN", "RG_BACKSTAP", "RG_TUNNELDRIVE", "RG_RAID",
        "RG_STRIPWEAPON", "RG_STRIPSHIELD", "RG_STRIPARMOR", "RG_STRIPHELM", "RG_INTIMIDATE",
        "RG_GRAFFITI", "RG_FLAGGRAFFITI", "RG_CLEANER", "RG_GANGSTER", "RG_COMPULSION",
        "RG_PLAGIARISM", "RG_CLOSECONFINE"
    ],
    "Alchemist": [
        "AM_AXEMASTERY", "AM_LEARNINGPOTION", "AM_PHARMACY", "AM_DEMONSTRATION", "AM_ACIDTERROR",
        "AM_POTIONPITCHER", "AM_CANNIBALIZE", "AM_SPHEREMINE", "AM_CP_WEAPON", "AM_CP_SHIELD",
        "AM_CP_ARMOR", "AM_CP_HELM", "AM_BIOETHICS", "AM_BIOTECHNOLOGY", "AM_CREATECREATURE",
        "AM_CULTIVATION", "AM_FLAMECONTROL", "AM_CALLHOMUN", "AM_REST", "AM_DRILLMASTER",
        "AM_HEALHOMUN", "AM_RESURRECTHOMUN", "AM_BERSERKPITCHER", "AM_TWILIGHT1", "AM_TWILIGHT2",
        "AM_TWILIGHT3"
    ],
    "Bard_Dancer": [
        "BA_MUSICALLESSON", "BA_MUSICALSTRIKE", "BA_DISSONANCE", "BA_FROSTJOKE", "BA_WHISTLE",
        "BA_ASSASSINCROSS", "BA_POEMBRAGI", "BA_APPLEIDUN", "BA_PANGVOICE",
        "DC_DANCINGLESSON", "DC_THROWARROW", "DC_UGLYDANCE", "DC_SCREAM", "DC_HUMMING",
        "DC_DONTFORGETME", "DC_FORTUNEKISS", "DC_SERVICEFORYOU", "DC_WINKCHARM",
        "BD_ADAPTATION", "BD_ENCORE", "BD_LULLABY", "BD_RICHMANKIM", "BD_ETERNALCHAOS",
        "BD_DRUMBATTLEFIELD", "BD_RINGNIBELUNGEN", "BD_ROKISWEIL", "BD_INTOABYSS", "BD_SIEGFRIED"
    ]
}

for job, skill_list in jobs.items():
    print(f"=== {job} ({len(skill_list)} skills) ===")
    for sk in skill_list:
        sid = consts.get(sk)
        eff = sk_effects.get(sk, "{}")
        elem = elements.get(sid, "Neutral")
        print(f"  {sid:4d} | {sk:20s} | Element: {elem:7s} | Effect: {eff}")

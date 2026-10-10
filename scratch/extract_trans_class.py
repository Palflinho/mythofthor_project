import os, re, json

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

trans_prefixes = (
    'LK_', 'HW_', 'HP_', 'SN_', 'WS_', 'ASC_', 'AS_', 'PA_', 'PF_', 'CH_', 'CR_', 'CG_', 'ST_'
)

# Specifically transclass skills (IDs generally between 355 and 500)
trans_skills = {}
for name, sid in sorted(consts.items(), key=lambda x: x[1]):
    if (350 <= sid <= 505) and any(name.startswith(p) for p in trans_prefixes):
        eff = sk_effects.get(name, '{}')
        elem = elements.get(sid, "Neutral")
        trans_skills[sid] = {"name": name, "effect": eff, "element": elem}
        print(f"{sid:4d} | {name:22s} | Element: {elem:7s} | Effect: {eff}")

print(f"\nTotal skills transclasse encontradas: {len(trans_skills)}")

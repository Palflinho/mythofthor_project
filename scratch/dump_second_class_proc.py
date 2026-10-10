import os, re, json

JS_DIR = r"\\wsl.localhost\Ubuntu\home\palf\references\roBrowserLegacy\src\DB"
with open(os.path.join(JS_DIR, "Effects", "EffectTable.js"), "r", encoding="latin-1") as f:
    text = f.read()

pattern = re.compile(r'\n\t(?:(\d+)|\'([^\']+)\'|(\w+)):\s*\[', re.S)
matches = list(pattern.finditer(text))
effects = {}
for i, m in enumerate(matches):
    key = m.group(1) or m.group(2) or m.group(3)
    start = m.end()
    end = matches[i+1].start() if i+1 < len(matches) else len(text)
    block = text[start:end]
    depth = 1
    bracket_end = -1
    for idx, ch in enumerate(block):
        if ch == '[': depth += 1
        elif ch == ']':
            depth -= 1
            if depth == 0:
                bracket_end = idx
                break
    effects[key] = block[:bracket_end].strip() if bracket_end != -1 else block.strip()

important_eids = [
    # Grand Cross, Asura Strike, Auto Guard, Defender, Reflect Shield, Absorb Spirits, Explosion Spirits, Combo
    "226", "328", "266", "336", "222", "252", "253", "261", "262", "263", "265", "274", "276",
    # Sanctuary, Magnus Exorcismus, Fire Pillar, Jupitel Thunder, Waterball, Sonic Blow
    "318", "319", "138", "93", "94", "116", "117", "121", "122", "143",
    # Projectiles: Spear Boomerang, Acid Terror, Venom Knife
    "ef_spear_projectile", "298", "600",
    # Traps & Bard/Dancer
    "278_ground", "288_ground", "289_ground", "291_ground", "294_ground"
]

out_lines = []
for eid in important_eids:
    out_lines.append("="*70)
    out_lines.append(f"EFFECT ID: {eid}")
    out_lines.append("="*70)
    out_lines.append(effects.get(eid, "[NAO ENCONTRADO]"))
    out_lines.append("\n")

with open("scratch/second_class_proc_details.txt", "w", encoding="utf-8") as f:
    f.write("\n".join(out_lines))

print(f"Salvo scratch/second_class_proc_details.txt com {len(important_eids)} efeitos!")

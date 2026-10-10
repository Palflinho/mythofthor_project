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

# Key 2nd class effect IDs
key_effects = [
    # Knight
    "70", "80", "130", "131", "144", "147", "148", "149", "150", "151", "ef_spear_projectile",
    # Priest
    "75", "76", "78", "83", "84", "85", "86", "87", "88", "91", "112", "113", "136", "318", "319",
    # Wizard
    "62", "74", "79", "89", "90", "92", "93", "94", "95", "96", "97", "116", "117", "138", "142",
    # Blacksmith
    "69", "98", "98_beforecast", "101", "102", "103", "104", "128",
    # Hunter
    "99", "100", "106", "107", "108", "111", "115", "119", "139", "145", "146",
    # Assassin
    "120", "121", "122", "123", "124", "126", "127", "129", "132", "143", "171", "600",
    # Crusader
    "222", "226", "245", "246", "248", "249", "250", "251", "252", "336",
    # Monk
    "166", "253", "254", "261", "262", "263", "265", "266", "267", "273", "328", "329",
    # Sage
    "234", "235", "239", "240", "241", "242", "244", "255", "256", "257", "258",
    # Rogue
    "227", "268", "269", "270", "271", "272", "274", "275", "276", "604",
    # Alchemist
    "298", "300", "302",
    # Bard / Dancer
    "278", "278_ground", "279", "279_ground", "280", "280_ground", "281", "281_ground",
    "282", "282_ground", "283", "283_ground", "284", "284_ground", "285", "285_ground",
    "286", "286_ground", "287", "287_ground", "288", "288_ground", "289", "289_ground",
    "291", "291_ground", "292", "292_ground", "293", "293_ground", "294", "294_ground",
    "295", "296", "606", "607"
]

summary = {}
for eid in key_effects:
    raw = effects.get(eid, "")
    obj_pattern = re.compile(r'\{([^{}]*(?:\{[^{}]*\}[^{}]*)*)\}', re.S)
    objs = obj_pattern.findall(raw)
    parts_info = []
    for obj in objs:
        type_m = re.search(r"type:\s*'([^']+)'", obj)
        t = type_m.group(1) if type_m else ("WAV" if "wav:" in obj else "3D")
        f_m = re.search(r"(?:file|textureName|absoluteSpriteName):\s*'([^']+)'", obj)
        f = f_m.group(1) if f_m else ""
        w_m = re.search(r"wav:\s*'([^']+)'", obj)
        w = w_m.group(1) if w_m else ""
        
        info = {"type": t, "file": f, "wav": w}
        for k in ['duration', 'alphaMax', 'blendMode', 'size', 'sizeStart', 'sizeEnd', 'bottomSize', 'topSize', 'height', 'posZ', 'posz', 'animation', 'duplicate', 'rotate', 'fade', 'toSrc', 'repeat']:
            m = re.search(rf"\b{k}:\s*([^,\n]+)", obj)
            if m:
                info[k] = m.group(1).strip()
        parts_info.append(info)
    summary[eid] = parts_info

with open("scratch/second_class_recipes_summary.json", "w", encoding="utf-8") as out:
    json.dump(summary, out, indent=2)

print(f"Resumo salvo com {len(summary)} efeitos detalhados!")

import os, re

JS_DIR = r"\\wsl.localhost\Ubuntu\home\palf\references\roBrowserLegacy\src\DB"
with open(os.path.join(JS_DIR, "Effects", "EffectTable.js"), "r", encoding="latin-1") as f:
    lines = f.readlines()

missing = ['225', '236', '237', '238', '299', '330', '497', '498', '499', '514', '541', '599', '601', '602', 'ef_anklesnare']

for target in missing:
    found_lines = []
    for idx, l in enumerate(lines):
        if re.search(rf"\b{target}\b", l):
            found_lines.append((idx + 1, l.strip()))
    print(f"=== TARGET {target} ===")
    for num, txt in found_lines[:5]:
        print(f"  L{num}: {txt}")

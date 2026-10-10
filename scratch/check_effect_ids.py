import os, re

with open(r"\\wsl.localhost\Ubuntu\home\palf\references\roBrowserLegacy\src\DB\Effects\EffectTable.js", "r", encoding="latin-1") as f:
    txt = f.read()

targets = [328, 510, 266, 226, 89, 90, 112, 83, 319, 113, 318, 148, 147, 144, 70, 150, 151, 80, 130, 149]
for tid in targets:
    m = re.search(rf"\n\t{tid}:\s*\[", txt)
    print(f"Effect {tid}: {'FOUND' if m else 'NOT FOUND'}")

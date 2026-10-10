import os, re

JS_DIR = r"\\wsl.localhost\Ubuntu\home\palf\references\roBrowserLegacy\src\DB"
with open(os.path.join(JS_DIR, "Skills", "SkillEffect.js"), "r", encoding="latin-1") as f:
    txt = f.read()

for m in re.finditer(r".{0,40}anklesnare.{0,40}", txt, re.IGNORECASE):
    print(m.group(0))

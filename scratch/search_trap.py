import re

with open(r"\\wsl.localhost\Ubuntu\home\palf\references\roBrowserLegacy\src\DB\Effects\EffectTable.js", "r", encoding="latin-1") as f:
    txt = f.read()

for m in re.finditer(r"\n\t(?:'([^']+)'|(\d+)):\s*\[[^\n]*trap", txt, re.IGNORECASE):
    print(m.group(0))

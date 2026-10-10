import sys
sys.stdout.reconfigure(encoding='utf-8', errors='replace')
with open(r"\\wsl.localhost\Ubuntu\home\palf\references\roBrowserLegacy\src\DB\Effects\EffectTable.js", "r", encoding="latin-1") as f:
    txt = f.read()

for k in ['277_ground', '278_ground', '288_ground', '290_ground']:
    pos = txt.find(f"\t'{k}':")
    if pos != -1:
        print(f"=== {k} ===")
        print(txt[pos:pos+400])

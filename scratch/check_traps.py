import re

with open(r"\\wsl.localhost\Ubuntu\home\palf\references\roBrowserLegacy\src\DB\Effects\EffectTable.js", "r", encoding="latin-1") as f:
    txt = f.read()

traps = ['ef_ht_landmine', 'ef_anklesnare', 'white_pulse', 'enemy_hit_normal1', 'maximize_power_sounds', 'ef_bs_adrenaline2', 'ef_greed_sound']
for t in traps:
    m = re.search(rf"\b{t}\b", txt)
    print(f"{t}: {'FOUND' if m else 'NOT FOUND'}")

with open(r"\\wsl.localhost\Ubuntu\home\palf\references\roBrowserLegacy\src\DB\Effects\EffectTable.js", "r", encoding="latin-1") as f:
    txt = f.read()

for k in ['white_pulse', 'enemy_hit_normal1', 'maximize_power_sounds', 'ef_bs_adrenaline2', 'ef_greed_sound']:
    pos = txt.find(f"\t{k}:")
    if pos != -1:
        print(f"=== {k} ===")
        print(txt[pos:pos+300])

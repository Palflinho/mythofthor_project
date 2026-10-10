with open(r"\\wsl.localhost\Ubuntu\home\palf\references\roBrowserLegacy\src\DB\Effects\EffectTable.js", "r", encoding="latin-1") as f:
    txt = f.read()

pos = txt.find("ef_ht_landmine")
if pos != -1:
    print(txt[pos-20:pos+600])

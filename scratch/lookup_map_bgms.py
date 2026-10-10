import os

mp3_table = r"E:\Dev\Ragnarok_Assets\data\mp3nametable.txt"
bgms = {}
with open(mp3_table, "r", encoding="latin-1") as f:
    for line in f:
        line = line.strip()
        if not line or line.startswith("//"):
            continue
        parts = line.split("#")
        if len(parts) >= 2:
            map_name = parts[0].strip().replace(".gat", "").replace(".rsw", "").lower()
            bgm_name = parts[1].strip()
            bgms[map_name] = bgm_name

target_maps = [
    # Cidades das classes
    "izlude", "geffen", "payon", "morocc", "alberta",
    # Interiores
    "izlude_in", "geffen_in", "payon_in02", "moc_prydb1", "alberta_in",
    "gef_tower", "in_hunter", "prt_monk", "moc_ruins", "alde_alche", "comodo", "yuno_in02", "valkyrie",
    # Cavernas iniciais
    "prt_sewb1", "pay_dun00", "pay_dun01", "iz_dun00", "iz_dun01",
    # Campos
    "prt_fild01", "prt_fild02", "prt_fild03", "prt_fild04", "prt_fild07", "prt_fild09", "prt_fild10", "prt_fild11",
    "pay_fild01", "pay_fild08", "pay_fild10", "gef_fild00", "gef_fild04", "gef_fild07", "moc_fild01", "moc_fild07"
]

print("=== MAP BGM LOOKUP ===")
for m in target_maps:
    bgm = bgms.get(m.lower(), "01.mp3")
    bgm_path = f"E:/Dev/Ragnarok_Assets/BGM/{bgm}"
    exists = os.path.exists(bgm_path)
    print(f"{m:15s} -> {bgm:12s} (arquivo existe: {exists})")

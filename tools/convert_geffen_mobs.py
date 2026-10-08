import os
import sys
sys.path.append(r"D:\Repositórios\mythofthor_project\tools")
from extract_and_convert_sprites import decode_spr, build_spritesheet

out_dir = r"D:\Ragnarok_Assets\Sprites\PNG"
m_dir = r"D:\Ragnarok_Assets\sprite\몬스터"
npc_dir = r"D:\Ragnarok_Assets\sprite\npc"

# Indexa uma única vez
file_index = {}
for d in [m_dir, npc_dir]:
    if os.path.exists(d):
        for f in os.listdir(d):
            if f.lower().endswith('.spr'):
                file_index[f.lower()] = os.path.join(d, f)

geffen_targets = {
    'orc_warrior': 'orc_warrior',
    'orc_lady': 'orc_lady',
    'orc_baby': 'orc_baby',
    'orc_zombie': 'orc_zombie',
    'orc_skeleton': 'orc_skeleton',
    'orc_archer': 'orc_archer',
    'high_orc': 'high_orc',
    'whisper': 'whisper',
    'ghoul': 'ghoul',
    'nightmare': 'nightmare',
    'hunter_fly': 'hunter_fly',
    'jakk': 'jakk',
    'marionette': 'marionette',
    'deviruchi': 'deviruchi',
    'doppelganger': 'doppelganger',
    'orc_hero': 'orc_hero',
    'orc_lord': 'orc_lord'
}

converted = 0
for name, query in geffen_targets.items():
    out_sheet = os.path.join(out_dir, f"{name}_sheet.png")
    if os.path.exists(out_sheet):
        print(f"[-] {name} ja convertido.")
        converted += 1
        continue
        
    found_spr = file_index.get(f"{query}.spr") or file_index.get(f"dr_{query}.spr")
    if not found_spr:
        for k, v in file_index.items():
            if query in k:
                found_spr = v
                break
                
    if found_spr:
        try:
            frames, meta = decode_spr(found_spr)
            if frames:
                build_spritesheet(frames, out_sheet)
                print(f"[+] {name:15} -> {out_sheet} ({len(frames)} frames) de {os.path.basename(found_spr)}")
                converted += 1
        except Exception as e:
            print(f"[ERRO] {name}: {e}")
    else:
        print(f"[?] {name} .spr nao localizado no indice.")

print(f"=== Total de Monstros de Geffen convertidos: {converted}/{len(geffen_targets)} ===")

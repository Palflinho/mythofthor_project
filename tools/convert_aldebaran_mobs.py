import os
import sys
sys.path.append(r"D:\Repositórios\mythofthor_project\tools")
from extract_and_convert_sprites import decode_spr, build_spritesheet

out_dir = r"D:\Ragnarok_Assets\Sprites\PNG"
m_dir = r"D:\Ragnarok_Assets\sprite\몬스터"
npc_dir = r"D:\Ragnarok_Assets\sprite\npc"

# Indexa arquivos .spr
file_index = {}
for d in [m_dir, npc_dir]:
    if os.path.exists(d):
        for f in os.listdir(d):
            if f.lower().endswith('.spr'):
                file_index[f.lower()] = os.path.join(d, f)

aldebaran_targets = {
    'alarm': 'alarm.spr',
    'clock': 'clock.spr',
    'bathory': 'bathory.spr',
    'rideword': 'rideword.spr',
    'punk': 'punk.spr',
    'arclouse': 'arclouse.spr',
    'owl_baron': 'owl_baron.spr',
    'owl_duke': 'owl_duke.spr'
}

converted = 0
for name, spr_name in aldebaran_targets.items():
    out_sheet = os.path.join(out_dir, f"{name}_sheet.png")
    if os.path.exists(out_sheet):
        print(f"[-] {name} ja convertido.")
        converted += 1
        continue
        
    found_spr = file_index.get(spr_name.lower())
    if not found_spr:
        for k, v in file_index.items():
            if spr_name.lower().replace('.spr', '') in k:
                found_spr = v
                break
                
    if found_spr:
        try:
            frames, meta = decode_spr(found_spr)
            if frames:
                build_spritesheet(frames, out_sheet)
                print(f"[+] {name:15} -> {out_sheet} ({len(frames)} frames) de {os.path.basename(found_spr)}")
                converted += 1
            else:
                print(f"[!] {name}: nenhum frame decodificado.")
        except Exception as e:
            print(f"[x] Erro em {name}: {e}")
    else:
        print(f"[?] Nao encontrado: {spr_name}")

print(f"\nTotal monstros de Aldebaran prontos: {converted}/{len(aldebaran_targets)}")

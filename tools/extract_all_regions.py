"""
Myth of Thor - Extrator e Compilador de Regiões do Mundo
---------------------------------------------------------
Extrai arquivos .rsw, .gnd, .gat de data.grf para as regiões do roadmap
e compila diretamente para Wavefront .obj/.mtl com texturas e modelos compartilhados.
"""

import os
import sys
import subprocess

GRFCL_DIR = r"D:\Downloads\GrfCL"
GRFCL_EXE = os.path.join(GRFCL_DIR, "GrfCL.exe")
GRF_FILE = r"D:\Downloads\data.grf"
WORLD_DIR = r"D:\Ragnarok_Assets\World"
SHARED_DIR = r"D:\Ragnarok_Assets\shared\data"

REGIONS = {
    "Izlude": [
        "izlude",
        "iz_dun00", "iz_dun01", "iz_dun02", "iz_dun03", "iz_dun04"
    ],
    "Geffen": [
        "geffen", "gef_fild10",
        "orcsdun01", "orcsdun02",
        "gef_dun00", "gef_dun01", "gef_dun02", "gef_dun03"
    ],
    "Payon": [
        "payon", "pay_arche",
        "pay_dun00", "pay_dun01", "pay_dun02", "pay_dun03", "pay_dun04"
    ],
    "Morroc": [
        "morocc",
        "moc_pryd01", "moc_pryd02", "moc_pryd03", "moc_pryd04", "moc_pryd05", "moc_pryd06",
        "in_sphinx1", "in_sphinx2", "in_sphinx3", "in_sphinx4", "in_sphinx5"
    ],
    "Alberta": [
        "alberta",
        "treasure01", "treasure02",
        "tur_dun01", "tur_dun02", "tur_dun03", "tur_dun04"
    ]
}


def extract_region_maps(region_name):
    if region_name not in REGIONS:
        print(f"Região '{region_name}' não encontrada no roadmap.")
        return False
        
    maps = REGIONS[region_name]
    region_path = os.path.join(WORLD_DIR, region_name)
    os.makedirs(region_path, exist_ok=True)
    
    print(f"\n=== Extraindo mapas da Região: {region_name} ({len(maps)} mapas) ===")
    
    for m in maps:
        map_dir = os.path.join(region_path, m)
        os.makedirs(map_dir, exist_ok=True)
        
        # Extrai .rsw, .gnd, .gat
        for ext in [".rsw", ".gnd", ".gat"]:
            target_grf = f"data\\{m}{ext}"
            cmd = [
                GRFCL_EXE,
                "-breakOnExceptions", "false",
                "-encoding", "949",
                "-open", GRF_FILE,
                "-extractFiles", target_grf, map_dir
            ]
            p = subprocess.Popen(cmd, cwd=GRFCL_DIR, creationflags=0x00000010)
            p.wait()
            
        print(f"  [OK] {m} extraído para {map_dir}")
        
    return True


if __name__ == "__main__":
    target_reg = sys.argv[1] if len(sys.argv) > 1 else "Izlude"
    extract_region_maps(target_reg)

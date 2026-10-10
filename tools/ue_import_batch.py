"""
Myth of Thor - Importador em Lote de Mapas para Unreal Editor
Executa a importacao de multiplos mapas em uma unica sessao do UnrealEditor-Cmd.
"""
import os
import sys
import time

# Garante acesso aos modulos locais
TOOLS_DIR = os.path.dirname(os.path.abspath(__file__))
if TOOLS_DIR not in sys.path:
    sys.path.insert(0, TOOLS_DIR)

import ue_import_map
import unreal

def load_bgm_table():
    mp3_table = r"E:\Dev\Ragnarok_Assets\data\mp3nametable.txt"
    bgms = {}
    if os.path.exists(mp3_table):
        with open(mp3_table, "r", encoding="latin-1") as f:
            for line in f:
                line = line.strip()
                if not line or line.startswith("//"):
                    continue
                parts = line.split("#")
                if len(parts) >= 2:
                    m = parts[0].strip().replace(".gat", "").replace(".rsw", "").lower()
                    b = parts[1].strip().replace("\\", "/").split("/")[-1]
                    bgms[m] = b
    return bgms

def main():
    bgm_map = load_bgm_table()
    
    # Se passado via argumento --maps map1,map2,map3
    maps_to_import = []
    for arg in sys.argv[1:]:
        if arg.startswith("--maps="):
            maps_to_import.extend(arg[7:].split(","))
        elif not arg.startswith("-"):
            maps_to_import.append(arg)
            
    if not maps_to_import:
        ue_import_map.log("Nenhum mapa especificado para importacao em lote.")
        return

    maps_dir = r"E:\Dev\Ragnarok_Assets\Maps"
    bgm_dir = r"E:\Dev\Ragnarok_Assets\BGM"
    
    success = []
    failed = []
    
    start_total = time.time()
    ue_import_map.log(f"Iniciando importacao em lote de {len(maps_to_import)} mapas: {', '.join(maps_to_import)}")
    
    for m in maps_to_import:
        m = m.strip()
        if not m:
            continue
            
        obj_path = os.path.join(maps_dir, m, f"{m}_full.obj").replace("\\", "/")
        if not os.path.exists(obj_path):
            ue_import_map.warn(f"OBJ nao encontrado para {m}: {obj_path}")
            failed.append((m, "OBJ nao encontrado"))
            continue
            
        bgm_file = bgm_map.get(m.lower(), "01.mp3")
        bgm_path = os.path.join(bgm_dir, bgm_file).replace("\\", "/")
        
        cmd_args = [obj_path]
        if os.path.exists(bgm_path):
            cmd_args.extend(["--bgm", bgm_path])
        else:
            ue_import_map.warn(f"BGM {bgm_file} nao encontrada para {m}, usando fallback se houver")
            
        t0 = time.time()
        ue_import_map.log(f"==================================================")
        ue_import_map.log(f"IMPORTANDO MAPA: {m}")
        ue_import_map.log(f"  OBJ: {obj_path}")
        ue_import_map.log(f"  BGM: {bgm_path}")
        ue_import_map.log(f"==================================================")
        
        try:
            ue_import_map.run(cmd_args)
            dt = time.time() - t0
            ue_import_map.log(f"SUCESSO: {m} importado em {dt:.1f}s")
            success.append(m)
        except Exception as e:
            dt = time.time() - t0
            ue_import_map.warn(f"FALHA ao importar {m} apos {dt:.1f}s: {e}")
            failed.append((m, str(e)))
            
    total_dt = time.time() - start_total
    ue_import_map.log(f"==================================================")
    ue_import_map.log(f"LOTE CONCLUIDO em {total_dt:.1f}s")
    ue_import_map.log(f"  Sucesso ({len(success)}): {', '.join(success)}")
    if failed:
        ue_import_map.warn(f"  Falhas ({len(failed)}): {failed}")
    ue_import_map.log(f"==================================================")

if __name__ == "__main__":
    main()

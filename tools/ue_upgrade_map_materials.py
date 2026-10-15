"""
Upgrades M_RO_Opaque and M_RO_Masked across all maps to support HD-2D dynamic rain wetness & procedural puddles.
"""
import os
import sys

tools_dir = "E:/Documentos/Repositórios/mythofthor_project/tools"
if tools_dir not in sys.path:
    sys.path.insert(0, tools_dir)

import unreal
import ue_import_map

eal = unreal.EditorAssetLibrary

maps_root = "E:/Dev/mythofthor/Content/Maps"
map_dirs = []
for entry in os.listdir(maps_root):
    full_path = os.path.join(maps_root, entry, "Materials")
    if os.path.isdir(full_path):
        map_dirs.append(f"/Game/Maps/{entry}/Materials")

unreal.log(f"Found {len(map_dirs)} map material directories to upgrade.")

upgraded = 0
for folder in map_dirs:
    try:
        unreal.log(f"Rebuilding parent materials in {folder}...")
        res = ue_import_map.build_parent_materials(folder)
        for masked, mat in res.items():
            eal.save_loaded_asset(mat)
        upgraded += 1
    except Exception as e:
        unreal.log_error(f"Error updating {folder}: {e}")

unreal.log(f"=== Successfully upgraded {upgraded} map folders with HD-2D Rain Puddles & Wetness ===")

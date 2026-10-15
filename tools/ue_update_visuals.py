"""
Updates visual elements:
1. Recompiles M_OcclusionOutline (no feet clipping, softer 35% opacity)
2. Recompiles M_CloudShadows (visible fluffy drifting cloud shadows)
3. Spawns AMythNightLight torches with live flame in Lvl_PrtSewb1
"""
import os
import sys
import json
import struct

tools_dir = "E:/Documentos/Repositórios/mythofthor_project/tools"
if tools_dir not in sys.path:
    sys.path.insert(0, tools_dir)

import unreal
import importlib.util

eal = unreal.EditorAssetLibrary
les = unreal.get_editor_subsystem(unreal.LevelEditorSubsystem)
eas = unreal.get_editor_subsystem(unreal.EditorActorSubsystem)

# 1. Recompile M_OcclusionOutline
unreal.log("--- Recompiling M_OcclusionOutline ---")
try:
    spec = importlib.util.spec_from_file_location("ue_make_occlusion_material", "E:/Documentos/Repositórios/mythofthor_project/tools/ue_make_occlusion_material.py")
    mod = importlib.util.module_from_spec(spec)
    spec.loader.exec_module(mod)
    unreal.log("M_OcclusionOutline recompiled successfully.")
except Exception as e:
    unreal.log_error(f"Error compiling M_OcclusionOutline: {e}")

# 2. Recompile M_CloudShadows
unreal.log("--- Recompiling M_CloudShadows ---")
try:
    spec = importlib.util.spec_from_file_location("ue_make_cloudshadow_material", "E:/Documentos/Repositórios/mythofthor_project/tools/ue_make_cloudshadow_material.py")
    mod = importlib.util.module_from_spec(spec)
    spec.loader.exec_module(mod)
    unreal.log("M_CloudShadows recompiled successfully.")
except Exception as e:
    unreal.log_error(f"Error compiling M_CloudShadows: {e}")

# 3. Recompile M_RoSprite_Lit (without white edge outline)
unreal.log("--- Recompiling M_RoSprite_Lit ---")
try:
    spec = importlib.util.spec_from_file_location("ue_make_sprite_lit_material", "E:/Documentos/Repositórios/mythofthor_project/tools/ue_make_sprite_lit_material.py")
    mod = importlib.util.module_from_spec(spec)
    spec.loader.exec_module(mod)
    unreal.log("M_RoSprite_Lit recompiled successfully.")
except Exception as e:
    unreal.log_error(f"Error compiling M_RoSprite_Lit: {e}")

# 4. Recompile M_WaterModern (crystal-clear HD-2D water)
unreal.log("--- Recompiling M_WaterModern ---")
try:
    spec = importlib.util.spec_from_file_location("ue_make_water_material", "E:/Documentos/Repositórios/mythofthor_project/tools/ue_make_water_material.py")
    mod = importlib.util.module_from_spec(spec)
    spec.loader.exec_module(mod)
    unreal.log("M_WaterModern recompiled successfully.")
except Exception as e:
    unreal.log_error(f"Error compiling M_WaterModern: {e}")

# 5. Recompile M_TiltShift (asymmetric tilt-shift: Top -15%, Bottom -60%)
unreal.log("--- Recompiling M_TiltShift ---")
try:
    spec = importlib.util.spec_from_file_location("ue_make_tiltshift_material", "E:/Documentos/Repositórios/mythofthor_project/tools/ue_make_tiltshift_material.py")
    mod = importlib.util.module_from_spec(spec)
    spec.loader.exec_module(mod)
    unreal.log("M_TiltShift recompiled successfully.")
except Exception as e:
    unreal.log_error(f"Error compiling M_TiltShift: {e}")

# 6. Add live torches to Lvl_PrtSewb1
unreal.log("--- Adding Live Torches to Lvl_PrtSewb1 ---")
level_path = "/Game/Maps/PrtSewb1/Lvl_PrtSewb1"
if eal.does_asset_exist(level_path):
    les.load_level(level_path)

    # Read GAT size for center
    gat_path = "E:/Dev/Ragnarok_Assets/Maps/prt_sewb1/prt_sewb1.gat"
    cx, cy = 0.0, 0.0
    if os.path.exists(gat_path):
        with open(gat_path, 'rb') as f:
            f.seek(6)
            gw, gh = struct.unpack('<II', f.read(8))
            cx = gw * 5.0 / 2.0
            cy = gh * 5.0 / 2.0
            unreal.log(f"PrtSewb1 GAT: {gw}x{gh}, cx={cx}, cy={cy}")

    lights_path = "E:/Dev/Ragnarok_Assets/Maps/prt_sewb1/prt_sewb1_full_lights.json"
    scale = 10.0
    if os.path.exists(lights_path):
        with open(lights_path, 'r', encoding='utf-8') as f:
            data = json.load(f)
            lights = data.get('lights', [])
            unreal.log(f"Found {len(lights)} lights in JSON.")

            # Remove existing NightLights to avoid duplicates
            all_actors = eas.get_all_level_actors()
            removed = 0
            for a in all_actors:
                if a.get_class().get_name() == 'MythNightLight':
                    eas.destroy_actor(a)
                    removed += 1
            unreal.log(f"Removed {removed} existing NightLights.")

            spawned = 0
            flame_count = 0
            for i, l in enumerate(lights):
                lx = (l['x'] - cx) * scale
                ly = -(l['y'] - cy) * scale
                lz = l['z'] * scale
                loc = unreal.Vector(lx, ly, lz)
                actor = eas.spawn_actor_from_class(unreal.MythNightLight, loc)
                if actor:
                    r, g, b = l.get('color', [1.0, 0.6, 0.2])
                    is_torch = (r > 0.35 and b < 0.15)
                    actor.set_actor_label(f"{'Torch' if is_torch else 'Ambient'}_Sewb1_{i:03d}")
                    try:
                        actor.set_editor_property('always_on', True)
                    except Exception:
                        pass
                    try:
                        actor.set_editor_property('show_flame', is_torch)
                    except Exception:
                        pass
                    actor.set_editor_property('night_intensity', 65.0 if is_torch else 40.0)
                    actor.set_editor_property('flicker_amount', 0.18 if is_torch else 0.04)
                    peak = max(r, g, b, 0.01)
                    comp = actor.get_editor_property('light')
                    if comp:
                        comp.set_editor_property('light_color', unreal.Color(r=int(255 * r / peak), g=int(255 * g / peak), b=int(255 * b / peak), a=255))
                        comp.set_editor_property('attenuation_radius', 450.0 if is_torch else 550.0)
                    spawned += 1
                    if is_torch:
                        flame_count += 1

            unreal.log(f"Spawned {spawned} lights ({flame_count} live torches with animated flames) in Lvl_PrtSewb1.")
            les.save_current_level()

unreal.log("=== Visual updates completed successfully ===")

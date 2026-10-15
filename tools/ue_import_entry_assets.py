"""
Imports Entry Screen assets (Login Background, Char Select Background, and Title BGM) into Unreal Engine.
"""
import os
import unreal

eal = unreal.EditorAssetLibrary
tools = unreal.AssetToolsHelpers.get_asset_tools()

project_root = "E:/Documentos/Repositórios/mythofthor_project"

# 1. Import Login Background
login_src = f"{project_root}/Content/UI/LoginBackground.jpg"
if os.path.exists(login_src):
    unreal.log(f"Importing {login_src}...")
    t = unreal.AssetImportTask()
    t.set_editor_property('filename', login_src)
    t.set_editor_property('destination_path', '/Game/UI')
    t.set_editor_property('destination_name', 'T_LoginBackground')
    t.set_editor_property('replace_existing', True)
    t.set_editor_property('automated', True)
    t.set_editor_property('save', True)
    tools.import_asset_tasks([t])
    tex = eal.load_asset('/Game/UI/T_LoginBackground')
    if tex:
        tex.set_editor_property('srgb', True)
        eal.save_loaded_asset(tex)
        unreal.log("T_LoginBackground imported successfully.")

# 2. Import Char Select Background
char_src = f"{project_root}/Content/UI/CharSelectBackground.jpg"
if os.path.exists(char_src):
    unreal.log(f"Importing {char_src}...")
    t = unreal.AssetImportTask()
    t.set_editor_property('filename', char_src)
    t.set_editor_property('destination_path', '/Game/UI')
    t.set_editor_property('destination_name', 'T_CharSelectBackground')
    t.set_editor_property('replace_existing', True)
    t.set_editor_property('automated', True)
    t.set_editor_property('save', True)
    tools.import_asset_tasks([t])
    tex = eal.load_asset('/Game/UI/T_CharSelectBackground')
    if tex:
        tex.set_editor_property('srgb', True)
        eal.save_loaded_asset(tex)
        unreal.log("T_CharSelectBackground imported successfully.")

# 3. Import Title BGM (01.mp3)
bgm_src = f"{project_root}/Content/Audio/01_Title.mp3"
if os.path.exists(bgm_src):
    unreal.log(f"Importing {bgm_src}...")
    t = unreal.AssetImportTask()
    t.set_editor_property('filename', bgm_src)
    t.set_editor_property('destination_path', '/Game/Audio')
    t.set_editor_property('destination_name', 'BGM_Title')
    t.set_editor_property('replace_existing', True)
    t.set_editor_property('automated', True)
    t.set_editor_property('save', True)
    tools.import_asset_tasks([t])
    snd = eal.load_asset('/Game/Audio/BGM_Title')
    if snd:
        try:
            snd.set_editor_property('looping', True)
        except Exception:
            pass
        eal.save_loaded_asset(snd)
        unreal.log("BGM_Title imported successfully.")

unreal.log("=== Entry assets imported successfully ===")

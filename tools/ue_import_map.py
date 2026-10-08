"""
Myth of Thor - Importador Automatizado de Mapas 3D do Ragnarok (Unreal Engine Python)
-------------------------------------------------------------------------------------
Importa o arquivo Wavefront .obj/.mtl gerado por compile_map_to_obj.py para a Unreal Engine:
  1. Importa a Static Mesh em /Game/Maps/<MapName>/SM_<MapName>_Full
  2. Configura a colisão para Complex As Simple (permite andar no terreno exato)
  3. Cria o nível /Game/Maps/<MapName>/Lvl_<MapName>
  4. Adiciona o AMythMapInfo (vinculando com o grid de células do rAthena)
  5. Adiciona o AMythDayNightActor (Sol, Lua, Atmosfera Lumen)
  6. Adiciona o NavMeshBoundsVolume cobrindo toda a extensão da cidade
  7. Adiciona um PlayerStart centralizado na praça/fonte

Uso dentro do Editor (Output Log > Python, ou Tools > Execute Python Script):
  py "D:/Repositórios/mythofthor_project/tools/ue_import_map.py" prontera "D:/Ragnarok_Assets/Prontera/Prontera_Cidade/prontera_full.obj"

Ou via linha de comando com UnrealEditor-Cmd.exe:
  & "D:\UE_5.8\Engine\Binaries\Win64\UnrealEditor-Cmd.exe" "D:\Repositórios\mythofthor_project\MythOfThor.uproject" -run=pythonscript -script="D:\Repositórios\mythofthor_project\tools\ue_import_map.py"
"""

import os
import sys
import unreal

tools = unreal.AssetToolsHelpers.get_asset_tools()
eal = unreal.EditorAssetLibrary
ell = unreal.EditorLevelLibrary if hasattr(unreal, 'EditorLevelLibrary') else None


def import_map_mesh(obj_path, target_folder, mesh_name):
    print(f"[MythOfThor] Importando mesh 3D de: {obj_path}...")
    task = unreal.AssetImportTask()
    task.set_editor_property('filename', obj_path)
    task.set_editor_property('destination_path', target_folder)
    task.set_editor_property('destination_name', mesh_name)
    task.set_editor_property('replace_existing', True)
    task.set_editor_property('automated', True)
    task.set_editor_property('save', True)

    tools.import_asset_tasks([task])
    
    asset_path = f"{target_folder}/{mesh_name}"
    mesh = eal.load_asset(asset_path)
    if not mesh:
        print(f"[MythOfThor] ERRO: Falha ao carregar a Static Mesh importada em {asset_path}")
        return None

    # Configura colisao para usar a geometria exata (Complex as Simple)
    body_setup = mesh.get_editor_property('body_setup')
    if body_setup:
        body_setup.set_editor_property('collision_trace_flag', unreal.CollisionTraceFlag.CTF_USE_COMPLEX_AS_SIMPLE)
        eal.save_asset(asset_path)
        print(f"[MythOfThor] Colisao 'Complex as Simple' configurada com sucesso para {mesh_name}!")
        
    return mesh


def setup_map_level(map_name, static_mesh, target_folder):
    level_path = f"{target_folder}/Lvl_{map_name.capitalize()}"
    print(f"[MythOfThor] Criando nível em: {level_path}...")

    # Cria novo nivel
    world = unreal.EditorLevelLibrary.new_level(level_path) if hasattr(unreal, 'EditorLevelLibrary') else None
    
    # 1. Spawna o ator da cidade com a Static Mesh
    mesh_actor = unreal.EditorLevelLibrary.spawn_actor_from_object(static_mesh, unreal.Vector(0, 0, 0), unreal.Rotator(0, 0, 0))
    if mesh_actor:
        mesh_actor.set_actor_label(f"WorldMesh_{map_name}")
        print(f"[MythOfThor] Ator de malha 3D posicionado em (0,0,0)")

    # 2. Spawna o AMythMapInfo para integrar com rAthena
    try:
        map_info_class = unreal.load_class(None, "/Script/MythOfThor.MythMapInfo")
        if map_info_class:
            map_info = unreal.EditorLevelLibrary.spawn_actor_from_class(map_info_class, unreal.Vector(0, 0, 0), unreal.Rotator(0, 0, 0))
            if map_info:
                map_info.set_editor_property("map_name", map_name.lower())
                map_info.set_editor_property("cell_size_uu", 50.0)
                map_info.set_actor_label(f"MythMapInfo_{map_name}")
                print(f"[MythOfThor] AMythMapInfo configurado para mapa '{map_name}'!")
    except Exception as e:
        print(f"[MythOfThor] Aviso ao configurar MythMapInfo: {e}")

    # 3. Spawna o Ciclo Dia/Noite e Atmosfera
    try:
        day_night_class = unreal.load_class(None, "/Script/MythOfThor.MythDayNightActor")
        if day_night_class:
            day_night = unreal.EditorLevelLibrary.spawn_actor_from_class(day_night_class, unreal.Vector(0, 0, 0), unreal.Rotator(0, 0, 0))
            if day_night:
                day_night.set_actor_label("MythDayNightActor")
                print(f"[MythOfThor] AMythDayNightActor adicionado ao nível!")
    except Exception as e:
        print(f"[MythOfThor] Aviso ao configurar DayNightActor: {e}")

    # 4. Spawna NavMeshBoundsVolume para permitir Point & Click e movimentação de IA
    try:
        nav_mesh_class = unreal.load_class(None, "/Script/NavigationSystem.NavMeshBoundsVolume")
        if nav_mesh_class:
            nav_volume = unreal.EditorLevelLibrary.spawn_actor_from_class(nav_mesh_class, unreal.Vector(0, 0, 0), unreal.Rotator(0, 0, 0))
            if nav_volume:
                # Prontera tem aproximadamente 400x400 células -> 20.000 x 20.000 UU
                nav_volume.set_actor_scale3d(unreal.Vector(300.0, 300.0, 50.0))
                nav_volume.set_actor_label("NavMeshBounds_Prontera")
                print(f"[MythOfThor] NavMeshBoundsVolume dimensionado e posicionado!")
    except Exception as e:
        print(f"[MythOfThor] Aviso ao configurar NavMesh: {e}")

    # 5. Spawna PlayerStart
    try:
        player_start_class = unreal.load_class(None, "/Script/Engine.PlayerStart")
        if player_start_class:
            # Posição central na praça de Prontera (ligeiramente acima do chão)
            p_start = unreal.EditorLevelLibrary.spawn_actor_from_class(player_start_class, unreal.Vector(0, 0, 100), unreal.Rotator(0, 0, 0))
            if p_start:
                p_start.set_actor_label("PlayerStart_Center")
                print(f"[MythOfThor] PlayerStart posicionado na praça central!")
    except Exception as e:
        print(f"[MythOfThor] Aviso ao configurar PlayerStart: {e}")

    # Salva o nivel
    if hasattr(unreal, 'EditorLevelLibrary'):
        unreal.EditorLevelLibrary.save_current_level()
        print(f"[MythOfThor] Nível {level_path} salvo com sucesso!")


def main():
    map_name = sys.argv[1] if len(sys.argv) > 1 else "prontera"
    obj_path = sys.argv[2] if len(sys.argv) > 2 else "D:/Ragnarok_Assets/Prontera/Prontera_Cidade/prontera_full.obj"

    if not os.path.exists(obj_path):
        print(f"[MythOfThor] ERRO: Arquivo OBJ não encontrado: {obj_path}")
        return

    folder = f"/Game/Maps/{map_name.capitalize()}"
    mesh_name = f"SM_{map_name.capitalize()}_Full"

    mesh = import_map_mesh(obj_path, folder, mesh_name)
    if mesh:
        setup_map_level(map_name, mesh, folder)
        print(f"[MythOfThor] >>> IMPORTACAO DO MAPA '{map_name}' CONCLUIDA COM SUCESSO! <<<")


if __name__ == "__main__":
    main()

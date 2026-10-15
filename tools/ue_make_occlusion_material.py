"""
Creates /Game/Myth/M_OcclusionOutline: HD-2D occlusion silhouette (GDD technical, section 6.2).
When an actor (Player = Stencil 1 Blue, Monster = Stencil 2 Red, NPC = Stencil 3 Gold) is occluded
behind 3D world geometry (CustomDepth > SceneDepth), renders a clean silhouette.

Usage:
  UnrealEditor-Cmd.exe mythofthor.uproject -run=pythonscript -script="tools/ue_make_occlusion_material.py"
"""
import unreal

PATH = '/Game/Myth'
NAME = 'M_OcclusionOutline'
lib = unreal.MaterialEditingLibrary
tools = unreal.AssetToolsHelpers.get_asset_tools()

full = f"{PATH}/{NAME}"
mat = None
if unreal.EditorAssetLibrary.does_asset_exist(full):
    mat = unreal.EditorAssetLibrary.load_asset(full)
    if mat:
        lib.delete_all_material_expressions(mat)

if not mat:
    mat = tools.create_asset(NAME, PATH, unreal.Material, unreal.MaterialFactoryNew())

mat.set_editor_property('material_domain', unreal.MaterialDomain.MD_POST_PROCESS)

def expr(cls, x, y, **props):
    e = lib.create_material_expression(mat, cls, x, y)
    for k, v in props.items():
        e.set_editor_property(k, v)
    return e

# Scene Textures
st_color = expr(unreal.MaterialExpressionSceneTexture, -800, -200,
                scene_texture_id=unreal.SceneTextureId.PPI_POST_PROCESS_INPUT0)
st_scenedepth = expr(unreal.MaterialExpressionSceneTexture, -800, 0,
                     scene_texture_id=unreal.SceneTextureId.PPI_SCENE_DEPTH)
st_customdepth = expr(unreal.MaterialExpressionSceneTexture, -800, 200,
                      scene_texture_id=unreal.SceneTextureId.PPI_CUSTOM_DEPTH)
st_stencil = expr(unreal.MaterialExpressionSceneTexture, -800, 400,
                  scene_texture_id=unreal.SceneTextureId.PPI_CUSTOM_STENCIL)

# Custom HLSL Expression
custom = expr(unreal.MaterialExpressionCustom, -300, 0)
custom.set_editor_property('output_type', unreal.CustomMaterialOutputType.CMOT_FLOAT3)
custom.set_editor_property('description', 'OcclusionSilhouette')

hlsl_code = """
int StencilVal = (int)(CustomStencil.r + 0.5);
float CD = CustomDepth.r;
float SD = SceneDepth.r;

if (StencilVal > 0 && CD < 50000.0 && CD > SD + 5.0)
{
    float3 OutlineColor = float3(0.2, 0.6, 1.0); // Player: Blue (Stencil 1)
    if (StencilVal == 2)
    {
        OutlineColor = float3(1.0, 0.2, 0.2); // Monster: Red
    }
    else if (StencilVal == 3)
    {
        OutlineColor = float3(1.0, 0.8, 0.1); // NPC: Gold
    }
    return lerp(SceneColor.rgb, OutlineColor, 0.65);
}
return SceneColor.rgb;
"""
custom.set_editor_property('code', hlsl_code)

inputs = []
for name in ['SceneColor', 'SceneDepth', 'CustomDepth', 'CustomStencil']:
    inp = unreal.CustomInput()
    inp.set_editor_property('input_name', name)
    inputs.append(inp)
custom.set_editor_property('inputs', inputs)

lib.connect_material_expressions(st_color, 'Color', custom, 'SceneColor')
lib.connect_material_expressions(st_scenedepth, 'Color', custom, 'SceneDepth')
lib.connect_material_expressions(st_customdepth, 'Color', custom, 'CustomDepth')
lib.connect_material_expressions(st_stencil, 'Color', custom, 'CustomStencil')

lib.connect_material_property(custom, '', unreal.MaterialProperty.MP_EMISSIVE_COLOR)

lib.recompile_material(mat)
unreal.EditorAssetLibrary.save_loaded_asset(mat)
unreal.log(f"[MythOcclusion] Successfully created and compiled {full}")

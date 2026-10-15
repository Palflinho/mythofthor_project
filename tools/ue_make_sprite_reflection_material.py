"""
Creates /Game/Myth/M_RoSprite_Reflection: Translucent material for HD-2D water ghost reflections.
Adheres to GDD HD-2D Section 3.2 (Ghost Sprite in Water).

Usage:
  UnrealEditor-Cmd.exe mythofthor.uproject -run=pythonscript -script="tools/ue_make_sprite_reflection_material.py"
"""
import unreal

PATH = '/Game/Myth'
NAME = 'M_RoSprite_Reflection'
lib = unreal.MaterialEditingLibrary
tools = unreal.AssetToolsHelpers.get_asset_tools()

full = f"{PATH}/{NAME}"
if unreal.EditorAssetLibrary.does_asset_exist(full):
    unreal.EditorAssetLibrary.delete_asset(full)

mat = tools.create_asset(NAME, PATH, unreal.Material, unreal.MaterialFactoryNew())
mat.set_editor_property('material_domain', unreal.MaterialDomain.MD_SURFACE)
mat.set_editor_property('blend_mode', unreal.BlendMode.BLEND_TRANSLUCENT)
mat.set_editor_property('shading_model', unreal.MaterialShadingModel.MSM_UNLIT)
mat.set_editor_property('two_sided', True)

def expr(cls, x, y, **props):
    e = lib.create_material_expression(mat, cls, x, y)
    for k, v in props.items():
        e.set_editor_property(k, v)
    return e

# UV Coordinate
uv = expr(unreal.MaterialExpressionTextureCoordinate, -1000, 0)

# Sprite Texture Parameter
tex_sample = expr(unreal.MaterialExpressionTextureSampleParameter2D, -700, 0, parameter_name='SpriteTexture')
lib.connect_material_expressions(uv, '', tex_sample, 'UVs')

# Reflection Opacity Parameter (default 0.35 per GDD)
opacity_param = expr(unreal.MaterialExpressionScalarParameter, -700, 200, parameter_name='ReflectionOpacity', default_value=0.35)

# Water Tint Parameter (soft aquatic cyan tint)
tint_param = expr(unreal.MaterialExpressionVectorParameter, -700, -250, parameter_name='WaterTint',
                  default_value=unreal.LinearColor(0.65, 0.85, 1.0, 1.0))

# Multiply Sprite RGB by WaterTint
color_mult = expr(unreal.MaterialExpressionMultiply, -400, -100)
lib.connect_material_expressions(tex_sample, 'RGB', color_mult, 'A')
lib.connect_material_expressions(tint_param, '', color_mult, 'B')

# Multiply Sprite Alpha by ReflectionOpacity
opacity_mult = expr(unreal.MaterialExpressionMultiply, -400, 150)
lib.connect_material_expressions(tex_sample, 'A', opacity_mult, 'A')
lib.connect_material_expressions(opacity_param, '', opacity_mult, 'B')

# Connect Emissive and Opacity
lib.connect_material_property(color_mult, '', unreal.MaterialProperty.MP_EMISSIVE_COLOR)
lib.connect_material_property(opacity_mult, '', unreal.MaterialProperty.MP_OPACITY)

lib.recompile_material(mat)
unreal.EditorAssetLibrary.save_loaded_asset(mat)
unreal.log(f"[MythSpriteReflection] Successfully created and compiled {full}")

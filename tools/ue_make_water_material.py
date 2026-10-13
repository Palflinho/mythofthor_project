"""
Cria /Game/Myth/M_Water: material da agua dos mapas (translucido, sem luz, dois lados) com os parametros Tex (textura da agua,
water0NN.jpg do cliente), Brightness e Opacity. Usado por AMythWater (Source/MythOfThor/World/MythWater.cpp).

Uso (UnrealEditor-Cmd): -run=pythonscript -script="tools/ue_make_water_material.py"
"""
import unreal

PATH = '/Game/Myth'
NAME = 'M_Water'
lib = unreal.MaterialEditingLibrary
tools = unreal.AssetToolsHelpers.get_asset_tools()

if unreal.EditorAssetLibrary.does_asset_exist('%s/%s' % (PATH, NAME)):
    unreal.EditorAssetLibrary.delete_asset('%s/%s' % (PATH, NAME))
mat = tools.create_asset(NAME, PATH, unreal.Material, unreal.MaterialFactoryNew())
mat.set_editor_property('blend_mode', unreal.BlendMode.BLEND_TRANSLUCENT)
mat.set_editor_property('shading_model', unreal.MaterialShadingModel.MSM_UNLIT)
mat.set_editor_property('two_sided', True)

tex = lib.create_material_expression(mat, unreal.MaterialExpressionTextureSampleParameter2D, -700, 0)
tex.set_editor_property('parameter_name', 'Tex')
bright = lib.create_material_expression(mat, unreal.MaterialExpressionScalarParameter, -700, 250)
bright.set_editor_property('parameter_name', 'Brightness')
bright.set_editor_property('default_value', 1.0)
opacity = lib.create_material_expression(mat, unreal.MaterialExpressionScalarParameter, -400, 400)
opacity.set_editor_property('parameter_name', 'Opacity')
opacity.set_editor_property('default_value', 0.75)
mul = lib.create_material_expression(mat, unreal.MaterialExpressionMultiply, -350, 100)

lib.connect_material_expressions(tex, 'RGB', mul, 'A')
lib.connect_material_expressions(bright, '', mul, 'B')
lib.connect_material_property(mul, '', unreal.MaterialProperty.MP_EMISSIVE_COLOR)
lib.connect_material_property(opacity, '', unreal.MaterialProperty.MP_OPACITY)
lib.recompile_material(mat)
unreal.EditorAssetLibrary.save_asset('%s/%s' % (PATH, NAME))
print('M_Water criado em %s/%s' % (PATH, NAME))

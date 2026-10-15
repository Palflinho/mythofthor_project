"""
Creates /Game/Myth/M_CloudShadows: moving cloud shadows (GDD technical, section 7.1). A Light Function material for the sun: world position
(xy) / scale scrolls with time through a noise and is remapped to 0.45 (in the shadow of a cloud) .. 1.0 (full sun).
Parameters: Scale (world uu per noise cell, default 6000), SpeedX / SpeedY (default 0.004, 0.0025), Darkness (shadow brightness, default 0.45).

Usage (UnrealEditor-Cmd): -run=pythonscript -script="tools/ue_make_cloudshadow_material.py"
"""
import unreal

PATH = '/Game/Myth'
lib = unreal.MaterialEditingLibrary
tools = unreal.AssetToolsHelpers.get_asset_tools()


def expr(mat, cls, x, y, **props):
    e = lib.create_material_expression(mat, cls, x, y)
    for k, v in props.items():
        e.set_editor_property(k, v)
    return e


full = '%s/M_CloudShadows' % PATH
if unreal.EditorAssetLibrary.does_asset_exist(full):
    unreal.EditorAssetLibrary.delete_asset(full)
mat = tools.create_asset('M_CloudShadows', PATH, unreal.Material, unreal.MaterialFactoryNew())
mat.set_editor_property('material_domain', unreal.MaterialDomain.MD_LIGHT_FUNCTION)

wp = expr(mat, unreal.MaterialExpressionWorldPosition, -1200, 0)
xy = expr(mat, unreal.MaterialExpressionComponentMask, -1000, 0, r=True, g=True, b=False, a=False)
lib.connect_material_expressions(wp, '', xy, '')
scale = expr(mat, unreal.MaterialExpressionScalarParameter, -1000, 150, parameter_name='Scale', default_value=6000.0)
div = expr(mat, unreal.MaterialExpressionDivide, -800, 0)
lib.connect_material_expressions(xy, '', div, 'A')
lib.connect_material_expressions(scale, '', div, 'B')

time = expr(mat, unreal.MaterialExpressionTime, -1000, 300)
sx = expr(mat, unreal.MaterialExpressionScalarParameter, -1000, 400, parameter_name='SpeedX', default_value=0.004)
sy = expr(mat, unreal.MaterialExpressionScalarParameter, -1000, 480, parameter_name='SpeedY', default_value=0.0025)
mx = expr(mat, unreal.MaterialExpressionMultiply, -800, 300)
lib.connect_material_expressions(time, '', mx, 'A')
lib.connect_material_expressions(sx, '', mx, 'B')
my = expr(mat, unreal.MaterialExpressionMultiply, -800, 400)
lib.connect_material_expressions(time, '', my, 'A')
lib.connect_material_expressions(sy, '', my, 'B')
app = expr(mat, unreal.MaterialExpressionAppendVector, -650, 350)
lib.connect_material_expressions(mx, '', app, 'A')
lib.connect_material_expressions(my, '', app, 'B')
add = expr(mat, unreal.MaterialExpressionAdd, -500, 100)
lib.connect_material_expressions(div, '', add, 'A')
lib.connect_material_expressions(app, '', add, 'B')

# Noise over the scrolling coordinates (needs a 3-vector position: append a constant z)
z = expr(mat, unreal.MaterialExpressionConstant, -500, 250, r=0.0)
pos3 = expr(mat, unreal.MaterialExpressionAppendVector, -350, 150)
lib.connect_material_expressions(add, '', pos3, 'A')
lib.connect_material_expressions(z, '', pos3, 'B')
noise = expr(mat, unreal.MaterialExpressionNoise, -200, 150, scale=1.0, quality=1, levels=3, output_min=0.0, output_max=1.0,
             level_scale=2.0, turbulence=False)
lib.connect_material_expressions(pos3, '', noise, 'Position')

# remap: soft threshold so that clouds are patches, not a uniform haze
lo = expr(mat, unreal.MaterialExpressionConstant, -200, 320, r=0.38)
hi = expr(mat, unreal.MaterialExpressionConstant, -200, 380, r=0.62)
sub = expr(mat, unreal.MaterialExpressionSubtract, 0, 150)
lib.connect_material_expressions(noise, '', sub, 'A')
lib.connect_material_expressions(lo, '', sub, 'B')
rng = expr(mat, unreal.MaterialExpressionSubtract, 0, 330)
lib.connect_material_expressions(hi, '', rng, 'A')
lib.connect_material_expressions(lo, '', rng, 'B')
dv = expr(mat, unreal.MaterialExpressionDivide, 150, 150)
lib.connect_material_expressions(sub, '', dv, 'A')
lib.connect_material_expressions(rng, '', dv, 'B')
sat = expr(mat, unreal.MaterialExpressionSaturate, 300, 150)
lib.connect_material_expressions(dv, '', sat, '')

dark = expr(mat, unreal.MaterialExpressionScalarParameter, 300, 300, parameter_name='Darkness', default_value=0.45)
one = expr(mat, unreal.MaterialExpressionConstant, 300, 360, r=1.0)
lerp = expr(mat, unreal.MaterialExpressionLinearInterpolate, 500, 150)
lib.connect_material_expressions(dark, '', lerp, 'A')   # in the cloud
lib.connect_material_expressions(one, '', lerp, 'B')    # full sun
lib.connect_material_expressions(sat, '', lerp, 'Alpha')
lib.connect_material_property(lerp, '', unreal.MaterialProperty.MP_EMISSIVE_COLOR)

lib.recompile_material(mat)
unreal.EditorAssetLibrary.save_loaded_asset(mat)
unreal.log('[MythCloudShadows] %s created' % full)

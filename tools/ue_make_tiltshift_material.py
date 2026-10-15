"""
Creates /Game/Myth/M_TiltShift: the HD-2D "miniature" blur (GDD technical, section 6.2). A post-process material that blurs the screen more
the further a pixel is, vertically, from the focus band around the character (the physical depth of field of a telephoto lens is too weak to
show at this camera distance). 9-tap disk blur whose radius grows with |uv.y - FocusY| (outside the clear band).
Parameters: FocusY (0..1, default 0.52), ClearBand (default 0.18), MaxBlurPx (default 3.5), BlurPower (default 1.6).

Usage (UnrealEditor-Cmd): -run=pythonscript -script="tools/ue_make_tiltshift_material.py"
"""
import math
import unreal

PATH = '/Game/Myth'
lib = unreal.MaterialEditingLibrary
tools = unreal.AssetToolsHelpers.get_asset_tools()


def expr(mat, cls, x, y, **props):
    e = lib.create_material_expression(mat, cls, x, y)
    for k, v in props.items():
        e.set_editor_property(k, v)
    return e


full = '%s/M_TiltShift' % PATH
if unreal.EditorAssetLibrary.does_asset_exist(full):
    unreal.EditorAssetLibrary.delete_asset(full)
mat = tools.create_asset('M_TiltShift', PATH, unreal.Material, unreal.MaterialFactoryNew())
mat.set_editor_property('material_domain', unreal.MaterialDomain.MD_POST_PROCESS)

uv = expr(mat, unreal.MaterialExpressionTextureCoordinate, -1400, 0)

focus = expr(mat, unreal.MaterialExpressionScalarParameter, -1400, 200, parameter_name='FocusY', default_value=0.52)
band = expr(mat, unreal.MaterialExpressionScalarParameter, -1400, 300, parameter_name='ClearBand', default_value=0.14)
maxpx = expr(mat, unreal.MaterialExpressionScalarParameter, -1400, 400, parameter_name='MaxBlurPx', default_value=6.5)
power = expr(mat, unreal.MaterialExpressionScalarParameter, -1400, 500, parameter_name='BlurPower', default_value=1.6)

# v = uv.y
vy = expr(mat, unreal.MaterialExpressionComponentMask, -1200, 0, r=False, g=True, b=False, a=False)
lib.connect_material_expressions(uv, '', vy, '')
# d = saturate(max(|v - focus| - band, 0) / (0.5 - band))
sub = expr(mat, unreal.MaterialExpressionSubtract, -1000, 0)
lib.connect_material_expressions(vy, '', sub, 'A')
lib.connect_material_expressions(focus, '', sub, 'B')
ab = expr(mat, unreal.MaterialExpressionAbs, -850, 0)
lib.connect_material_expressions(sub, '', ab, '')
sub2 = expr(mat, unreal.MaterialExpressionSubtract, -700, 0)
lib.connect_material_expressions(ab, '', sub2, 'A')
lib.connect_material_expressions(band, '', sub2, 'B')
mx = expr(mat, unreal.MaterialExpressionMax, -550, 0)
lib.connect_material_expressions(sub2, '', mx, 'A')
zero = expr(mat, unreal.MaterialExpressionConstant, -700, 100, r=0.0)
lib.connect_material_expressions(zero, '', mx, 'B')
half = expr(mat, unreal.MaterialExpressionConstant, -700, 160, r=0.5)
den = expr(mat, unreal.MaterialExpressionSubtract, -550, 140)
lib.connect_material_expressions(half, '', den, 'A')
lib.connect_material_expressions(band, '', den, 'B')
dv = expr(mat, unreal.MaterialExpressionDivide, -400, 0)
lib.connect_material_expressions(mx, '', dv, 'A')
lib.connect_material_expressions(den, '', dv, 'B')
sat = expr(mat, unreal.MaterialExpressionSaturate, -250, 0)
lib.connect_material_expressions(dv, '', sat, '')
pw = expr(mat, unreal.MaterialExpressionPower, -100, 0)
lib.connect_material_expressions(sat, '', pw, 'Base')
lib.connect_material_expressions(power, '', pw, 'Exponent')
radius = expr(mat, unreal.MaterialExpressionMultiply, 50, 0)  # blur radius in pixels
lib.connect_material_expressions(pw, '', radius, 'A')
lib.connect_material_expressions(maxpx, '', radius, 'B')

# one SceneTexture just to read InvSize (1 / viewport size)
inv_src = expr(mat, unreal.MaterialExpressionSceneTexture, -1400, 700,
               scene_texture_id=unreal.SceneTextureId.PPI_POST_PROCESS_INPUT0)
scale = expr(mat, unreal.MaterialExpressionMultiply, 200, 100)  # float2: radius * InvSize
lib.connect_material_expressions(inv_src, 'InvSize', scale, 'A')
lib.connect_material_expressions(radius, '', scale, 'B')

taps = [(0.0, 0.0)]
for ring, n in ((1.0, 8),):
    for i in range(n):
        a = 2.0 * math.pi * i / n
        taps.append((math.cos(a) * ring, math.sin(a) * ring))

acc = None
y = -300
for dx, dy in taps:
    off = expr(mat, unreal.MaterialExpressionConstant2Vector, 400, y, r=dx, g=dy)
    mul = expr(mat, unreal.MaterialExpressionMultiply, 550, y)
    lib.connect_material_expressions(off, '', mul, 'A')
    lib.connect_material_expressions(scale, '', mul, 'B')
    add_uv = expr(mat, unreal.MaterialExpressionAdd, 700, y)
    lib.connect_material_expressions(uv, '', add_uv, 'A')
    lib.connect_material_expressions(mul, '', add_uv, 'B')
    st = expr(mat, unreal.MaterialExpressionSceneTexture, 850, y,
              scene_texture_id=unreal.SceneTextureId.PPI_POST_PROCESS_INPUT0)
    lib.connect_material_expressions(add_uv, '', st, 'UVs')
    if acc is None:
        acc = st
        acc_out = 'Color'
    else:
        s = expr(mat, unreal.MaterialExpressionAdd, 1000, y)
        lib.connect_material_expressions(acc, acc_out, s, 'A')
        lib.connect_material_expressions(st, 'Color', s, 'B')
        acc, acc_out = s, ''
    y += 160

inv_n = expr(mat, unreal.MaterialExpressionConstant, 1150, 0, r=1.0 / len(taps))
avg = expr(mat, unreal.MaterialExpressionMultiply, 1300, 0)
lib.connect_material_expressions(acc, acc_out, avg, 'A')
lib.connect_material_expressions(inv_n, '', avg, 'B')
lib.connect_material_property(avg, '', unreal.MaterialProperty.MP_EMISSIVE_COLOR)

lib.recompile_material(mat)
unreal.EditorAssetLibrary.save_loaded_asset(mat)
unreal.log('[MythTiltShift] %s created (%d taps)' % (full, len(taps)))

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
top_band = expr(mat, unreal.MaterialExpressionScalarParameter, -1400, 300, parameter_name='TopBand', default_value=0.16)
bot_band = expr(mat, unreal.MaterialExpressionScalarParameter, -1400, 380, parameter_name='BotBand', default_value=0.28)
maxpx = expr(mat, unreal.MaterialExpressionScalarParameter, -1400, 460, parameter_name='MaxBlurPx', default_value=6.0)
power = expr(mat, unreal.MaterialExpressionScalarParameter, -1400, 540, parameter_name='BlurPower', default_value=1.6)

# Custom HLSL node para raio de blur com reducao assimetrica (Topo -15%, Fundo -60%)
custom_radius = expr(mat, unreal.MaterialExpressionCustom, -400, 0)
custom_radius.set_editor_property('output_type', unreal.CustomMaterialOutputType.CMOT_FLOAT1)
custom_radius.set_editor_property('description', 'AsymmetricTiltShiftRadius')
custom_radius.set_editor_property('code', """
float v = UV.y;
float f = Focus;
float d = 0.0;
float blurMul = 1.0;
if (v < f)
{
    float dist = f - v;
    d = saturate(max(dist - TopBand, 0.0) / max(f - TopBand, 0.01));
    blurMul = 0.85; // Topo: reduzido em 15%
}
else
{
    float dist = v - f;
    d = saturate(max(dist - BotBand, 0.0) / max((1.0 - f) - BotBand, 0.01));
    blurMul = 0.40; // Fundo: reduzido em 60% (chao e pes nitidos)
}
return pow(d, Power) * blurMul * MaxPx;
""")

inps = []
for name in ['UV', 'Focus', 'TopBand', 'BotBand', 'Power', 'MaxPx']:
    inp = unreal.CustomInput()
    inp.set_editor_property('input_name', name)
    inps.append(inp)
custom_radius.set_editor_property('inputs', inps)

lib.connect_material_expressions(uv, '', custom_radius, 'UV')
lib.connect_material_expressions(focus, '', custom_radius, 'Focus')
lib.connect_material_expressions(top_band, '', custom_radius, 'TopBand')
lib.connect_material_expressions(bot_band, '', custom_radius, 'BotBand')
lib.connect_material_expressions(power, '', custom_radius, 'Power')
lib.connect_material_expressions(maxpx, '', custom_radius, 'MaxPx')
radius = custom_radius

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

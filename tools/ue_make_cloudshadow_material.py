"""
Creates /Game/Myth/M_CloudShadows: moving cloud shadows (GDD technical, section 7.1). A Light Function material for the sun.
Uses analytical C-infinity smooth trigonometric FBM in HLSL Custom node (zero texture/simplex lookup grain)
so cloud shadows are 100% smooth and organic across the terrain.

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

wp = expr(mat, unreal.MaterialExpressionWorldPosition, -900, 0)
time = expr(mat, unreal.MaterialExpressionTime, -900, 150)
scale = expr(mat, unreal.MaterialExpressionScalarParameter, -900, 260, parameter_name='Scale', default_value=3800.0)
sx = expr(mat, unreal.MaterialExpressionScalarParameter, -900, 360, parameter_name='SpeedX', default_value=0.06)
sy = expr(mat, unreal.MaterialExpressionScalarParameter, -900, 460, parameter_name='SpeedY', default_value=0.035)
dark = expr(mat, unreal.MaterialExpressionScalarParameter, -900, 560, parameter_name='Darkness', default_value=0.42)

cloud = expr(mat, unreal.MaterialExpressionCustom, -350, 150)
cloud.set_editor_property('output_type', unreal.CustomMaterialOutputType.CMOT_FLOAT3)
cloud.set_editor_property('description', 'SmoothCloudShadowsFBM')
cloud.set_editor_property('code', """
float2 p = WorldPos.xy / max(Scale, 100.0) + float2(Time * SpeedX, Time * SpeedY);

// Domain warping suave com ondas senoidais analiticas (zero granulado de tabela de ruido)
float2 warp = float2(
    sin(p.y * 1.3 + Time * 0.02) + 0.5 * cos(p.x * 2.1 - p.y * 1.1),
    cos(p.x * 1.5 - Time * 0.015) + 0.5 * sin(p.y * 1.9 + p.x * 1.2)
);
float2 q = p + warp * 0.45;

// 3 oitavas suaves de nuvens organicas (Cumulus)
float c1 = sin(q.x * 1.7 + cos(q.y * 1.4)) * cos(q.y * 1.6 - sin(q.x * 1.3));
float c2 = sin(q.x * 3.1 - q.y * 2.3 + 1.7) * cos(q.y * 2.9 + q.x * 2.1 - 0.8);
float c3 = sin(q.x * 5.3 + q.y * 4.7 - 2.4) * cos(q.x * 4.1 - q.y * 5.1 + 1.1);

float fbm = c1 * 0.60 + c2 * 0.28 + c3 * 0.12;
float density = saturate(fbm * 0.5 + 0.5);

// Transicao macia entre sol pleno (1.0) e sombra de nuvem (Darkness)
float sunMask = smoothstep(0.42, 0.68, density);
float val = lerp(Darkness, 1.0, sunMask);
return float3(val, val, val);
""")

inps = []
for name in ['WorldPos', 'Time', 'Scale', 'SpeedX', 'SpeedY', 'Darkness']:
    inp = unreal.CustomInput()
    inp.set_editor_property('input_name', name)
    inps.append(inp)
cloud.set_editor_property('inputs', inps)

lib.connect_material_expressions(wp, '', cloud, 'WorldPos')
lib.connect_material_expressions(time, '', cloud, 'Time')
lib.connect_material_expressions(scale, '', cloud, 'Scale')
lib.connect_material_expressions(sx, '', cloud, 'SpeedX')
lib.connect_material_expressions(sy, '', cloud, 'SpeedY')
lib.connect_material_expressions(dark, '', cloud, 'Darkness')
lib.connect_material_property(cloud, '', unreal.MaterialProperty.MP_EMISSIVE_COLOR)

lib.recompile_material(mat)
unreal.EditorAssetLibrary.save_loaded_asset(mat)
unreal.log('[MythCloudShadows] %s created (analytical grain-free FBM)' % full)

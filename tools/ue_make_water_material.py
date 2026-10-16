"""
Cria os materiais da agua dos mapas (usados por AMythWater, Source/MythOfThor/World/MythWater.cpp):
  /Game/Myth/M_Water        agua classica do RO: translucida, sem luz, dois lados; parametros Tex (water0NN.jpg), Brightness, Opacity.
  /Game/Myth/M_WaterModern  agua moderna HD-2D: Single Layer Water da Unreal (refracao, absorcao com a profundidade, reflexo do ceu com
                            Lumen) com normais em espaco do mundo (sem distorcao cilindrica de tangentes) e causticas animadas do RO (Tex).
                            Parametros: Tex, Tint, Opacity, NormalStrength.
  /Game/Myth/T_WaterNormal  normal map (tools/data/water_normal.png, gerado por tools/gen_water_normal.py).

Uso (UnrealEditor-Cmd): -run=pythonscript -script="tools/ue_make_water_material.py"
"""
import os
import unreal

PATH = '/Game/Myth'
lib = unreal.MaterialEditingLibrary
tools = unreal.AssetToolsHelpers.get_asset_tools()
here = os.path.dirname(os.path.abspath(__file__))


def fresh(name, klass, factory):
    full = '%s/%s' % (PATH, name)
    if unreal.EditorAssetLibrary.does_asset_exist(full):
        unreal.EditorAssetLibrary.delete_asset(full)
    return tools.create_asset(name, PATH, klass, factory)


def expr(mat, cls, x, y, **props):
    e = lib.create_material_expression(mat, cls, x, y)
    for k, v in props.items():
        e.set_editor_property(k, v)
    return e


# ------------------------------------------------------------------ normal map
png = os.path.join(here, 'data', 'water_normal.png')
task = unreal.AssetImportTask()
task.set_editor_property('filename', png)
task.set_editor_property('destination_path', PATH)
task.set_editor_property('destination_name', 'T_WaterNormal')
task.set_editor_property('replace_existing', True)
task.set_editor_property('automated', True)
task.set_editor_property('save', True)
tools.import_asset_tasks([task])
normal_tex = unreal.load_asset('%s/T_WaterNormal' % PATH)
normal_tex.set_editor_property('compression_settings', unreal.TextureCompressionSettings.TC_NORMALMAP)
normal_tex.set_editor_property('srgb', False)
unreal.EditorAssetLibrary.save_loaded_asset(normal_tex)

# ------------------------------------------------------------------ classica
mat = fresh('M_Water', unreal.Material, unreal.MaterialFactoryNew())
mat.set_editor_property('blend_mode', unreal.BlendMode.BLEND_TRANSLUCENT)
mat.set_editor_property('shading_model', unreal.MaterialShadingModel.MSM_UNLIT)
mat.set_editor_property('two_sided', True)
tex = expr(mat, unreal.MaterialExpressionTextureSampleParameter2D, -700, 0, parameter_name='Tex')
bright = expr(mat, unreal.MaterialExpressionScalarParameter, -700, 250, parameter_name='Brightness', default_value=1.0)
opacity = expr(mat, unreal.MaterialExpressionScalarParameter, -400, 400, parameter_name='Opacity', default_value=0.75)
mul = expr(mat, unreal.MaterialExpressionMultiply, -350, 100)
lib.connect_material_expressions(tex, 'RGB', mul, 'A')
lib.connect_material_expressions(bright, '', mul, 'B')
lib.connect_material_property(mul, '', unreal.MaterialProperty.MP_EMISSIVE_COLOR)
lib.connect_material_property(opacity, '', unreal.MaterialProperty.MP_OPACITY)
lib.recompile_material(mat)
unreal.EditorAssetLibrary.save_loaded_asset(mat)

# ------------------------------------------------------------------ moderna (Single Layer Water HD-2D)
m = fresh('M_WaterModern', unreal.Material, unreal.MaterialFactoryNew())
m.set_editor_property('shading_model', unreal.MaterialShadingModel.MSM_SINGLE_LAYER_WATER)
m.set_editor_property('blend_mode', unreal.BlendMode.BLEND_OPAQUE)
m.set_editor_property('two_sided', True)
# Normal em World Space para nunca sofrer deformacao de tangentes da ProceduralMesh ("efeito massinha")
m.set_editor_property('tangent_space_normal', False)

wp = expr(m, unreal.MaterialExpressionWorldPosition, -1400, 100)
time = expr(m, unreal.MaterialExpressionTime, -1400, 250)

def layer(y, scale, speed_x, speed_y):
    pan = expr(m, unreal.MaterialExpressionPanner, -1100, y, speed_x=speed_x, speed_y=speed_y)
    scl = expr(m, unreal.MaterialExpressionTextureCoordinate, -1300, y, u_tiling=scale, v_tiling=scale)
    lib.connect_material_expressions(scl, '', pan, 'Coordinate')
    lib.connect_material_expressions(time, '', pan, 'Time')
    n = expr(m, unreal.MaterialExpressionTextureSampleParameter2D, -800, y, parameter_name='Normal%d' % int(y), texture=normal_tex,
             sampler_type=unreal.MaterialSamplerType.SAMPLERTYPE_NORMAL)
    lib.connect_material_expressions(pan, '', n, 'UVs')
    return n

n1 = layer(0, 0.45, 0.018, 0.012)
n2 = layer(250, 0.85, -0.014, 0.020)

# Custom Expression para Normal em World Space (Z para cima = 0,0,1) + ondas analiticas suaves
custom_norm = expr(m, unreal.MaterialExpressionCustom, -450, 150)
custom_norm.set_editor_property('output_type', unreal.CustomMaterialOutputType.CMOT_FLOAT3)
custom_norm.set_editor_property('description', 'WorldSpaceWaterNormal')
custom_norm.set_editor_property('code', """
float2 p = WorldPos.xy * 0.008;
float2 wave = float2(
    sin(p.x * 1.7 + Time * 2.2) + 0.5 * cos(p.y * 2.6 - Time * 1.8),
    cos(p.y * 1.9 + Time * 1.9) + 0.5 * sin(p.x * 2.3 + Time * 2.5)
) * 0.08;
float2 nxy = (N1.xy + N2.xy) * 0.5 + wave;
return normalize(float3(nxy * Strength, 1.0));
""")
inps = []
for name in ['N1', 'N2', 'Strength', 'WorldPos', 'Time']:
    inp = unreal.CustomInput()
    inp.set_editor_property('input_name', name)
    inps.append(inp)
custom_norm.set_editor_property('inputs', inps)

strength = expr(m, unreal.MaterialExpressionScalarParameter, -700, 380, parameter_name='NormalStrength', default_value=0.14)
lib.connect_material_expressions(n1, 'RGB', custom_norm, 'N1')
lib.connect_material_expressions(n2, 'RGB', custom_norm, 'N2')
lib.connect_material_expressions(strength, '', custom_norm, 'Strength')
lib.connect_material_expressions(wp, '', custom_norm, 'WorldPos')
lib.connect_material_expressions(time, '', custom_norm, 'Time')
lib.connect_material_property(custom_norm, '', unreal.MaterialProperty.MP_NORMAL)

# Textura animada de causticas da agua do RO (water100..131.jpg) combinada com o tom cristalino
ro_tex = expr(m, unreal.MaterialExpressionTextureSampleParameter2D, -850, 550, parameter_name='Tex')
tint = expr(m, unreal.MaterialExpressionVectorParameter, -850, 750, parameter_name='Tint',
            default_value=unreal.LinearColor(0.06, 0.36, 0.55, 1.0))

water_col = expr(m, unreal.MaterialExpressionCustom, -450, 600)
water_col.set_editor_property('output_type', unreal.CustomMaterialOutputType.CMOT_FLOAT3)
water_col.set_editor_property('description', 'WaterCausticsColor')
water_col.set_editor_property('code', """
float luma = dot(TexColor.rgb, float3(0.299, 0.587, 0.114));
float caustic = smoothstep(0.42, 0.88, luma);
float3 base = lerp(Tint.rgb * 0.75, TexColor.rgb * float3(0.55, 0.85, 1.05), 0.52);
return base + float3(0.22, 0.36, 0.45) * (caustic * 0.65);
""")
col_inps = []
for name in ['TexColor', 'Tint']:
    inp = unreal.CustomInput()
    inp.set_editor_property('input_name', name)
    col_inps.append(inp)
water_col.set_editor_property('inputs', col_inps)
lib.connect_material_expressions(ro_tex, 'RGB', water_col, 'TexColor')
lib.connect_material_expressions(tint, '', water_col, 'Tint')
lib.connect_material_property(water_col, '', unreal.MaterialProperty.MP_BASE_COLOR)

# Roughness espelhada (0.04) e Specular (0.75) para reflexos suaves de agua real
rough = expr(m, unreal.MaterialExpressionConstant, -450, 780, r=0.04)
lib.connect_material_property(rough, '', unreal.MaterialProperty.MP_ROUGHNESS)
spec = expr(m, unreal.MaterialExpressionConstant, -450, 840, r=0.75)
lib.connect_material_property(spec, '', unreal.MaterialProperty.MP_SPECULAR)

opac = expr(m, unreal.MaterialExpressionScalarParameter, -450, 910, parameter_name='Opacity', default_value=0.68)
lib.connect_material_property(opac, '', unreal.MaterialProperty.MP_OPACITY)

# Refração suave da agua (IOR = 1.333)
ior = expr(m, unreal.MaterialExpressionConstant, -450, 980, r=1.08)
lib.connect_material_property(ior, '', unreal.MaterialProperty.MP_REFRACTION)

# Saida Single Layer Water: baixa dispersao leitosa (sem aspecto de massinha!) e absorcao cristalina
out = expr(m, unreal.MaterialExpressionSingleLayerWaterMaterialOutput, 0, 600)
scat = expr(m, unreal.MaterialExpressionConstant3Vector, -300, 600, constant=unreal.LinearColor(0.015, 0.055, 0.085, 1.0))
absorb = expr(m, unreal.MaterialExpressionConstant3Vector, -300, 700, constant=unreal.LinearColor(0.035, 0.012, 0.003, 1.0))
phase = expr(m, unreal.MaterialExpressionConstant, -300, 800, r=0.25)
lib.connect_material_expressions(scat, '', out, 'ScatteringCoefficients')
lib.connect_material_expressions(absorb, '', out, 'AbsorptionCoefficients')
lib.connect_material_expressions(phase, '', out, 'PhaseG')
lib.recompile_material(m)
unreal.EditorAssetLibrary.save_loaded_asset(m)
print('MATERIAIS DE AGUA criados em', PATH)

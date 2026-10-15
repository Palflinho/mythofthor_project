"""
Cria os materiais da agua dos mapas (usados por AMythWater, Source/MythOfThor/World/MythWater.cpp):
  /Game/Myth/M_Water        agua classica do RO: translucida, sem luz, dois lados; parametros Tex (water0NN.jpg), Brightness, Opacity.
  /Game/Myth/M_WaterModern  agua moderna: Single Layer Water da Unreal (refracao, absorcao com a profundidade, reflexo do ceu com
                            Lumen) com duas ondas de normal map correndo em direcoes diferentes; a textura do RO entra como tinta.
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

# ------------------------------------------------------------------ moderna (Single Layer Water)
m = fresh('M_WaterModern', unreal.Material, unreal.MaterialFactoryNew())
m.set_editor_property('shading_model', unreal.MaterialShadingModel.MSM_SINGLE_LAYER_WATER)
m.set_editor_property('blend_mode', unreal.BlendMode.BLEND_OPAQUE)
m.set_editor_property('two_sided', True)

# Duas camadas de onda de agua cristalina animada
time = expr(m, unreal.MaterialExpressionTime, -1500, 500)

def layer(y, scale, speed_x, speed_y):
    pan = expr(m, unreal.MaterialExpressionPanner, -1100, y, speed_x=speed_x, speed_y=speed_y)
    scl = expr(m, unreal.MaterialExpressionTextureCoordinate, -1300, y, u_tiling=scale, v_tiling=scale)
    lib.connect_material_expressions(scl, '', pan, 'Coordinate')
    lib.connect_material_expressions(time, '', pan, 'Time')
    n = expr(m, unreal.MaterialExpressionTextureSampleParameter2D, -800, y, parameter_name='Normal%d' % int(y), texture=normal_tex,
             sampler_type=unreal.MaterialSamplerType.SAMPLERTYPE_NORMAL)
    lib.connect_material_expressions(pan, '', n, 'UVs')
    return n

n1 = layer(0, 1.2, 0.012, 0.008)
n2 = layer(250, 2.0, -0.009, 0.014)

# Custom Expression para mescla de normais com Angle-Corrected Normal Blend
custom_norm = expr(m, unreal.MaterialExpressionCustom, -450, 150)
custom_norm.set_editor_property('output_type', unreal.CustomMaterialOutputType.CMOT_FLOAT3)
custom_norm.set_editor_property('description', 'BlendWaterNormals')
custom_norm.set_editor_property('code', """
float3 v1 = N1.xyz;
float3 v2 = N2.xyz;
float3 blended = float3(v1.xy + v2.xy, v1.z * v2.z);
return normalize(lerp(float3(0, 0, 1), blended, Strength));
""")
inps = []
for name in ['N1', 'N2', 'Strength']:
    inp = unreal.CustomInput()
    inp.set_editor_property('input_name', name)
    inps.append(inp)
custom_norm.set_editor_property('inputs', inps)

strength = expr(m, unreal.MaterialExpressionScalarParameter, -700, 320, parameter_name='NormalStrength', default_value=0.20)
lib.connect_material_expressions(n1, 'RGB', custom_norm, 'N1')
lib.connect_material_expressions(n2, 'RGB', custom_norm, 'N2')
lib.connect_material_expressions(strength, '', custom_norm, 'Strength')
lib.connect_material_property(custom_norm, '', unreal.MaterialProperty.MP_NORMAL)

# Base Color de agua limpa, cristalina e vibrante
tint = expr(m, unreal.MaterialExpressionVectorParameter, -800, 600, parameter_name='Tint',
            default_value=unreal.LinearColor(0.04, 0.42, 0.58, 1.0))
lib.connect_material_property(tint, '', unreal.MaterialProperty.MP_BASE_COLOR)

# Roughness super polida (0.02) e Specular (0.85) para reflexos vibrantes de sol e ceu
rough = expr(m, unreal.MaterialExpressionConstant, -450, 750, r=0.02)
lib.connect_material_property(rough, '', unreal.MaterialProperty.MP_ROUGHNESS)
spec = expr(m, unreal.MaterialExpressionConstant, -450, 820, r=0.85)
lib.connect_material_property(spec, '', unreal.MaterialProperty.MP_SPECULAR)

opac = expr(m, unreal.MaterialExpressionScalarParameter, -450, 900, parameter_name='Opacity', default_value=0.82)
lib.connect_material_property(opac, '', unreal.MaterialProperty.MP_OPACITY)

# Saida Single Layer Water: Scattering e Absorption calibrados para transparencia cristalina
out = expr(m, unreal.MaterialExpressionSingleLayerWaterMaterialOutput, 0, 600)
scat = expr(m, unreal.MaterialExpressionConstant3Vector, -300, 600, constant=unreal.LinearColor(0.20, 0.50, 0.65, 1.0))
absorb = expr(m, unreal.MaterialExpressionConstant3Vector, -300, 700, constant=unreal.LinearColor(0.06, 0.02, 0.005, 1.0))
lib.connect_material_expressions(scat, '', out, 'ScatteringCoefficients')
lib.connect_material_expressions(absorb, '', out, 'AbsorptionCoefficients')
lib.recompile_material(m)
unreal.EditorAssetLibrary.save_loaded_asset(m)
print('MATERIAIS DE AGUA criados em', PATH)

"""
Creates /Game/Myth/M_RoSprite_Lit: HD-2D Sprite Material with procedural Rim Light, Hit-Flash, and Death Dissolve.
Adheres to GDD HD-2D Section 4.1 (Hit-Flash), Section 4.3 (Rim Light), and Section 6.1 (Procedural Dissolve).

Usage:
  UnrealEditor-Cmd.exe mythofthor.uproject -run=pythonscript -script="tools/ue_make_sprite_lit_material.py"
"""
import unreal

PATH = '/Game/Myth'
NAME = 'M_RoSprite_Lit'
lib = unreal.MaterialEditingLibrary
tools = unreal.AssetToolsHelpers.get_asset_tools()

full = f"{PATH}/{NAME}"
if unreal.EditorAssetLibrary.does_asset_exist(full):
    unreal.EditorAssetLibrary.delete_asset(full)

mat = tools.create_asset(NAME, PATH, unreal.Material, unreal.MaterialFactoryNew())
mat.set_editor_property('material_domain', unreal.MaterialDomain.MD_SURFACE)
mat.set_editor_property('blend_mode', unreal.BlendMode.BLEND_MASKED)
mat.set_editor_property('shading_model', unreal.MaterialShadingModel.MSM_DEFAULT_LIT)
mat.set_editor_property('two_sided', True)

def expr(cls, x, y, **props):
    e = lib.create_material_expression(mat, cls, x, y)
    for k, v in props.items():
        e.set_editor_property(k, v)
    return e

# UV Coordinate
uv = expr(unreal.MaterialExpressionTextureCoordinate, -1400, -200)

# Sprite Texture Parameter
tex_sample = expr(unreal.MaterialExpressionTextureSampleParameter2D, -1100, -200, parameter_name='SpriteTexture')
lib.connect_material_expressions(uv, '', tex_sample, 'UVs')

# Vertex Color (modula luz ambiente dia/noite e alpha da camada do .act)
vert_color = expr(unreal.MaterialExpressionVertexColor, -1100, -50)

# Base Color = SpriteTexture.RGB * VertexColor.RGB
base_mult = expr(unreal.MaterialExpressionMultiply, -850, -200)
lib.connect_material_expressions(tex_sample, 'RGB', base_mult, 'A')
lib.connect_material_expressions(vert_color, 'RGB', base_mult, 'B')
lib.connect_material_property(base_mult, '', unreal.MaterialProperty.MP_BASE_COLOR)

# Dissolve Parameters
dissolve_prog = expr(unreal.MaterialExpressionScalarParameter, -1100, 100, parameter_name='DissolveProgress', default_value=0.0)
dissolve_color = expr(unreal.MaterialExpressionVectorParameter, -1100, 220, parameter_name='DissolveGlowColor',
                      default_value=unreal.LinearColor(1.0, 0.45, 0.1, 1.0))

# Opacity Mask Custom Node with Dissolve and Vertex Alpha
alpha_mult = expr(unreal.MaterialExpressionMultiply, -850, 50)
lib.connect_material_expressions(tex_sample, 'A', alpha_mult, 'A')
lib.connect_material_expressions(vert_color, 'A', alpha_mult, 'B')

mask_custom = expr(unreal.MaterialExpressionCustom, -700, 50)
mask_custom.set_editor_property('output_type', unreal.CustomMaterialOutputType.CMOT_FLOAT1)
mask_custom.set_editor_property('description', 'DissolveMask')

mask_hlsl = """
if (Alpha < 0.33) return 0.0;
if (Progress <= 0.001) return 1.0;

float2 p = floor(UV * 512.0);
float n = frac(sin(dot(p, float2(12.9898, 78.233))) * 43758.5453);
float threshold = Progress * 1.3 - (1.0 - UV.y) * 0.3;
return (n >= threshold) ? 1.0 : 0.0;
"""
mask_custom.set_editor_property('code', mask_hlsl)

mask_inputs = []
for name in ['Alpha', 'UV', 'Progress']:
    inp = unreal.CustomInput()
    inp.set_editor_property('input_name', name)
    mask_inputs.append(inp)
mask_custom.set_editor_property('inputs', mask_inputs)

lib.connect_material_expressions(alpha_mult, '', mask_custom, 'Alpha')
lib.connect_material_expressions(uv, '', mask_custom, 'UV')
lib.connect_material_expressions(dissolve_prog, '', mask_custom, 'Progress')

lib.connect_material_property(mask_custom, '', unreal.MaterialProperty.MP_OPACITY_MASK)


# Hit Flash Parameters
hit_flash = expr(unreal.MaterialExpressionScalarParameter, -1100, 350, parameter_name='HitFlash', default_value=0.0)
hit_color = expr(unreal.MaterialExpressionVectorParameter, -1100, 500, parameter_name='HitFlashColor',
                 default_value=unreal.LinearColor(1.0, 1.0, 1.0, 1.0))
flash_mult = expr(unreal.MaterialExpressionMultiply, -750, 400)
lib.connect_material_expressions(hit_flash, '', flash_mult, 'A')
lib.connect_material_expressions(hit_color, '', flash_mult, 'B')

# Rim Light Parameters (desativado por padrao para evitar contorno branco artificial)
rim_intensity = expr(unreal.MaterialExpressionScalarParameter, -1100, -500, parameter_name='RimIntensity', default_value=0.0)
rim_color = expr(unreal.MaterialExpressionVectorParameter, -1100, -350, parameter_name='RimColor',
                 default_value=unreal.LinearColor(0.0, 0.0, 0.0, 0.0))

# Texel Size for Atlas sampling (approx 1024x1024)
texel_size = expr(unreal.MaterialExpressionConstant2Vector, -1100, -650, r=0.0009765625, g=0.0009765625)

# Texture Object for HLSL sampling
tex_obj = expr(unreal.MaterialExpressionTextureObjectParameter, -1100, -800, parameter_name='SpriteTexture')

# Custom Expression for Dissolve Glow (sem borda branca artificial)
custom = expr(unreal.MaterialExpressionCustom, -600, -500)
custom.set_editor_property('output_type', unreal.CustomMaterialOutputType.CMOT_FLOAT3)
custom.set_editor_property('description', 'SpriteDissolveGlow')

hlsl_code = """
float centerA = Texture2DSample(Tex, TexSampler, UV).a;
if (centerA < 0.5) return float3(0,0,0);

float3 result = float3(0.0, 0.0, 0.0);

if (Progress > 0.001)
{
    float2 p = floor(UV * 512.0);
    float n = frac(sin(dot(p, float2(12.9898, 78.233))) * 43758.5453);
    float threshold = Progress * 1.3 - (1.0 - UV.y) * 0.3;
    float diff = n - threshold;
    if (diff >= 0.0 && diff < 0.08)
    {
        result += DissolveCol.rgb * ((1.0 - diff / 0.08) * 3.5);
    }
}
return result;
"""
custom.set_editor_property('code', hlsl_code)

inputs = []
for name in ['Tex', 'UV', 'TexelSize', 'RimCol', 'Intensity', 'Progress', 'DissolveCol']:
    inp = unreal.CustomInput()
    inp.set_editor_property('input_name', name)
    inputs.append(inp)
custom.set_editor_property('inputs', inputs)

lib.connect_material_expressions(tex_obj, '', custom, 'Tex')
lib.connect_material_expressions(uv, '', custom, 'UV')
lib.connect_material_expressions(texel_size, '', custom, 'TexelSize')
lib.connect_material_expressions(rim_color, '', custom, 'RimCol')
lib.connect_material_expressions(rim_intensity, '', custom, 'Intensity')
lib.connect_material_expressions(dissolve_prog, '', custom, 'Progress')
lib.connect_material_expressions(dissolve_color, '', custom, 'DissolveCol')

# Combine Rim Light + Hit Flash into Emissive
emissive_add = expr(unreal.MaterialExpressionAdd, -300, 0)
lib.connect_material_expressions(custom, '', emissive_add, 'A')
lib.connect_material_expressions(flash_mult, '', emissive_add, 'B')

lib.connect_material_property(emissive_add, '', unreal.MaterialProperty.MP_EMISSIVE_COLOR)

lib.recompile_material(mat)
unreal.EditorAssetLibrary.save_loaded_asset(mat)
unreal.log(f"[MythSpriteLit] Successfully created and compiled {full}")

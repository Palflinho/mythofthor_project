"""
Creates /Game/Myth/M_RoSprite_Lit: HD-2D Sprite Material with procedural Rim Light and Hit-Flash.
Adheres to GDD HD-2D Section 4.1 (Multilayer Hit-Flash) and Section 4.3 (Sprite Rim Light).

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
uv = expr(unreal.MaterialExpressionTextureCoordinate, -1200, -200)

# Sprite Texture Parameter
tex_sample = expr(unreal.MaterialExpressionTextureSampleParameter2D, -900, -200, parameter_name='SpriteTexture')
lib.connect_material_expressions(uv, '', tex_sample, 'UVs')

# Base Color & Opacity Mask
lib.connect_material_property(tex_sample, 'RGB', unreal.MaterialProperty.MP_BASE_COLOR)
lib.connect_material_property(tex_sample, 'A', unreal.MaterialProperty.MP_OPACITY_MASK)

# Hit Flash Parameters
hit_flash = expr(unreal.MaterialExpressionScalarParameter, -900, 200, parameter_name='HitFlash', default_value=0.0)
hit_color = expr(unreal.MaterialExpressionVectorParameter, -900, 350, parameter_name='HitFlashColor',
                 default_value=unreal.LinearColor(1.0, 1.0, 1.0, 1.0))
flash_mult = expr(unreal.MaterialExpressionMultiply, -650, 250)
lib.connect_material_expressions(hit_flash, '', flash_mult, 'A')
lib.connect_material_expressions(hit_color, '', flash_mult, 'B')

# Rim Light Parameters
rim_intensity = expr(unreal.MaterialExpressionScalarParameter, -900, -500, parameter_name='RimIntensity', default_value=0.75)
rim_color = expr(unreal.MaterialExpressionVectorParameter, -900, -350, parameter_name='RimColor',
                 default_value=unreal.LinearColor(0.85, 0.9, 1.0, 1.0))

# Texel Size for Atlas sampling (approx 1024x1024)
texel_size = expr(unreal.MaterialExpressionConstant2Vector, -900, -650, r=0.0009765625, g=0.0009765625)

# Texture Object for HLSL sampling
tex_obj = expr(unreal.MaterialExpressionTextureObjectParameter, -900, -800, parameter_name='SpriteTexture')

# Custom Expression for Rim Light Edge Detection
custom = expr(unreal.MaterialExpressionCustom, -500, -500)
custom.set_editor_property('output_type', unreal.CustomMaterialOutputType.CMOT_FLOAT3)
custom.set_editor_property('description', 'SpriteRimLight')

hlsl_code = """
float centerA = Texture2DSample(Tex, TexSampler, UV).a;
if (centerA < 0.5) return float3(0,0,0);

float aL = Texture2DSample(Tex, TexSampler, UV - float2(TexelSize.x, 0)).a;
float aR = Texture2DSample(Tex, TexSampler, UV + float2(TexelSize.x, 0)).a;
float aU = Texture2DSample(Tex, TexSampler, UV - float2(0, TexelSize.y)).a;
float aD = Texture2DSample(Tex, TexSampler, UV + float2(0, TexelSize.y)).a;

float minN = min(min(aL, aR), min(aU, aD));
float edge = saturate(1.0 - minN);
return RimCol.rgb * (edge * Intensity);
"""
custom.set_editor_property('code', hlsl_code)

inputs = []
for name in ['Tex', 'UV', 'TexelSize', 'RimCol', 'Intensity']:
    inp = unreal.CustomInput()
    inp.set_editor_property('input_name', name)
    inputs.append(inp)
custom.set_editor_property('inputs', inputs)

lib.connect_material_expressions(tex_obj, '', custom, 'Tex')
lib.connect_material_expressions(uv, '', custom, 'UV')
lib.connect_material_expressions(texel_size, '', custom, 'TexelSize')
lib.connect_material_expressions(rim_color, '', custom, 'RimCol')
lib.connect_material_expressions(rim_intensity, '', custom, 'Intensity')

# Combine Rim Light + Hit Flash into Emissive
emissive_add = expr(unreal.MaterialExpressionAdd, -300, 0)
lib.connect_material_expressions(custom, '', emissive_add, 'A')
lib.connect_material_expressions(flash_mult, '', emissive_add, 'B')

lib.connect_material_property(emissive_add, '', unreal.MaterialProperty.MP_EMISSIVE_COLOR)

lib.recompile_material(mat)
unreal.EditorAssetLibrary.save_loaded_asset(mat)
unreal.log(f"[MythSpriteLit] Successfully created and compiled {full}")

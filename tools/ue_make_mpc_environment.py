"""
Creates /Game/Myth/MPC_Environment: Material Parameter Collection for HD-2D dynamic weather wetness.
Parameters:
  - Wetness (Scalar, default 0.0, range 0.0..1.0)
  - RainIntensity (Scalar, default 0.0, range 0.0..1.0)
  - PuddleColor (Vector, default [0.05, 0.08, 0.12, 1.0])
"""
import unreal

PATH = '/Game/Myth'
NAME = 'MPC_Environment'
full = f"{PATH}/{NAME}"

tools = unreal.AssetToolsHelpers.get_asset_tools()

if unreal.EditorAssetLibrary.does_asset_exist(full):
    unreal.EditorAssetLibrary.delete_asset(full)

factory = unreal.MaterialParameterCollectionFactoryNew()
mpc = tools.create_asset(NAME, PATH, unreal.MaterialParameterCollection, factory)

# Adiciona o scalar parameter Wetness
sp_wetness = unreal.CollectionScalarParameter()
sp_wetness.set_editor_property('parameter_name', 'Wetness')
sp_wetness.set_editor_property('default_value', 0.0)

sp_rain = unreal.CollectionScalarParameter()
sp_rain.set_editor_property('parameter_name', 'RainIntensity')
sp_rain.set_editor_property('default_value', 0.0)

# Adiciona vector parameter
vp_puddle = unreal.CollectionVectorParameter()
vp_puddle.set_editor_property('parameter_name', 'PuddleColor')
vp_puddle.set_editor_property('default_value', unreal.LinearColor(0.05, 0.08, 0.12, 1.0))

mpc.set_editor_property('scalar_parameters', [sp_wetness, sp_rain])
mpc.set_editor_property('vector_parameters', [vp_puddle])

unreal.EditorAssetLibrary.save_loaded_asset(mpc)
print(f"Successfully created {full} with Wetness and RainIntensity parameters.")

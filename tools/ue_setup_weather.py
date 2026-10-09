"""
Myth of Thor - materiais do clima HD-2D (roda DENTRO do editor / headless)
---------------------------------------------------------------------------
Cria /Game/Weather/M_RainStreak e /Game/Weather/M_SnowFlake: translucidos, sem iluminacao, dois lados.
  chuva: risco fino (opacidade cai para as bordas laterais e para as pontas);
  neve : floco redondo (opacidade radial).
Usados pelo AMythWeatherActor (C++), que espalha instancias de um plano em volta da camera.

Uso headless:
  UnrealEditor-Cmd.exe mythofthor.uproject -run=pythonscript -script="tools/ue_setup_weather.py"
"""
import unreal

eal = unreal.EditorAssetLibrary
mel = unreal.MaterialEditingLibrary
tools = unreal.AssetToolsHelpers.get_asset_tools()
FOLDER = '/Game/Weather'


def new_material(name):
    path = '%s/%s' % (FOLDER, name)
    if eal.does_asset_exist(path):
        mat = eal.load_asset(path)
        mel.delete_all_material_expressions(mat)
    else:
        mat = tools.create_asset(name, FOLDER, unreal.Material, unreal.MaterialFactoryNew())
    mat.set_editor_property('blend_mode', unreal.BlendMode.BLEND_TRANSLUCENT)
    mat.set_editor_property('shading_model', unreal.MaterialShadingModel.MSM_UNLIT)
    mat.set_editor_property('two_sided', True)
    mat.set_editor_property('used_with_instanced_static_meshes', True)  # particulas sao instancias (ISM)
    return mat


def const(mat, value, x, y):
    c = mel.create_material_expression(mat, unreal.MaterialExpressionConstant, x, y)
    c.set_editor_property('r', value)
    return c


def uv_channel(mat, uv, wu, wv, x, y):
    w = mel.create_material_expression(mat, unreal.MaterialExpressionConstant2Vector, x - 120, y + 40)
    w.set_editor_property('r', wu)
    w.set_editor_property('g', wv)
    d = mel.create_material_expression(mat, unreal.MaterialExpressionDotProduct, x, y)
    mel.connect_material_expressions(uv, '', d, 'A')
    mel.connect_material_expressions(w, '', d, 'B')
    return d


def build_rain():
    mat = new_material('M_RainStreak')
    color = mel.create_material_expression(mat, unreal.MaterialExpressionConstant3Vector, -400, -200)
    color.set_editor_property('constant', unreal.LinearColor(0.78, 0.84, 0.95, 1.0))
    mel.connect_material_property(color, '', unreal.MaterialProperty.MP_EMISSIVE_COLOR)

    # opacidade = (1 - |U*2-1|) * sin(V*pi) * 0.45  -> mais forte no meio do risco
    uv = mel.create_material_expression(mat, unreal.MaterialExpressionTextureCoordinate, -900, 100)
    u = uv_channel(mat, uv, 1.0, 0.0, -750, 50)  # ComponentMask criado pelo Python nao isola o canal
    v = uv_channel(mat, uv, 0.0, 1.0, -750, 200)

    u2 = mel.create_material_expression(mat, unreal.MaterialExpressionMultiply, -600, 50)
    mel.connect_material_expressions(u, '', u2, 'A')
    mel.connect_material_expressions(const(mat, 2.0, -750, 120), '', u2, 'B')
    um1 = mel.create_material_expression(mat, unreal.MaterialExpressionSubtract, -480, 50)
    mel.connect_material_expressions(u2, '', um1, 'A')
    mel.connect_material_expressions(const(mat, 1.0, -600, 120), '', um1, 'B')
    ua = mel.create_material_expression(mat, unreal.MaterialExpressionAbs, -360, 50)
    mel.connect_material_expressions(um1, '', ua, '')
    uo = mel.create_material_expression(mat, unreal.MaterialExpressionOneMinus, -250, 50)
    mel.connect_material_expressions(ua, '', uo, '')

    vpi = mel.create_material_expression(mat, unreal.MaterialExpressionMultiply, -600, 200)
    mel.connect_material_expressions(v, '', vpi, 'A')
    mel.connect_material_expressions(const(mat, 3.14159, -750, 260), '', vpi, 'B')
    vs = mel.create_material_expression(mat, unreal.MaterialExpressionSine, -480, 200)
    vs.set_editor_property('period', 6.28318)
    mel.connect_material_expressions(vpi, '', vs, '')

    m1 = mel.create_material_expression(mat, unreal.MaterialExpressionMultiply, -150, 100)
    mel.connect_material_expressions(uo, '', m1, 'A')
    mel.connect_material_expressions(vs, '', m1, 'B')
    m2 = mel.create_material_expression(mat, unreal.MaterialExpressionMultiply, -40, 100)
    mel.connect_material_expressions(m1, '', m2, 'A')
    mel.connect_material_expressions(const(mat, 0.6, -150, 200), '', m2, 'B')
    mel.connect_material_property(m2, '', unreal.MaterialProperty.MP_OPACITY)
    mel.recompile_material(mat)
    eal.save_loaded_asset(mat)
    unreal.log('[Weather] M_RainStreak pronto')


def build_snow():
    mat = new_material('M_SnowFlake')
    color = mel.create_material_expression(mat, unreal.MaterialExpressionConstant3Vector, -400, -200)
    color.set_editor_property('constant', unreal.LinearColor(0.96, 0.97, 1.0, 1.0))
    mel.connect_material_property(color, '', unreal.MaterialProperty.MP_EMISSIVE_COLOR)

    # opacidade = saturate(1 - distancia(UV, centro) * 2) * 0.9
    uv = mel.create_material_expression(mat, unreal.MaterialExpressionTextureCoordinate, -800, 100)
    center = mel.create_material_expression(mat, unreal.MaterialExpressionConstant2Vector, -800, 220)
    center.set_editor_property('r', 0.5)
    center.set_editor_property('g', 0.5)
    dist = mel.create_material_expression(mat, unreal.MaterialExpressionDistance, -600, 150)
    mel.connect_material_expressions(uv, '', dist, 'A')
    mel.connect_material_expressions(center, '', dist, 'B')
    d2 = mel.create_material_expression(mat, unreal.MaterialExpressionMultiply, -460, 150)
    mel.connect_material_expressions(dist, '', d2, 'A')
    mel.connect_material_expressions(const(mat, 2.0, -600, 250), '', d2, 'B')
    om = mel.create_material_expression(mat, unreal.MaterialExpressionOneMinus, -340, 150)
    mel.connect_material_expressions(d2, '', om, '')
    sat = mel.create_material_expression(mat, unreal.MaterialExpressionSaturate, -230, 150)
    mel.connect_material_expressions(om, '', sat, '')
    m = mel.create_material_expression(mat, unreal.MaterialExpressionMultiply, -120, 150)
    mel.connect_material_expressions(sat, '', m, 'A')
    mel.connect_material_expressions(const(mat, 0.9, -230, 250), '', m, 'B')
    mel.connect_material_property(m, '', unreal.MaterialProperty.MP_OPACITY)
    mel.recompile_material(mat)
    eal.save_loaded_asset(mat)
    unreal.log('[Weather] M_SnowFlake pronto')


eal.make_directory(FOLDER)
build_rain()
build_snow()


def build_indoor_void():
    """Piso preto sem iluminacao sob os mapas internos (o vazio entre as salas fica preto, como no RO)."""
    path = '/Game/Myth/M_IndoorVoid'
    eal.make_directory('/Game/Myth')
    if eal.does_asset_exist(path):
        mat = eal.load_asset(path)
        mel.delete_all_material_expressions(mat)
    else:
        mat = tools.create_asset('M_IndoorVoid', '/Game/Myth', unreal.Material, unreal.MaterialFactoryNew())
    mat.set_editor_property('shading_model', unreal.MaterialShadingModel.MSM_UNLIT)
    black = mel.create_material_expression(mat, unreal.MaterialExpressionConstant3Vector, -300, 0)
    black.set_editor_property('constant', unreal.LinearColor(0.0, 0.0, 0.0, 1.0))
    mel.connect_material_property(black, '', unreal.MaterialProperty.MP_EMISSIVE_COLOR)
    mel.recompile_material(mat)
    eal.save_loaded_asset(mat)
    unreal.log('[Weather] M_IndoorVoid pronto')


build_indoor_void()


def build_item_beam():
    """
    Feixe de luz dos itens raros no chao (estilo Diablo 4): aditivo, sem iluminacao, cor por parametro.
    Brilho = Color * Strength * (1 - |2U - 1|)^2 * (1 - V)  (forte no centro e na base, some para cima).
    """
    path = '/Game/Myth/M_ItemBeam'
    eal.make_directory('/Game/Myth')
    if eal.does_asset_exist(path):
        mat = eal.load_asset(path)
        mel.delete_all_material_expressions(mat)
    else:
        mat = tools.create_asset('M_ItemBeam', '/Game/Myth', unreal.Material, unreal.MaterialFactoryNew())
    mat.set_editor_property('blend_mode', unreal.BlendMode.BLEND_ADDITIVE)
    mat.set_editor_property('shading_model', unreal.MaterialShadingModel.MSM_UNLIT)
    mat.set_editor_property('two_sided', True)

    uv = mel.create_material_expression(mat, unreal.MaterialExpressionTextureCoordinate, -1000, 0)
    u = uv_channel(mat, uv, 1.0, 0.0, -850, -60)
    v = uv_channel(mat, uv, 0.0, 1.0, -850, 120)

    # horizontal: (1 - |2U - 1|)^2
    u2 = mel.create_material_expression(mat, unreal.MaterialExpressionMultiply, -700, -60)
    mel.connect_material_expressions(u, '', u2, 'A')
    mel.connect_material_expressions(const(mat, 2.0, -850, 0), '', u2, 'B')
    um1 = mel.create_material_expression(mat, unreal.MaterialExpressionSubtract, -580, -60)
    mel.connect_material_expressions(u2, '', um1, 'A')
    mel.connect_material_expressions(const(mat, 1.0, -700, 10), '', um1, 'B')
    ua = mel.create_material_expression(mat, unreal.MaterialExpressionAbs, -470, -60)
    mel.connect_material_expressions(um1, '', ua, '')
    uo = mel.create_material_expression(mat, unreal.MaterialExpressionOneMinus, -370, -60)
    mel.connect_material_expressions(ua, '', uo, '')
    usq = mel.create_material_expression(mat, unreal.MaterialExpressionMultiply, -270, -60)
    mel.connect_material_expressions(uo, '', usq, 'A')
    mel.connect_material_expressions(uo, '', usq, 'B')

    # vertical: 1 - V (V = 0 no topo do plano, 1 na base -> invertido abaixo conforme a orientacao do plano)
    vo = mel.create_material_expression(mat, unreal.MaterialExpressionOneMinus, -700, 120)
    mel.connect_material_expressions(v, '', vo, '')

    fade = mel.create_material_expression(mat, unreal.MaterialExpressionMultiply, -150, 20)
    mel.connect_material_expressions(usq, '', fade, 'A')
    mel.connect_material_expressions(vo, '', fade, 'B')

    color = mel.create_material_expression(mat, unreal.MaterialExpressionVectorParameter, -300, 220)
    color.set_editor_property('parameter_name', 'Color')
    color.set_editor_property('default_value', unreal.LinearColor(1.0, 0.6, 0.1, 1.0))
    strength = mel.create_material_expression(mat, unreal.MaterialExpressionScalarParameter, -300, 320)
    strength.set_editor_property('parameter_name', 'Strength')
    strength.set_editor_property('default_value', 3.0)
    cs = mel.create_material_expression(mat, unreal.MaterialExpressionMultiply, -150, 240)
    mel.connect_material_expressions(color, '', cs, 'A')
    mel.connect_material_expressions(strength, '', cs, 'B')
    out = mel.create_material_expression(mat, unreal.MaterialExpressionMultiply, 0, 100)
    mel.connect_material_expressions(cs, '', out, 'A')
    mel.connect_material_expressions(fade, '', out, 'B')
    mel.connect_material_property(out, '', unreal.MaterialProperty.MP_EMISSIVE_COLOR)
    mel.recompile_material(mat)
    eal.save_loaded_asset(mat)
    unreal.log('[Weather] M_ItemBeam pronto')


build_item_beam()

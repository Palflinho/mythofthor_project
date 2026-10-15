"""
Myth of Thor - Importador de mapas compilados (.obj) para o Unreal Editor
-------------------------------------------------------------------------
A Unreal nao tem importador de OBJ, entao este script le o .obj/.mtl gerado por
tools/compile_map_to_obj.py e monta tudo pela API Python do editor:

  1. Importa as texturas (corrigindo os nomes coreanos corrompidos das pastas) e cria 2 materiais-mae
     (opaco e mascarado por magenta) + 1 Material Instance por material do .mtl;
  2. Constroi o StaticMesh (SM_<nome>) com "Use Complex Collision As Simple";
  3. Cria o Level <dest>/Lvl_<Nome> com a malha em (0,0,0), iluminacao HD-2D (Directional Light, SkyLight,
     SkyAtmosphere, Exponential Height Fog, PostProcessVolume com Lumen), NavMeshBoundsVolume, PlayerStart
     e um AMythMapInfo (nome do mapa, origem do grid de celulas e BGM);
  4. Importa a BGM (wav/ogg/flac/mp3) e a liga ao AMythMapInfo (toca pelo submix dia/noite).

Uso headless (a partir da raiz do projeto; o caminho do projeto nao pode ter acentos no Build/UBT):
  UnrealEditor-Cmd.exe mythofthor.uproject -run=pythonscript -script="tools/ue_import_map.py <obj> [opcoes]"
ou no editor (Output Log > Python):
  py tools/ue_import_map.py <obj> [opcoes]

Opcoes:
  --name NOME          nome base dos assets (padrao: nome do .obj sem "_full")
  --map-name NOME      nome do mapa no rAthena (padrao: --name)
  --dest /Game/...     pasta de destino (padrao: /Game/Maps/<Nome>)
  --scale N            unidades da Unreal por unidade do .obj (padrao 10 -> celula do RO = 50 cm)
  --texture-dir DIR    pasta de texturas (padrao: <pasta do obj>/texture)
  --bgm ARQUIVO        musica de fundo (wav/ogg/flac/mp3, importada como copia)
  --start-cell X,Y     celula do PlayerStart (padrao: centro do mapa)
  --no-lighting        nao cria a iluminacao
Exemplos:
  ue_import_map.py D:/Ragnarok_Assets/Prontera/Prontera_Cidade/prontera_full.obj --bgm D:/Ragnarok_Assets/BGM/08.mp3 --start-cell 156,191
  ue_import_map.py D:/Ragnarok_Assets/.../new_zone01_full.obj --map-name new_1-1

Log completo em <projeto>/Saved/MythImport/import.log.
"""
import os
import re
import shutil
import sys

import unreal

LOG = 'MythImport'
_LOG_FILE = None


def _emit(level, msg):
    global _LOG_FILE
    line = '[%s] %s' % (LOG, msg)
    (unreal.log_warning if level == 'W' else unreal.log)(line)
    try:
        if _LOG_FILE is None:
            d = os.path.join(unreal.Paths.project_saved_dir(), 'MythImport')
            os.makedirs(d, exist_ok=True)
            _LOG_FILE = open(os.path.join(d, 'import.log'), 'w', encoding='utf-8')
        _LOG_FILE.write(level + ' ' + line + chr(10))
        _LOG_FILE.flush()
    except Exception:  # noqa: BLE001
        pass


def log(msg):
    _emit('I', msg)


def warn(msg):
    _emit('W', msg)


# ----------------------------------------------------------------------------- argumentos
def parse_args(argv):
    args = {'obj': None, 'name': None, 'map_name': None, 'dest': None, 'scale': 10.0, 'texture_dir': None,
            'bgm': None, 'start_cell': None, 'lighting': True, 'water_level': None}
    i = 0
    while i < len(argv):
        a = argv[i]
        if a in ('--name', '--map-name', '--dest', '--scale', '--texture-dir', '--bgm', '--start-cell', '--water-level'):
            args[a[2:].replace('-', '_')] = argv[i + 1]
            i += 2
        elif a == '--no-lighting':
            args['lighting'] = False
            i += 1
        else:
            if args['obj'] is None:
                args['obj'] = a
            i += 1
    if not args['obj']:
        raise SystemExit('Uso: ue_import_map.py <arquivo.obj> [opcoes] (veja o cabecalho do script)')
    args['scale'] = float(args['scale'])
    base = re.sub(r'_full$', '', os.path.splitext(os.path.basename(args['obj']))[0])
    args['name'] = re.sub(r'[^A-Za-z0-9_]', '_', args['name'] or base)
    args['map_name'] = args['map_name'] or args['name']
    args['title'] = ''.join(p.capitalize() for p in re.split(r'_+', args['name']) if p)
    args['dest'] = (args['dest'] or '/Game/Maps/%s' % args['title']).rstrip('/')
    if args['start_cell']:
        x, y = args['start_cell'].split(',')
        args['start_cell'] = (int(x), int(y))
    return args


# ----------------------------------------------------------------------------- parsers
def parse_mtl(path):
    """material -> nome do arquivo de textura (ou None)."""
    mats, cur = {}, None
    with open(path, encoding='utf-8', errors='replace') as f:
        for line in f:
            line = line.strip()
            if line.startswith('newmtl '):
                cur = line[7:].strip()
                mats[cur] = None
            elif line.startswith('map_Kd') and cur:
                mats[cur] = os.path.basename(line.split(None, 1)[1].strip().replace(chr(92), '/'))
    return mats


def parse_obj(path):
    """Devolve vertices, uvs, triangulos [(material, (v0,v1,v2), (t0,t1,t2))], vertices do 'Ground' e o mtllib."""
    verts, uvs, tris = [], [], []
    ground_start, ground_end = None, None
    cur_obj, cur_mat, mtllib = None, None, None
    with open(path, encoding='utf-8', errors='replace') as f:
        for line in f:
            if line.startswith('v '):
                p = line.split()
                verts.append((float(p[1]), float(p[2]), float(p[3])))
            elif line.startswith('vt '):
                p = line.split()
                uvs.append((float(p[1]), float(p[2])))
            elif line.startswith('f '):
                idx = []
                for tok in line.split()[1:]:
                    parts = tok.split('/')
                    v = int(parts[0])
                    t = int(parts[1]) if len(parts) > 1 and parts[1] else 0
                    idx.append((v - 1 if v > 0 else len(verts) + v, t - 1 if t > 0 else (len(uvs) + t if t < 0 else -1)))
                for k in range(1, len(idx) - 1):  # triangulacao em leque
                    tris.append((cur_mat, (idx[0][0], idx[k][0], idx[k + 1][0]), (idx[0][1], idx[k][1], idx[k + 1][1])))
            elif line.startswith('usemtl '):
                cur_mat = line[7:].strip()
            elif line.startswith('o ') or line.startswith('g '):
                if cur_obj == 'Ground' and ground_end is None:
                    ground_end = len(verts)
                cur_obj = line[2:].strip()
                if cur_obj == 'Ground' and ground_start is None:
                    ground_start = len(verts)
            elif line.startswith('mtllib '):
                mtllib = line[7:].strip()
    if ground_start is not None and ground_end is None:
        ground_end = len(verts)
    ground = verts[ground_start:ground_end] if ground_start is not None else verts
    return verts, uvs, tris, ground, mtllib


# ----------------------------------------------------------------------------- texturas
def fix_korean(name):
    """Os nomes no disco sao CP949 lidos como CP1252/Latin-1 ('ÇÁ·ÐÅ×¶ó'); reverte para coreano."""
    raw = bytearray()
    for ch in name:
        try:
            raw += ch.encode('cp1252')
        except UnicodeEncodeError:
            try:
                raw += ch.encode('latin-1')
            except UnicodeEncodeError:
                raw += ch.encode('utf-8')
    try:
        return bytes(raw).decode('cp949')
    except UnicodeDecodeError:
        return name


def index_textures(tex_dir):
    found = {}
    for root, _, files in os.walk(tex_dir):
        for fn in files:
            for key in (fn.lower(), fix_korean(fn).lower()):
                found.setdefault(key, os.path.join(root, fn))
    return found


def bmp_has_magenta(path):
    """True se o BMP tem pixels magenta (255,0,255): chave de transparencia do Ragnarok."""
    try:
        with open(path, 'rb') as f:
            d = f.read()
        if d[:2] != b'BM':
            return False
        off = int.from_bytes(d[10:14], 'little')
        hdr = int.from_bytes(d[14:18], 'little')
        w = int.from_bytes(d[18:22], 'little', signed=True)
        h = abs(int.from_bytes(d[22:26], 'little', signed=True))
        bpp = int.from_bytes(d[28:30], 'little')
        if bpp == 8:
            pal = d[14 + hdr:off]
            for i in set(d[off:off + w * h + 4 * h]):
                if i * 4 + 2 < len(pal):
                    b, g, r = pal[i * 4], pal[i * 4 + 1], pal[i * 4 + 2]
                    if r > 230 and g < 30 and b > 230:
                        return True
            return False
        if bpp in (24, 32):
            step = bpp // 8
            row = ((w * step + 3) // 4) * 4
            for y in range(h):
                base = off + y * row
                for x in range(0, w * step, step):
                    b, g, r = d[base + x], d[base + x + 1], d[base + x + 2]
                    if r > 230 and g < 30 and b > 230:
                        return True
    except Exception as e:  # noqa: BLE001
        warn('nao foi possivel inspecionar %s: %s' % (path, e))
    return False


def ascii_stem(name, idx):
    stem = re.sub(r'[^A-Za-z0-9_]+', '', os.path.splitext(name)[0]).strip('_')
    return ('%s_%03d' % (stem, idx)) if stem else ('tex_%03d' % idx)


# ----------------------------------------------------------------------------- helpers de asset
eal = unreal.EditorAssetLibrary
tools = unreal.AssetToolsHelpers.get_asset_tools()
mel = unreal.MaterialEditingLibrary


def try_set(obj, prop, value):
    try:
        obj.set_editor_property(prop, value)
        return True
    except Exception as e:  # noqa: BLE001
        warn('propriedade %s.%s nao aplicada: %s' % (type(obj).__name__, prop, e))
        return False


def make_asset(name, folder, cls, factory):
    path = '%s/%s' % (folder, name)
    if eal.does_asset_exist(path):
        eal.delete_asset(path)
    return tools.create_asset(name, folder, cls, factory)


def build_parent_materials(folder):
    """Cria M_RO_Opaque e M_RO_Masked (parametro de textura 'Tex'). Devolve {masked: material}."""
    result = {}
    for masked in (False, True):
        mat = make_asset('M_RO_Masked' if masked else 'M_RO_Opaque', folder, unreal.Material, unreal.MaterialFactoryNew())
        mat.set_editor_property('two_sided', True)  # as normais/winding do .obj nao sao confiaveis
        tex = mel.create_material_expression(mat, unreal.MaterialExpressionTextureSampleParameter2D, -700, 0)
        tex.set_editor_property('parameter_name', 'Tex')
        mel.connect_material_property(tex, 'RGB', unreal.MaterialProperty.MP_BASE_COLOR)

        rough = mel.create_material_expression(mat, unreal.MaterialExpressionConstant, -300, 200)
        rough.set_editor_property('r', 0.95)
        mel.connect_material_property(rough, '', unreal.MaterialProperty.MP_ROUGHNESS)
        spec = mel.create_material_expression(mat, unreal.MaterialExpressionConstant, -300, 300)
        spec.set_editor_property('r', 0.0)
        mel.connect_material_property(spec, '', unreal.MaterialProperty.MP_SPECULAR)

        if masked:
            # opacidade = saturate((distancia(cor, magenta) - 0.2) * 30): magenta vira buraco
            mat.set_editor_property('blend_mode', unreal.BlendMode.BLEND_MASKED)
            key = mel.create_material_expression(mat, unreal.MaterialExpressionConstant3Vector, -700, 250)
            key.set_editor_property('constant', unreal.LinearColor(1.0, 0.0, 1.0, 1.0))
            dist = mel.create_material_expression(mat, unreal.MaterialExpressionDistance, -450, 150)
            mel.connect_material_expressions(tex, 'RGB', dist, 'A')
            mel.connect_material_expressions(key, '', dist, 'B')
            sub = mel.create_material_expression(mat, unreal.MaterialExpressionSubtract, -300, 100)
            mel.connect_material_expressions(dist, '', sub, 'A')
            thr = mel.create_material_expression(mat, unreal.MaterialExpressionConstant, -450, 260)
            thr.set_editor_property('r', 0.2)
            mel.connect_material_expressions(thr, '', sub, 'B')
            mul = mel.create_material_expression(mat, unreal.MaterialExpressionMultiply, -150, 100)
            mel.connect_material_expressions(sub, '', mul, 'A')
            gain = mel.create_material_expression(mat, unreal.MaterialExpressionConstant, -300, 220)
            gain.set_editor_property('r', 30.0)
            mel.connect_material_expressions(gain, '', mul, 'B')
            sat = mel.create_material_expression(mat, unreal.MaterialExpressionSaturate, 0, 100)
            mel.connect_material_expressions(mul, '', sat, '')
            mel.connect_material_property(sat, '', unreal.MaterialProperty.MP_OPACITY_MASK)
        mel.recompile_material(mat)
        result[masked] = mat
    return result


def import_textures(mats, tex_index, dest, staging):
    """Copia cada textura usada para um nome ASCII e importa em lote. Devolve {nome_original: (Texture2D, mascarada)}."""
    os.makedirs(staging, exist_ok=True)
    tasks, meta = [], {}
    for i, tex_name in enumerate(sorted({t for t in mats.values() if t})):
        src = tex_index.get(tex_name.lower())
        if not src:
            warn('textura nao encontrada: %s' % tex_name)
            continue
        asset = 'T_%s' % ascii_stem(tex_name, i)
        local = os.path.join(staging, asset + os.path.splitext(src)[1].lower())
        shutil.copyfile(src, local)  # copia: os originais nunca sao tocados
        t = unreal.AssetImportTask()
        t.set_editor_property('filename', local)
        t.set_editor_property('destination_path', dest + '/Textures')
        t.set_editor_property('destination_name', asset)
        t.set_editor_property('replace_existing', True)
        t.set_editor_property('automated', True)
        t.set_editor_property('save', False)
        tasks.append(t)
        meta[tex_name] = (asset, bmp_has_magenta(src))
    log('importando %d texturas...' % len(tasks))
    tools.import_asset_tasks(tasks)
    out = {}
    for tex_name, (asset, masked) in meta.items():
        tex = eal.load_asset('%s/Textures/%s' % (dest, asset))
        if tex:
            try_set(tex, 'filter', unreal.TextureFilter.TF_NEAREST)  # pixel art sem filtro (HD-2D)
            out[tex_name] = (tex, masked)
        else:
            warn('falha ao importar %s' % tex_name)
    log('%d texturas importadas, %d com transparencia magenta' % (len(out), sum(1 for _, m in out.values() if m)))
    return out


# ----------------------------------------------------------------------------- malha
def build_mesh(dest, name, verts, uvs, tris, mat_to_instance, scale, center):
    cx, cy = center
    mat_names = []
    seen = set()
    for m, _, _ in tris:
        if m not in seen:
            seen.add(m)
            mat_names.append(m)
    section_of = {m: i for i, m in enumerate(mat_names)}
    slot_names = ['M_%03d' % i for i in range(len(mat_names))]

    # Mapa centrado em (0,0): X = (x-cx)*S ; Y = -(y-cy)*S (espelha Y para nao espelhar a imagem) ; Z = z*S
    log('convertendo %d vertices...' % len(verts))
    px, py, pz = [], [], []
    positions = []
    for (x, y, z) in verts:
        px.append((x - cx) * scale)
        py.append(-(y - cy) * scale)
        pz.append(z * scale)
        positions.extend((px[-1], py[-1], pz[-1]))

    # Sentido dos triangulos: pelo chao, a normal (b-a)x(c-a) tem de apontar para +Z (frente horaria da Unreal).
    up = down = 0
    for _, v, _ in tris[:20000]:
        nz = (px[v[1]] - px[v[0]]) * (py[v[2]] - py[v[0]]) - (py[v[1]] - py[v[0]]) * (px[v[2]] - px[v[0]])
        if nz > 0:
            up += 1
        elif nz < 0:
            down += 1
    order = (0, 2, 1) if down > up else (0, 1, 2)
    log('orientacao dos triangulos: %d para cima / %d para baixo -> %s' % (up, down, 'invertendo' if order[1] == 2 else 'mantendo'))

    log('preparando %d triangulos...' % len(tris))
    corner_vertex, corner_uv, tri_section = [], [], []
    for m, v, t in tris:
        tri_section.append(section_of[m])
        for k in order:
            corner_vertex.append(v[k])
            uv = uvs[t[k]] if 0 <= t[k] < len(uvs) else (0.0, 0.0)
            # O compilador grava v = 1 - v (convencao OBJ); a Unreal usa origem no topo.
            corner_uv.extend((uv[0], 1.0 - uv[1]))

    # O Python nao expoe a fabrica de StaticMesh: duplica o cubo basico da engine e troca a geometria.
    mesh_path = '%s/SM_%s' % (dest, name)
    if eal.does_asset_exist(mesh_path):
        eal.delete_asset(mesh_path)
    mesh = eal.duplicate_asset('/Engine/BasicShapes/Cube', mesh_path)

    log('construindo o StaticMesh (C++)...')
    ok = unreal.MythMeshBuilderLibrary.build_static_mesh_from_triangles(mesh, positions, corner_vertex, corner_uv, tri_section, slot_names)
    if not ok:
        raise RuntimeError('MythMeshBuilderLibrary.BuildStaticMeshFromTriangles falhou (veja o log da Unreal)')

    # liga cada slot ao seu Material Instance
    slot_to_inst = {slot_names[section_of[m]]: mat_to_instance.get(m) for m in mat_names}
    for i, sm in enumerate(mesh.get_editor_property('static_materials')):
        inst = slot_to_inst.get(str(sm.get_editor_property('material_slot_name')))
        if inst:
            mesh.set_material(i, inst)

    # Use Complex Collision As Simple: anda-se no terreno exato
    body = mesh.get_editor_property('body_setup')
    body.set_editor_property('collision_trace_flag', unreal.CollisionTraceFlag.CTF_USE_COMPLEX_AS_SIMPLE)
    eal.save_loaded_asset(mesh)
    return mesh


# ----------------------------------------------------------------------------- audio
def import_bgm(path, dest, name):
    if not path:
        return None
    if not os.path.exists(path):
        warn('BGM nao encontrada: %s' % path)
        return None
    t = unreal.AssetImportTask()
    t.set_editor_property('filename', path)  # o SoundFactory do UE 5.8 importa mp3; o arquivo original nao e alterado
    t.set_editor_property('destination_path', dest + '/Audio')
    t.set_editor_property('destination_name', 'BGM_%s' % name)
    t.set_editor_property('replace_existing', True)
    t.set_editor_property('automated', True)
    t.set_editor_property('save', False)
    tools.import_asset_tasks([t])
    snd = eal.load_asset('%s/Audio/BGM_%s' % (dest, name))
    if snd:
        try_set(snd, 'looping', True)
        eal.save_loaded_asset(snd)
        log('BGM importada: %s' % snd.get_path_name())
    else:
        warn('falha ao importar a BGM')
    return snd


# ----------------------------------------------------------------------------- level
def spawn(cls, loc=(0, 0, 0), rot=(0, 0, 0), label=None):
    sub = unreal.get_editor_subsystem(unreal.EditorActorSubsystem)
    actor = sub.spawn_actor_from_class(cls, unreal.Vector(*loc), unreal.Rotator(*rot))
    if actor and label:
        actor.set_actor_label(label)
    return actor


def add_lighting():
    sun = spawn(unreal.DirectionalLight, (0, 0, 2000), (-50, -35, 0), 'Sun')
    c = sun.get_component_by_class(unreal.DirectionalLightComponent)
    try_set(c, 'mobility', unreal.ComponentMobility.MOVABLE)
    try_set(c, 'intensity', 10.0)
    try_set(c, 'atmosphere_sun_light', True)
    try_set(c, 'dynamic_shadow_distance_movable_light', 20000.0)

    sky = spawn(unreal.SkyLight, (0, 0, 2500), label='SkyLight')
    sc = sky.get_component_by_class(unreal.SkyLightComponent)
    try_set(sc, 'mobility', unreal.ComponentMobility.MOVABLE)
    try_set(sc, 'real_time_capture', True)

    spawn(unreal.SkyAtmosphere, (0, 0, 0), label='SkyAtmosphere')

    fog = spawn(unreal.ExponentialHeightFog, (0, 0, 300), label='HeightFog')
    fc = fog.get_component_by_class(unreal.ExponentialHeightFogComponent)
    try_set(fc, 'fog_density', 0.004)
    try_set(fc, 'fog_height_falloff', 0.3)

    ppv = spawn(unreal.PostProcessVolume, (0, 0, 0), label='PostProcess_HD2D')
    try_set(ppv, 'unbound', True)
    s = ppv.get_editor_property('settings')
    # HD-2D: Lumen GI/reflexos, bloom suave, exposicao estavel, leve vinheta.
    for flag, prop, val in (
        ('override_dynamic_global_illumination_method', 'dynamic_global_illumination_method', unreal.DynamicGlobalIlluminationMethod.LUMEN),
        ('override_reflection_method', 'reflection_method', unreal.ReflectionMethod.LUMEN),
        ('override_bloom_intensity', 'bloom_intensity', 0.6),
        ('override_vignette_intensity', 'vignette_intensity', 0.35),
        ('override_auto_exposure_min_brightness', 'auto_exposure_min_brightness', 0.5),
        ('override_auto_exposure_max_brightness', 'auto_exposure_max_brightness', 2.0),
        ('override_motion_blur_amount', 'motion_blur_amount', 0.0),
        ('override_depth_of_field_focal_distance', 'depth_of_field_focal_distance', 800.0),
        ('override_depth_of_field_depth_blur_radius', 'depth_of_field_depth_blur_radius', 3.5),
    ):
        try_set(s, flag, True)
        try_set(s, prop, val)
    ppv.set_editor_property('settings', s)


def add_navigation(center, extent):
    vol = spawn(unreal.NavMeshBoundsVolume, (center.x, center.y, center.z), label='NavBounds')
    # O brush padrao tem 200 uu de lado; escala para cobrir a malha inteira (com folga em Z).
    vol.set_actor_scale3d(unreal.Vector(extent.x * 2 / 200.0, extent.y * 2 / 200.0, (extent.z * 2 + 600) / 200.0))
    # Sem um RecastNavMesh no nivel o jogo nao gera navegacao ("Unable to find RecastNavMesh instance").
    # A geracao dinamica vem de Config/DefaultEngine.ini ([RecastNavMesh] RuntimeGeneration=Dynamic).
    nav = spawn(unreal.RecastNavMesh, (center.x, center.y, center.z), label='RecastNavMesh')
    if not nav:
        warn('RecastNavMesh nao foi criado: o Point & Click cai no movimento em linha reta')


def run():
    args = parse_args(sys.argv[1:])
    obj_path = args['obj'].replace(chr(92), '/')
    name, dest, scale = args['name'], args['dest'], args['scale']
    base_dir = os.path.dirname(obj_path)
    log('mapa "%s" -> %s (escala %.1f)' % (name, dest, scale))

    verts, uvs, tris, ground, mtllib = parse_obj(obj_path)
    mtl_path = os.path.join(base_dir, mtllib) if mtllib else os.path.splitext(obj_path)[0] + '.mtl'
    mats = parse_mtl(mtl_path)
    log('%d vertices, %d triangulos, %d materiais' % (len(verts), len(tris), len(mats)))

    xs = [v[0] for v in ground]
    ys = [v[1] for v in ground]
    cx, cy = (min(xs) + max(xs)) / 2.0, (min(ys) + max(ys)) / 2.0

    tex_index = index_textures(args['texture_dir'] or os.path.join(base_dir, 'texture'))
    staging = os.path.join(unreal.Paths.project_saved_dir(), 'MythImport', name)

    for sub in ('', '/Textures', '/Materials', '/Audio'):
        eal.make_directory(dest + sub)

    textures = import_textures(mats, tex_index, dest, staging)
    parents = build_parent_materials(dest + '/Materials')

    mat_to_instance = {}
    for i, (mat_name, tex_name) in enumerate(mats.items()):
        tex, masked = textures.get(tex_name, (None, False))
        inst = make_asset('MI_%s_%03d' % (name, i), dest + '/Materials', unreal.MaterialInstanceConstant,
                          unreal.MaterialInstanceConstantFactoryNew())
        inst.set_editor_property('parent', parents[masked])
        if tex:
            mel.set_material_instance_texture_parameter_value(inst, 'Tex', tex)
        mat_to_instance[mat_name] = inst
    log('%d material instances criados' % len(mat_to_instance))

    mesh = build_mesh(dest, name, verts, uvs, tris, mat_to_instance, scale, (cx, cy))
    bgm = import_bgm(args['bgm'], dest, name)

    # --- Level
    level_path = '%s/Lvl_%s' % (dest, args['title'])
    lvl = unreal.get_editor_subsystem(unreal.LevelEditorSubsystem)
    if eal.does_asset_exist(level_path):
        eal.delete_asset(level_path)
    lvl.new_level(level_path)

    mesh_actor = spawn(unreal.StaticMeshActor, (0, 0, 0), label='SM_%s' % name)
    mesh_actor.static_mesh_component.set_static_mesh(mesh)
    origin, extent = mesh_actor.get_actor_bounds(False)
    log('limites da malha: centro %s, extensao %s' % (origin, extent))

    if args['lighting']:
        add_lighting()
    add_navigation(origin, extent)

    # Grid do servidor: celula (0,0) = obj (0,0) -> mundo (-cx*S, +cy*S); 1 celula = 5 unidades do obj
    origin_x, origin_y = -cx * scale, cy * scale
    cell_size = scale * 5.0
    sx, sy = args['start_cell'] if args['start_cell'] else (int(cx / 5.0), int(cy / 5.0))
    wx = origin_x + (sx + 0.5) * cell_size
    wy = origin_y - (sy + 0.5) * cell_size
    # altura do chao: maior Z do terreno num raio de 1 celula
    near = [v[2] for v in ground if abs((v[0] - cx) * scale - wx) < cell_size and abs(-(v[1] - cy) * scale - wy) < cell_size]
    wz = (max(near) if near else 0.0) * scale + 120.0
    spawn(unreal.PlayerStart, (wx, wy, wz), (0, 0, 0), 'PlayerStart')
    log('PlayerStart na celula (%d,%d) -> mundo (%.0f, %.0f, %.0f)' % (sx, sy, wx, wy, wz))

    info = spawn(unreal.MythMapInfo, (0, 0, 0), label='MapInfo')
    try_set(info, 'map_name', args['map_name'])
    try_set(info, 'cell_size_uu', cell_size)
    try_set(info, 'cell_origin_uu', unreal.Vector2D(origin_x, origin_y))
    if bgm:
        try_set(info, 'bgm', bgm)
    if args.get('water_level') is not None:
        try_set(info, 'b_has_water', True)
        try_set(info, 'water_level_uu', float(args['water_level']) * scale)
        log('Plano de agua configurado: %.1f UU' % (float(args['water_level']) * scale))

    # Popula dados de ambiente e fuso horario a partir de Config/map_environment_table.json
    env_cfg_path = os.path.join(unreal.Paths.project_config_dir(), 'map_environment_table.json')
    if os.path.exists(env_cfg_path):
        try:
            with open(env_cfg_path, 'r', encoding='utf-8') as f:
                env_table = json.load(f)
            ed = env_table.get(args['map_name'].lower())
            if ed:
                env_struct = unreal.MapEnvironmentData()
                env_struct.set_editor_property('map_id', args['map_name'])
                env_struct.set_editor_property('display_name', ed.get('name', args['map_name']))
                env_struct.set_editor_property('grid_x', ed.get('grid_x', 0))
                env_struct.set_editor_property('grid_y', ed.get('grid_y', 0))
                env_struct.set_editor_property('b_is_dungeon', ed.get('dungeon', False))
                env_struct.set_editor_property('b_is_perpetual_night', ed.get('perpetual_night', False))
                env_struct.set_editor_property('biome_tag', ed.get('biome', 'Temperate'))
                weathers = ed.get('allowed_weathers', [])
                if weathers and hasattr(unreal, 'MythWeatherType'):
                    w_list = []
                    for w_name in weathers:
                        enum_val = getattr(unreal.MythWeatherType, w_name.upper(), getattr(unreal.MythWeatherType, w_name, None))
                        if enum_val is not None:
                            w_list.append(enum_val)
                    if w_list:
                        env_struct.set_editor_property('allowed_weathers', w_list)
                def_w = ed.get('default_weather', '')
                if def_w and hasattr(unreal, 'MythWeatherType'):
                    def_val = getattr(unreal.MythWeatherType, def_w.upper(), getattr(unreal.MythWeatherType, def_w, None))
                    if def_val is not None:
                        env_struct.set_editor_property('default_weather', def_val)
                try_set(info, 'environment_data', env_struct)
                log('Ambiente configurado: %s (Grid %d, %d | %s | Clima: %s)' % (ed.get('name'), ed.get('grid_x', 0), ed.get('grid_y', 0), ed.get('biome'), ed.get('default_weather', 'ClearSunny')))
        except Exception as e:
            warn('Nao foi possivel carregar dados de ambiente: ' + str(e))

    eal.save_directory(dest, only_if_is_dirty=False, recursive=True)
    lvl.save_current_level()
    log('PRONTO: %s' % level_path)


try:
    run()
except SystemExit as e:
    warn(str(e))
except Exception:  # noqa: BLE001
    import traceback
    warn('ERRO NO IMPORTADOR:' + chr(10) + traceback.format_exc())
    raise

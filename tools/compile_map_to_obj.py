#!/usr/bin/env python3
"""
Myth of Thor - Full Ragnarok Map Assembler (GND + RSW + RSM -> Wavefront OBJ)
-----------------------------------------------------------------------------
Compila um mapa completo do Ragnarok Online (terreno .gnd + posicionamento .rsw +
modelos 3D .rsm) diretamente para um arquivo .OBJ e .MTL consolidado, pronto para
importação na Unreal Engine 5.
"""

import os
import sys
import glob
import json
import math
import struct
from typing import Dict, List, Tuple

# Pastas raiz dos assets extraidos do RO (sobrescreva com a variavel de ambiente MYTH_ASSETS)
CANDIDATE_ROOTS = [
    os.environ.get('MYTH_ASSETS', ''),
    r'E:\Dev\Ragnarok_Assets\grf\data',
    r'E:\Dev\Ragnarok_Assets',
    r'D:\Ragnarok_Assets',
]
ASSETS_ROOT = next((p for p in CANDIDATE_ROOTS if p and os.path.exists(p)), 'D:/Ragnarok_Assets')


def resolve_asset_path(base_dir: str, rel_path: str) -> str:
    """
    Resolve caminhos relativos suportando tanto codificação nativa CP949
    quanto mojibake CP1252 do Windows.
    """
    rel_clean = rel_path.replace('/', os.sep).replace('\\', os.sep)
    direct_path = os.path.join(base_dir, rel_clean)
    if os.path.exists(direct_path):
        return direct_path

    # Tenta conversão CP949 -> Latin1 (mojibake)
    try:
        mojibake = rel_clean.encode('cp949').decode('latin1')
        mojibake_path = os.path.join(base_dir, mojibake)
        if os.path.exists(mojibake_path):
            return mojibake_path
    except Exception:
        pass

    # Tenta conversão Latin1 -> CP949
    try:
        korean = rel_clean.encode('latin1').decode('cp949')
        korean_path = os.path.join(base_dir, korean)
        if os.path.exists(korean_path):
            return korean_path
    except Exception:
        pass

    # Tenta busca case-insensitive por nome base
    basename = os.path.basename(rel_clean)
    for root, _, files in os.walk(base_dir):
        for f in files:
            if f.lower() == basename.lower():
                return os.path.join(root, f)

    return ""


def read_gnd(gnd_path: str):
    """Lê a geometria do terreno do arquivo .gnd"""
    with open(gnd_path, 'rb') as f:
        magic = f.read(4)
        if magic != b'GRGN':
            raise ValueError(f"Arquivo GND inválido: {magic}")
        ver_major, ver_minor = struct.unpack('2B', f.read(2))
        width, height = struct.unpack('2I', f.read(8))
        scale = struct.unpack('f', f.read(4))[0]
        texture_count, texture_name_length = struct.unpack('2I', f.read(8))
        
        textures = []
        for _ in range(texture_count):
            raw_name = f.read(texture_name_length)
            name = raw_name.split(b'\x00')[0].decode('cp949', errors='ignore')
            textures.append(name)
            
        lightmap_count = struct.unpack('I', f.read(4))[0]
        lightmap_size = struct.unpack('3I', f.read(12))
        f.seek(lightmap_count * (64 + 192), os.SEEK_CUR)
        
        face_count = struct.unpack('I', f.read(4))[0]
        faces = []
        for _ in range(face_count):
            texcoords = struct.unpack('8f', f.read(32))
            tex_idx, _ = struct.unpack('2H', f.read(4))
            f.read(4) # lighting
            faces.append({
                'texcoords': texcoords,
                'texture_index': tex_idx
            })
            
        tiles = []
        for _ in range(width * height):
            heights = struct.unpack('4f', f.read(16))
            face_indices = struct.unpack('3i', f.read(12))
            tiles.append({
                'heights': heights,
                'faces': face_indices
            })
            
        return {
            'width': width,
            'height': height,
            'scale': scale,
            'textures': textures,
            'faces': faces,
            'tiles': tiles
        }


def read_rsw(rsw_path: str):
    """Lê a lista de modelos e instâncias do arquivo .rsw"""
    with open(rsw_path, 'rb') as f:
        magic = f.read(4)
        if magic != b'GRSW':
            raise ValueError(f"Arquivo RSW inválido: {magic}")
        ver_major, ver_minor = struct.unpack('2B', f.read(2))
        version = ver_major + ver_minor / 10.0
        
        ini_file = f.read(40).split(b'\x00')[0].decode('cp949', errors='ignore')
        gnd_file = f.read(40).split(b'\x00')[0].decode('cp949', errors='ignore')
        gat_file = f.read(40).split(b'\x00')[0].decode('cp949', errors='ignore') if version >= 1.4 else ""
        src_file = f.read(40).split(b'\x00')[0].decode('cp949', errors='ignore')
        
        # Pula água
        if version >= 1.3:
            f.seek(4, os.SEEK_CUR)
        if version >= 1.8:
            f.seek(16, os.SEEK_CUR)
        if version >= 1.9:
            f.seek(4, os.SEEK_CUR)
            
        # Pula luz
        if version >= 1.5:
            f.seek(8, os.SEEK_CUR)
        f.seek(28, os.SEEK_CUR) # diffuse, ambient, alpha
        
        # Pula bounds do chão
        if version >= 1.6:
            f.seek(16, os.SEEK_CUR)
            
        object_count = struct.unpack('I', f.read(4))[0]
        models = []
        
        for _ in range(object_count):
            obj_type = struct.unpack('I', f.read(4))[0]
            if obj_type == 1: # MODEL
                name = ""
                if version >= 1.3:
                    name = f.read(40).split(b'\x00')[0].decode('cp949', errors='ignore')
                    anim_type, anim_speed, block_type = struct.unpack('IfI', f.read(12))
                filename = f.read(80).split(b'\x00')[0].decode('cp949', errors='ignore')
                node_name = f.read(80).split(b'\x00')[0].decode('cp949', errors='ignore')
                pos = struct.unpack('3f', f.read(12))
                rot = struct.unpack('3f', f.read(12))
                scale = struct.unpack('3f', f.read(12))
                models.append({
                    'name': name,
                    'filename': filename,
                    'node_name': node_name,
                    'position': pos,
                    'rotation': rot,
                    'scale': scale
                })
            elif obj_type == 2: # LIGHT
                f.seek(80 + 12 + 12 + 4, os.SEEK_CUR)
            elif obj_type == 3: # SOUND
                f.seek(80 + 80 + 12 + 4 + 4 + 4 + 4, os.SEEK_CUR)
                if version >= 2.0:
                    f.seek(4, os.SEEK_CUR)
            elif obj_type == 4: # EFFECT
                f.seek(80 + 12 + 4 + 4 + 16, os.SEEK_CUR)
                
        return {
            'gnd_file': gnd_file,
            'gat_file': gat_file,
            'models': models
        }


def read_rsm(rsm_path: str):
    """Lê a malha e nós de um modelo .rsm"""
    with open(rsm_path, 'rb') as f:
        magic = f.read(4)
        if magic != b'GRSM':
            raise ValueError(f"Arquivo RSM inválido: {magic}")
        ver_major, ver_minor = struct.unpack('2B', f.read(2))
        version = ver_major + ver_minor / 10.0
        
        anim_len, shade_type = struct.unpack('2I', f.read(8))
        if ver_major > 1 or (ver_major == 1 and ver_minor >= 4):
            alpha = struct.unpack('B', f.read(1))[0]
        f.seek(16, os.SEEK_CUR) # padding
        
        tex_count = struct.unpack('I', f.read(4))[0]
        textures = []
        for _ in range(tex_count):
            tname = f.read(40).split(b'\x00')[0].decode('cp949', errors='ignore')
            textures.append(tname)
            
        main_node = f.read(40).split(b'\x00')[0].decode('cp949', errors='ignore')
        node_count = struct.unpack('I', f.read(4))[0]
        nodes = []
        
        min_v = [float('inf'), float('inf'), float('inf')]
        max_v = [float('-inf'), float('-inf'), float('-inf')]
        
        for _ in range(node_count):
            n_name = f.read(40).split(b'\x00')[0].decode('cp949', errors='ignore')
            p_name = f.read(40).split(b'\x00')[0].decode('cp949', errors='ignore')
            node_tex_count = struct.unpack('I', f.read(4))[0]
            tex_indices = list(struct.unpack(f'{node_tex_count}I', f.read(4 * node_tex_count)))
            # Ordem do formato (igual ao roBrowser/BrowEdit): mat3, offset (pivo local), pos (relativa ao pai),
            # rotacao eixo-angulo e escala.
            mat3 = struct.unpack('9f', f.read(36))
            offset = struct.unpack('3f', f.read(12))
            pos = struct.unpack('3f', f.read(12))
            rot_angle = struct.unpack('f', f.read(4))[0]
            rot_axis = struct.unpack('3f', f.read(12))
            scale = struct.unpack('3f', f.read(12))

            vert_count = struct.unpack('I', f.read(4))[0]
            verts = [struct.unpack('3f', f.read(12)) for _ in range(vert_count)]

            has_color = ver_major > 1 or (ver_major == 1 and ver_minor >= 2)
            texcoord_count = struct.unpack('I', f.read(4))[0]
            texcoords = []
            for _ in range(texcoord_count):
                if has_color:
                    f.read(4)  # cor (uint32), so a partir da 1.2
                texcoords.append(struct.unpack('2f', f.read(8)))

            face_count = struct.unpack('I', f.read(4))[0]
            faces = []
            for _ in range(face_count):
                v_idx = struct.unpack('3H', f.read(6))
                t_idx = struct.unpack('3H', f.read(6))
                tex_idx = struct.unpack('H', f.read(2))[0]
                f.read(2)  # padding
                f.read(4)  # two_sided
                if has_color:
                    f.read(4)  # smooth_group, so a partir da 1.2
                faces.append({
                    'vert_indices': v_idx,
                    'tex_indices': t_idx,
                    'texture_index': tex_idx
                })

            # Keyframes: a malha e estatica, mas o primeiro quaternion de rotacao substitui o eixo-angulo
            if ver_major > 1 or (ver_major == 1 and ver_minor >= 5):
                loc_kf = struct.unpack('I', f.read(4))[0]
                f.seek(loc_kf * (4 + 12), os.SEEK_CUR)
            rot_kf = struct.unpack('I', f.read(4))[0]
            rot_quat = None
            rot_keys = []
            for k in range(rot_kf):
                frame, qx, qy, qz, qw = struct.unpack('I4f', f.read(20))
                rot_keys.append((frame, (qx, qy, qz, qw)))
                if k == 0:
                    rot_quat = (qx, qy, qz, qw)

            nodes.append({
                'name': n_name,
                'parent_name': p_name,
                'texture_indices': tex_indices,
                'mat3': mat3,
                'offset': offset,
                'pos': pos,
                'rot_angle': rot_angle,
                'rot_axis': rot_axis,
                'rot_quat': rot_quat,
                'rot_keys': rot_keys,
                'scale': scale,
                'vertices': verts,
                'texcoords': texcoords,
                'faces': faces
            })
            
        # Matriz final de cada no (espaco do modelo, eixos do RO: Y para baixo), percorrendo a hierarquia
        is_only = len(nodes) == 1
        by_name = {n['name']: n for n in nodes}
        root = by_name.get(main_node) or next((n for n in nodes if not n['parent_name'] or n['parent_name'] not in by_name), nodes[0] if nodes else None)

        def place(node, parent_matrix, depth=0):
            m = mat4_mul(parent_matrix, mat4_translation(node['pos']))
            if node['rot_quat'] is not None:
                m = mat4_mul(m, mat4_from_quat(node['rot_quat']))
            else:
                m = mat4_mul(m, mat4_axis_angle(node['rot_axis'], node['rot_angle']))
            m = mat4_mul(m, mat4_scaling(node['scale']))
            node['world'] = m  # repassado aos filhos (sem offset/mat3, que sao so desta malha)
            mesh = m if is_only else mat4_mul(m, mat4_translation(node['offset']))
            node['mesh_matrix'] = mat4_mul(mesh, mat3_to_mat4(node['mat3']))
            for v in node['vertices']:
                p = mat4_apply(node['mesh_matrix'], v)
                for c in range(3):
                    min_v[c] = min(min_v[c], p[c])
                    max_v[c] = max(max_v[c], p[c])
            if depth < 64:
                for child in nodes:
                    if child is not node and child['parent_name'] == node['name'] and child['name'] != node['name']:
                        place(child, m, depth + 1)

        if root is not None:
            place(root, mat4_identity())
        for n in nodes:  # nos orfaos (pai inexistente) ficam na origem do modelo
            if 'mesh_matrix' not in n:
                place(n, mat4_identity())

        box_center = [(min_v[i] + max_v[i]) / 2.0 if min_v[i] != float('inf') else 0.0 for i in range(3)]
        box_max = [max_v[i] if max_v[i] != float('-inf') else 0.0 for i in range(3)]
        box_min = [min_v[i] if min_v[i] != float('inf') else 0.0 for i in range(3)]

        return {
            'textures': textures,
            'nodes': nodes,
            'box_center': box_center,
            'box_max': box_max,
            'box_min': box_min
        }


# ----------------------------------------------------------------------------- matrizes 4x4 (linha-maior, vetor-coluna: p' = M * p)
def mat4_identity():
    return [[1.0 if r == c else 0.0 for c in range(4)] for r in range(4)]


def mat4_mul(a, b):
    return [[sum(a[r][k] * b[k][c] for k in range(4)) for c in range(4)] for r in range(4)]


def mat4_translation(t):
    m = mat4_identity()
    m[0][3], m[1][3], m[2][3] = t[0], t[1], t[2]
    return m


def mat4_scaling(s):
    m = mat4_identity()
    m[0][0], m[1][1], m[2][2] = s[0], s[1], s[2]
    return m


def mat4_axis_angle(axis, angle):
    x, y, z = axis
    length = math.sqrt(x * x + y * y + z * z)
    if length < 1e-6 or abs(angle) < 1e-9:
        return mat4_identity()
    x, y, z = x / length, y / length, z / length
    c, s = math.cos(angle), math.sin(angle)
    t = 1.0 - c
    return [
        [x * x * t + c,     x * y * t - z * s, x * z * t + y * s, 0.0],
        [y * x * t + z * s, y * y * t + c,     y * z * t - x * s, 0.0],
        [z * x * t - y * s, z * y * t + x * s, z * z * t + c,     0.0],
        [0.0, 0.0, 0.0, 1.0],
    ]


def mat4_rot_x(rad):
    c, s = math.cos(rad), math.sin(rad)
    return [[1, 0, 0, 0], [0, c, -s, 0], [0, s, c, 0], [0, 0, 0, 1]]


def mat4_rot_y(rad):
    c, s = math.cos(rad), math.sin(rad)
    return [[c, 0, s, 0], [0, 1, 0, 0], [-s, 0, c, 0], [0, 0, 0, 1]]


def mat4_rot_z(rad):
    c, s = math.cos(rad), math.sin(rad)
    return [[c, -s, 0, 0], [s, c, 0, 0], [0, 0, 1, 0], [0, 0, 0, 1]]


def mat4_from_quat(q):
    x, y, z, w = q
    n = math.sqrt(x * x + y * y + z * z + w * w) or 1.0
    x, y, z, w = x / n, y / n, z / n, w / n
    return [
        [1 - 2 * (y * y + z * z), 2 * (x * y - z * w),     2 * (x * z + y * w),     0.0],
        [2 * (x * y + z * w),     1 - 2 * (x * x + z * z), 2 * (y * z - x * w),     0.0],
        [2 * (x * z - y * w),     2 * (y * z + x * w),     1 - 2 * (x * x + y * y), 0.0],
        [0.0, 0.0, 0.0, 1.0],
    ]


def mat3_to_mat4(m3):
    # Os 9 floats estao em ordem de coluna (como o gl-matrix do roBrowser): coluna c = m3[3c:3c+3]
    m = mat4_identity()
    for r in range(3):
        for c in range(3):
            m[r][c] = m3[c * 3 + r]
    return m


def mat4_apply(m, v):
    x, y, z = v
    return (m[0][0] * x + m[0][1] * y + m[0][2] * z + m[0][3],
            m[1][0] * x + m[1][1] * y + m[1][2] * z + m[1][3],
            m[2][0] * x + m[2][1] * y + m[2][2] * z + m[2][3])


def compile_map(map_dir: str, output_obj: str = None, map_name: str = None):
    """Compila o mapa completo para OBJ"""
    map_dir = os.path.abspath(map_dir)
    
    if map_name:
        rsw_path = ""
        candidate_dirs = [map_dir, ASSETS_ROOT] + CANDIDATE_ROOTS
        # 1. Busca direta O(1)
        for cd in candidate_dirs:
            if not cd or not os.path.exists(cd):
                continue
            direct = os.path.join(cd, f"{map_name}.rsw")
            if os.path.exists(direct):
                rsw_path = direct
                break
        # 2. Busca com wildcard raso ou recursivo se não achou direto
        if not rsw_path:
            for cd in candidate_dirs:
                if not cd or not os.path.exists(cd):
                    continue
                found = glob.glob(os.path.join(cd, f"{map_name}.rsw")) or glob.glob(os.path.join(cd, f"**/{map_name}.rsw"), recursive=True)
                if found:
                    rsw_path = found[0]
                    break
        if not rsw_path:
            raise FileNotFoundError(f"Arquivo {map_name}.rsw não encontrado em nenhum dos diretórios: {candidate_dirs}")
    else:
        rsw_files = glob.glob(os.path.join(map_dir, "*.rsw")) or glob.glob(os.path.join(map_dir, "data", "*.rsw")) or glob.glob(os.path.join(map_dir, "**", "*.rsw"), recursive=True)
        if not rsw_files:
            raise FileNotFoundError(f"Nenhum arquivo .rsw encontrado em {map_dir}")
        rsw_path = rsw_files[0]


    map_dir = os.path.dirname(rsw_path)
    map_name = os.path.splitext(os.path.basename(rsw_path))[0]
    
    if output_obj is None:
        output_obj = os.path.join(map_dir, f"{map_name}_full.obj")
    output_mtl = output_obj.replace('.obj', '.mtl')
    
    print(f"=== Compilando Mapa: {map_name} ===")
    print(f"Diretório: {map_dir}")
    
    # 1. Carrega RSW e GND
    print("Lendo RSW...")
    rsw = read_rsw(rsw_path)
    gnd_path = os.path.join(map_dir, f"{map_name}.gnd")
    if not os.path.exists(gnd_path):
        gnd_path = resolve_asset_path(map_dir, rsw['gnd_file'])
        
    print(f"Lendo GND ({gnd_path})...")
    gnd = read_gnd(gnd_path)
    
    width = gnd['width']
    height = gnd['height']
    scale = gnd['scale']
    center_x = (width * scale) / 2.0
    center_y = (height * scale) / 2.0
    
    # Gerenciamento de materiais
    materials = {} # texture_basename -> mat_name
    
    def get_material_name(tex_path: str) -> str:
        tex_base = os.path.basename(tex_path).replace(' ', '_')
        if tex_base not in materials:
            mat_id = len(materials)
            materials[tex_base] = f"M_{mat_id}_{tex_base}"
        return materials[tex_base]

    # Registra texturas do GND
    for t in gnd['textures']:
        get_material_name(t)

    # 2. Carrega RSMs em cache
    models_search_dirs = [os.path.join(map_dir, 'model')]
    for cr in [map_dir, ASSETS_ROOT] + CANDIDATE_ROOTS:
        if cr and os.path.exists(cr):
            for sub in ['model', os.path.join('shared', 'data', 'model'), os.path.join('Prontera', 'Prontera_Cidade', 'model')]:
                p = os.path.join(cr, sub)
                if p not in models_search_dirs and os.path.exists(p):
                    models_search_dirs.append(p)

    rsm_cache: Dict[str, dict] = {}
    model_texture_names = set()  # texturas usadas por modelos (candidatas a janela acesa)
    
    unique_models = set(m['filename'] for m in rsw['models'])
    print(f"Modelos referenciados no mapa: {len(rsw['models'])} ({len(unique_models)} únicos)")
    
    for m_filename in unique_models:
        rsm_path = ""
        for m_dir in models_search_dirs:
            if os.path.exists(m_dir):
                rsm_path = resolve_asset_path(m_dir, m_filename)
                if rsm_path and os.path.exists(rsm_path):
                    break
        if rsm_path and os.path.exists(rsm_path):
            try:
                rsm_data = read_rsm(rsm_path)
                rsm_cache[m_filename] = rsm_data
                for t in rsm_data['textures']:
                    get_material_name(t)
                    model_texture_names.add(os.path.basename(t.replace(chr(92), '/')).replace(' ', '_'))
            except Exception as e:
                pass
                
    print(f"Modelos carregados com sucesso: {len(rsm_cache)}/{len(unique_models)}")
    
    # 3. Escreve arquivo MTL resolvendo caminhos de texturas existentes
    tex_search_dirs = [os.path.join(map_dir, 'texture')]
    for cr in [map_dir, ASSETS_ROOT] + CANDIDATE_ROOTS:
        if cr and os.path.exists(cr):
            for sub in ['texture', os.path.join('shared', 'data', 'texture'), os.path.join('Prontera', 'Prontera_Cidade', 'texture')]:
                p = os.path.join(cr, sub)
                if p not in tex_search_dirs and os.path.exists(p):
                    tex_search_dirs.append(p)

    def safe_relpath(path: str, start: str) -> str:
        try:
            return os.path.relpath(path, start).replace('\\', '/')
        except ValueError:
            p_clean = path.replace('\\', '/')
            lower = p_clean.lower()
            if '/texture/' in lower:
                idx = lower.find('/texture/')
                return p_clean[idx + 1:]
            return p_clean

    tex_index = {}
    for td in tex_search_dirs:
        if os.path.exists(td):
            for root, _, files in os.walk(td):
                for f in files:
                    tex_index.setdefault(f.lower(), safe_relpath(os.path.join(root, f), map_dir))
                    try:
                        korean = f.encode('latin1').decode('cp949', errors='ignore').lower()
                        tex_index.setdefault(korean, safe_relpath(os.path.join(root, f), map_dir))
                    except Exception:
                        pass


    print(f"Escrevendo materiais em {output_mtl}...")
    with open(output_mtl, 'w', encoding='utf-8') as mtl:
        for tex_base, mat_name in materials.items():
            mtl.write(f"newmtl {mat_name}\n")
            mtl.write("Ka 1.0 1.0 1.0\nKd 1.0 1.0 1.0\nKs 0.0 0.0 0.0\n")
            rel_tex = tex_index.get(tex_base.lower(), f"texture/{tex_base}")
            mtl.write(f"map_Kd {rel_tex}\n\n")

    # 4. Escreve arquivo OBJ
    print(f"Escrevendo geometria em {output_obj}...")
    mtl_filename = os.path.basename(output_mtl)
    
    v_offset = 1
    vt_offset = 1
    
    with open(output_obj, 'w', encoding='utf-8') as obj:
        obj.write(f"# Myth of Thor - Mapa Completo montado: {map_name}\n")
        obj.write(f"mtllib {mtl_filename}\n\n")
        
        # --- PARTE A: TERRENO GND ---
        obj.write("o Ground\n")
        tiles = gnd['tiles']
        faces = gnd['faces']
        gnd_textures = gnd['textures']
        
        gnd_faces_by_mat: Dict[str, List[Tuple[List[int], List[int]]]] = {}
        
        for y in range(height):
            for x in range(width):
                tile = tiles[y * width + x]
                h = tile['heights']
                
                # Face superior (Top Face)
                if tile['faces'][0] != -1:
                    f_info = faces[tile['faces'][0]]
                    tex_name = gnd_textures[f_info['texture_index']]
                    mat_name = get_material_name(tex_name)
                    
                    # Coordenadas mundiais centralizadas
                    p0 = ((x + 0) * scale, (y + 0) * scale, -h[0])
                    p1 = ((x + 1) * scale, (y + 0) * scale, -h[1])
                    p2 = ((x + 1) * scale, (y + 1) * scale, -h[3])
                    p3 = ((x + 0) * scale, (y + 1) * scale, -h[2])
                    
                    for p in (p0, p1, p2, p3):
                        obj.write(f"v {p[0]:.2f} {p[1]:.2f} {p[2]:.2f}\n")
                        
                    # O .gnd guarda u1 u2 u3 u4 v1 v2 v3 v4 (nao intercalado); cantos 0..3 = (x,y) (x+1,y) (x,y+1) (x+1,y+1)
                    uv = f_info['texcoords']
                    uvs = [
                        (uv[0], 1.0 - uv[4]),  # p0 (x,   y)
                        (uv[1], 1.0 - uv[5]),  # p1 (x+1, y)
                        (uv[3], 1.0 - uv[7]),  # p2 (x+1, y+1)
                        (uv[2], 1.0 - uv[6])   # p3 (x,   y+1)
                    ]
                    for u, v in uvs:
                        obj.write(f"vt {u:.4f} {v:.4f}\n")
                        
                    if mat_name not in gnd_faces_by_mat:
                        gnd_faces_by_mat[mat_name] = []
                    gnd_faces_by_mat[mat_name].append((
                        [v_offset, v_offset + 1, v_offset + 2, v_offset + 3],
                        [vt_offset, vt_offset + 1, vt_offset + 2, vt_offset + 3]
                    ))
                    v_offset += 4
                    vt_offset += 4

        for mat_name, f_list in gnd_faces_by_mat.items():
            obj.write(f"usemtl {mat_name}\n")
            for v_idx, vt_idx in f_list:
                obj.write(f"f {v_idx[0]}/{vt_idx[0]} {v_idx[1]}/{vt_idx[1]} {v_idx[2]}/{vt_idx[2]} {v_idx[3]}/{vt_idx[3]}\n")

        # --- PARTE B: MODELOS RSW / RSM ---
        obj.write("\no Buildings_And_Objects\n")
        
        placed_count = 0
        night_lights = []
        for inst in rsw['models']:
            m_fn = inst['filename']
            if m_fn not in rsm_cache:
                continue
                
            rsm = rsm_cache[m_fn]
            inst_pos = inst['position']
            inst_rot = inst['rotation'] # (deg_x, deg_y, deg_z)
            inst_scale = inst['scale']
            
            # Matriz da instancia no espaco do RO (X leste, Y para baixo, Z norte), como no roBrowser:
            # T(pos + centro do mapa) * Rz * Rx * Ry * S * T(-centro da caixa, -base da caixa, -centro da caixa)
            box_c, box_max = rsm['box_center'], rsm['box_max']
            inst_m = mat4_translation((inst_pos[0] + center_x, inst_pos[1], inst_pos[2] + center_y))
            inst_m = mat4_mul(inst_m, mat4_rot_z(math.radians(inst_rot[2])))
            inst_m = mat4_mul(inst_m, mat4_rot_x(math.radians(inst_rot[0])))
            inst_m = mat4_mul(inst_m, mat4_rot_y(math.radians(inst_rot[1])))
            inst_m = mat4_mul(inst_m, mat4_scaling(inst_scale))
            inst_m = mat4_mul(inst_m, mat4_translation((-box_c[0], -box_max[1], -box_c[2])))

            # Postes: uma luz noturna na lanterna (perto do topo do modelo), em coordenadas do OBJ
            lamp_kind = night_light_kind(m_fn)
            if lamp_kind:
                box_min = rsm['box_min']
                top_y = box_min[1] + (box_max[1] - box_min[1]) * 0.12  # Y para baixo: min = topo
                lx, ly, lz = mat4_apply(inst_m, (box_c[0], top_y, box_c[2]))
                night_lights.append({'kind': lamp_kind, 'model': m_fn, 'x': round(lx, 2), 'y': round(lz, 2), 'z': round(-ly, 2)})

            # Processa cada nó do modelo
            for node in rsm['nodes']:
                node_verts = node['vertices']
                node_faces = node['faces']
                node_texcoords = node['texcoords']
                node_textures = rsm['textures']
                
                if not node_verts or not node_faces:
                    continue
                    
                local_v_start = v_offset
                local_vt_start = vt_offset
                
                # Transformacao e escrita dos vertices: instancia * no, depois RO (X, Y-baixo, Z) -> OBJ (X, Z, -Y)
                full = mat4_mul(inst_m, node['mesh_matrix'])
                for v in node_verts:
                    rx, ry, rz = mat4_apply(full, v)
                    obj.write(f"v {rx:.2f} {rz:.2f} {-ry:.2f}\n")
                    v_offset += 1
                    
                # Escrita das coordenadas UV
                for uv in node_texcoords:
                    obj.write(f"vt {uv[0]:.4f} {1.0 - uv[1]:.4f}\n")
                    vt_offset += 1
                    
                # Agrupa e escreve as faces por material
                faces_by_mat: Dict[str, List[Tuple[List[int], List[int]]]] = {}
                for face in node_faces:
                    # O indice da face aponta para a lista de texturas DO NO, que por sua vez indexa as do modelo
                    t_idx = face['texture_index']
                    node_tex = node['texture_indices']
                    if t_idx < len(node_tex) and node_tex[t_idx] < len(node_textures):
                        mat_name = get_material_name(node_textures[node_tex[t_idx]])
                    elif t_idx < len(node_textures):
                        mat_name = get_material_name(node_textures[t_idx])
                    else:
                        mat_name = list(materials.values())[0]
                        
                    v_idx = [local_v_start + vi for vi in face['vert_indices']]
                    vt_idx = [local_vt_start + ti for ti in face['tex_indices']]
                    
                    if mat_name not in faces_by_mat:
                        faces_by_mat[mat_name] = []
                    faces_by_mat[mat_name].append((v_idx, vt_idx))
                    
                for mat_name, f_list in faces_by_mat.items():
                    obj.write(f"usemtl {mat_name}\n")
                    for vi, vti in f_list:
                        obj.write(f"f {vi[0]}/{vti[0]} {vi[1]}/{vti[1]} {vi[2]}/{vti[2]}\n")

            placed_count += 1
            
    # O RSW de Prontera nao tem luzes: as noturnas saem dos modelos de poste (lido pelo ue_import_map.py)
    window_textures = find_window_textures(map_dir, tex_index, model_texture_names)
    lights_path = os.path.splitext(output_obj)[0] + '_lights.json'
    with open(lights_path, 'w', encoding='utf-8') as lf:
        json.dump({'coords': 'obj', 'lights': night_lights, 'window_textures': window_textures}, lf, ensure_ascii=False, indent=1)
    print(f"Texturas com janelas (brilham a noite): {len(window_textures)}")

    # Caminhabilidade do .gat: a mesma grade do servidor vira o piso/paredes de colisao no cliente
    gat_path = os.path.join(map_dir, f"{map_name}.gat")
    if not os.path.exists(gat_path) and rsw.get('gat_file'):
        gat_path = resolve_asset_path(map_dir, rsw['gat_file'])
    if gat_path and os.path.exists(gat_path):
        floors, walls = write_gat_collision(read_gat(gat_path), scale / 2.0, os.path.splitext(output_obj)[0])
        print(f"Colisao do GAT: {floors} celulas caminhaveis, {walls} trechos de parede")
    else:
        print("AVISO: .gat nao encontrado; o mapa fica sem colisao de caminhabilidade")

    print(f"Instâncias de modelos 3D compiladas: {placed_count}")
    print(f"Luzes noturnas (postes): {len(night_lights)}")
    print(f"Arquivo final gerado com sucesso:")
    print(f"  -> {output_obj}")
    print(f"  -> {output_mtl}")
    print(f"  -> {lights_path}")
    return output_obj


# ----------------------------------------------------------------------------- GAT (caminhabilidade)
# Tipos de celula como no rAthena (map_gat2cell): 1 = parede e 5 = vao (atiravel) nao sao caminhaveis.
GAT_BLOCKED_TYPES = (1, 5)
GAT_WALL_HEIGHT = 40.0  # unidades do OBJ (x10 = 4 m na Unreal)


def read_gat(gat_path: str):
    """Le o .gat: grade de celulas (2 por ladrilho do .gnd) com 4 alturas e o tipo."""
    with open(gat_path, 'rb') as f:
        data = f.read()
    if data[:4] != b'GRAT':
        raise ValueError(f"Arquivo GAT invalido: {data[:4]}")
    width, height = struct.unpack_from('<II', data, 6)
    cells = [struct.unpack_from('<4fI', data, 14 + i * 20) for i in range(width * height)]
    return {'width': width, 'height': height, 'cells': cells}


def write_gat_collision(gat: dict, cell_size: float, out_base: str):
    """
    Gera as malhas de colisao do mapa a partir do .gat (o mesmo que o servidor usa):
      <base>_gat_floor.obj: o piso das celulas caminhaveis (recebe o clique e define a navegacao);
      <base>_gat_walls.obj: paredes verticais nas bordas entre caminhavel e bloqueado (impedem o WASD de atravessar).
    Coordenadas do OBJ iguais as do mapa visual: x = cx * tamanho, y = cy * tamanho, z = -altura.
    """
    w, h, cells = gat['width'], gat['height'], gat['cells']

    def walkable(x, y):
        return 0 <= x < w and 0 <= y < h and cells[y * w + x][4] not in GAT_BLOCKED_TYPES

    def corner(x, y, i):
        # cantos do .gat: 0 = (x, y), 1 = (x+1, y), 2 = (x, y+1), 3 = (x+1, y+1)
        c = cells[y * w + x]
        dx, dy = (0, 1, 0, 1)[i], (0, 0, 1, 1)[i]
        return ((x + dx) * cell_size, (y + dy) * cell_size, -c[i])

    floor_quads, wall_quads = [], []
    for y in range(h):
        for x in range(w):
            if not walkable(x, y):
                continue
            p0, p1, p2, p3 = (corner(x, y, i) for i in range(4))
            floor_quads.append((p0, p1, p3, p2))
            # borda -> (canto A, canto B) de cada vizinho
            for (nx, ny), (a, b) in (((x, y - 1), (p0, p1)), ((x + 1, y), (p1, p3)), ((x, y + 1), (p3, p2)), ((x - 1, y), (p2, p0))):
                if not walkable(nx, ny):
                    top_a = (a[0], a[1], a[2] + GAT_WALL_HEIGHT)
                    top_b = (b[0], b[1], b[2] + GAT_WALL_HEIGHT)
                    wall_quads.append((a, b, top_b, top_a))

    for suffix, quads in (('_gat_floor.obj', floor_quads), ('_gat_walls.obj', wall_quads)):
        with open(out_base + suffix, 'w', encoding='utf-8') as f:
            f.write('# Myth of Thor - colisao gerada do .gat\no GAT\nusemtl GAT\n')
            for q in quads:
                for p in q:
                    f.write(f"v {p[0]:.2f} {p[1]:.2f} {p[2]:.2f}\n")
            for i in range(len(quads)):
                b = i * 4 + 1
                f.write(f"f {b} {b + 1} {b + 2} {b + 3}\n")
    return len(floor_quads), len(wall_quads)



def find_window_textures(map_dir: str, tex_index: dict, names) -> list:
    """
    Texturas de predio com vidro de janela: pixels azulados (azul > vermelho), o vidro das casas do RO.
    A Unreal faz esses pixels brilharem com luz quente a noite (parametro WindowGlow do material).
    """
    try:
        from PIL import Image
    except ImportError:
        print("AVISO: Pillow ausente; sem deteccao de janelas")
        return []
    found = []
    skip = ('bench', 'wag', 'tomb', 'flo', 'tree', 'leaf', 'grass', 'water')
    for name in sorted(names):
        if any(k in name.lower() for k in skip):
            continue  # objetos de rua, nao predios
        rel = tex_index.get(name.lower())
        path = os.path.join(map_dir, rel) if rel else None
        if not path or not os.path.exists(path):
            continue
        try:
            im = Image.open(path).convert('RGB').resize((64, 64))
        except Exception:
            continue
        px = list(im.getdata())
        glass = 0
        for r, g, b in px:
            if r > 240 and g < 15 and b > 240:
                continue  # magenta = transparente
            lum = (0.3 * r + 0.59 * g + 0.11 * b) / 255.0
            # vidro do RO: azul-acinzentado ou verde-agua (azul acima do vermelho); tijolo, pedra e madeira puxam para o vermelho
            if 0.12 < lum < 0.85 and b >= r + 10 and b + 10 >= g:
                glass += 1
        ratio = glass / float(len(px))
        if 0.004 <= ratio <= 0.20:  # fachada com janelas (nem textura toda escura, nem sem vidro)
            found.append(name)
    return found

# Modelos que viram luzes noturnas (nome do arquivo .rsm, sem pasta): poste comum e poste com estandarte
NIGHT_LIGHT_MODELS = {
    '가로등01.rsm': 'lamp',          # 가로등01
    '휘장가로등.rsm': 'lamp',  # 휘장가로등
}


def night_light_kind(model_filename: str):
    base = model_filename.replace('/', '\\').split('\\')[-1]
    return NIGHT_LIGHT_MODELS.get(base)


PHASE1_MAPS = [
    'prontera',
    'new_1-1',
    'new_1-2',
    'new_1-3',
    'new_1-4',
    'prt_fild08',
    'prt_sewb1',
    'prt_sewb2',
    'prt_sewb3',
    'prt_sewb4'
]

REGIONAL_FLAGSHIP_MAPS = [
    'prontera',
    'izlude',
    'geffen',
    'payon',
    'morocc',
    'alberta',
    'aldebaran',
    'glast_01',
    'xmas'
]


if __name__ == '__main__':
    import argparse
    parser = argparse.ArgumentParser(description="Myth of Thor - Compilador de Mapas RO (GND+RSW+RSM -> OBJ)")
    parser.add_argument("target", nargs="?", default=ASSETS_ROOT, help="Diretório ou arquivo alvo")
    parser.add_argument("--map", nargs="+", help="Nomes específicos de mapas a compilar (ex: prontera izlude geffen)")
    parser.add_argument("--phase1", action="store_true", help="Compila os 10 mapas da Fase 1 (Prontera, Treinamento, Campo Sul, Esgotos 1-4)")
    parser.add_argument("--regions", action="store_true", help="Compila os 9 mapas capitais das regiões 1 a 9")
    parser.add_argument("--all", action="store_true", help="Compila todos os mapas encontrados no diretório")
    args = parser.parse_args()

    base_dir = args.target if (args.target and os.path.isdir(args.target)) else ASSETS_ROOT

    if args.phase1:
        print("=== Compilando Mapas da Fase 1 (Prontera, Treinamento, Campo Sul, Esgotos 1-4) ===")
        for m in PHASE1_MAPS:
            try:
                compile_map(base_dir, map_name=m)
            except Exception as e:
                print(f"Erro ao compilar mapa {m}: {e}")
    elif args.regions:
        print("=== Compilando Mapas Capitais das Regiões 1 a 9 ===")
        for m in REGIONAL_FLAGSHIP_MAPS:
            try:
                compile_map(base_dir, map_name=m)
            except Exception as e:
                print(f"Erro ao compilar mapa {m}: {e}")
    elif args.map:
        for m in args.map:
            try:
                compile_map(base_dir, map_name=m)
            except Exception as e:
                print(f"Erro ao compilar mapa {m}: {e}")
    elif args.all:
        root_dir = args.target
        print(f"Buscando todos os mapas em {root_dir}...")
        for r, d, f in os.walk(root_dir):
            if any(file.endswith('.rsw') for file in f) and any(file.endswith('.gnd') for file in f):
                try:
                    compile_map(r)
                except Exception as e:
                    print(f"Erro ao compilar {r}: {e}")
    else:
        if os.path.isdir(args.target) and any(f.endswith('.rsw') for f in os.listdir(args.target)):
            compile_map(args.target)
        else:
            compile_map(base_dir, map_name='prontera')


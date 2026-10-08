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
import math
import struct
from typing import Dict, List, Tuple

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
            mat3 = struct.unpack('9f', f.read(36))
            offset_ = struct.unpack('3f', f.read(12))
            offset = struct.unpack('3f', f.read(12))
            rot_axis_angle = struct.unpack('4f', f.read(16))
            scale = struct.unpack('3f', f.read(12))
            
            vert_count = struct.unpack('I', f.read(4))[0]
            verts = []
            for _ in range(vert_count):
                v = struct.unpack('3f', f.read(12))
                verts.append(v)
                for c in range(3):
                    min_v[c] = min(min_v[c], v[c])
                    max_v[c] = max(max_v[c], v[c])
                    
            texcoord_count = struct.unpack('I', f.read(4))[0]
            texcoords = []
            for _ in range(texcoord_count):
                # uint32 color + 2 floats (u, v)
                f.read(4) # color/padding
                uv = struct.unpack('2f', f.read(8))
                texcoords.append(uv)
                
            face_count = struct.unpack('I', f.read(4))[0]
            faces = []
            for _ in range(face_count):
                v_idx = struct.unpack('3H', f.read(6))
                t_idx = struct.unpack('3H', f.read(6))
                tex_idx = struct.unpack('H', f.read(2))[0]
                f.read(2) # padding
                two_sided, smooth_group = struct.unpack('2I', f.read(8))
                faces.append({
                    'vert_indices': v_idx,
                    'tex_indices': t_idx,
                    'texture_index': tex_idx
                })
                
            # Keyframes (ignora para malha estática)
            if ver_major > 1 or (ver_major == 1 and ver_minor >= 5):
                loc_kf = struct.unpack('I', f.read(4))[0]
                f.seek(loc_kf * (4 + 12), os.SEEK_CUR)
            rot_kf = struct.unpack('I', f.read(4))[0]
            f.seek(rot_kf * (4 + 16), os.SEEK_CUR)
            
            nodes.append({
                'name': n_name,
                'parent_name': p_name,
                'texture_indices': tex_indices,
                'mat3': mat3,
                'offset': offset,
                'scale': scale,
                'vertices': verts,
                'texcoords': texcoords,
                'faces': faces
            })
            
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


def compile_map(map_dir: str, output_obj: str = None):
    """Compila o mapa completo para OBJ"""
    map_dir = os.path.abspath(map_dir)
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
    models_search_dirs = [
        os.path.join(map_dir, 'model'),
        r"D:\Ragnarok_Assets\shared\data\model",
        r"D:\Ragnarok_Assets\Prontera\Prontera_Cidade\model"
    ]
    rsm_cache: Dict[str, dict] = {}
    
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
            except Exception as e:
                pass
                
    print(f"Modelos carregados com sucesso: {len(rsm_cache)}/{len(unique_models)}")
    
    # 3. Escreve arquivo MTL resolvendo caminhos de texturas existentes
    tex_search_dirs = [
        os.path.join(map_dir, 'texture'),
        r"D:\Ragnarok_Assets\shared\data\texture",
        r"D:\Ragnarok_Assets\Prontera\Prontera_Cidade\texture"
    ]
    tex_index = {}
    for td in tex_search_dirs:
        if os.path.exists(td):
            for root, _, files in os.walk(td):
                for f in files:
                    tex_index.setdefault(f.lower(), os.path.relpath(os.path.join(root, f), map_dir).replace('\\', '/'))
                    try:
                        korean = f.encode('latin1').decode('cp949', errors='ignore').lower()
                        tex_index.setdefault(korean, os.path.relpath(os.path.join(root, f), map_dir).replace('\\', '/'))
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
                        
                    uv = f_info['texcoords']
                    uvs = [
                        (uv[0], 1.0 - uv[1]),
                        (uv[2], 1.0 - uv[3]),
                        (uv[6], 1.0 - uv[7]),
                        (uv[4], 1.0 - uv[5])
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
        for inst in rsw['models']:
            m_fn = inst['filename']
            if m_fn not in rsm_cache:
                continue
                
            rsm = rsm_cache[m_fn]
            inst_pos = inst['position']
            inst_rot = inst['rotation'] # (deg_x, deg_y, deg_z)
            inst_scale = inst['scale']
            
            # Centro da instância no espaço de mundo do GND
            # No GND: X vai de 0 a width*scale, Y vai de 0 a height*scale, Z é altitude
            # No RSW: X é Leste-Oeste, Z é Norte-Sul, Y é altitude
            world_x = inst_pos[0] + center_x
            world_y = inst_pos[2] + center_y
            world_z = -inst_pos[1]
            
            # Rotações em radianos
            # Rot Z, Rot X, Rot Y
            rz = math.radians(inst_rot[2])
            rx = math.radians(inst_rot[0])
            ry = math.radians(inst_rot[1])
            
            cos_z, sin_z = math.cos(rz), math.sin(rz)
            cos_x, sin_x = math.cos(rx), math.sin(rx)
            cos_y, sin_y = math.cos(ry), math.sin(ry)
            
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
                
                # Transformação e escrita dos vértices
                for v in node_verts:
                    # 1. Bounding box centering (alinha base com o chão)
                    vx = v[0] - rsm['box_center'][0]
                    vy = v[1] - rsm['box_max'][1]
                    vz = v[2] - rsm['box_center'][2]
                    
                    # 2. Offset local do nó
                    vx += node['offset'][0]
                    vy += node['offset'][1]
                    vz += node['offset'][2]
                    
                    # 3. Escala do nó e da instância
                    vx *= (node['scale'][0] * inst_scale[0])
                    vy *= (node['scale'][1] * inst_scale[1])
                    vz *= (node['scale'][2] * inst_scale[2])
                    
                    # 4. Rotação (Z, depois X, depois Y)
                    # Rot Z (no plano XY)
                    x1 = vx * cos_z - vy * sin_z
                    y1 = vx * sin_z + vy * cos_z
                    z1 = vz
                    # Rot X (no plano YZ)
                    x2 = x1
                    y2 = y1 * cos_x - z1 * sin_x
                    z2 = y1 * sin_x + z1 * cos_x
                    # Rot Y (no plano XZ)
                    x3 = x2 * cos_y + z2 * sin_y
                    y3 = y2
                    z3 = -x2 * sin_y + z2 * cos_y
                    
                    # 5. Translação para o mundo (X mund, Y mund, Z altitude)
                    final_x = x3 + world_x
                    final_y = z3 + world_y
                    final_z = -y3 + world_z
                    
                    obj.write(f"v {final_x:.2f} {final_y:.2f} {final_z:.2f}\n")
                    v_offset += 1
                    
                # Escrita das coordenadas UV
                for uv in node_texcoords:
                    obj.write(f"vt {uv[0]:.4f} {1.0 - uv[1]:.4f}\n")
                    vt_offset += 1
                    
                # Agrupa e escreve as faces por material
                faces_by_mat: Dict[str, List[Tuple[List[int], List[int]]]] = {}
                for face in node_faces:
                    t_idx = face['texture_index']
                    if t_idx < len(node_textures):
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
            
    print(f"Instâncias de modelos 3D compiladas: {placed_count}")
    print(f"Arquivo final gerado com sucesso:")
    print(f"  -> {output_obj}")
    print(f"  -> {output_mtl}")
    return output_obj


if __name__ == '__main__':
    if len(sys.argv) > 1 and sys.argv[1] == '--all':
        root_dir = sys.argv[2] if len(sys.argv) > 2 else 'D:/Ragnarok_Assets'
        print(f"Buscando todos os mapas em {root_dir}...")
        for r, d, f in os.walk(root_dir):
            if any(file.endswith('.rsw') for file in f) and any(file.endswith('.gnd') for file in f):
                try:
                    compile_map(r)
                except Exception as e:
                    print(f"Erro ao compilar {r}: {e}")
    else:
        target_dir = sys.argv[1] if len(sys.argv) > 1 else 'D:/Ragnarok_Assets/Prontera/Prontera_Cidade'
        compile_map(target_dir)

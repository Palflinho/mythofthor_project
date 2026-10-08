#!/usr/bin/env python3
"""
Myth of Thor - Standalone Ragnarok Map to Wavefront OBJ Converter
-----------------------------------------------------------------
Converte os arquivos nativos de mapa do Ragnarok (.gnd, .rsw, .rsm)
diretamente para arquivos .OBJ e .MTL para importação na Unreal Engine 5.
"""

import os
import sys
import math
import struct
from typing import List, Tuple

def read_gnd(gnd_path: str):
    with open(gnd_path, 'rb') as f:
        magic = f.read(4)
        if magic != b'GRGN':
            raise ValueError(f"Arquivo inválido: {magic}")
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
        
        # Pula lightmaps (64B luminosidade + 192B cor por lightmap)
        f.seek(lightmap_count * (64 + 192), os.SEEK_CUR)
        
        face_count = struct.unpack('I', f.read(4))[0]
        faces = []
        for _ in range(face_count):
            texcoords = struct.unpack('8f', f.read(32))
            tex_idx, lightmap_idx = struct.unpack('2H', f.read(4))
            lighting = f.read(4)
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

def export_gnd_to_obj(gnd_data, output_obj: str, output_mtl: str):
    width = gnd_data['width']
    height = gnd_data['height']
    scale = gnd_data['scale']
    tiles = gnd_data['tiles']
    faces = gnd_data['faces']
    textures = gnd_data['textures']
    
    # Cria o arquivo MTL
    with open(output_mtl, 'w', encoding='utf-8') as mtl:
        for i, tex in enumerate(textures):
            mat_name = f"Mat_{i}"
            mtl.write(f"newmtl {mat_name}\n")
            mtl.write("Ka 1.0 1.0 1.0\nKd 1.0 1.0 1.0\nKs 0.0 0.0 0.0\n")
            tex_clean = os.path.basename(tex)
            mtl.write(f"map_Kd texture/{tex_clean}\n\n")

    mtl_filename = os.path.basename(output_mtl)
    
    with open(output_obj, 'w', encoding='utf-8') as obj:
        obj.write(f"# Myth of Thor - Terreno de Prontera\n")
        obj.write(f"mtllib {mtl_filename}\n\n")
        
        vertices = []
        texcoords = []
        quad_faces = [] # (material_index, [v1, v2, v3, v4], [vt1, vt2, vt3, vt4])
        
        v_idx = 1
        vt_idx = 1
        
        for y in range(height):
            for x in range(width):
                tile_idx = y * width + x
                tile = tiles[tile_idx]
                h = tile['heights']
                
                # Face do topo (Top Face)
                if tile['faces'][0] != -1:
                    face_info = faces[tile['faces'][0]]
                    mat_idx = face_info['texture_index']
                    
                    # 4 vértices do topo
                    # No Unreal: X = x * scale, Y = y * scale, Z = -height
                    p0 = ((x + 0) * scale, (y + 0) * scale, -h[0])
                    p1 = ((x + 1) * scale, (y + 0) * scale, -h[1])
                    p2 = ((x + 1) * scale, (y + 1) * scale, -h[3])
                    p3 = ((x + 0) * scale, (y + 1) * scale, -h[2])
                    
                    for p in (p0, p1, p2, p3):
                        obj.write(f"v {p[0]:.2f} {p[1]:.2f} {p[2]:.2f}\n")
                        
                    uv = face_info['texcoords']
                    # UVs: (u0,v0), (u1,v1), (u2,v2), (u3,v3)
                    # No OBJ, invertemos o V: 1.0 - v
                    uvs = [
                        (uv[0], 1.0 - uv[1]),
                        (uv[2], 1.0 - uv[3]),
                        (uv[6], 1.0 - uv[7]),
                        (uv[4], 1.0 - uv[5])
                    ]
                    for u, v in uvs:
                        obj.write(f"vt {u:.4f} {v:.4f}\n")
                        
                    quad_faces.append((
                        mat_idx,
                        [v_idx, v_idx + 1, v_idx + 2, v_idx + 3],
                        [vt_idx, vt_idx + 1, vt_idx + 2, vt_idx + 3]
                    ))
                    v_idx += 4
                    vt_idx += 4
                    
        # Agrupa e escreve faces por material
        current_mat = -1
        for mat_idx, v_list, vt_list in quad_faces:
            if mat_idx != current_mat:
                current_mat = mat_idx
                obj.write(f"\nusemtl Mat_{mat_idx}\n")
            obj.write(f"f {v_list[0]}/{vt_list[0]} {v_list[1]}/{vt_list[1]} {v_list[2]}/{vt_list[2]} {v_list[3]}/{vt_list[3]}\n")

if __name__ == '__main__':
    gnd_file = sys.argv[1] if len(sys.argv) > 1 else 'D:/Ragnarok_Assets/Prontera/prontera.gnd'
    obj_out = sys.argv[2] if len(sys.argv) > 2 else 'D:/Ragnarok_Assets/Prontera/prontera_ground.obj'
    mtl_out = obj_out.replace('.obj', '.mtl')
    
    print(f"Lendo {gnd_file}...")
    data = read_gnd(gnd_file)
    print(f"Dimensões: {data['width']}x{data['height']}, Texturas: {len(data['textures'])}")
    print(f"Exportando para {obj_out}...")
    export_gnd_to_obj(data, obj_out, mtl_out)
    print("Concluído com sucesso!")

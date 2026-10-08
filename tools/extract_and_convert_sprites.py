#!/usr/bin/env python3
"""
Myth of Thor - Sprite Extraction & Spritesheet Converter
--------------------------------------------------------
Extrai sprites (.spr, .act) de data.grf preservando a codificação
coreana CP949 e converte os frames para PNG com canal alfa transparente,
gerando spritesheets prontos para Unreal Engine 5 (Paper2D / PaperZD).
"""

import os
import sys
import math
import struct
import subprocess
from typing import List, Tuple
from PIL import Image

sys.stdout.reconfigure(encoding='utf-8')

# Sprites prioritários para o Vertical Slice (Fase 1: Treinamento + Prontera)
PHASE_1_TARGETS = [
    # Personagens
    r"data\sprite\인간족\몸통\남\초보자_남", # Aprendiz Masculino
    r"data\sprite\인간족\몸통\여\초보자_여", # Aprendiz Feminino
    # Monstros Iniciais
    r"data\sprite\몬스터\poring",           # Poring
    r"data\sprite\몬스터\lunatic",          # Lunatic
    r"data\sprite\몬스터\fabre",            # Fabre
    r"data\sprite\몬스터\pupa",             # Pupa
]

def extract_from_grf(grf_path: str, grfcl_dir: str, output_dir: str, targets: List[str]):
    """Extrai os arquivos .spr e .act do data.grf usando GrfCL com CP949"""
    grfcl_exe = os.path.join(grfcl_dir, "GrfCL.exe")
    if not os.path.exists(grfcl_exe):
        raise FileNotFoundError(f"GrfCL.exe não encontrado em {grfcl_dir}")

    os.makedirs(output_dir, exist_ok=True)
    print(f"=== Extraindo Sprites de {grf_path} ===")
    print(f"Destino: {output_dir}")

    for target in targets:
        for ext in ['.spr', '.act']:
            target_file = target + ext
            cmd = [
                grfcl_exe,
                '-breakOnExceptions', 'false',
                '-encoding', '949',
                '-open', grf_path,
                '-extractFiles', target_file, output_dir
            ]
            # 0x00000010 = CREATE_NEW_CONSOLE (necessário para compatibilidade com console Win32 no .NET)
            p = subprocess.Popen(cmd, cwd=grfcl_dir, creationflags=0x00000010)
            p.wait()
            print(f"  Extraído: {target_file}")

    print("Extração concluída!")


def decode_spr(spr_path: str) -> Tuple[List[Image.Image], dict]:
    """Decodifica arquivo .spr para lista de imagens PIL RGBA transparentes"""
    with open(spr_path, 'rb') as f:
        magic = f.read(2)
        if magic != b'SP':
            raise ValueError(f"Arquivo SPR inválido: {magic}")
            
        ver_min, ver_maj = struct.unpack('2B', f.read(2))
        indexed_count = struct.unpack('H', f.read(2))[0]
        rgba_count = struct.unpack('H', f.read(2))[0] if (ver_maj > 1 or (ver_maj == 1 and ver_min >= 1)) else 0
        
        # Carrega paleta de 256 cores nos últimos 1024 bytes
        f.seek(-1024, os.SEEK_END)
        palette_data = f.read(1024)
        palette = []
        for i in range(256):
            r, g, b, _ = struct.unpack('4B', palette_data[i*4:(i+1)*4])
            # Índice 0 é estritamente transparente
            alpha = 0 if i == 0 else 255
            palette.append((r, g, b, alpha))
            
        frames = []
        # Frames indexados iniciam no offset 8
        f.seek(8)
        
        for _ in range(indexed_count):
            w, h = struct.unpack('2H', f.read(4))
            if ver_maj > 2 or (ver_maj == 2 and ver_min >= 1):
                # Versão 2.1+: RLE Comprimido
                comp_size = struct.unpack('H', f.read(2))[0]
                rle_data = f.read(comp_size)
                pixels = []
                idx = 0
                while idx < len(rle_data) and len(pixels) < w * h:
                    b = rle_data[idx]
                    idx += 1
                    if b == 0:
                        count = rle_data[idx]
                        idx += 1
                        pixels.extend([0] * count)
                    else:
                        pixels.append(b)
            else:
                # Versão não-comprimida
                pixels = list(f.read(w * h))
                
            img = Image.new('RGBA', (w, h))
            img.putdata([palette[p] for p in pixels])
            frames.append(img)
            
        # Frames RGBA diretos (v2.0+)
        for _ in range(rgba_count):
            w, h = struct.unpack('2H', f.read(4))
            raw_rgba = f.read(w * h * 4)
            img = Image.frombytes('RGBA', (w, h), raw_rgba)
            frames.append(img)
            
        meta = {
            'version': f"{ver_maj}.{ver_min}",
            'indexed_count': indexed_count,
            'rgba_count': rgba_count
        }
        return frames, meta


def build_spritesheet(frames: List[Image.Image], output_sheet_path: str, columns: int = 8) -> str:
    """Monta os frames em uma única textura spritesheet (grid) com canal alfa"""
    if not frames:
        return ""
        
    max_w = max(f.width for f in frames)
    max_h = max(f.height for f in frames)
    
    total = len(frames)
    cols = min(columns, total)
    rows = math.ceil(total / cols)
    
    sheet_w = cols * max_w
    sheet_h = rows * max_h
    
    sheet = Image.new('RGBA', (sheet_w, sheet_h), (0, 0, 0, 0))
    
    for i, frame in enumerate(frames):
        c = i % cols
        r = i // cols
        
        # Alinha pela base (bottom-center) característico do Ragnarok
        x_offset = c * max_w + (max_w - frame.width) // 2
        y_offset = r * max_h + (max_h - frame.height)
        
        sheet.paste(frame, (x_offset, y_offset), frame)
        
    os.makedirs(os.path.dirname(output_sheet_path), exist_ok=True)
    sheet.save(output_sheet_path)
    return output_sheet_path


def process_all_sprites(raw_dir: str, output_png_dir: str):
    """Varre todos os .spr extraídos e gera PNGs individuais e spritesheets"""
    print(f"\n=== Processando Sprites para PNG (HD-2D) ===")
    converted = 0
    
    for root, _, files in os.walk(raw_dir):
        for f in files:
            if f.endswith('.spr'):
                spr_path = os.path.join(root, f)
                base_name = os.path.splitext(f)[0]
                
                try:
                    frames, meta = decode_spr(spr_path)
                    if not frames:
                        continue
                        
                    # 1. Salva frames individuais
                    sprite_folder = os.path.join(output_png_dir, base_name)
                    os.makedirs(sprite_folder, exist_ok=True)
                    for i, frame in enumerate(frames):
                        frame.save(os.path.join(sprite_folder, f"frame_{i:03d}.png"))
                        
                    # 2. Gera spritesheet consolidada
                    sheet_path = os.path.join(output_png_dir, f"{base_name}_sheet.png")
                    build_spritesheet(frames, sheet_path)
                    
                    print(f"  [OK] {base_name}: {len(frames)} frames -> {sheet_path}")
                    converted += 1
                except Exception as e:
                    print(f"  [ERRO] Falha ao processar {f}: {e}")
                    
    print(f"Total de spritesheets geradas: {converted}")


if __name__ == '__main__':
    grf = r"D:\Downloads\data.grf"
    grfcl = r"D:\Downloads\GrfCL"
    raw_out = r"D:\Ragnarok_Assets\Sprites\Raw"
    png_out = r"D:\Ragnarok_Assets\Sprites\PNG"
    
    # Executa extração de data.grf
    if os.path.exists(grf) and os.path.exists(grfcl):
        extract_from_grf(grf, grfcl, raw_out, PHASE_1_TARGETS)
        
    # Converte para PNG e Spritesheet
    if os.path.exists(raw_out):
        process_all_sprites(raw_out, png_out)

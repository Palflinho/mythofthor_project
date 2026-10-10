#!/usr/bin/env python3
"""
Analisa os blocos de receitas de efeito da 1ª classe e gera um resumo estruturado.
"""
import os
import re
import json

JS_DIR = r"\\wsl.localhost\Ubuntu\home\palf\references\roBrowserLegacy\src\DB"

def load_effect_table():
    path = os.path.join(JS_DIR, "Effects", "EffectTable.js")
    with open(path, "r", encoding="latin-1") as f:
        text = f.read()
    
    effects = {}
    pattern = re.compile(r'\n\t(?:(\d+)|\'([^\']+)\'|(\w+)):\s*\[', re.S)
    matches = list(pattern.finditer(text))
    for i, m in enumerate(matches):
        key = m.group(1) or m.group(2) or m.group(3)
        start = m.end()
        end = matches[i+1].start() if i+1 < len(matches) else len(text)
        block = text[start:end]
        depth = 1
        bracket_end = -1
        for idx, ch in enumerate(block):
            if ch == '[':
                depth += 1
            elif ch == ']':
                depth -= 1
                if depth == 0:
                    bracket_end = idx
                    break
        if bracket_end != -1:
            effects[key] = block[:bracket_end].strip()
        else:
            effects[key] = block.strip()
    return effects

target_ids = [
    "1", "10", "11", "15", "16", "17", "quake_magnum", "18", "20", "21", "23", "24", "25",
    "27", "28", "29", "30", "37", "38", "39", "40", "41", "42", "49", "51", "52",
    "66", "67", "141", "152", "153", "169", "170", "308", "309", "310", "311", "312", "320",
    "ef_coldbolt", "ef_firebolt", "ef_arrow_projectile", "ef_arrow_shower_projectile"
]

eff_table = load_effect_table()

for tid in target_ids:
    raw = eff_table.get(tid, "")
    print(f"\n==================== [EFFECT {tid}] ====================")
    # Divide os objetos {...} dentro do array
    obj_pattern = re.compile(r'\{([^{}]*(?:\{[^{}]*\}[^{}]*)*)\}', re.S)
    objs = obj_pattern.findall(raw)
    if not objs:
        print("  (Vazio ou nao encontrado)")
        continue
    for idx, obj in enumerate(objs):
        lines = [l.strip() for l in obj.splitlines() if l.strip() and not l.strip().startswith("//")]
        type_m = re.search(r"type:\s*'([^']+)'", obj)
        etype = type_m.group(1) if type_m else "(sem type explícito)"
        wav_m = re.search(r"wav:\s*'([^']+)'", obj)
        ewav = wav_m.group(1) if wav_m else ""
        file_m = re.search(r"file:\s*'([^']+)'", obj)
        efile = file_m.group(1) if file_m else ""
        tex_m = re.search(r"textureName:\s*'([^']+)'", obj)
        etex = tex_m.group(1) if tex_m else ""
        print(f"  Parte {idx+1}: Type={etype} | Wav={ewav} | File={efile} | Tex={etex}")
        # Mostra propriedades chave
        for k in ['duration', 'alphaMax', 'blendMode', 'size', 'sizeStart', 'sizeEnd', 'bottomSize', 'topSize', 'height', 'posZ', 'rotate', 'animation', 'duplicate', 'toSrc', 'fromSrc', 'repeat']:
            m = re.search(rf"\b{k}:\s*([^,\n]+)", obj)
            if m:
                print(f"    - {k}: {m.group(1).strip()}")

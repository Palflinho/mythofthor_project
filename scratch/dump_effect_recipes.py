#!/usr/bin/env python3
"""
Extrai a receita exata de cada efeito da 1ª classe do EffectTable.js e salva em UTF-8.
"""
import os
import re

JS_DIR = r"\\wsl.localhost\Ubuntu\home\palf\references\roBrowserLegacy\src\DB"

def load_effect_table():
    path = os.path.join(JS_DIR, "Effects", "EffectTable.js")
    with open(path, "r", encoding="latin-1") as f:
        text = f.read()
    
    effects = {}
    pattern = re.compile(r'\n\t(?:(\d+)|\'([^\']+)\'):\s*\[', re.S)
    matches = list(pattern.finditer(text))
    for i, m in enumerate(matches):
        key = m.group(1) if m.group(1) is not None else m.group(2)
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

if __name__ == "__main__":
    eff_table = load_effect_table()
    out_lines = []
    for tid in target_ids:
        content = eff_table.get(tid)
        out_lines.append("="*60)
        out_lines.append(f"EFFECT ID: {tid}")
        out_lines.append("="*60)
        if content:
            out_lines.append(content)
        else:
            out_lines.append("[NAO ENCONTRADO NO EFFECT_TABLE]")
        out_lines.append("\n")
    
    with open(r"scratch\first_class_effect_recipes.txt", "w", encoding="utf-8") as f:
        f.write("\n".join(out_lines))
    print(f"Salvo scratch\\first_class_effect_recipes.txt com {len(target_ids)} efeitos.")

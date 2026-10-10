#!/usr/bin/env python3
"""
Gera Config/robe_sprites.json: id visual da capa/asas (LOOK_ROBE, campo robe dos pacotes) -> pasta em data/sprite/로브.

Fonte: data/luafiles514/lua files/datainfo/spriterobename.lub (RobeNameTable: id -> nome da pasta; RobeTopLayer: ids
desenhados na camada de cima), avaliado com tools/lub_eval.py. A capa fica em
data/sprite/로브/<nome>/<남|여>/<sprite da classe>_<남|여>.spr (o mesmo nome de sprite do corpo da classe).

Uso: python3 tools/lub_eval.py robe.json spriterobeid.lub spriterobename.lub
     python3 tools/gen_robe_sprites.py robe.json Config/robe_sprites.json
"""
import json
import sys

src = json.load(open(sys.argv[1], encoding='utf-8'))
names = {str(k): v for k, v in src['RobeNameTable'].items()}
top = sorted(set(src.get('RobeTopLayer', {}).values()))
json.dump({'robes': names, 'top_layer': top}, open(sys.argv[2], 'w', encoding='utf-8'), ensure_ascii=False, indent=0)
print(len(names), 'capas,', len(top), 'na camada de cima')

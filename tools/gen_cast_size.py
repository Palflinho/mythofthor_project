"""
Gera Config/cast_size.json: tamanho (lado, em celulas) do circulo magico de cada skill de chao por nivel, do roBrowserLegacy
(src/Renderer/Effects/MagicTarget.js, CastSize[SkillId.X] = [tamanho do nivel 1, 2, ...]). Skills sem entrada usam 1 celula, como o cliente.

Uso (WSL): python3 tools/gen_cast_size.py [~/references/roBrowserLegacy]
"""
import json
import os
import re
import sys

rob = os.path.expanduser(sys.argv[1] if len(sys.argv) > 1 else '~/references/roBrowserLegacy')
consts = dict((n, int(v)) for n, v in re.findall(r'^\s*(\w+)\s*:\s*(\d+)', open(os.path.join(rob, 'src/DB/Skills/SkillConst.js'), encoding='latin-1').read(), re.M))
text = open(os.path.join(rob, 'src/Renderer/Effects/MagicTarget.js'), encoding='utf-8').read()
out = {}
# CastSize[SkillId.A] = //comentario\n\tCastSize[SkillId.B] = [3, 5];   (encadeadas: todas recebem a lista)
for m in re.finditer(r'((?:^\s*CastSize\[SkillId\.\w+\]\s*=\s*(?://[^\n]*)?\s*)+)\[([\d,\s]+)\];', text, re.M):
    sizes = [int(x) for x in m.group(2).split(',') if x.strip()]
    for name in re.findall(r'CastSize\[SkillId\.(\w+)\]', m.group(1)):
        if not re.match(r'^\s*//', m.group(1)) and name in consts:
            out[str(consts[name])] = sizes
dest = os.path.join(os.path.dirname(os.path.abspath(__file__)), '..', 'Config', 'cast_size.json')
json.dump(out, open(dest, 'w'), indent=0, sort_keys=True)
print('cast_size.json: %d skills -> %s' % (len(out), os.path.normpath(dest)))

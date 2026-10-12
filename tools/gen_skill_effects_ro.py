"""
Gera Config/skill_effects_full.json a partir do roBrowserLegacy, sem tabela feita a mao:
  - effects: src/DB/Effects/EffectTable.js inteiro (cada parte com TODOS os campos: STR, SPR, 3D, 2D, CYLINDER, FUNC,
    QuadHorn, TRAIL com o "spawn" aninhado, WATERFALL, RSM...). Ids em texto ('ef_arrow_projectile') ganham um id numerico
    (os 2001-2011 que o cliente ja usa e depois 100000+), listado em "aliases".
  - skills: src/DB/Skills/SkillEffect.js com todos os momentos (effectId, effectIdOnCaster, hitEffectId, groundEffectId,
    successEffectId(OnCaster), releaseEffectId, beforeHitEffectId(OnCaster), hitEffectIdOnCaster, beginCastEffectId,
    hideCastAura, hideCastBar) e as copias "SkillEffect[SK.A] = SkillEffect[SK.B]".
  - element: db/re/skill_db.yml do rAthena (cor da luz).
Os .js sao lidos como DADOS por um leitor de literal JavaScript (nada e executado; funcoes viram {"func": "<texto>"}).

Uso (WSL): python3 tools/gen_skill_effects_ro.py [~/references/roBrowserLegacy] [~/mythofthor-server/rathena]
"""
import json
import os
import re
import sys

STAGE_KEYS = {
    'effectId': 'effect', 'effectIdOnCaster': 'cast', 'hitEffectId': 'hit', 'groundEffectId': 'ground',
    'successEffectId': 'success', 'successEffectIdOnCaster': 'successCaster', 'releaseEffectId': 'release',
    'beforeHitEffectId': 'beforeHit', 'beforeHitEffectIdOnCaster': 'beforeHitCaster',
    'hitEffectIdOnCaster': 'hitCaster', 'beginCastEffectId': 'beginCast',
}
# Ids numericos que o cliente ja usa para projeteis de texto (MythNetWorld: 2003 = flecha)
FIXED_ALIASES = {
    'ef_coldbolt': 2001, 'ef_firebolt': 2002, 'ef_arrow_projectile': 2003, 'ef_arrow_shower_projectile': 2004,
    'quake_magnum': 2005, 'ef_spear_projectile': 2006, 'ef_ht_landmine': 2007, 'ef_shield_projectile': 2008,
    'ef_tomahawk_projectile': 2009, 'ef_bs_adrenaline2': 2010, 'ef_greed_sound': 2011,
}


class JsReader:
    """Leitor de literal JavaScript (objeto/array/string/numero/bool) tolerante a comentarios, virgulas sobrando,
    funcoes e expressoes simples."""

    def __init__(self, text):
        self.t = text
        self.i = 0

    def ws(self):
        t = self.t
        while self.i < len(t):
            c = t[self.i]
            if c in ' \t\r\n':
                self.i += 1
            elif t.startswith('//', self.i):
                j = t.find('\n', self.i)
                self.i = len(t) if j < 0 else j + 1
            elif t.startswith('/*', self.i):
                j = t.find('*/', self.i)
                self.i = len(t) if j < 0 else j + 2
            else:
                break

    def peek(self):
        self.ws()
        return self.t[self.i] if self.i < len(self.t) else ''

    def string(self):
        q = self.t[self.i]
        self.i += 1
        raw = bytearray()
        while self.i < len(self.t) and self.t[self.i] != q:
            c = self.t[self.i]
            if c == '\\':
                n = self.t[self.i + 1]
                if n == 'x':
                    raw.append(int(self.t[self.i + 2:self.i + 4], 16))
                    self.i += 4
                    continue
                if n == 'u':
                    raw += chr(int(self.t[self.i + 2:self.i + 6], 16)).encode('utf-8')
                    self.i += 6
                    continue
                raw += {'n': '\n', 't': '\t', '\\': '\\', "'": "'", '"': '"'}.get(n, n).encode('latin-1', 'ignore')
                self.i += 2
                continue
            raw += c.encode('latin-1', 'ignore') if ord(c) < 256 else c.encode('utf-8')
            self.i += 1
        self.i += 1
        b = bytes(raw)
        for enc in ('cp949',):
            try:
                return b.decode(enc)
            except UnicodeDecodeError:
                pass
        return b.decode('latin-1')

    def skip_balanced(self, open_c, close_c):
        depth = 0
        start = self.i
        while self.i < len(self.t):
            c = self.t[self.i]
            if c in '\'"`':
                self.string()
                continue
            if c == open_c:
                depth += 1
            elif c == close_c:
                depth -= 1
                if depth == 0:
                    self.i += 1
                    return self.t[start:self.i]
            self.i += 1
        return self.t[start:]

    def value(self):
        c = self.peek()
        if c == '{':
            return self.obj()
        if c == '[':
            return self.arr()
        if c in '\'"':
            return self.string()
        if self.t.startswith('function', self.i):
            start = self.i
            self.i = self.t.index('{', self.i)
            self.skip_balanced('{', '}')
            return {'func': self.t[start:self.i]}
        # expressao ate , } ] no nivel 0 (numero, identificador, conta simples, arrow function)
        start = self.i
        depth = 0
        while self.i < len(self.t):
            ch = self.t[self.i]
            if ch in '\'"':
                self.string()
                continue
            if ch in '([{':
                depth += 1
            elif ch in ')]}':
                if depth == 0:
                    break
                depth -= 1
            elif ch == ',' and depth == 0:
                break
            self.i += 1
        expr = self.t[start:self.i].strip()
        if expr in ('true', 'false'):
            return expr == 'true'
        if expr in ('null', 'undefined'):
            return None
        if re.fullmatch(r'-?\d+', expr):
            return int(expr)
        if re.fullmatch(r'-?(\d+\.\d*|\.\d+|\d+)(e-?\d+)?', expr):
            return float(expr)
        if re.fullmatch(r'[\d\s.+\-*/()]+|[\d\s.+\-*/()]*Math\.PI[\d\s.+\-*/()]*', expr):
            try:
                return eval(expr.replace('Math.PI', '3.141592653589793'), {'__builtins__': {}})
            except Exception:
                pass
        if '=>' in expr:
            return {'func': expr}
        return {'expr': expr}

    def obj(self):
        self.i += 1
        out = {}
        while True:
            c = self.peek()
            if c == '}':
                self.i += 1
                return out
            if c == ',':
                self.i += 1
                continue
            if c in '\'"':
                key = self.string()
            elif c == '[':  # [chave calculada]
                key = self.skip_balanced('[', ']')
            else:
                m = re.match(r'[A-Za-z_$][\w$]*|\d+', self.t[self.i:])
                if not m:
                    self.i += 1
                    continue
                key = m.group(0)
                self.i += len(key)
            if self.peek() == '(':  # metodo: nome(...) { ... }
                self.skip_balanced('(', ')')
                self.ws()
                self.skip_balanced('{', '}')
                out[key] = {'func': key}
                continue
            if self.peek() == ':':
                self.i += 1
            out[key] = self.value()

    def arr(self):
        self.i += 1
        out = []
        while True:
            c = self.peek()
            if c == ']':
                self.i += 1
                return out
            if c == ',':
                self.i += 1
                continue
            out.append(self.value())


def read_effect_table(path):
    text = open(path, encoding='latin-1').read()
    start = text.index('{', text.index('export default {'))
    reader = JsReader(text)
    reader.i = start
    return reader.obj()


def normalize_part(part):
    """Campos que o cliente le direto (file, attached, wav, type, func quake), alem de todos os originais."""
    if not isinstance(part, dict):
        return None
    p = {}
    for k, v in part.items():
        if isinstance(v, dict) and set(v) == {'expr'}:
            continue  # referencia a outra coisa do roBrowser (classe de efeito, constante): nao serve como dado
        p[k] = v
    p.setdefault('type', 'WAV' if 'wav' in p and 'file' not in p else '3D')
    for alt in ('file', 'textureName', 'textureFile', 'absoluteSpriteName'):
        if isinstance(p.get(alt), str) and p[alt]:
            p['file'] = p[alt]
            break
    p.setdefault('file', '')
    p['attached'] = bool(p.get('attachedEntity', True))
    p.setdefault('wav', '')
    funcs = ' '.join(v['func'] for v in part.values() if isinstance(v, dict) and 'func' in v)
    if 'setQuake' in funcs or 'quake' in funcs.lower():
        p['type'] = 'FUNC'
        p['func'] = 'quake'
        p.setdefault('duration', 50)
    return p


def main():
    rob = os.path.expanduser(sys.argv[1] if len(sys.argv) > 1 else '~/references/roBrowserLegacy')
    rat = os.path.expanduser(sys.argv[2] if len(sys.argv) > 2 else '~/mythofthor-server/rathena')
    db = os.path.join(rob, 'src', 'DB')

    raw_effects = read_effect_table(os.path.join(db, 'Effects', 'EffectTable.js'))
    aliases = dict(FIXED_ALIASES)
    next_id = 100000
    for key in raw_effects:
        if not str(key).isdigit() and key not in aliases:
            aliases[key] = next_id
            next_id += 1

    def eid(key):
        key = str(key)
        return int(key) if key.isdigit() else aliases.get(key)

    effects = {}
    for key, parts in raw_effects.items():
        if not isinstance(parts, list):
            continue
        out = [n for n in (normalize_part(p) for p in parts) if n]
        if out and eid(key) is not None:
            effects[str(eid(key))] = out

    consts = dict((n, int(v)) for n, v in re.findall(r'^\s*(\w+)\s*:\s*(\d+)', open(os.path.join(db, 'Skills', 'SkillConst.js'), encoding='latin-1').read(), re.M))
    text = open(os.path.join(db, 'Skills', 'SkillEffect.js'), encoding='latin-1').read()
    skills = {}
    for m in re.finditer(r'SkillEffect\[SK\.(\w+)\]\s*=\s*', text):
        name = m.group(1)
        if name not in consts:
            continue
        rest = text[m.end():]
        alias = re.match(r'SkillEffect\[SK\.(\w+)\]', rest)
        if alias:
            src = consts.get(alias.group(1))
            if src is not None and str(src) in skills:
                skills[str(consts[name])] = dict(skills[str(src)])
            continue
        if not rest.lstrip().startswith('{'):
            continue
        reader = JsReader(text)
        reader.i = m.end()
        reader.ws()
        entry = reader.obj()
        out = {}
        for k, v in entry.items():
            if k in STAGE_KEYS:
                vals = v if isinstance(v, list) else [v]
                ids = [eid(x) for x in vals if eid(x) is not None]
                if ids:
                    out[STAGE_KEYS[k]] = ids[0] if len(ids) == 1 else ids
            elif k in ('hideCastAura', 'hideCastBar') and v is True:
                out[k] = True
        skills[str(consts[name])] = out

    elements = {}
    cur = None
    for line in open(os.path.join(rat, 'db', 're', 'skill_db.yml'), encoding='utf-8', errors='replace'):
        m = re.match(r'^\s*-\s*Id:\s*(\d+)', line)
        if m:
            cur = m.group(1)
            continue
        m = re.match(r'^\s{4}Element:\s*(\w+)', line)
        if m and cur and cur not in elements:
            elements[cur] = m.group(1)
    for sid, el in elements.items():
        if el != 'Neutral':
            skills.setdefault(sid, {})['element'] = el

    dest = os.path.join(os.path.dirname(os.path.abspath(__file__)), '..', 'Config', 'skill_effects_full.json')
    with open(dest, 'w', encoding='utf-8') as f:
        json.dump({'effects': effects, 'skills': skills, 'aliases': aliases}, f, ensure_ascii=False, indent=0, sort_keys=True)
    types = {}
    for parts in effects.values():
        for p in parts:
            types[p['type']] = types.get(p['type'], 0) + 1
    print('efeitos %d (%d com nome), skills %d -> %s' % (len(effects), len(aliases), len(skills), os.path.normpath(dest)))
    print('tipos:', dict(sorted(types.items(), key=lambda x: -x[1])))


if __name__ == '__main__':
    main()

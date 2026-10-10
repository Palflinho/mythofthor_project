#!/usr/bin/env python3
"""
Executa .lub do cliente (bytecode Lua 5.1) que so montam tabelas de constantes e exporta as globais como JSON.

Os .lub de dados do cliente (skillid, skilltreeview, jobidentity, skillinfolist...) sao scripts do tipo
`SKID = { NV_BASIC = 1, ... }` ou `SKILL_TREEVIEW_FOR_JOB = { [JOBID.JT_NOVICE] = { [0] = SKID.NV_BASIC } }`.
Este interpretador cobre so as instrucoes que esses scripts usam (tabelas, constantes, globais, aritmetica
simples, closures sem executar). Funcoes definidas no script nao sao chamadas.

Uso: python3 tools/lub_eval.py saida.json arq1.lub [arq2.lub ...]
  (os arquivos rodam em sequencia no mesmo ambiente: skillid.lub antes de skilltreeview.lub, por exemplo)
"""
import json
import struct
import sys


class LuaFunction:
    def __init__(self):
        self.code = []
        self.consts = []
        self.protos = []
        self.maxstack = 0


class Reader:
    def __init__(self, data):
        self.d = data
        self.p = 0

    def u8(self):
        v = self.d[self.p]
        self.p += 1
        return v

    def i32(self):
        v = struct.unpack_from('<i', self.d, self.p)[0]
        self.p += 4
        return v

    def u32(self):
        v = struct.unpack_from('<I', self.d, self.p)[0]
        self.p += 4
        return v

    def f64(self):
        v = struct.unpack_from('<d', self.d, self.p)[0]
        self.p += 8
        return v

    def string(self):
        n = self.u32()
        if n == 0:
            return None
        s = self.d[self.p:self.p + n - 1]  # sem o \0 final
        self.p += n
        for enc in ('utf-8', 'cp949', 'latin-1'):
            try:
                return s.decode(enc)
            except UnicodeDecodeError:
                continue
        return s.decode('latin-1')

    def function(self):
        f = LuaFunction()
        self.string()  # source
        self.i32(); self.i32()  # linedefined, lastlinedefined
        self.u8(); self.u8(); self.u8()  # nups, numparams, is_vararg
        f.maxstack = self.u8()
        n = self.i32()
        f.code = [self.u32() for _ in range(n)]
        n = self.i32()
        for _ in range(n):
            t = self.u8()
            if t == 0:
                f.consts.append(None)
            elif t == 1:
                f.consts.append(self.u8() != 0)
            elif t == 3:
                v = self.f64()
                f.consts.append(int(v) if v.is_integer() else v)
            elif t == 4:
                f.consts.append(self.string())
            else:
                raise ValueError('constante de tipo %d' % t)
        n = self.i32()
        f.protos = [self.function() for _ in range(n)]
        # debug
        n = self.i32(); self.p += 4 * n
        n = self.i32()
        for _ in range(n):
            self.string(); self.i32(); self.i32()
        n = self.i32()
        for _ in range(n):
            self.string()
        return f


def load(path):
    with open(path, 'rb') as fh:
        data = fh.read()
    if data[:4] != b'\x1bLua' or data[4] != 0x51:
        raise ValueError('%s nao e bytecode Lua 5.1' % path)
    r = Reader(data)
    r.p = 12
    return r.function()


class Closure:
    pass


def run(f, env):
    R = [None] * (f.maxstack + 256)
    K = f.consts

    def RK(x):
        return K[x & 0xFF] if x & 0x100 else R[x]

    pc = 0
    while pc < len(f.code):
        i = f.code[pc]
        pc += 1
        op = i & 0x3F
        a = (i >> 6) & 0xFF
        c = (i >> 14) & 0x1FF
        b = (i >> 23) & 0x1FF
        bx = i >> 14
        sbx = bx - 131071
        if op == 0:  # MOVE
            R[a] = R[b]
        elif op == 1:  # LOADK
            R[a] = K[bx]
        elif op == 2:  # LOADBOOL
            R[a] = bool(b)
            if c:
                pc += 1
        elif op == 3:  # LOADNIL
            for k in range(a, b + 1):
                R[k] = None
        elif op == 5:  # GETGLOBAL
            R[a] = env.get(K[bx])
        elif op == 6:  # GETTABLE
            t = R[b]
            R[a] = t.get(RK(c)) if isinstance(t, dict) else None
        elif op == 7:  # SETGLOBAL
            env[K[bx]] = R[a]
        elif op == 9:  # SETTABLE
            if isinstance(R[a], dict):
                R[a][RK(b)] = RK(c)
        elif op == 10:  # NEWTABLE
            R[a] = {}
        elif op in (12, 13, 14, 15, 16):  # ADD SUB MUL DIV MOD
            x, y = RK(b), RK(c)
            try:
                R[a] = {12: x + y, 13: x - y, 14: x * y, 15: x / y, 16: x % y}[op]
            except TypeError:
                R[a] = None
        elif op == 18:  # UNM
            R[a] = -R[b] if isinstance(R[b], (int, float)) else None
        elif op == 21:  # CONCAT
            R[a] = ''.join(str(R[k]) for k in range(b, c + 1))
        elif op == 22:  # JMP
            pc += sbx
        elif op in (28, 29):  # CALL / TAILCALL: funcoes do script nao rodam
            R[a] = None
        elif op == 30:  # RETURN
            return
        elif op == 34:  # SETLIST
            n = b if b else 0
            if c == 0:
                c = f.code[pc]
                pc += 1
            if n == 0:
                n = 0
                while R[a + 1 + n] is not None:
                    n += 1
            base = (c - 1) * 50
            for k in range(1, n + 1):
                R[a][base + k] = R[a + k]
        elif op == 35:  # CLOSE
            pass
        elif op == 36:  # CLOSURE (nao executa)
            R[a] = Closure()
        else:
            raise ValueError('instrucao %d nao suportada (pc %d)' % (op, pc - 1))


def to_json(v):
    if isinstance(v, dict):
        return {str(k): to_json(x) for k, x in v.items()}
    if isinstance(v, Closure):
        return None
    return v


def main():
    out = sys.argv[1]
    env = {}
    for path in sys.argv[2:]:
        run(load(path), env)
        # jobidentity.lub do LATAM define JTtbl; os outros scripts usam JOBID
        if 'JTtbl' in env and 'JOBID' not in env:
            env['JOBID'] = env['JTtbl']
    with open(out, 'w', encoding='utf-8') as fh:
        json.dump({k: to_json(v) for k, v in env.items() if not isinstance(v, Closure)}, fh, ensure_ascii=False, indent=1)
    print('%d globais em %s' % (len(env), out))


if __name__ == '__main__':
    main()

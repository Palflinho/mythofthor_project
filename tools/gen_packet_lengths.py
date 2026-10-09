"""
Gera Config/packet_lengths_<PACKETVER>.json a partir do codigo do proprio rAthena (regra do projeto: nunca digitar
IDs/tamanhos de pacote de memoria). Fontes, em ordem de prioridade:
  1. src/map/clif_packetdb.hpp compilado com o PACKETVER escolhido: packet()/parseable_packet() viram printf,
     entao valem tambem as entradas escritas como sizeof(PACKET_X)
  2. (vence o 1) structs PACKET_<NOME> com DEFINE_PACKET_HEADER(<NOME>, id) de src/common/packets.hpp e src/map/packets.hpp:
     sizeof() compilado; -1 quando a struct tem campo de tamanho logo apos o ID (pacote dinamico)
  3. pacotes de unidade de packets_struct.hpp (0x09fd/0x09fe/0x09ff) -> dinamicos

Uso (WSL, com g++):
  python3 tools/gen_packet_lengths.py ~/mythofthor-server/rathena [PACKETVER] [saida.json]
"""
import json
import os
import re
import subprocess
import sys
import tempfile

BS = chr(92)  # barra invertida (evita confusao de escapes nos trechos de C++ gerados)
NL = BS + 'n'


def run(cmd, cwd):
    p = subprocess.run(cmd, cwd=cwd, capture_output=True, text=True)
    if p.returncode != 0:
        sys.stderr.write(p.stderr[-4000:])
        raise SystemExit('falhou: ' + ' '.join(cmd[:3]))
    return p.stdout


def main():
    if len(sys.argv) < 2:
        print(__doc__)
        sys.exit(1)
    root = os.path.expanduser(sys.argv[1])
    packetver = sys.argv[2] if len(sys.argv) > 2 else '20211103'
    dest = sys.argv[3] if len(sys.argv) > 3 else os.path.join(
        os.path.dirname(os.path.abspath(__file__)), '..', 'Config', 'packet_lengths_%s.json' % packetver)
    src = os.path.join(root, 'src')
    third = os.path.join(root, '3rdparty')
    incs = ['-I' + src, '-I' + os.path.join(src, 'map'),
            '-I' + os.path.join(third, 'rapidyaml', 'src'), '-I' + os.path.join(third, 'rapidyaml', 'ext', 'c4core', 'src'),
            '-I' + os.path.join(third, 'yaml-cpp', 'include'), '-I' + os.path.join(third, 'json', 'include'),
            '-I' + os.path.join(third, 'libconfig'), '-I' + os.path.join(third, 'httplib'),
            '-I' + os.path.join(third, 'mysql', 'include'), '-I/usr/include/mysql', '-I/usr/include/mariadb']
    defs = ['-DPACKETVER=%s' % packetver, '-DPRERE', '-std=c++17', '-w']  # mesmas flags do Makefile do servidor
    # common/packets.hpp (login/char) e map/packets.hpp usam o mesmo include guard: uma passagem para cada
    map_cpp = '#include <cstdio>' + chr(10) + '#include <map/packets.hpp>' + chr(10)
    common_cpp = '#include <cstdio>' + chr(10) + '#include <common/packets.hpp>' + chr(10)
    headers_cpp = map_cpp
    lengths = {}

    with tempfile.TemporaryDirectory() as tmp:
        # 1) clif_packetdb.hpp: tira os #define originais de packet/parseable_packet e troca por printf
        db = open(os.path.join(src, 'map', 'clif_packetdb.hpp'), encoding='utf-8', errors='replace').read()
        db = re.sub(r'^[ \t]*#define[ \t]+(packet|parseable_packet)\(.*$', '', db, flags=re.M)
        with open(os.path.join(tmp, 'myth_packetdb.hpp'), 'w') as f:
            f.write(db)
        with open(os.path.join(tmp, 'db.cpp'), 'w') as f:
            f.write(headers_cpp)
            f.write('#define packet(cmd, length) printf("%d %d' + NL + '", (int)(uint16)(cmd), (int)(length))' + chr(10))
            f.write('#define parseable_packet(cmd, length, ...) packet(cmd, length)' + chr(10))
            f.write('int main() {' + chr(10) + '#include "myth_packetdb.hpp"' + chr(10) + 'return 0; }' + chr(10))
        # Entradas cujo struct vive em outro header (ex.: sistemas novos) nao compilam aqui: comenta e tenta de novo
        for _ in range(60):
            p = subprocess.run(['g++'] + defs + incs + ['-I' + tmp, 'db.cpp', '-o', 'db'], cwd=tmp, capture_output=True, text=True)
            if p.returncode == 0:
                break
            bad = set(int(n) for n in re.findall(r'myth_packetdb\.hpp:(\d+):\d+: error', p.stderr))
            if not bad:
                sys.stderr.write(p.stderr[-3000:])
                raise SystemExit('falhou ao compilar o packetdb')
            lines = db.split(chr(10))
            for n in bad:
                lines[n - 1] = '// ' + lines[n - 1]
            db = chr(10).join(lines)
            with open(os.path.join(tmp, 'myth_packetdb.hpp'), 'w') as f:
                f.write(db)
        for line in run([os.path.join(tmp, 'db')], tmp).splitlines():
            pid, ln = (int(x) for x in line.split())
            lengths[pid] = ln  # a ultima definicao vale, como no packetdb_addpacket do servidor
        from_db = len(lengths)

        # 2) structs PACKET_* com header: sizeof() com o mesmo PACKETVER (map-server e depois login/char)
        added = 0
        for tag, inc in (('map', map_cpp), ('common', common_cpp)):
            with open(os.path.join(tmp, 'all_%s.cpp' % tag), 'w') as f:
                f.write(inc)
            pre = run(['g++', '-E', '-P'] + defs + incs + ['all_%s.cpp' % tag], tmp)
            headers = dict((n, int(v, 0)) for n, v in re.findall(r'const\s+int16\s+HEADER_(\w+)\s*=\s*(0x[0-9a-fA-F]+|\d+)', pre))
            structs = dict((m.group(1), m.group(2)) for m in re.finditer(r'struct\s+PACKET_(\w+)\s*\{(.*?)\}\s*__attribute__', pre, re.S))
            names = [n for n in headers if n in structs]
            with open(os.path.join(tmp, 'sizes_%s.cpp' % tag), 'w') as f:
                f.write(inc + 'int main(){' + chr(10))
                for n in names:
                    f.write('printf("%d %d' + NL + '", (int)(uint16)HEADER_' + n + ', (int)sizeof(PACKET_' + n + '));' + chr(10))
                f.write('return 0;}' + chr(10))
            run(['g++'] + defs + incs + ['sizes_%s.cpp' % tag, '-o', 'sizes_' + tag], tmp)
            for line, n in zip(run([os.path.join(tmp, 'sizes_' + tag)], tmp).splitlines(), names):
                pid, size = (int(x) for x in line.split())
                members = [m.strip() for m in structs[n].split(';') if m.strip()]
                second = members[1] if len(members) > 1 else ''
                # dinamico: o 2o campo e o tamanho do pacote (ex.: "int16 packetLength")
                dynamic = second.split()[-1] in ('packetLength', 'PacketLength', 'packetSize', 'packet_len', 'len', 'length') if second else False
                # o servidor envia sizeof(struct): vale mais que uma entrada antiga do packetdb (ex.: 0x01d7 = 11)
                if lengths.get(pid) != (-1 if dynamic else size):
                    added += 1
                lengths[pid] = -1 if dynamic else size

        # 3) pacotes por enum (packets_struct.hpp: sendLookType = 0x1d7 ...): no clif.cpp cada um e montado numa
        #    "struct packet_<x> p" com "p.PacketType = <tipo>"; o tamanho enviado e sizeof(packet_<x>)
        pre = run(['g++', '-E', '-P'] + defs + incs + ['all_map.cpp'], tmp)
        enum_ids = dict((n, int(v, 0)) for n, v in re.findall(r'\b(\w+Type)\s*=\s*(0x[0-9a-fA-F]+)', pre))
        clif = open(os.path.join(src, 'map', 'clif.cpp'), encoding='utf-8', errors='replace').read()
        pairs = {}
        for m in re.finditer(r'\b(\w+)(?:\.|->)[Pp]acket[Tt]ype\s*=\s*(\w+Type)\b', clif):
            var, typ = m.group(1), m.group(2)
            if typ not in enum_ids or typ in pairs:
                continue
            before = clif[max(0, m.start() - 4000):m.start()]
            decls = re.findall(r'(?:struct\s+)?((?:packet|PACKET)_\w+)\s*\*?\s*' + re.escape(var) + r'\b', before)
            if decls:
                pairs[typ] = decls[-1]
        pstructs = dict((m.group(1), m.group(2)) for m in re.finditer(r'struct\s+((?:packet|PACKET)_\w+)\s*\{(.*?)\}\s*__attribute__', pre, re.S))
        pairs = dict((t, s) for t, s in pairs.items() if s in pstructs)
        with open(os.path.join(tmp, 'enum_sizes.cpp'), 'w') as f:
            f.write(map_cpp + 'int main(){' + chr(10))
            for t, s in pairs.items():
                f.write('printf("%d %d' + NL + '", (int)(uint16)' + t + ', (int)sizeof(struct ' + s + '));' + chr(10))
            f.write('return 0;}' + chr(10))
        run(['g++'] + defs + incs + ['enum_sizes.cpp', '-o', 'enum_sizes'], tmp)
        for line, (t, s) in zip(run([os.path.join(tmp, 'enum_sizes')], tmp).splitlines(), pairs.items()):
            pid, size = (int(x) for x in line.split())
            members = [x.strip() for x in pstructs[s].split(';') if x.strip()]
            dynamic = len(members) > 1 and members[1].split()[-1] in ('PacketLength', 'packetLength', 'packetSize')
            lengths[pid] = -1 if dynamic else size

    for pid in (0x09fd, 0x09fe, 0x09ff):
        lengths.setdefault(pid, -1)

    with open(dest, 'w', encoding='utf-8') as f:
        json.dump({'0x%04x' % k: v for k, v in sorted(lengths.items())}, f, indent=2)
    print('clif_packetdb: %d, structs: %d, total: %d -> %s' % (from_db, added, len(lengths), os.path.normpath(dest)))
    for pid in (0x0080, 0x0086, 0x0087, 0x0091, 0x09a0, 0x008e, 0x00b0, 0x08c8, 0x0a30, 0x0b09, 0x0b39):
        print('  0x%04x = %s' % (pid, lengths.get(pid)))


if __name__ == '__main__':
    main()

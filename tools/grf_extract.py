#!/usr/bin/env python3
"""
Myth of Thor - GRF Archive Extractor (GRF 0x200 & 0x300)
---------------------------------------------------------
Lê e extrai arquivos de arquivos .grf do Ragnarok Online preservando a
codificação CP949 convertida para UTF-8 no sistema de arquivos moderno.

Suporta:
  - Cabeçalhos GRF 0x200 (32-bit offset) e 0x300 (64-bit offset)
  - Descompressão zlib da tabela e dos arquivos
  - Decodificação das cifras proprietárias DES da Gravity (Header DES e Mixed/Shuffle DES)
  - Filtros por padrões glob (ex: data\\sprite\\인간족\\**, data\\*.rsw, data\\texture\\유저인터페이스\\**)
  - Geração de manifest.json com caminhos originais, tamanhos, flags e CRC32

Uso:
  python tools/grf_extract.py --test
  python tools/grf_extract.py --filter "data\\sprite\\인간족\\**" --out D:\\Ragnarok_Assets
  python tools/grf_extract.py --filter "data\\*.gnd" "data\\*.rsw"
"""

import os
import sys
import struct
import zlib
import fnmatch
import json
import argparse
from typing import Dict, List, Optional, Tuple

if hasattr(sys.stdout, 'reconfigure'):
    sys.stdout.reconfigure(encoding='utf-8', errors='replace')
if hasattr(sys.stderr, 'reconfigure'):
    sys.stderr.reconfigure(encoding='utf-8', errors='replace')

try:
    from tools.grf_des import decodeHeader, decodeFull
except ImportError:
    from grf_des import decodeHeader, decodeFull

HEADER_SIZE = 46


class GrfEntry:
    __slots__ = ('name', 'csize', 'csize_aligned', 'rsize', 'flags', 'offset', 'crc32')

    def __init__(self, name: str, csize: int, csize_aligned: int, rsize: int, flags: int, offset: int):
        self.name = name
        self.csize = csize
        self.csize_aligned = csize_aligned
        self.rsize = rsize
        self.flags = flags
        self.offset = offset
        self.crc32: Optional[str] = None


class GrfArchive:
    def __init__(self, grf_path: str):
        self.grf_path = grf_path
        if not os.path.isfile(grf_path):
            raise FileNotFoundError(f"Arquivo GRF não encontrado: {grf_path}")

        self.version = 0x200
        self.file_table_offset = 0
        self.raw_file_count = 0
        self.file_count = 0
        self.entries: Dict[str, GrfEntry] = {}
        self._load_table()

    def _load_table(self):
        with open(self.grf_path, 'rb') as f:
            header = f.read(HEADER_SIZE)
            if len(header) < HEADER_SIZE:
                raise ValueError("Cabeçalho do GRF corrompido ou menor que 46 bytes.")

            magic = header[:16].split(b'\x00')[0]
            # Key de 14 bytes: header[16:30]
            
            # Detecta 0x200 vs 0x300
            # Em 0x300: offset de 64 bits em header[30:38], count em [38:42], version em [42:46]
            version_cand_300 = struct.unpack('<I', header[42:46])[0]
            if version_cand_300 == 0x300:
                self.version = 0x300
                self.file_table_offset = struct.unpack('<Q', header[30:38])[0]
                self.raw_file_count = struct.unpack('<I', header[38:42])[0]
                is_64bit = True
            else:
                # 0x200: offset em header[30:34], seed em [34:38], count em [38:42], version em [42:46]
                self.version = struct.unpack('<I', header[42:46])[0]
                self.file_table_offset = struct.unpack('<I', header[30:34])[0]
                self.raw_file_count = struct.unpack('<I', header[38:42])[0]
                is_64bit = False

            # No GRF, o offset é relativo ao fim do cabeçalho (+46)
            table_real_offset = self.file_table_offset + HEADER_SIZE
            f.seek(table_real_offset)

            # Cabeçalho da tabela de arquivos
            # Em 0x300: 3 uint32 (flags/magic, csize, rsize) ou 2 uint32
            tbl_head = f.read(12)
            csize_test, rsize_test = struct.unpack('<II', tbl_head[:8])
            if csize_test > 0 and rsize_test > 0 and csize_test < 50 * 1024 * 1024:
                compressed_size = csize_test
                uncompressed_size = rsize_test
                zlib_offset = table_real_offset + 8
            else:
                # Variante com 12 bytes: uint32 0, uint32 csize, uint32 rsize
                _, csize_12, rsize_12 = struct.unpack('<III', tbl_head)
                compressed_size = csize_12
                uncompressed_size = rsize_12
                zlib_offset = table_real_offset + 12

            f.seek(zlib_offset)
            compressed_table = f.read(compressed_size)
            decompressed_table = zlib.decompress(compressed_table)

        # Parse das entradas
        entry_meta_len = 21 if is_64bit else 17
        pos = 0
        total_len = len(decompressed_table)

        while pos < total_len:
            null_pos = decompressed_table.find(b'\x00', pos)
            if null_pos == -1:
                break
            raw_name = decompressed_table[pos:null_pos]
            pos = null_pos + 1
            if pos + entry_meta_len > total_len:
                break

            meta = decompressed_table[pos:pos + entry_meta_len]
            pos += entry_meta_len

            if is_64bit:
                csize, csize_aligned, rsize, flags = struct.unpack('<IIIB', meta[:13])
                offset, = struct.unpack('<Q', meta[13:21])
            else:
                csize, csize_aligned, rsize, flags = struct.unpack('<IIIB', meta[:13])
                offset, = struct.unpack('<I', meta[13:17])

            try:
                name = raw_name.decode('cp949')
            except UnicodeDecodeError:
                name = raw_name.decode('latin1', errors='replace')

            # Normaliza separadores de caminho
            name_norm = name.replace('/', '\\')
            entry = GrfEntry(name_norm, csize, csize_aligned, rsize, flags, offset)
            self.entries[name_norm.lower()] = entry

        self.file_count = len(self.entries)

    def get_entry(self, path: str) -> Optional[GrfEntry]:
        path_key = path.replace('/', '\\').lower()
        return self.entries.get(path_key)

    def extract_entry(self, entry: GrfEntry) -> bytes:
        real_offset = entry.offset + HEADER_SIZE
        read_len = entry.csize_aligned if (entry.flags & 6) else entry.csize
        with open(self.grf_path, 'rb') as f:
            f.seek(real_offset)
            raw = bytearray(f.read(read_len))

        # Decodificação de cifras conforme flags
        # flags & 1: é arquivo comprimido
        # flags & 2: cifra mista DES (full)
        # flags & 4: cifra de cabeçalho DES (header)
        if entry.flags & 2:
            decodeFull(raw, entry.csize_aligned, entry.csize)
        elif entry.flags & 4:
            decodeHeader(raw, entry.csize_aligned)

        # Trunca para o tamanho comprimido exato antes de descompressão zlib
        raw = raw[:entry.csize]

        # Descompressão
        if entry.csize == entry.rsize:
            data = bytes(raw)
        else:
            data = zlib.decompress(bytes(raw))

        return data

    def extract_file(self, internal_path: str, output_base_dir: str) -> Optional[str]:
        entry = self.get_entry(internal_path)
        if not entry:
            return None

        # Garante caminhos corretos e portáveis no Windows e Linux/WSL
        clean_name = entry.name.replace('/', '\\')
        parts = [p for p in clean_name.split('\\') if p]
        if not parts:
            return None

        base_name = parts[-1]
        # Pula entradas de diretório no GRF (tamanho 0 ou sem extensão)
        # para evitar criar arquivos vazios que bloqueiam a criação de subpastas reais
        if entry.rsize == 0 or '.' not in base_name:
            return None

        # Se output_base_dir já termina com 'data', evita criar subpasta 'data/data/...'
        norm_base = os.path.normpath(output_base_dir)
        if os.path.basename(norm_base).lower() == 'data' and parts and parts[0].lower() == 'data':
            parts = parts[1:]

        out_path = os.path.join(output_base_dir, *parts)
        parent_dir = os.path.dirname(out_path)

        # Se houver algum arquivo vazio antigo bloqueando a criação do diretório, remove-o
        cur = output_base_dir
        for p in parts[:-1]:
            cur = os.path.join(cur, p)
            if os.path.isfile(cur):
                try:
                    os.remove(cur)
                except OSError:
                    pass

        os.makedirs(parent_dir, exist_ok=True)

        data = self.extract_entry(entry)

        with open(out_path, 'wb') as f:
            f.write(data)

        entry.crc32 = f"{zlib.crc32(data) & 0xffffffff:08x}"
        return out_path

    def find(self, pattern: str) -> List[GrfEntry]:
        pat = pattern.replace('/', '\\').lower()
        matched = []
        for key, entry in self.entries.items():
            # Pula entradas de diretório (tamanho 0 ou sem extensão)
            base = entry.name.split('\\')[-1]
            if entry.rsize == 0 or '.' not in base:
                continue
            if fnmatch.fnmatch(key, pat) or fnmatch.fnmatch(entry.name.lower(), pat):
                matched.append(entry)
        return matched


def run_test(grf_path: str, out_dir: str):
    print("=" * 60)
    print("MYTH OF THOR — TESTE DE VALIDAÇÃO DO EXTRATOR GRF (A1)")
    print("=" * 60)
    print(f"Abrindo arquivo: {grf_path}")
    archive = GrfArchive(grf_path)
    print(f"[OK] Versão GRF detectada: {archive.version:#x}")
    print(f"[OK] Total de arquivos indexados: {archive.file_count:,}")

    test_targets = [
        "data\\sprite\\인간족\\몸통\\남\\초보자_남.act",
        "data\\sprite\\인간족\\몸통\\남\\초보자_남.spr",
        "data\\sprite\\몬스터\\poring.act",
        "data\\sprite\\몬스터\\poring.spr",
        "data\\prontera.rsw",
        "data\\prontera.gnd"
    ]

    all_passed = True
    manifest = {}

    for t in test_targets:
        entry = archive.get_entry(t)
        if not entry:
            print(f"[FALHA] Alvo não encontrado no índice: {t}")
            all_passed = False
            continue

        try:
            out_file = archive.extract_file(t, out_dir)
            file_size = os.path.getsize(out_file)
            passed = (file_size == entry.rsize)
            status = "PASS" if passed else "ERRO_TAMANHO"
            if not passed:
                all_passed = False

            print(f"[{status}] {t}")
            print(f"       -> Extraído em: {out_file}")
            print(f"       -> Tamanho real: {file_size} bytes (Esperado: {entry.rsize} | Flags: {entry.flags})")
            print(f"       -> CRC32: {entry.crc32}")

            manifest[t] = {
                "extracted_path": out_file,
                "real_size": file_size,
                "expected_size": entry.rsize,
                "flags": entry.flags,
                "crc32": entry.crc32
            }
        except Exception as e:
            print(f"[ERRO] Falha ao extrair {t}: {e}")
            all_passed = False

    manifest_path = os.path.join(out_dir, "manifest_test.json")
    with open(manifest_path, 'w', encoding='utf-8') as mf:
        json.dump(manifest, mf, indent=2, ensure_ascii=False)
    print(f"\n[OK] Manifest de teste gerado em: {manifest_path}")

    print("=" * 60)
    if all_passed:
        print("RESULTADO: TODOS OS TESTES PASSARAM COM 100% DE SUCESSO! ✅")
    else:
        print("RESULTADO: OCORRERAM FALHAS NA EXTRAÇÃO! ❌")
    print("=" * 60)
    return all_passed


def main():
    default_grf = r"C:\Gravity\Ragnarok\data.grf"
    default_out = os.environ.get("MYTH_ASSETS", r"E:\Dev\Ragnarok_Assets\grf\data")

    parser = argparse.ArgumentParser(description="Myth of Thor - Extrator Fiel de data.grf")
    parser.add_argument("--grf", default=default_grf, help="Caminho do arquivo data.grf")
    parser.add_argument("--out", default=default_out, help="Diretório de destino dos assets extraídos")
    parser.add_argument("--filter", nargs="+", help="Filtros glob para extrair (ex: data\\sprite\\인간족\\**)")
    parser.add_argument("--list", action="store_true", help="Apenas lista os arquivos encontrados pelo filtro")
    parser.add_argument("--test", action="store_true", help="Executa o teste oficial de validação com Poring e Aprendiz")
    parser.add_argument("--all-essential", action="store_true", help="Extrai o conjunto essencial para o Claude (Sprites, UI, Sons, Mapas Fase 1, Modelos e Texturas)")
    args = parser.parse_args()

    if args.test:
        test_out = os.path.join(args.out, "test_extraction")
        success = run_test(args.grf, test_out)
        sys.exit(0 if success else 1)

    print(f"Abrindo GRF: {args.grf}")
    archive = GrfArchive(args.grf)
    print(f"Versão: {archive.version:#x} | Arquivos: {archive.file_count:,}")

    filters = args.filter or []
    if args.all_essential:
        filters = [
            "data\\sprite\\인간족\\**",
            "data\\sprite\\몬스터\\**",
            "data\\texture\\유저인터페이스\\**",
            "data\\texture\\**",
            "data\\model\\**",
            "data\\wav\\**",
            # Mapas Fase 1 (Prontera, Campos e Esgotos)
            "data\\prontera.*",
            "data\\new_1-1.*",
            "data\\new_1-2.*",
            "data\\new_1-3.*",
            "data\\new_1-4.*",
            "data\\prt_fild08.*",
            "data\\prt_sewb1.*",
            "data\\prt_sewb2.*",
            "data\\prt_sewb3.*",
            "data\\prt_sewb4.*"
        ]

    if not filters:
        print("Nenhum filtro especificado. Use --filter, --test ou --all-essential.")
        return

    matched_entries = []
    seen = set()
    for pat in filters:
        for entry in archive.find(pat):
            if entry.name not in seen:
                seen.add(entry.name)
                matched_entries.append(entry)

    print(f"Total de arquivos selecionados pelos filtros: {len(matched_entries):,}")

    if args.list:
        for e in matched_entries[:100]:
            print(f"  {e.name} (Tamanho: {e.rsize:,} bytes | Flags: {e.flags})")
        if len(matched_entries) > 100:
            print(f"  ... e mais {len(matched_entries) - 100:,} arquivos.")
        return

    print(f"Iniciando extração para: {args.out}")
    manifest = {}
    total = len(matched_entries)
    for i, entry in enumerate(matched_entries, start=1):
        if i % 500 == 0 or i == total:
            print(f"  Progresso: [{i:,}/{total:,}] ({(i/total)*100:.1f}%)")
        try:
            out_file = archive.extract_file(entry.name, args.out)
            if out_file:
                manifest[entry.name] = {
                    "path": out_file,
                    "rsize": entry.rsize,
                    "crc32": entry.crc32,
                    "flags": entry.flags
                }
        except Exception as ex:
            print(f"  [ERRO] Falha ao extrair {entry.name}: {ex}")

    manifest_path = os.path.join(args.out, "manifest.json")
    with open(manifest_path, 'w', encoding='utf-8') as mf:
        json.dump(manifest, mf, indent=2, ensure_ascii=False)
    print(f"\nExtração concluída com sucesso! Manifest salvo em: {manifest_path}")


if __name__ == "__main__":
    main()
